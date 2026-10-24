#pragma once

#include "PluginForms.h"

namespace LO
{
	// Forms provided by LostObjects.esp plus the vanilla main workshop menu.
	//
	// The plugin contains a pool of generic sub menus and categories. Lost
	// Objects names and fills them at runtime, so the menu structure can be
	// built from whatever the scan finds - no fixed lists.
	class OwnedForms
	{
	public:
		RE::BGSListForm* workshopMenuMain{ nullptr };  // Fallout4.esm "WorkshopMenuMain"

		RE::BGSListForm* rootMenu{ nullptr };  // the "Lost Objects" tab
		RE::BGSKeyword* rootKeyword{ nullptr };  // name + icon of the tab (entry 0)

		std::vector<RE::BGSListForm*> menus;  // pool: sub menus (entry 0 = matching title keyword)
		std::vector<RE::BGSKeyword*> menuTitles;  // pool: names of the sub menus
		std::vector<RE::BGSKeyword*> leaves;  // pool: categories lost recipes are tagged with

		RE::BGSArtObject* menuIcon{ nullptr };  // optional: our own workshop menu icon (the logo)
		RE::BGSTransform* menuIconTransform{ nullptr };  // optional: how the menu shows it

		RE::BGSNote* holotape{ nullptr };  // optional: the settings holotape
		RE::BGSTerminal* holotapeTerminal{ nullptr };  // its terminal (menu items, script)
		std::vector<RE::TESGlobal*> optionGlobals;  // one per holotape option (1 = on), see Holotape.h
		RE::BGSConstructibleObject* holotapeRecipe{ nullptr };  // optional: crafts the holotape (chemistry station)
		std::vector<RE::BGSTransform*> previews;  // pool: preview transforms for recovered items without one

		// Looks everything up. Returns false (and logs why) if something is missing.
		bool Resolve();

		[[nodiscard]] bool IsValid() const noexcept { return _valid; }
		[[nodiscard]] bool IsOwned(const RE::TESForm* a_form) const;
		[[nodiscard]] bool IsPreview(const RE::BGSTransform* a_transform) const;

	private:
		std::unordered_set<const RE::TESForm*> _owned;
		std::unordered_set<const RE::BGSTransform*> _previews;
		bool _valid{ false };
	};
}
