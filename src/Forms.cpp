#include "Forms.h"

namespace LO
{
	namespace
	{
		template <class T>
		T* Find(RE::TESDataHandler* a_dataHandler, std::uint32_t a_localID, std::string_view a_file)
		{
			return a_dataHandler->FindForm<T>(a_localID, a_file);
		}
	}

	bool OwnedForms::Resolve()
	{
		_valid = false;
		_owned.clear();
		slotMenus.clear();
		slotKeywords.clear();
		slotUncategorizedKeywords.clear();

		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler) {
			REX::LogError("Forms: TESDataHandler is not available");
			return false;
		}

		if (!dataHandler->FindLoadedFileByName(Forms::PluginFile)) {
			REX::LogError("Forms: '{}' is not active. Enable it in your mod manager.", Forms::PluginFile);
			return false;
		}

		workshopMenuMain = Find<RE::BGSListForm>(dataHandler, Forms::WorkshopMenuMain, Forms::BaseGameFile);
		rootMenu = Find<RE::BGSListForm>(dataHandler, Forms::RootMenu, Forms::PluginFile);
		rootKeyword = Find<RE::BGSKeyword>(dataHandler, Forms::RootKeyword, Forms::PluginFile);
		uncategorizedKeyword = Find<RE::BGSKeyword>(dataHandler, Forms::UncategorizedKeyword, Forms::PluginFile);

		for (std::uint32_t i = 0; i < Forms::SlotCount; ++i) {
			slotMenus.push_back(Find<RE::BGSListForm>(dataHandler, Forms::FirstSlotMenu + i, Forms::PluginFile));
			slotKeywords.push_back(Find<RE::BGSKeyword>(dataHandler, Forms::FirstSlotKeyword + i, Forms::PluginFile));
			slotUncategorizedKeywords.push_back(Find<RE::BGSKeyword>(dataHandler, Forms::FirstSlotUncategorizedKeyword + i, Forms::PluginFile));
		}

		bool ok = true;
		const auto require = [&ok](const void* a_form, std::string_view a_what) {
			if (!a_form) {
				REX::LogError("Forms: missing {}", a_what);
				ok = false;
			}
		};

		require(workshopMenuMain, "WorkshopMenuMain (Fallout4.esm 106DA2)");
		require(rootMenu, "LO_RootMenu");
		require(rootKeyword, "LO_RootKeyword");
		require(uncategorizedKeyword, "LO_UncategorizedKeyword");
		for (std::uint32_t i = 0; i < Forms::SlotCount; ++i) {
			require(slotMenus[i], "LO_SlotMenu");
			require(slotKeywords[i], "LO_SlotKeyword");
			require(slotUncategorizedKeywords[i], "LO_SlotUncategorizedKeyword");
		}

		if (!ok) {
			REX::LogError("Forms: LostObjects.esp does not match this DLL version. Reinstall the mod.");
			return false;
		}

		_owned.insert(rootMenu);
		_owned.insert(rootKeyword);
		_owned.insert(uncategorizedKeyword);
		for (std::uint32_t i = 0; i < Forms::SlotCount; ++i) {
			_owned.insert(slotMenus[i]);
			_owned.insert(slotKeywords[i]);
			_owned.insert(slotUncategorizedKeywords[i]);
		}

		_valid = true;
		REX::LogInformation("Forms: resolved WorkshopMenuMain and {} plugin forms", _owned.size());
		return true;
	}

	bool OwnedForms::IsOwned(const RE::TESForm* a_form) const
	{
		return a_form && _owned.contains(a_form);
	}
}
