#include "Scanner.h"

#include "Util.h"

// -----------------------------------------------------------------------------
// How the workshop menu works (short version, details in docs/HOW_IT_WORKS.md):
//
//  * The menu is a tree of form lists (FLST) that starts at WorkshopMenuMain.
//  * A form list is a sub menu. Its entry 0 is a keyword that only gives the
//    sub menu its name and icon. (The main menu itself has no such entry.)
//  * Every other keyword in a form list is a category ("leaf").
//  * A recipe (COBJ) shows up in every category whose keyword it carries.
//
// A recipe is "lost" when none of its categories can be reached from
// WorkshopMenuMain. Typical reasons: a menu that was never injected (for
// example a broken Settlement Menu Manager setup), a menu replacer that
// dropped a category, or two mods overwriting the same vanilla form list.
// -----------------------------------------------------------------------------

namespace LO
{
	namespace
	{
		using KeywordSet = std::unordered_set<const RE::BGSKeyword*>;
		using ListSet = std::unordered_set<const RE::BGSListForm*>;

		constexpr std::array OFFICIAL_FILES{
			"fallout4.esm"sv,
			"dlcrobot.esm"sv,
			"dlcworkshop01.esm"sv,
			"dlccoast.esm"sv,
			"dlcworkshop02.esm"sv,
			"dlcworkshop03.esm"sv,
			"dlcnukaworld.esm"sv,
			"dlcultrahighresolution.esm"sv
		};

		bool IsOfficialFile(std::string_view a_fileName)
		{
			const auto lower = Util::ToLower(a_fileName);
			return std::ranges::find(OFFICIAL_FILES, std::string_view(lower)) != OFFICIAL_FILES.end();
		}

		KeywordSet GetRecipeFilterKeywords()
		{
			KeywordSet result;
			const auto typedKeywords = RE::BGSKeyword::GetTypedKeywords();
			for (auto* keyword : typedKeywords[static_cast<std::size_t>(std::to_underlying(RE::KeywordType::kRecipeFilter))]) {
				if (keyword) {
					result.insert(keyword);
				}
			}
			return result;
		}

		// Walks the menu tree from WorkshopMenuMain and collects every category
		// (leaf keyword) and every sub menu that the player can actually open.
		void CollectReachable(const RE::BGSListForm* a_main, const OwnedForms& a_forms, KeywordSet& a_leaves, ListSet& a_lists)
		{
			std::vector<std::pair<const RE::BGSListForm*, bool>> stack;
			stack.emplace_back(a_main, true);

			while (!stack.empty()) {
				const auto [list, isMainMenu] = stack.back();
				stack.pop_back();

				if (!list || !a_lists.insert(list).second) {
					continue;
				}

				const auto forms = list->GetAllForms();
				for (std::size_t i = 0; i < forms.size(); ++i) {
					auto* form = forms[i];
					if (!form || a_forms.IsOwned(form)) {
						continue;  // our own tab must never count as "reachable"
					}

					if (auto* keyword = form->As<RE::BGSKeyword>()) {
						if (isMainMenu || i != 0) {
							a_leaves.insert(keyword);
						}
					} else if (auto* subList = form->As<RE::BGSListForm>()) {
						stack.emplace_back(subList, false);
					}
				}
			}
		}

		struct MenuTreeInfo
		{
			std::vector<const RE::BGSKeyword*> leaves;  // unique category keywords below this menu
		};

		MenuTreeInfo CollectMenuTree(const RE::BGSListForm* a_root, const OwnedForms& a_forms)
		{
			MenuTreeInfo info;
			KeywordSet seenLeaves;
			ListSet seenLists;

			std::vector<const RE::BGSListForm*> stack{ a_root };
			while (!stack.empty()) {
				const auto* list = stack.back();
				stack.pop_back();

				if (!list || !seenLists.insert(list).second) {
					continue;
				}

				const auto forms = list->GetAllForms();
				for (std::size_t i = 0; i < forms.size(); ++i) {
					auto* form = forms[i];
					if (!form || a_forms.IsOwned(form)) {
						continue;
					}

					if (auto* keyword = form->As<RE::BGSKeyword>()) {
						if (i != 0 && seenLeaves.insert(keyword).second) {
							info.leaves.push_back(keyword);
						}
					} else if (auto* subList = form->As<RE::BGSListForm>()) {
						stack.push_back(subList);
					}
				}
			}

			return info;
		}
	}

	ScanResult Scanner::Run(const OwnedForms& a_forms, const Settings& a_settings, const KeywordMap& a_originalKeywords)
	{
		ScanResult result;

		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler || !a_forms.IsValid()) {
			return result;
		}

		const auto recipeFilters = GetRecipeFilterKeywords();

		// 1) What can the player reach right now?
		KeywordSet reachableLeaves;
		ListSet reachableLists;
		CollectReachable(a_forms.workshopMenuMain, a_forms, reachableLeaves, reachableLists);

		result.reachableCategoryCount = static_cast<std::uint32_t>(reachableLeaves.size());
		result.reachableMenuCount = static_cast<std::uint32_t>(reachableLists.size());

		// 2) Sort all recipes into reachable / unreachable.
		struct Candidate
		{
			RE::BGSConstructibleObject* recipe;
			std::vector<RE::BGSKeyword*> keywords;
		};

		std::unordered_map<const RE::BGSKeyword*, std::uint32_t> workbenchCounts;
		std::vector<Candidate> unreachable;

		for (auto* recipe : dataHandler->GetFormArray<RE::BGSConstructibleObject>()) {
			if (!recipe || recipe->IsDeleted() || recipe->formFlags.any(RE::FormFlags::kIgnored) || !recipe->createdItem) {
				continue;
			}

			std::vector<RE::BGSKeyword*> keywords;
			if (const auto it = a_originalKeywords.find(recipe); it != a_originalKeywords.end()) {
				keywords = it->second;
			} else {
				keywords = recipe->GetRecipeKeywords();
			}
			std::erase(keywords, nullptr);
			std::erase_if(keywords, [&a_forms](const RE::BGSKeyword* a_keyword) { return a_forms.IsOwned(a_keyword); });

			const bool reachable = std::ranges::any_of(keywords, [&reachableLeaves](const RE::BGSKeyword* a_keyword) {
				return reachableLeaves.contains(a_keyword);
			});

			if (reachable) {
				++workbenchCounts[recipe->benchKeyword];
				++result.reachableRecipeCount;
			} else {
				unreachable.push_back({ recipe, std::move(keywords) });
			}
		}

		if (result.reachableRecipeCount == 0) {
			REX::LogWarning("Scanner: the workshop menu looks empty right now, skipping this scan");
			return result;
		}

		// 3) Which workbench keyword marks a workshop (settlement) recipe?
		//    Instead of hard coding it, use the workbench keywords that the
		//    reachable workshop recipes actually use.
		const auto threshold = std::max<std::uint32_t>(5, result.reachableRecipeCount / 50);
		std::unordered_set<const RE::BGSKeyword*> workshopBenches;
		for (const auto& [bench, count] : workbenchCounts) {
			if (count >= threshold) {
				workshopBenches.insert(bench);
				const auto editorID = Util::GetEditorID(bench);
				result.workbenchKeywords.push_back(std::format("{} ({} recipes)", editorID.empty() ? "<none>"s : editorID, count));
			}
		}

		// 4) Lost recipes and the categories they use.
		std::unordered_map<RE::BGSKeyword*, std::uint32_t> lostCategories;  // category -> recipe count

		// Which plugins the lost recipes of a category come from. A single
		// category keyword is often vanilla (e.g. "Scrap") while its lost
		// recipes come from a mod, so the folder follows the recipes.
		struct Origin
		{
			std::uint32_t count{ 0 };
			std::uint32_t loadOrderKey{ 0 };
		};
		std::unordered_map<RE::BGSKeyword*, std::map<std::string, Origin>> categoryOrigins;

		for (auto& candidate : unreachable) {
			auto* recipe = candidate.recipe;
			if (!workshopBenches.contains(recipe->benchKeyword)) {
				continue;  // not a workshop recipe (chem station, armor bench, ...)
			}

			const auto definedIn = Util::GetFileName(recipe, 0);
			const auto lastChangedIn = Util::GetFileName(recipe, -1);

			if (a_settings.IsPluginIgnored(definedIn) || a_settings.IsPluginIgnored(lastChangedIn)) {
				continue;
			}

			// Optional filter: vanilla recipes that no mod touched (the base game
			// hides a few recipes on purpose).
			if (a_settings.skipUntouchedBaseGame && IsOfficialFile(lastChangedIn)) {
				continue;
			}

			// A category is usable if it is a recipe filter with a name. If the
			// game's name table is not available, names cannot be checked.
			const bool namesAvailable = NameMap::IsAvailable();
			std::vector<RE::BGSKeyword*> usable;
			for (auto* keyword : candidate.keywords) {
				if (recipeFilters.contains(keyword) && (!namesAvailable || !Util::GetFormName(keyword).empty())) {
					usable.push_back(keyword);
				}
			}

			const bool hasUsableCategory = !usable.empty();
			for (auto* keyword : usable) {
				++lostCategories[keyword];
				auto& origin = categoryOrigins[keyword][definedIn];
				++origin.count;
				origin.loadOrderKey = Util::GetLoadOrderKey(recipe);
			}

			if (!hasUsableCategory) {
				// Optional filter: recipes without a named category. Many mods use
				// them only to define scrap results (often hundreds of statics).
				if (a_settings.skipWithoutCategory) {
					++result.skippedWithoutCategory;
					continue;
				}
				result.uncategorized.push_back(recipe);
			}

			result.lostRecipes.push_back({ recipe, !hasUsableCategory });
		}

		// 5) Lost whole menus: form lists that look like a workshop sub menu
		//    (entry 0 is a category keyword), are not reachable and are not part
		//    of another such list. Restoring them keeps the mod's own structure.
		std::vector<RE::BGSListForm*> menuLists;
		ListSet menuListSet;
		for (auto* list : dataHandler->GetFormArray<RE::BGSListForm>()) {
			if (!list || list->IsDeleted() || a_forms.IsOwned(list) || reachableLists.contains(list)) {
				continue;
			}

			auto* first = list->GetNthForm(0);
			auto* firstKeyword = first ? first->As<RE::BGSKeyword>() : nullptr;
			if (!firstKeyword || !recipeFilters.contains(firstKeyword)) {
				continue;
			}

			menuLists.push_back(list);
			menuListSet.insert(list);
		}

		ListSet hasParent;
		for (auto* list : menuLists) {
			for (auto* form : list->GetAllForms()) {
				auto* subList = form ? form->As<RE::BGSListForm>() : nullptr;
				if (subList && subList != list && menuListSet.contains(subList)) {
					hasParent.insert(subList);
				}
			}
		}

		std::ranges::sort(menuLists, [](const RE::BGSListForm* a_lhs, const RE::BGSListForm* a_rhs) {
			return a_lhs->GetFormID() < a_rhs->GetFormID();
		});

		std::unordered_set<const RE::BGSKeyword*> coveredCategories;

		for (auto* list : menuLists) {
			if (hasParent.contains(list)) {
				continue;
			}

			const auto tree = CollectMenuTree(list, a_forms);

			std::uint32_t lostLeaves = 0;
			std::uint32_t recipeCount = 0;
			for (const auto* leaf : tree.leaves) {
				const auto it = lostCategories.find(const_cast<RE::BGSKeyword*>(leaf));
				if (it != lostCategories.end() && !coveredCategories.contains(leaf)) {
					++lostLeaves;
					recipeCount += it->second;
				}
			}

			const auto* titleKeyword = list->GetNthForm(0);
			auto name = Util::GetFormName(titleKeyword);
			if (name.empty() && !NameMap::IsAvailable()) {
				name = Util::GetEditorID(titleKeyword);
			}

			// Only restore the whole menu if it is mostly lost, otherwise the
			// tab would fill up with duplicates of reachable categories.
			if (lostLeaves == 0 || name.empty() || lostLeaves * 2 < tree.leaves.size()) {
				continue;
			}

			for (const auto* leaf : tree.leaves) {
				if (lostCategories.contains(const_cast<RE::BGSKeyword*>(leaf))) {
					coveredCategories.insert(leaf);
				}
			}

			result.entries.push_back(MenuEntry{
				.form = list,
				.isMenu = true,
				.displayName = std::move(name),
				.pluginFile = Util::GetFileName(list, 0),
				.loadOrderKey = Util::GetLoadOrderKey(list),
				.recipeCount = recipeCount });
		}

		// 6) Single lost categories that are not part of a restored menu.
		for (const auto& [keyword, count] : lostCategories) {
			if (coveredCategories.contains(keyword)) {
				continue;
			}

			auto displayName = Util::GetFormName(keyword);
			if (displayName.empty()) {
				displayName = Util::GetEditorID(keyword);
			}

			// Folder: the plugin that contributes most of the lost recipes.
			std::string pluginFile = Util::GetFileName(keyword, 0);
			std::uint32_t loadOrderKey = Util::GetLoadOrderKey(keyword);
			if (const auto origins = categoryOrigins.find(keyword); origins != categoryOrigins.end()) {
				std::uint32_t best = 0;
				for (const auto& [file, origin] : origins->second) {
					if (origin.count > best && !file.empty()) {
						best = origin.count;
						pluginFile = file;
						loadOrderKey = origin.loadOrderKey;
					}
				}
			}

			result.entries.push_back(MenuEntry{
				.form = keyword,
				.isMenu = false,
				.displayName = std::move(displayName),
				.pluginFile = std::move(pluginFile),
				.loadOrderKey = loadOrderKey,
				.recipeCount = count });
		}

		std::ranges::sort(result.entries, [](const MenuEntry& a_lhs, const MenuEntry& a_rhs) {
			if (a_lhs.loadOrderKey != a_rhs.loadOrderKey) {
				return a_lhs.loadOrderKey < a_rhs.loadOrderKey;
			}
			if (a_lhs.isMenu != a_rhs.isMenu) {
				return a_lhs.isMenu;  // whole menus first
			}
			if (a_lhs.displayName != a_rhs.displayName) {
				return a_lhs.displayName < a_rhs.displayName;
			}
			return a_lhs.form->GetFormID() < a_rhs.form->GetFormID();
		});

		result.valid = true;
		return result;
	}
}
