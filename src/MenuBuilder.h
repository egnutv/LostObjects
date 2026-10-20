#pragma once

#include "Scanner.h"

namespace LO
{
	enum class PlacedIn : std::uint8_t
	{
		kExistingCategory,  // sorted into a category the mod author made
		kModFolder,         // "Lost Objects" folder inside the mod's own menu
		kTab                // the "Lost Objects" tab in the main menu
	};

	struct Placed
	{
		std::string path;      // "Snappy Builds > Lost Objects > Stairs"
		PlacedIn where{ PlacedIn::kTab };
		std::string evidence;  // why it was (or was not) sorted into a category
	};

	struct ApplyResult
	{
		bool changed{ false };  // menu content differs from before
		bool tabVisible{ false };  // the "Lost Objects" tab is part of WorkshopMenuMain
		std::uint32_t folderCount{ 0 };  // folders in the tab
		std::uint32_t entryCount{ 0 };  // menus + categories created
		std::uint32_t recipeCount{ 0 };  // recovered recipes
		std::uint32_t sortedIn{ 0 };  // recipes sorted into existing categories
		std::uint32_t inModFolder{ 0 };  // recipes in a "Lost Objects" folder of their mod
		std::uint32_t inTab{ 0 };  // recipes in the "Lost Objects" tab
		std::uint32_t overflow{ 0 };  // recipes that did not fit into the pool

		std::unordered_map<const RE::BGSConstructibleObject*, Placed> placement;
		// Where things were added to existing menus (for the report).
		std::vector<std::string> injections;
	};

	// Turns a ScanResult into menus.
	//
	// Every recovered recipe gets exactly one extra category:
	//   1. an existing category of the menu that fits it (Matcher), so it
	//      shows up right next to the items it belongs to,
	//   2. otherwise a category in a "Lost Objects" folder inside the plugin's
	//      own top menu ("Snappy Builds > Lost Objects"),
	//   3. otherwise a category in the "Lost Objects" tab, one folder per plugin.
	// The categories of 2. and 3. come from the pool in LostObjects.esp, are
	// named after the original category and, if large, split into groups.
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
