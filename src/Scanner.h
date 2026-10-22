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
		std::vector<RE::BGSKeyword*> categories;  // usable (named) categories of the recipe
	};

	// A recipe the player can reach, with the reachable categories it is shown in.
	struct VisibleRecipe
	{
		RE::BGSConstructibleObject* recipe{ nullptr };
		std::vector<const RE::BGSKeyword*> categories;
	};

	// A lost recipe that is not shown, and why.
	struct SkippedRecipe
	{
		LostRecipe lost;
		std::string shownAt;  // already buildable: where the item can be built
		std::string reason;
	};

	// A category or sub menu that is put back into the menu its plugin had it
	// in (see PluginHistory): `node` goes into `list`, right behind `after`
	// (nullptr: at the end, in front of the entries scripts added).
	struct Restore
	{
		RE::BGSListForm* list{ nullptr };
		RE::TESForm* node{ nullptr };
		RE::TESForm* after{ nullptr };
		std::string path;  // "Structures > Snap'n Build > Real Estate > Stairs" (for the report)
		std::string removedBy;  // the plugin whose version of the list does not have it (or "a script")
		std::uint32_t recipeCount{ 0 };  // lost recipes that are reachable again through it
	};

	// The places Lost Objects put something back at - the scan does not count them.
	using RestoreSet = std::set<std::pair<const RE::BGSListForm*, const RE::TESForm*>>;

	struct RestoredRecipe
	{
		RE::BGSConstructibleObject* recipe{ nullptr };
		std::string path;  // the category it is shown in again
	};

	struct ScanResult
	{
		bool valid{ false };  // false = scan aborted, keep the previous menu

		std::vector<MenuEntry> entries;
		std::vector<RE::BGSConstructibleObject*> uncategorized;  // lost recipes without a usable category
		std::vector<LostRecipe> lostRecipes;  // still lost: they get a place from Lost Objects (MenuBuilder)

		// Lost recipes whose category (or its menu) is put back where its plugin had it.
		std::vector<Restore> restores;
		std::vector<RestoredRecipe> restored;

		std::uint32_t reachableRecipeCount{ 0 };
		std::uint32_t reachableCategoryCount{ 0 };
		std::uint32_t reachableMenuCount{ 0 };
		std::uint32_t skippedWithoutCategory{ 0 };  // recipes removed by bSkipWithoutCategory=1
		std::vector<SkippedRecipe> alreadyBuildable;  // skipped by bSkipAlreadyBuildable=1
		std::vector<SkippedRecipe> missingModel;      // ghost objects, skipped by bSkipMissingModels=1
		std::vector<VisibleRecipe> visible;  // reachable workshop recipes

		// Recipes that are put back where their plugin had them, with the category
		// they are shown in again. They are not reachable before Lost Objects acts,
		// but they show where the mod keeps its items - the matcher learns from
		// them like from visible recipes.
		std::vector<VisibleRecipe> restoredVisible;
		std::vector<std::string> workbenchKeywords;  // for the log

		// Category keyword -> the lost menu or category it was found in.
		std::unordered_map<const RE::BGSKeyword*, const RE::TESForm*> entryOfCategory;

		// homeOfPlugin: lower-case plugin file -> the plugin's own top menu, if it
		//               has exactly one (e.g. "Snappy Builds"). The "Lost Objects"
		//               folder of that plugin goes there.
		std::unordered_map<std::string, RE::BGSListForm*> homeOfPlugin;
		std::unordered_map<const RE::BGSListForm*, std::string> menuPath;  // reachable sub menu -> "Furniture > Containers"
		std::unordered_map<const RE::BGSKeyword*, std::string> categoryPath;  // reachable category -> "Furniture > Containers > Boxes"

		[[nodiscard]] bool Empty() const noexcept { return entries.empty() && uncategorized.empty(); }
	};

	// Read-only analysis of the workshop menu tree. Changes nothing in the game.
	class Scanner
	{
	public:
		// a_originalKeywords: recipes that Lost Objects tagged earlier, with their
		// original categories. The scan always judges recipes by their original categories.
		// a_ourRestores: what Lost Objects put back earlier - it does not count as reachable.
		[[nodiscard]] static ScanResult Run(const OwnedForms& a_forms, const Settings& a_settings, const KeywordMap& a_originalKeywords, const RestoreSet& a_ourRestores);
	};
}
