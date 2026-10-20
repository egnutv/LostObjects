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
		menus.clear();
		menuTitles.clear();
		leaves.clear();
		optionGlobals.clear();

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
		menuIcon = Find<RE::BGSArtObject>(dataHandler, Forms::MenuIcon, Forms::PluginFile);  // optional
		menuIconTransform = Find<RE::BGSTransform>(dataHandler, Forms::MenuIconTransform, Forms::PluginFile);  // optional
		holotape = Find<RE::BGSNote>(dataHandler, Forms::Holotape, Forms::PluginFile);  // optional
		holotapeTerminal = Find<RE::BGSTerminal>(dataHandler, Forms::HolotapeTerminal, Forms::PluginFile);  // optional
		for (std::uint32_t i = 0; i < Forms::OptionCount; ++i) {
			optionGlobals.push_back(Find<RE::TESGlobal>(dataHandler, Forms::FirstOptionGlobal + i, Forms::PluginFile));
		}
		if (!holotape || !holotapeTerminal || std::ranges::find(optionGlobals, nullptr) != optionGlobals.end()) {
			REX::LogWarning("Forms: the settings holotape is missing in LostObjects.esp - it is not available");
			holotape = nullptr;
		}

		bool ok = workshopMenuMain && rootMenu && rootKeyword;
		if (!workshopMenuMain) {
			REX::LogError("Forms: missing WorkshopMenuMain (Fallout4.esm 106DA2)");
		}

		for (std::uint32_t i = 0; i < Forms::MenuCount; ++i) {
			auto* menu = Find<RE::BGSListForm>(dataHandler, Forms::FirstMenu + i, Forms::PluginFile);
			auto* title = Find<RE::BGSKeyword>(dataHandler, Forms::FirstMenuTitle + i, Forms::PluginFile);
			ok = ok && menu && title;
			menus.push_back(menu);
			menuTitles.push_back(title);
		}
		for (std::uint32_t i = 0; i < Forms::LeafCount; ++i) {
			auto* leaf = Find<RE::BGSKeyword>(dataHandler, Forms::FirstLeaf + i, Forms::PluginFile);
			ok = ok && leaf;
			leaves.push_back(leaf);
		}

		if (!ok) {
			REX::LogError("Forms: LostObjects.esp does not match this DLL version. Reinstall the mod.");
			return false;
		}

		_owned.insert(rootMenu);
		_owned.insert(rootKeyword);
		for (std::uint32_t i = 0; i < Forms::MenuCount; ++i) {
			_owned.insert(menus[i]);
			_owned.insert(menuTitles[i]);
		}
		for (auto* leaf : leaves) {
			_owned.insert(leaf);
		}

		_valid = true;
		REX::LogInformation("Forms: resolved WorkshopMenuMain and {} plugin forms ({} menus, {} categories)", _owned.size(), menus.size(), leaves.size());
		return true;
	}

	bool OwnedForms::IsOwned(const RE::TESForm* a_form) const
	{
		return a_form && _owned.contains(a_form);
	}
}
