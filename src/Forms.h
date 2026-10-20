#pragma once

#include "PluginForms.h"

namespace LO
{
	// Forms provided by LostObjects.esp plus the vanilla main workshop menu.
	class OwnedForms
	{
	public:
		RE::BGSListForm* workshopMenuMain{ nullptr };  // Fallout4.esm "WorkshopMenuMain"

		RE::BGSListForm* rootMenu{ nullptr };  // our tab
		RE::BGSKeyword* rootKeyword{ nullptr };  // name + icon of our tab (entry 0)
		RE::BGSKeyword* uncategorizedKeyword{ nullptr };

		std::vector<RE::BGSListForm*> slotMenus;  // per-mod folders
		std::vector<RE::BGSKeyword*> slotKeywords;
		std::vector<RE::BGSKeyword*> slotUncategorizedKeywords;  // "Uncategorized" inside each folder

		// Looks everything up. Returns false (and logs why) if something is missing.
		bool Resolve();

		[[nodiscard]] bool IsValid() const noexcept { return _valid; }
		[[nodiscard]] bool IsOwned(const RE::TESForm* a_form) const;

	private:
		std::unordered_set<const RE::TESForm*> _owned;
		bool _valid{ false };
	};
}
