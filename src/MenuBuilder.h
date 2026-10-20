#pragma once

#include "Scanner.h"

namespace LO
{
	struct ApplyResult
	{
		bool changed{ false };  // menu content differs from before
		bool tabVisible{ false };  // our tab is part of WorkshopMenuMain
		std::uint32_t folderCount{ 0 };  // origin folders in use (ByMod)
		std::uint32_t entryCount{ 0 };  // recovered menus + categories
		std::uint32_t recipeCount{ 0 };  // recovered recipes
	};

	// Writes a ScanResult into the game's menu forms. Everything happens in
	// memory only: nothing is stored in the save game, and menus of other mods
	// are never changed, with one small exception: lost recipes without a usable
	// category get one of our "Uncategorized" keywords added, so they can be
	// shown in the folder of their origin. That is undone automatically as soon
	// as the recipe is reachable again.
	class MenuBuilder
	{
	public:
		ApplyResult Apply(const OwnedForms& a_forms, const ScanResult& a_scan, const Settings& a_settings);

		// Recipes we tagged, with their original categories.
		[[nodiscard]] const KeywordMap& OriginalKeywords() const noexcept { return _originalKeywords; }

		// Forget all state (used when the game data is reloaded).
		void Reset();

	private:
		using TagMap = std::unordered_map<RE::BGSConstructibleObject*, RE::BGSKeyword*>;

		void UpdateTags(const TagMap& a_wanted);

		KeywordMap _originalKeywords;  // recipe -> categories before we tagged it
		TagMap _currentTags;  // recipe -> the keyword we added
	};

	// Renames a keyword that has a name in the game's sparse name map
	// (all keywords in LostObjects.esp have one).
	bool SetKeywordName(RE::BGSKeyword* a_keyword, std::string_view a_name);
}
