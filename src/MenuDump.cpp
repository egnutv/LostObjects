#include "MenuDump.h"

#include "Grouping.h"
#include "Util.h"

namespace LO::MenuDump
{
	namespace
	{
		constexpr std::size_t MAX_DEPTH = 16;
		constexpr std::size_t MAX_LISTED = 200;  // per section in the duplicates file

		struct Occurrence
		{
			std::string path;  // "Structures > Wood > Floors"
			const RE::BGSKeyword* category{ nullptr };
			bool viaLostObjects{ false };  // reached through a menu or category of Lost Objects
		};

		std::string Clean(std::string a_text)
		{
			for (auto& c : a_text) {
				if (c == '\t' || c == '\r' || c == '\n') {
					c = ' ';
				}
			}
			return a_text;
		}

		std::string Label(const RE::TESForm* a_form)
		{
			if (!a_form) {
				return "(none)";
			}
			auto name = Util::GetFormName(a_form);
			if (name.empty()) {
				name = Util::GetEditorID(a_form);
			}
			if (name.empty()) {
				name = Util::FormIDToString(a_form->GetFormID());
			}
			return name;
		}

		std::string Describe(const RE::TESForm* a_form)
		{
			if (!a_form) {
				return "(none)";
			}
			return std::format("{} {} \"{}\" from {}", a_form->GetFormTypeString(), Util::FormIDToString(a_form->GetFormID()), Label(a_form), Util::GetFileName(a_form, 0));
		}
	}

	void Write(const OwnedForms& a_forms, std::string_view a_reason)
	{
		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler || !a_forms.IsValid()) {
			return;
		}

		// Which recipes does each category show?
		std::unordered_map<const RE::BGSKeyword*, std::vector<const RE::BGSConstructibleObject*>> recipesByCategory;
		for (const auto* recipe : dataHandler->GetFormArray<RE::BGSConstructibleObject>()) {
			if (!recipe || recipe->IsDeleted() || !recipe->createdItem) {
				continue;
			}
			for (const auto* keyword : recipe->GetRecipeKeywords()) {
				if (keyword) {
					recipesByCategory[keyword].push_back(recipe);
				}
			}
		}

		// Walk the tree. Every occurrence is recorded; cycles are cut.
		std::vector<Occurrence> categories;
		std::unordered_map<const RE::BGSListForm*, std::vector<std::string>> listPaths;

		struct Frame
		{
			const RE::BGSListForm* list;
			std::string path;
			std::vector<const RE::BGSListForm*> ancestors;
			bool viaLostObjects{ false };
		};

		std::vector<Frame> stack;
		stack.push_back({ a_forms.workshopMenuMain, std::string(), {}, false });

		while (!stack.empty()) {
			auto frame = std::move(stack.back());
			stack.pop_back();

			const bool isMainMenu = frame.list == a_forms.workshopMenuMain;
			const auto forms = frame.list->GetAllForms();

			// Push sub menus in reverse so the output keeps the menu order.
			std::vector<Frame> children;

			for (std::size_t i = 0; i < forms.size(); ++i) {
				const auto* form = forms[i];
				if (!form) {
					continue;
				}

				if (const auto* keyword = form->As<RE::BGSKeyword>()) {
					if (isMainMenu || i != 0) {
						const auto path = frame.path.empty() ? Label(keyword) : frame.path + " > " + Label(keyword);
						categories.push_back({ path, keyword, frame.viaLostObjects || a_forms.IsOwned(keyword) });
					}
				} else if (const auto* subList = form->As<RE::BGSListForm>()) {
					if (std::ranges::find(frame.ancestors, subList) != frame.ancestors.end() || frame.ancestors.size() >= MAX_DEPTH) {
						continue;  // cycle or absurd depth
					}
					const auto* title = subList->GetNthForm(0);
					const auto path = frame.path.empty() ? Label(title) : frame.path + " > " + Label(title);
					listPaths[subList].push_back(path);

					auto ancestors = frame.ancestors;
					ancestors.push_back(frame.list);
					children.push_back({ subList, path, std::move(ancestors), frame.viaLostObjects || a_forms.IsOwned(subList) });
				}
			}

			for (auto it = children.rbegin(); it != children.rend(); ++it) {
				stack.push_back(std::move(*it));
			}
		}

		// --- Menu file ---------------------------------------------------------
		const auto menuPath = F4SE::GetLogDirectoryPath() / "LostObjects_Menu.tsv";
		std::ofstream menu(menuPath, std::ios::out | std::ios::trunc | std::ios::binary);
		if (!menu.is_open()) {
			REX::LogWarning("MenuDump: could not write {}", menuPath.filename().string());
			return;
		}

		menu << "Menu path\tCategory ID\tCategory plugin\tRecipe ID\tRecipe plugin\tItem ID\tItem type\tItem name\tItem plugin\tModel\tConditions\r\n";

		std::unordered_map<const RE::BGSConstructibleObject*, std::vector<std::string>> recipePaths;
		std::unordered_map<const RE::BGSConstructibleObject*, std::pair<bool, bool>> recipeVia;  // (via Lost Objects, via normal menu)
		std::unordered_map<const RE::TESForm*, std::set<const RE::BGSConstructibleObject*>> recipesByItem;
		std::unordered_map<const RE::BGSKeyword*, std::vector<std::string>> categoryPaths;
		std::map<std::string, std::uint32_t> pathCount;
		std::size_t rowCount = 0;

		for (const auto& occurrence : categories) {
			categoryPaths[occurrence.category].push_back(occurrence.path);
			++pathCount[occurrence.path];

			const auto it = recipesByCategory.find(occurrence.category);
			if (it == recipesByCategory.end()) {
				menu << Clean(occurrence.path) << '\t' << Util::FormIDToString(occurrence.category->GetFormID()) << '\t' << Clean(Util::GetFileName(occurrence.category, 0)) << "\t(empty category)\t\t\t\t\t\t\t\r\n";
				continue;
			}

			for (const auto* recipe : it->second) {
				const auto* item = recipe->createdItem;
				recipePaths[recipe].push_back(occurrence.viaLostObjects ? occurrence.path + "  [Lost Objects]" : occurrence.path);
				auto& via = recipeVia[recipe];
				(occurrence.viaLostObjects ? via.first : via.second) = true;
				recipesByItem[item].insert(recipe);

				menu << Clean(occurrence.path) << '\t'
					 << Util::FormIDToString(occurrence.category->GetFormID()) << '\t'
					 << Clean(Util::GetFileName(occurrence.category, 0)) << '\t'
					 << Util::FormIDToString(recipe->GetFormID()) << '\t'
					 << Clean(Util::GetFileName(recipe, 0)) << '\t'
					 << Util::FormIDToString(item->GetFormID()) << '\t'
					 << item->GetFormTypeString() << '\t'
					 << Clean(Util::GetFormName(item)) << '\t'
					 << Clean(Util::GetFileName(item, 0)) << '\t'
					 << Clean(Grouping::ItemModel(recipe)) << '\t'
					 << (recipe->conditions ? "yes" : "no") << "\r\n";
				++rowCount;
			}
		}
		menu.close();

		// --- Duplicates file ---------------------------------------------------
		const auto dupPath = F4SE::GetLogDirectoryPath() / "LostObjects_Duplicates.txt";
		std::ofstream dup(dupPath, std::ios::out | std::ios::trunc);
		if (!dup.is_open()) {
			REX::LogWarning("MenuDump: could not write {}", dupPath.filename().string());
			return;
		}

		std::size_t recipesMulti = 0, recipesTabAndMenu = 0, itemsMulti = 0, categoriesMulti = 0, listsMulti = 0, namesMulti = 0;
		for (const auto& [recipe, paths] : recipePaths) {
			if (paths.size() > 1) {
				++recipesMulti;
				const auto& via = recipeVia[recipe];
				if (via.first && via.second) {
					++recipesTabAndMenu;
				}
			}
		}
		for (const auto& [item, recipes] : recipesByItem) {
			itemsMulti += recipes.size() > 1 ? 1 : 0;
		}
		for (const auto& [category, paths] : categoryPaths) {
			categoriesMulti += paths.size() > 1 ? 1 : 0;
		}
		for (const auto& [list, paths] : listPaths) {
			listsMulti += paths.size() > 1 ? 1 : 0;
		}
		for (const auto& [path, count] : pathCount) {
			namesMulti += count > 1 ? 1 : 0;
		}

		dup << "Lost Objects - duplicates in the workshop menu\n";
		dup << "==============================================\n\n";
		dup << "Plugin version : " << F4SE::GetPluginVersion().ToString<char>() << "\n";
		dup << "Scan reason    : " << a_reason << "\n\n";
		dup << "Menu paths (categories) : " << categories.size() << "\n";
		dup << "Sub menus               : " << listPaths.size() << "\n";
		dup << "Recipe placements       : " << rowCount << "\n";
		dup << "Different recipes       : " << recipePaths.size() << "\n\n";
		dup << "Recipes in more than one place            : " << recipesMulti << "\n";
		dup << "  of these via Lost Objects AND normally  : " << recipesTabAndMenu << "  (should be 0)\n";
		dup << "Items buildable through several recipes   : " << itemsMulti << "\n";
		dup << "Categories reachable through several paths: " << categoriesMulti << "\n";
		dup << "Sub menus reachable through several paths : " << listsMulti << "\n";
		dup << "Menu paths with the same name             : " << namesMulti << "\n\n";
		dup << "Full menu: LostObjects_Menu.tsv\n";
		dup << "Each section lists at most " << MAX_LISTED << " entries.\n\n";

		const auto section = [&dup](std::string_view a_title) {
			dup << "\n" << a_title << "\n" << std::string(a_title.size(), '-') << "\n";
		};

		section("Recipes shown by Lost Objects that are ALSO reachable normally");
		std::size_t listed = 0;
		for (const auto& [recipe, paths] : recipePaths) {
			const auto& via = recipeVia[recipe];
			if (!via.first || !via.second || listed++ >= MAX_LISTED) {
				continue;
			}
			dup << Describe(recipe) << " -> " << Describe(recipe->createdItem) << "\n";
			for (const auto& path : paths) {
				dup << "    " << path << "\n";
			}
		}

		section("Recipes shown in more than one place");
		listed = 0;
		for (const auto& [recipe, paths] : recipePaths) {
			if (paths.size() < 2 || listed++ >= MAX_LISTED) {
				continue;
			}
			dup << Describe(recipe) << " -> " << Label(recipe->createdItem) << "\n";
			for (const auto& path : paths) {
				dup << "    " << path << "\n";
			}
		}

		section("Items buildable through several recipes");
		listed = 0;
		for (const auto& [item, recipes] : recipesByItem) {
			if (recipes.size() < 2 || listed++ >= MAX_LISTED) {
				continue;
			}
			dup << Describe(item) << "\n";
			for (const auto* recipe : recipes) {
				const auto& paths = recipePaths[recipe];
				dup << "    " << Describe(recipe) << "  at  " << (paths.empty() ? "?" : paths.front()) << (paths.size() > 1 ? std::format(" (+{} more)", paths.size() - 1) : "") << "\n";
			}
		}

		section("Categories reachable through several paths");
		listed = 0;
		for (const auto& [category, paths] : categoryPaths) {
			if (paths.size() < 2 || listed++ >= MAX_LISTED) {
				continue;
			}
			dup << Describe(category) << "\n";
			for (const auto& path : paths) {
				dup << "    " << path << "\n";
			}
		}

		section("Sub menus reachable through several paths");
		listed = 0;
		for (const auto& [list, paths] : listPaths) {
			if (paths.size() < 2 || listed++ >= MAX_LISTED) {
				continue;
			}
			dup << Describe(list) << "\n";
			for (const auto& path : paths) {
				dup << "    " << path << "\n";
			}
		}

		section("Menu paths with the same name (different categories, same label)");
		listed = 0;
		for (const auto& [path, count] : pathCount) {
			if (count < 2 || listed++ >= MAX_LISTED) {
				continue;
			}
			dup << std::format("{:>4}x  {}\n", count, path);
		}

		REX::LogInformation("MenuDump: {} menu paths, {} recipe placements; duplicates: {} recipes ({} Lost Objects + normal), {} items, {} categories, {} sub menus",
			categories.size(), rowCount, recipesMulti, recipesTabAndMenu, itemsMulti, categoriesMulti, listsMulti);
	}
}
