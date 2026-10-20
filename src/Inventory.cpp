#include "Inventory.h"

#include "Grouping.h"
#include "Util.h"

namespace LO::Inventory
{
	namespace
	{
		// One line of the inventory.
		struct Row
		{
			std::string folder;
			std::string status;
			std::string why;
			std::string entry;
			std::string recipeID;
			std::string recipePlugin;
			std::string recipeLastPlugin;
			std::string itemID;
			std::string itemType;
			std::string itemName;
			std::string itemEditorID;
			std::string itemPlugin;
			std::string model;
			std::string sizeX, sizeY, sizeZ;
			std::string categories;
			std::string workbench;
			std::string components;
			std::string hasConditions;
			std::string itemKeywords;
		};

		// TSV cells must not contain tabs or line breaks.
		std::string Clean(std::string a_text)
		{
			for (auto& c : a_text) {
				if (c == '\t' || c == '\r' || c == '\n') {
					c = ' ';
				}
			}
			return a_text;
		}

		std::string KeywordLabel(const RE::BGSKeyword* a_keyword)
		{
			if (!a_keyword) {
				return {};
			}
			auto editorID = Util::GetEditorID(a_keyword);
			auto name = Util::GetFormName(a_keyword);
			if (editorID.empty()) {
				editorID = Util::FormIDToString(a_keyword->GetFormID());
			}
			return name.empty() ? editorID : std::format("{} \"{}\"", editorID, name);
		}

		std::string ItemKeywords(const RE::TESForm* a_form)
		{
			const auto* keywordForm = RE::DynamicCast<const RE::BGSKeywordForm*>(a_form);
			if (!keywordForm) {
				return {};
			}

			std::string result;
			for (const auto* keyword : keywordForm->GetKeywords()) {
				if (!keyword) {
					continue;
				}
				auto label = Util::GetEditorID(keyword);
				if (label.empty()) {
					label = Util::FormIDToString(keyword->GetFormID());
				}
				if (!result.empty()) {
					result += " | ";
				}
				result += label;
			}
			return result;
		}

		std::string Components(const RE::BGSConstructibleObject* a_recipe)
		{
			const auto components = a_recipe->GetComponents();
			if (!components) {
				return {};
			}

			std::string result;
			for (const auto& component : *components) {
				auto name = Util::GetFormName(component.first);
				if (name.empty() && component.first) {
					name = Util::FormIDToString(component.first->GetFormID());
				}
				if (!result.empty()) {
					result += " | ";
				}
				result += std::format("{} x{}", name, component.second.int32);
			}
			return result;
		}

		Row MakeRow(const ScanResult& a_scan, const LostRecipe& a_lost, const ApplyResult& a_apply, const std::unordered_map<const RE::TESForm*, const MenuEntry*>& a_entries)
		{
			Row row;
			const auto* recipe = a_lost.recipe;
			const auto* item = recipe->createdItem;

			// Where it is shown now.
			if (const auto placed = a_apply.placement.find(recipe); placed != a_apply.placement.end()) {
				row.folder = placed->second.path;
				row.why = placed->second.evidence;
				switch (placed->second.where) {
				case PlacedIn::kExistingCategory:
					row.status = "sorted into category";
					break;
				case PlacedIn::kModFolder:
					row.status = "Lost Objects folder of the mod";
					break;
				default:
					row.status = "Lost Objects tab";
					break;
				}
			} else {
				row.folder = "(not placed)";
				row.status = "not placed";
			}

			// Where it came from (the lost menu or category the scan found).
			const MenuEntry* entry = nullptr;
			for (const auto* category : a_lost.categories) {
				const auto it = a_scan.entryOfCategory.find(category);
				if (it != a_scan.entryOfCategory.end()) {
					if (const auto e = a_entries.find(it->second); e != a_entries.end()) {
						entry = e->second;
						break;
					}
				}
			}
			if (entry) {
				row.entry = std::format("{}{} ({})", entry->isMenu ? "[menu] " : "", entry->displayName, entry->pluginFile);
			} else if (!a_lost.categories.empty()) {
				auto name = Util::GetFormName(a_lost.categories.front());
				row.entry = std::format("{} ({})", name.empty() ? Util::GetEditorID(a_lost.categories.front()) : name, Util::GetFileName(a_lost.categories.front(), 0));
			} else {
				row.entry = "Uncategorized";
			}

			row.recipeID = Util::FormIDToString(recipe->GetFormID());
			row.recipePlugin = Util::GetFileName(recipe, 0);
			row.recipeLastPlugin = Util::GetFileName(recipe, -1);

			if (item) {
				row.itemID = Util::FormIDToString(item->GetFormID());
				row.itemType = std::string(item->GetFormTypeString());
				row.itemName = Util::GetFormName(item);
				row.itemEditorID = Util::GetEditorID(item);
				row.itemPlugin = Util::GetFileName(item, 0);
				row.model = Grouping::ItemModel(recipe);
				row.itemKeywords = ItemKeywords(item);

				if (const auto* bound = RE::DynamicCast<const RE::TESBoundObject*>(item)) {
					const auto& b = bound->boundData;
					row.sizeX = std::to_string(static_cast<int>(b.boundMax.x) - b.boundMin.x);
					row.sizeY = std::to_string(static_cast<int>(b.boundMax.y) - b.boundMin.y);
					row.sizeZ = std::to_string(static_cast<int>(b.boundMax.z) - b.boundMin.z);
				}
			}

			std::string categories;
			for (const auto* category : recipe->GetRecipeKeywords()) {
				if (!category) {
					continue;
				}
				if (!categories.empty()) {
					categories += " | ";
				}
				categories += KeywordLabel(category);
			}
			row.categories = categories;
			row.workbench = Util::GetEditorID(recipe->benchKeyword);
			row.components = Components(recipe);
			row.hasConditions = recipe->conditions ? "yes" : "no";
			return row;
		}

		// --- Pattern statistics ---------------------------------------------

		using Counter = std::map<std::string, std::uint32_t>;

		void WriteTop(std::ofstream& a_file, std::string_view a_title, const Counter& a_counter, std::size_t a_limit)
		{
			std::vector<std::pair<std::string, std::uint32_t>> sorted(a_counter.begin(), a_counter.end());
			std::ranges::sort(sorted, [](const auto& a_lhs, const auto& a_rhs) {
				return a_lhs.second != a_rhs.second ? a_lhs.second > a_rhs.second : a_lhs.first < a_rhs.first;
			});

			a_file << a_title << " (" << a_counter.size() << " different)\n";
			a_file << std::string(a_title.size(), '-') << "\n";
			for (std::size_t i = 0; i < sorted.size() && i < a_limit; ++i) {
				a_file << std::format("{:>7}  {}\n", sorted[i].second, sorted[i].first);
			}
			a_file << "\n";
		}

		std::vector<std::string> Words(std::string_view a_text)
		{
			std::vector<std::string> words;
			std::string current;
			const auto flush = [&]() {
				if (current.size() >= 3 && !std::ranges::all_of(current, [](unsigned char c) { return std::isdigit(c) != 0; })) {
					words.push_back(current);
				}
				current.clear();
			};
			for (const unsigned char c : a_text) {
				if (std::isalnum(c) || c >= 0x80) {
					current += static_cast<char>(std::tolower(c));
				} else {
					flush();
				}
			}
			flush();
			return words;
		}

		// "SetDressing\HouseKit\Wall01.nif" -> "setdressing\housekit"
		std::string ModelFolder(std::string_view a_path, std::size_t a_depth)
		{
			std::string normalized = Util::ToLower(a_path);
			std::ranges::replace(normalized, '/', '\\');
			if (normalized.starts_with("meshes\\")) {
				normalized.erase(0, 7);
			}

			std::size_t pos = 0;
			for (std::size_t depth = 0; depth < a_depth; ++depth) {
				const auto next = normalized.find('\\', pos);
				if (next == std::string::npos) {
					return depth == 0 ? std::string("(root)") : normalized.substr(0, pos == 0 ? 0 : pos - 1);
				}
				pos = next + 1;
			}
			return normalized.substr(0, pos - 1);
		}

		void WritePatterns(const std::vector<Row>& a_rows, std::string_view a_reason)
		{
			Counter statuses, nameWords, modelFolders1, modelFolders2, itemTypes, folders, entries, recipePlugins, itemKeywords, workbenches, sizeClasses;
			std::uint32_t withConditions = 0, withoutComponents = 0, withoutModel = 0, withoutName = 0;

			for (const auto& row : a_rows) {
				++statuses[row.status];
				for (auto& word : Words(row.itemName)) {
					++nameWords[word];
				}
				if (row.model.empty()) {
					++withoutModel;
				} else {
					++modelFolders1[ModelFolder(row.model, 1)];
					++modelFolders2[ModelFolder(row.model, 2)];
				}
				if (row.itemName.empty()) {
					++withoutName;
				}
				++itemTypes[row.itemType.empty() ? "(none)" : row.itemType];
				++folders[row.folder];
				++entries[row.entry];
				++recipePlugins[row.recipePlugin];
				++workbenches[row.workbench.empty() ? "(none)" : row.workbench];
				if (row.hasConditions == "yes") {
					++withConditions;
				}
				if (row.components.empty()) {
					++withoutComponents;
				}

				std::size_t start = 0;
				while (start < row.itemKeywords.size()) {
					auto end = row.itemKeywords.find(" | ", start);
					if (end == std::string::npos) {
						end = row.itemKeywords.size();
					}
					++itemKeywords[row.itemKeywords.substr(start, end - start)];
					start = end + 3;
				}

				if (!row.sizeX.empty()) {
					const auto largest = std::max({ std::abs(std::stoi(row.sizeX)), std::abs(std::stoi(row.sizeY)), std::abs(std::stoi(row.sizeZ)) });
					const char* sizeClass = largest < 32 ? "tiny (<32)" : largest < 128 ? "small (32-127)" : largest < 512 ? "medium (128-511)" : largest < 2048 ? "large (512-2047)" : "huge (2048+)";
					++sizeClasses[sizeClass];
				}
			}

			const auto path = F4SE::GetLogDirectoryPath() / "LostObjects_Patterns.txt";
			std::ofstream file(path, std::ios::out | std::ios::trunc);
			if (!file.is_open()) {
				REX::LogWarning("Inventory: could not write {}", path.string());
				return;
			}

			file << "Lost Objects - patterns in the recovered items\n";
			file << "==============================================\n\n";
			file << "Plugin version : " << F4SE::GetPluginVersion().ToString<char>() << "\n";
			file << "Game version   : " << F4SE::GetRuntimeVersion().ToString<char>() << "\n";
			file << "Scan reason    : " << a_reason << "\n";
			file << "Recipes        : " << a_rows.size() << "\n";
			file << "With conditions: " << withConditions << "\n";
			file << "No components  : " << withoutComponents << "\n";
			file << "No model       : " << withoutModel << "\n";
			file << "No item name   : " << withoutName << "\n\n";
			file << "Full data: LostObjects_Inventory.tsv\n\n";

			WriteTop(file, "What happened to them", statuses, 10);
			WriteTop(file, "Where the recipes are shown now", folders, 300);
			WriteTop(file, "Original lost categories", entries, 200);
			WriteTop(file, "Recipe plugins", recipePlugins, 100);
			WriteTop(file, "Item form types", itemTypes, 50);
			WriteTop(file, "Item sizes (largest side, game units)", sizeClasses, 10);
			WriteTop(file, "Model folders (level 1)", modelFolders1, 60);
			WriteTop(file, "Model folders (level 2)", modelFolders2, 150);
			WriteTop(file, "Words in item names", nameWords, 300);
			WriteTop(file, "Item keywords", itemKeywords, 150);
			WriteTop(file, "Workbench keywords", workbenches, 20);
		}
	}

	void Write(const ScanResult& a_scan, const ApplyResult& a_apply, std::string_view a_reason)
	{
		std::unordered_map<const RE::TESForm*, const MenuEntry*> entries;
		for (const auto& entry : a_scan.entries) {
			entries.emplace(entry.form, &entry);
		}

		std::vector<Row> rows;
		rows.reserve(a_scan.lostRecipes.size() + a_scan.alreadyBuildable.size() + a_scan.missingModel.size());
		for (const auto& lost : a_scan.lostRecipes) {
			rows.push_back(MakeRow(a_scan, lost, a_apply, entries));
		}
		for (const auto& skipped : a_scan.alreadyBuildable) {
			auto row = MakeRow(a_scan, skipped.lost, a_apply, entries);
			row.folder = skipped.shownAt.empty() ? "(already buildable)"s : skipped.shownAt;
			row.status = "skipped, already buildable";
			row.why = skipped.reason + " (bSkipAlreadyBuildable)";
			rows.push_back(std::move(row));
		}
		for (const auto& skipped : a_scan.missingModel) {
			auto row = MakeRow(a_scan, skipped.lost, a_apply, entries);
			row.folder = "(not shown)";
			row.status = "skipped, model file missing";
			row.why = skipped.reason + " (bSkipMissingModels)";
			rows.push_back(std::move(row));
		}

		std::ranges::sort(rows, [](const Row& a_lhs, const Row& a_rhs) {
			return std::tie(a_lhs.status, a_lhs.folder, a_lhs.entry, a_lhs.itemName, a_lhs.recipeID) < std::tie(a_rhs.status, a_rhs.folder, a_rhs.entry, a_rhs.itemName, a_rhs.recipeID);
		});

		const auto path = F4SE::GetLogDirectoryPath() / "LostObjects_Inventory.tsv";
		std::ofstream file(path, std::ios::out | std::ios::trunc | std::ios::binary);
		if (!file.is_open()) {
			REX::LogWarning("Inventory: could not write {}", path.string());
			return;
		}

		file << "Status\tShown at\tWhy\tOriginal category\tRecipe ID\tRecipe plugin\tRecipe last changed in\tItem ID\tItem type\tItem name\tItem editor ID\tItem plugin\tModel\tSize X\tSize Y\tSize Z\tCategories\tWorkbench\tComponents\tConditions\tItem keywords\r\n";
		for (const auto& r : rows) {
			const std::array cells{ &r.status, &r.folder, &r.why, &r.entry, &r.recipeID, &r.recipePlugin, &r.recipeLastPlugin, &r.itemID, &r.itemType, &r.itemName, &r.itemEditorID, &r.itemPlugin, &r.model, &r.sizeX, &r.sizeY, &r.sizeZ, &r.categories, &r.workbench, &r.components, &r.hasConditions, &r.itemKeywords };
			for (std::size_t i = 0; i < cells.size(); ++i) {
				if (i != 0) {
					file << '\t';
				}
				file << Clean(*cells[i]);
			}
			file << "\r\n";
		}
		file.close();

		WritePatterns(rows, a_reason);
		REX::LogInformation("Inventory: wrote {} recipes to LostObjects_Inventory.tsv and LostObjects_Patterns.txt", rows.size());
	}
}
