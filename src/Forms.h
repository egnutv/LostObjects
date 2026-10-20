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

		RE::BGSArtObject* menuIcon{ nullptr };  // optional: our own workshop menu icon (picture frame with the logo)

		// Looks everything up. Returns false (and logs why) if something is missing.
		bool Resolve();

		[[nodiscard]] bool IsValid() const noexcept { return _valid; }
		[[nodiscard]] bool IsOwned(const RE::TESForm* a_form) const;

	private:
		std::unordered_set<const RE::TESForm*> _owned;
		bool _valid{ false };
	};
}
