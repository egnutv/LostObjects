#include "MenuBuilder.h"

#include "Util.h"

namespace LO
{
	namespace
	{
		constexpr std::size_t MAX_FOLDER_NAME = 48;
		constexpr auto OVERFLOW_FOLDER_NAME = "Other"sv;

		// Everything that goes into one origin folder ("ByMod").
		struct Group
		{
			std::string pluginFile;
			std::uint32_t loadOrderKey{ std::numeric_limits<std::uint32_t>::max() };
			std::vector<RE::TESForm*> forms;  // recovered menus and categories
			std::vector<RE::BGSConstructibleObject*> recipes;  // recipes without a category
		};

		// Replaces the editor entries of a form list. Returns true if anything changed.
		bool SetListContents(RE::BGSListForm* a_list, const std::vector<RE::TESForm*>& a_forms)
		{
			auto& array = a_list->arrayOfForms;

			bool same = array.size() == a_forms.size();
			for (std::size_t i = 0; same && i < a_forms.size(); ++i) {
				same = array[static_cast<std::uint32_t>(i)] == a_forms[i];
			}
			if (same) {
				return false;
			}

			array.clear();
			array.reserve(static_cast<std::uint32_t>(a_forms.size()));
			for (auto* form : a_forms) {
				array.push_back(form);
			}
			return true;
		}

		std::string MakeFolderName(std::string_view a_pluginFile)
		{
			auto name = Util::StripExtension(a_pluginFile);
			if (name.empty()) {
				name = "Unknown";
			}
			if (name.size() > MAX_FOLDER_NAME) {
				name.resize(MAX_FOLDER_NAME);
			}
			return name;
		}

		// Groups recovered menus/categories and category-less recipes by the file
		// that defined them, ordered roughly like the load order.
		std::vector<Group> BuildGroups(const ScanResult& a_scan)
		{
			std::unordered_map<std::string, std::size_t> indexByFile;  // lower case file -> index
			std::vector<Group> groups;

			const auto groupFor = [&](const std::string& a_file, std::uint32_t a_order) -> Group& {
				const auto key = Util::ToLower(a_file);
				auto it = indexByFile.find(key);
				if (it == indexByFile.end()) {
					it = indexByFile.emplace(key, groups.size()).first;
					Group group;
					group.pluginFile = a_file;
					groups.push_back(std::move(group));
				}
				auto& group = groups[it->second];
				group.loadOrderKey = std::min(group.loadOrderKey, a_order);
				return group;
			};

			for (const auto& entry : a_scan.entries) {
				groupFor(entry.pluginFile, entry.loadOrderKey).forms.push_back(entry.form);
			}

			for (auto* recipe : a_scan.uncategorized) {
				groupFor(Util::GetFileName(recipe, 0), Util::GetLoadOrderKey(recipe)).recipes.push_back(recipe);
			}

			std::ranges::stable_sort(groups, [](const Group& a_lhs, const Group& a_rhs) {
				if (a_lhs.loadOrderKey != a_rhs.loadOrderKey) {
					return a_lhs.loadOrderKey < a_rhs.loadOrderKey;
				}
				return Util::ToLower(a_lhs.pluginFile) < Util::ToLower(a_rhs.pluginFile);
			});

			return groups;
		}
	}

	bool SetKeywordName(RE::BGSKeyword* a_keyword, std::string_view a_name)
	{
		if (!a_keyword) {
			return false;
		}

		auto& nameMap = RE::TESFullName::GetSparseFullNameMap();
		const auto it = nameMap.find(a_keyword);
		if (it == nameMap.end()) {
			return false;
		}

		const char* current = it->second.c_str();
		if (current && std::string_view(current) == a_name) {
			return true;
		}

		const std::string copy(a_name);
		it->second = RE::BGSLocalizedString(copy.c_str());
		return true;
	}

	void MenuBuilder::Reset()
	{
		_originalKeywords.clear();
		_currentTags.clear();
	}

	void MenuBuilder::UpdateTags(const TagMap& a_wanted)
	{
		const auto applyTag = [](RE::BGSConstructibleObject* a_recipe, const std::vector<RE::BGSKeyword*>& a_original, RE::BGSKeyword* a_tag) {
			auto tagged = a_original;
			std::erase(tagged, nullptr);
			tagged.push_back(a_tag);
			a_recipe->SetRecipeKeywords(tagged);
		};

		// Remove or change tags that are no longer right.
		for (auto it = _currentTags.begin(); it != _currentTags.end();) {
			auto* recipe = it->first;
			const auto wanted = a_wanted.find(recipe);

			if (wanted == a_wanted.end()) {
				auto original = _originalKeywords[recipe];
				recipe->SetRecipeKeywords(original);
				_originalKeywords.erase(recipe);
				it = _currentTags.erase(it);
				continue;
			}

			if (wanted->second != it->second) {
				applyTag(recipe, _originalKeywords[recipe], wanted->second);
				it->second = wanted->second;
			}
			++it;
		}

		// Tag new recipes.
		for (const auto& [recipe, tag] : a_wanted) {
			if (_currentTags.contains(recipe)) {
				continue;
			}

			auto original = recipe->GetRecipeKeywords();
			applyTag(recipe, original, tag);
			_originalKeywords.emplace(recipe, std::move(original));
			_currentTags.emplace(recipe, tag);
		}
	}

	ApplyResult MenuBuilder::Apply(const OwnedForms& a_forms, const ScanResult& a_scan, const Settings& a_settings)
	{
		ApplyResult result;
		if (!a_forms.IsValid() || !a_scan.valid) {
			return result;
		}

		std::vector<RE::TESForm*> rootContents{ a_forms.rootKeyword };
		TagMap wantedTags;

		if (a_settings.sortMode == SortMode::kByMod) {
			const auto groups = BuildGroups(a_scan);
			const auto slotCount = a_forms.slotMenus.size();

			std::size_t slot = 0;
			for (std::size_t g = 0; g < groups.size() && slot < slotCount; ++g, ++slot) {
				const bool isOverflow = slot == slotCount - 1 && groups.size() > slotCount;

				// The overflow folder collects every remaining group.
				std::vector<const Group*> members;
				if (isOverflow) {
					for (auto rest = g; rest < groups.size(); ++rest) {
						members.push_back(&groups[rest]);
					}
					g = groups.size();
				} else {
					members.push_back(&groups[g]);
				}

				auto* folderKeyword = a_forms.slotKeywords[slot];
				auto* uncategorizedKeyword = a_forms.slotUncategorizedKeywords[slot];
				const auto folderName = isOverflow ? std::string(OVERFLOW_FOLDER_NAME) : MakeFolderName(members.front()->pluginFile);

				std::vector<RE::TESForm*> contents{ folderKeyword };
				bool hasRecipes = false;
				for (const auto* member : members) {
					contents.insert(contents.end(), member->forms.begin(), member->forms.end());
					for (auto* recipe : member->recipes) {
						wantedTags[recipe] = uncategorizedKeyword;
						hasRecipes = true;
					}
				}
				if (hasRecipes) {
					contents.push_back(uncategorizedKeyword);
				}

				if (!SetKeywordName(folderKeyword, folderName)) {
					REX::LogWarning("MenuBuilder: could not rename folder {} to '{}'", slot + 1, folderName);
				}

				result.changed |= SetListContents(a_forms.slotMenus[slot], contents);
				rootContents.push_back(a_forms.slotMenus[slot]);
				++result.folderCount;
			}

			// Reset folders that are not used any more.
			for (; slot < slotCount; ++slot) {
				result.changed |= SetListContents(a_forms.slotMenus[slot], { a_forms.slotKeywords[slot] });
			}
		} else {
			for (const auto& entry : a_scan.entries) {
				rootContents.push_back(entry.form);
			}
			for (auto* recipe : a_scan.uncategorized) {
				wantedTags[recipe] = a_forms.uncategorizedKeyword;
			}
			if (!a_scan.uncategorized.empty()) {
				rootContents.push_back(a_forms.uncategorizedKeyword);
			}
			for (std::size_t slot = 0; slot < a_forms.slotMenus.size(); ++slot) {
				result.changed |= SetListContents(a_forms.slotMenus[slot], { a_forms.slotKeywords[slot] });
			}
		}

		UpdateTags(wantedTags);

		result.changed |= SetListContents(a_forms.rootMenu, rootContents);

		// Show our tab only when there is something in it.
		auto* mainMenu = a_forms.workshopMenuMain;
		const bool wantTab = rootContents.size() > 1;
		const bool hasTab = mainMenu->ContainsEditorForm(a_forms.rootMenu);
		if (wantTab && !hasTab) {
			mainMenu->arrayOfForms.push_back(a_forms.rootMenu);
			result.changed = true;
		} else if (!wantTab && hasTab) {
			mainMenu->RemoveEditorForm(a_forms.rootMenu);
			result.changed = true;
		}

		result.tabVisible = wantTab;
		result.entryCount = static_cast<std::uint32_t>(a_scan.entries.size());
		result.recipeCount = static_cast<std::uint32_t>(a_scan.lostRecipes.size());
		return result;
	}
}
