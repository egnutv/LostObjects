#pragma once

#include "Scanner.h"

namespace LO
{
	struct ApplyResult
	{
		bool changed{ false };  // menu content differs from before
		bool tabVisible{ false };  // the "Lost Objects" tab is part of WorkshopMenuMain
		std::uint32_t folderCount{ 0 };  // folders in the tab
		std::uint32_t entryCount{ 0 };  // menus + categories created
		std::uint32_t recipeCount{ 0 };  // recovered recipes
		std::uint32_t placedHome{ 0 };  // recipes put back into the menu they belong to
		std::uint32_t placedTab{ 0 };  // recipes in the "Lost Objects" tab
		std::uint32_t overflow{ 0 };  // recipes that did not fit into the pool

		// Recipe -> where it is shown now ("Snappy HouseKit > Floors > Stairs").
		std::unordered_map<const RE::BGSConstructibleObject*, std::string> placement;
		// Where things were injected into existing menus (for the report).
		std::vector<std::string> injections;
	};

	// Turns a ScanResult into menus.
	//
	// Every recovered recipe is tagged with exactly one category from the pool
	// in LostObjects.esp. The categories are named after the original category
	// and, if large, split into groups by Grouping::Split. They are placed:
	//   1. into the sub menu whose title is the recipe's original category
	//      (a lost "Decorations" category goes back to "Decorations"),
	//   2. otherwise into the plugin's own top menu ("Snappy HouseKit"),
	//   3. otherwise into the "Lost Objects" tab, one folder per plugin.
	//
	// Everything happens in memory only. Nothing is stored in the save game;
	// the forms of other mods are only extended with references to our forms,
	// and those references are removed again on every rebuild.
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
		void RemoveInjections(const OwnedForms& a_forms);

		KeywordMap _originalKeywords;  // recipe -> categories before we tagged it
		TagMap _currentTags;  // recipe -> the keyword we added
		std::unordered_set<RE::BGSListForm*> _injectedInto;  // foreign menus we added entries to
		std::size_t _lastSignature{ 0 };
		ApplyResult _lastResult;
	};
}
