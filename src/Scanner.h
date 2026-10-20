#pragma once

#include "Forms.h"
#include "Settings.h"

namespace LO
{
	using KeywordMap = std::unordered_map<RE::BGSConstructibleObject*, std::vector<RE::BGSKeyword*>>;

	// One item that will be shown inside "Lost Objects".
	struct MenuEntry
	{
		RE::TESForm* form{ nullptr };  // a BGSListForm (whole lost menu) or a BGSKeyword (lost category)
		bool isMenu{ false };
		std::string displayName;
		std::string pluginFile;  // file that defined the menu or category
		std::uint32_t loadOrderKey{ 0 };
		std::uint32_t recipeCount{ 0 };
	};

	struct LostRecipe
	{
		RE::BGSConstructibleObject* recipe{ nullptr };
		bool uncategorized{ false };
	};

	struct ScanResult
	{
		bool valid{ false };  // false = scan aborted, keep the previous menu

		std::vector<MenuEntry> entries;
		std::vector<RE::BGSConstructibleObject*> uncategorized;  // lost recipes without a usable category
		std::vector<LostRecipe> lostRecipes;

		std::uint32_t reachableRecipeCount{ 0 };
		std::uint32_t reachableCategoryCount{ 0 };
		std::uint32_t reachableMenuCount{ 0 };
		std::uint32_t skippedWithoutCategory{ 0 };  // recipes removed by bSkipWithoutCategory=1
		std::vector<std::string> workbenchKeywords;  // for the log

		[[nodiscard]] bool Empty() const noexcept { return entries.empty() && uncategorized.empty(); }
	};

	// Read-only analysis of the workshop menu tree. Changes nothing in the game.
	class Scanner
	{
	public:
		// a_originalKeywords: recipes that Lost Objects tagged earlier, with their
		// original categories. The scan always judges recipes by their original categories.
		[[nodiscard]] static ScanResult Run(const OwnedForms& a_forms, const Settings& a_settings, const KeywordMap& a_originalKeywords);
	};
}
