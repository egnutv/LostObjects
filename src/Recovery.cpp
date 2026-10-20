#include "Recovery.h"

#include "Inventory.h"
#include "MenuDump.h"
#include "ModelCheck.h"
#include "NameMap.h"
#include "Notice.h"
#include "Report.h"
#include "Scanner.h"
#include "Settings.h"
#include "Util.h"

namespace LO
{
	namespace
	{
		constexpr auto WORKSHOP_MENU = "WorkshopMenu"sv;

		// Mods that inject their menus with Papyrus (for example through
		// Settlement Menu Manager) do so a few seconds after a save is loaded.
		// A few follow-up scans make sure those menus are seen as reachable.
		constexpr std::array FOLLOW_UP_DELAYS{ 3s, 10s, 30s, 90s };

		class WorkshopModeSink final : public RE::BSTEventSink<RE::Workshop::WorkshopModeEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(const RE::Workshop::WorkshopModeEvent& a_event, RE::BSTEventSource<RE::Workshop::WorkshopModeEvent>*) override
			{
				Recovery::Get().OnWorkshopModeChanged(a_event.start);
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		class MenuSink final : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent& a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
			{
				if (!a_event.opening && a_event.menuName == WORKSHOP_MENU.data()) {
					Recovery::Get().OnWorkshopMenuClosed();
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		WorkshopModeSink g_workshopModeSink;
		MenuSink g_menuSink;
	}

	Recovery& Recovery::Get() noexcept
	{
		static Recovery singleton;
		return singleton;
	}

	void Recovery::OnDataReady()
	{
		auto& settings = Settings::Get();
		settings.Load();

		if (settings.verboseLog) {
			if (auto logger = REX::GetDefaultLogger()) {
				logger->set_level(spdlog::level::debug);
			}
		}

		// After a data reload all forms are new, so forget the old state.
		_builder.Reset();
		_lastApply = {};
		_iconBorrowed = false;
		_ready = false;

		if (!settings.enabled) {
			REX::LogInformation("Lost Objects is disabled in the INI (bEnabled=0)");
			return;
		}

		if (!_forms.Resolve()) {
			REX::LogError("Lost Objects could not start. See the errors above.");
			return;
		}

		// Names of keywords (folder names, category names) come from the game's
		// name table. It is verified first; without it the mod still works.
		if (!NameMap::Initialize(_forms.rootKeyword, Forms::RootName) && settings.sortMode == SortMode::kByMod) {
			REX::LogWarning("Folder names are not available, using sSortMode=Flat for this session");
			settings.sortMode = SortMode::kFlat;
		}

		_ready = true;
		BorrowMenuIcon();
		RegisterEvents();

		REX::LogInformation("Lost Objects is ready");
	}

	void Recovery::OnGameLoaded(std::string_view a_reason)
	{
		Notice::ShowIfNeeded();
		RegisterEvents();  // in case the UI was not available at data load

		_summaryShown = false;
		RequestScan(a_reason);
		ScheduleFollowUpScans();
	}

	void Recovery::RequestScan(std::string_view a_reason)
	{
		const auto& settings = Settings::Get();
		if (!_ready || !settings.enabled) {
			return;
		}

		if (IsWorkshopMenuOpen()) {
			_scanPending = true;  // never change the menu while it is on screen
			return;
		}
		_scanPending = false;

		if (!ScanGuarded(*this, a_reason)) {
			OnScanFailed(a_reason);
		}
	}

	// Safety net: if anything inside the scan crashes, Lost Objects switches
	// itself off for this session instead of taking the game down with it.
	bool Recovery::ScanGuarded(Recovery& a_self, std::string_view a_reason)
	{
		__try {
			a_self.ScanNow(a_reason);
			return true;
		} __except (SEH_EXECUTE_HANDLER) {
			return false;
		}
	}

	void Recovery::OnScanFailed(std::string_view a_reason)
	{
		_ready = false;
		REX::LogError("Scan ({}) failed with an internal error. Lost Objects is switched off until the game is restarted.", a_reason);
		REX::LogError("Please send this log file (LostObjects.log) with your bug report.");
		RE::SendHUDMessage::ShowHUDMessage("Lost Objects stopped after an error. Please send LostObjects.log.");
	}

	void Recovery::ScanNow(std::string_view a_reason)
	{
		const auto& settings = Settings::Get();
		const bool traceSteps = ++_scanCount <= 5;  // detailed progress for the first scans only

		if (traceSteps) {
			REX::LogInformation("Scan ({}) started", a_reason);
		}

		const auto start = std::chrono::steady_clock::now();

		const auto scan = Scanner::Run(_forms, settings, _builder.OriginalKeywords());
		if (!scan.valid) {
			return;
		}

		if (traceSteps) {
			REX::LogInformation("Scan ({}): analysis done, updating the menu", a_reason);
		}

		const auto apply = _builder.Apply(_forms, scan, settings);

		const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);

		// Log and write the report whenever the result differs from the last one.
		std::size_t signature = scan.lostRecipes.size() * 31 + scan.uncategorized.size();
		for (const auto& entry : scan.entries) {
			signature = signature * 131 + entry.form->GetFormID() + entry.recipeCount;
		}

		if (apply.changed || !_hasLoggedOnce || signature != _lastSignature) {
			_hasLoggedOnce = true;
			_lastSignature = signature;
			Report::Log(scan, apply, a_reason, settings.verboseLog);
			if (settings.writeReport) {
				Report::Write(scan, apply, a_reason);
			}
			if (settings.writeInventory) {
				Inventory::Write(scan, apply, a_reason);
				MenuDump::Write(_forms, a_reason);
			}
		}
		REX::LogDebug("Scan ({}) finished in {} ms, changed={}", a_reason, elapsed.count(), apply.changed);

		_lastApply = apply;
	}

	void Recovery::OnWorkshopModeChanged(bool a_entering)
	{
		if (!a_entering) {
			return;
		}

		RequestScan("entering workshop mode"sv);

		if (!_summaryShown && _lastApply.recipeCount > 0) {
			ShowSummary(_lastApply);
			_summaryShown = true;  // once per loaded save
		}
	}

	void Recovery::OnWorkshopMenuClosed()
	{
		if (_scanPending) {
			RequestScan("workshop menu closed"sv);
		}
	}

	void Recovery::RegisterEvents()
	{
		if (_eventsRegistered) {
			return;
		}

		auto* workshopSource = RE::Workshop::WorkshopModeEvent::GetEventSource();
		auto* ui = RE::UI::GetSingleton();
		if (!workshopSource || !ui) {
			REX::LogDebug("Events: not available yet, will retry later");
			return;
		}

		workshopSource->RegisterSink(&g_workshopModeSink);
		static_cast<RE::BSTEventSource<RE::MenuOpenCloseEvent>*>(ui)->RegisterSink(&g_menuSink);

		_eventsRegistered = true;
		REX::LogInformation("Events: registered for workshop mode and menu events");
	}

	void Recovery::BorrowMenuIcon()
	{
		if (_iconBorrowed || !Settings::Get().borrowMenuIcon || !_forms.IsValid()) {
			return;
		}
		_iconBorrowed = true;

		// Workshop menu icons are art objects (ARTO) that list the category
		// keyword. The menu shows the art object's model the way its preview
		// transform (TRNS) says: position, rotation, scale and zoom.
		//
		// Our own icon (the logo, LO_MenuIcon in LostObjects.esp with
		// Meshes\LostObjects\MenuIcon.nif) is used if the mesh is there. Its
		// transform starts as a copy of the vanilla "Decorations" icon's
		// transform, so the menu frames it like the other icons. Without our
		// icon we link our keywords to the vanilla icon itself.
		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler) {
			return;
		}

		const auto linkTo = [this](RE::BGSArtObject* a_art) {
			auto* keywordForm = static_cast<RE::BGSKeywordForm*>(a_art);
			keywordForm->AddKeyword(_forms.rootKeyword);
			for (auto* title : _forms.menuTitles) {
				keywordForm->AddKeyword(title);
			}
		};

		const auto findArtFor = [dataHandler](const auto& a_match) -> RE::BGSArtObject* {
			for (auto* art : dataHandler->GetFormArray<RE::BGSArtObject>()) {
				if (!art) {
					continue;
				}
				auto* keywordForm = static_cast<RE::BGSKeywordForm*>(art);
				for (auto* keyword : keywordForm->GetKeywords()) {
					if (keyword && a_match(keyword)) {
						return art;
					}
				}
			}
			return nullptr;
		};

		const auto describe = [](const RE::BGSArtObject* a_art) {
			const char* model = static_cast<const RE::BGSModelMaterialSwap*>(a_art)->GetModel();
			const auto* transform = a_art->previewTransform.transform;
			if (!transform) {
				return std::format("{} from {}, model '{}', no preview transform", Util::FormIDToString(a_art->GetFormID()), Util::GetFileName(a_art, 0), model ? model : "");
			}
			const auto& d = transform->data;
			return std::format("{} from {}, model '{}', transform {} pos ({:.1f} {:.1f} {:.1f}) rot ({:.1f} {:.1f} {:.1f}) scale {:.2f} zoom {:.2f}..{:.2f} flags {:08X}",
				Util::FormIDToString(a_art->GetFormID()), Util::GetFileName(a_art, 0), model ? model : "", Util::FormIDToString(transform->GetFormID()),
				d.position.x, d.position.y, d.position.z, d.rotation.x, d.rotation.y, d.rotation.z, d.scale, d.minZoom, d.maxZoom, transform->formFlags.underlying());
		};

		// The vanilla "Decorations" icon (base game keyword WorkshopRecipeFilterDecor),
		// or the icon of the first vanilla sub menu.
		auto* donor = findArtFor([](const RE::BGSKeyword* a_keyword) {
			return Util::EqualsIgnoreCase(Util::GetEditorID(a_keyword), "WorkshopRecipeFilterDecor"sv);
		});
		if (!donor) {
			for (auto* form : _forms.workshopMenuMain->GetAllForms()) {
				auto* subMenu = form ? form->As<RE::BGSListForm>() : nullptr;
				auto* title = subMenu ? subMenu->GetNthForm(0) : nullptr;
				if (title) {
					donor = findArtFor([title](const RE::BGSKeyword* a_keyword) { return a_keyword == title; });
				}
				if (donor) {
					break;
				}
			}
		}

		// For the log: how the icons of the main menu are set up (helps to
		// tune our own icon).
		for (auto* form : _forms.workshopMenuMain->GetAllForms()) {
			auto* subMenu = form ? form->As<RE::BGSListForm>() : nullptr;
			auto* title = subMenu ? subMenu->GetNthForm(0) : nullptr;
			if (!title || _forms.IsOwned(subMenu)) {
				continue;
			}
			if (auto* art = findArtFor([title](const RE::BGSKeyword* a_keyword) { return a_keyword == title; })) {
				REX::LogInformation("Icon of '{}': {}", Util::GetFormName(title), describe(art));
			}
		}

		if (Settings::Get().customMenuIcon && _forms.menuIcon) {
			const auto exists = ModelCheck::FileExists(Forms::MenuIconModel);
			if (exists == 1) {
				auto* own = _forms.menuIcon->previewTransform.transform;
				if (!own) {
					own = _forms.menuIconTransform;
					_forms.menuIcon->previewTransform.transform = own;
				}
				const auto* donorTransform = donor ? donor->previewTransform.transform : nullptr;
				if (own && donorTransform && own != donorTransform) {
					own->data = donorTransform->data;

					// Same size as the vanilla icon: scale by the ratio of the bounds.
					const auto extent = [](const RE::TESBoundObject* a_object) {
						const auto& b = a_object->boundData;
						return static_cast<float>(std::max({ std::abs(b.boundMax.x - b.boundMin.x), std::abs(b.boundMax.y - b.boundMin.y), std::abs(b.boundMax.z - b.boundMin.z) }));
					};
					const auto ownExtent = extent(_forms.menuIcon);
					const auto donorExtent = extent(donor);
					if (ownExtent > 0.0f && donorExtent > 0.0f) {
						own->data.scale = donorTransform->data.scale * std::clamp(donorExtent / ownExtent, 0.25f, 8.0f);
					}
					constexpr std::uint32_t aroundOrigin = RE::BGSTransform::RecordFlag::kAroundOrigin;
					auto& ownFlags = *own->formFlags.underlying_address();
					ownFlags = (ownFlags & ~aroundOrigin) | (donorTransform->formFlags.underlying() & aroundOrigin);
				}
				linkTo(_forms.menuIcon);
				REX::LogInformation("Icon: using the Lost Objects icon: {}", describe(_forms.menuIcon));
				return;
			}
			REX::LogWarning("Icon: Meshes\\{} {} - using the vanilla icon instead", Forms::MenuIconModel, exists == 0 ? "was not found" : "could not be checked");
		}

		if (!donor) {
			REX::LogWarning("Icon: no vanilla menu icon found, the tab will show without an icon");
			return;
		}

		linkTo(donor);
		REX::LogInformation("Icon: using the vanilla icon: {}", describe(donor));
	}

	void Recovery::ScheduleFollowUpScans()
	{
		const auto generation = ++_loadGeneration;

		std::thread([generation]() {
			auto waited = 0s;
			for (const auto delay : FOLLOW_UP_DELAYS) {
				std::this_thread::sleep_for(delay - waited);
				waited = delay;

				if (Recovery::Get()._loadGeneration.load() != generation) {
					return;  // another save was loaded meanwhile
				}

				F4SE::GetTaskInterface()->AddTask([generation, delay]() {
					auto& recovery = Recovery::Get();
					if (recovery._loadGeneration.load() == generation) {
						recovery.RequestScan(std::format("follow-up scan after {} s", delay.count()));
					}
				});
			}
		}).detach();
	}

	void Recovery::ShowSummary(const ApplyResult& a_apply) const
	{
		if (!Settings::Get().showSummaryMessage) {
			return;
		}

		// Kept alive in static storage in case the HUD reads the text later.
		static std::string message;
		message = std::format("Lost Objects: {} workshop item(s) recovered, {} of them sorted into existing categories.", a_apply.recipeCount, a_apply.sortedIn);
		RE::SendHUDMessage::ShowHUDMessage(message.c_str());
	}

	bool Recovery::IsWorkshopMenuOpen()
	{
		const auto* ui = RE::UI::GetSingleton();
		if (!ui) {
			return false;
		}
		return ui->IsMenuOpen(RE::BSFixedString(WORKSHOP_MENU.data())).value_or(false);
	}
}
