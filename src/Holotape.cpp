#include "Holotape.h"

#include "Recovery.h"
#include "Util.h"

namespace LO::Holotape
{
	namespace
	{
		// Must match data/Scripts/Source/User/LostObjects.psc.
		constexpr auto NATIVE_SCRIPT = "LostObjects";
		constexpr auto NATIVE_FUNCTION = "HolotapeItem";

		// Papyrus thread -> main thread.
		void HolotapeItem(std::monostate, std::int32_t a_itemID)
		{
			F4SE::GetTaskInterface()->AddTask([a_itemID]() {
				Recovery::Get().OnHolotapeItem(a_itemID);
			});
		}

		bool F4SE_API RegisterFunctions(RE::BSScript::IVirtualMachine* a_vm)
		{
			if (!a_vm) {
				return false;
			}
			const bool bound = a_vm->BindNativeFunction(NATIVE_SCRIPT, NATIVE_FUNCTION, &HolotapeItem, true);
			REX::LogInformation("Holotape: native function {}.{} {}", NATIVE_SCRIPT, NATIVE_FUNCTION, bound ? "registered" : "could NOT be registered");
			return bound;
		}

		// The model of the vanilla holotapes (the one most of them use), so no
		// fixed path is needed and the holotape looks like all the others.
		std::string FindVanillaHolotapeModel()
		{
			auto* dataHandler = RE::TESDataHandler::GetSingleton();
			if (!dataHandler) {
				return {};
			}
			std::map<std::string, std::uint32_t> counts;
			for (auto* note : dataHandler->GetFormArray<RE::BGSNote>()) {
				if (!note || note->type.get() != RE::BGSNote::NOTE_TYPE::kTerminal) {
					continue;
				}
				if (!Util::EqualsIgnoreCase(Util::GetFileName(note, 0), Forms::BaseGameFile)) {
					continue;
				}
				const char* model = static_cast<RE::TESModel*>(note)->GetModel();
				if (model && *model) {
					++counts[model];
				}
			}
			std::string best;
			std::uint32_t bestCount = 0;
			for (const auto& [model, count] : counts) {
				if (count > bestCount) {
					best = model;
					bestCount = count;
				}
			}
			return best;
		}
	}

	void RegisterPapyrus()
	{
		F4SE::GetPapyrusInterface()->Register(&RegisterFunctions);
	}

	bool& OptionValue(Settings& a_settings, std::size_t a_index)
	{
		switch (a_index) {
		case 0:
			return a_settings.sortIntoCategories;
		case 1:
			return a_settings.skipMissingModels;
		case 2:
			return a_settings.skipAlreadyBuildable;
		case OPTION_SUMMARY:
			return a_settings.showSummaryMessage;
		default:
			return a_settings.writeInventory;
		}
	}

	void SyncGlobals(const OwnedForms& a_forms)
	{
		if (!a_forms.holotape) {
			return;
		}
		auto& settings = Settings::Get();
		for (std::size_t i = 0; i < a_forms.optionGlobals.size(); ++i) {
			a_forms.optionGlobals[i]->SetValue(OptionValue(settings, i) ? 1.0f : 0.0f);
		}
	}

	void Setup(const OwnedForms& a_forms)
	{
		if (!a_forms.holotape) {
			return;
		}

		auto* model = static_cast<RE::TESModel*>(a_forms.holotape);
		const char* current = model->GetModel();
		if (!current || !*current) {
			const auto vanilla = FindVanillaHolotapeModel();
			if (!vanilla.empty()) {
				static std::string storage;  // the game may keep the pointer
				storage = vanilla;
				model->SetModel(storage.c_str());
				REX::LogInformation("Holotape: using the model of the vanilla holotapes ({})", vanilla);
			} else {
				REX::LogWarning("Holotape: no vanilla holotape model found, the holotape has no 3D model");
			}
		}

		SyncGlobals(a_forms);
	}

	void GiveIfMissing(const OwnedForms& a_forms)
	{
		if (!a_forms.holotape) {
			return;
		}
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}
		if (player->GetInventoryObjectCount(a_forms.holotape) > 0) {
			return;
		}
		player->AddObjectToContainer(a_forms.holotape, RE::BSTSmartPointer<RE::ExtraDataList>{}, 1, nullptr, RE::ITEM_REMOVE_REASON::kNone);
		RE::SendHUDMessage::ShowHUDMessage("Lost Objects: the settings holotape is in your inventory.");
		REX::LogInformation("Holotape: added to the player's inventory");
	}
}
