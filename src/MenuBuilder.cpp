#include "MenuBuilder.h"

#include "Grouping.h"
#include "Matcher.h"
#include "NameMap.h"
#include "Util.h"

namespace LO
{
	namespace
	{
		constexpr std::size_t MAX_FOLDER_NAME = 48;
		constexpr std::uint32_t MAX_SPLIT_DEPTH = 3;
		constexpr auto UNCATEGORIZED = "Uncategorized"sv;
		constexpr auto OVERFLOW_NAME = "More (pool full)"sv;

		std::string ShortName(std::string a_name)
		{
			if (a_name.size() > MAX_FOLDER_NAME) {
				a_name.resize(MAX_FOLDER_NAME);
			}
			return a_name;
		}

		std::string CategoryName(const RE::BGSKeyword* a_keyword)
		{
			auto name = Util::GetFormName(a_keyword);
			if (name.empty()) {
				name = Util::GetEditorID(a_keyword);
			}
			return name.empty() ? std::string(UNCATEGORIZED) : name;
		}

		std::size_t Mix(std::size_t a_seed, std::size_t a_value)
		{
			return a_seed ^ (a_value + 0x9E3779B97F4A7C15ull + (a_seed << 6) + (a_seed >> 2));
		}

		// --- What was found, grouped by plugin ----------------------------------

		struct Leaf
		{
			std::string name;
			std::vector<RE::BGSConstructibleObject*> recipes;
			bool borrowed{ false };  // the category comes from another plugin (or there is none): its name says little
		};

		// Inside a mod's "Lost Objects" folder: a borrowed category (for example
		// the base game's "Scrap" used by a mod) says nothing about its items, so
		// they are grouped by what they have in common themselves (words of the
		// names, tags, model folders - see Grouping). Small sets stay together.
		std::vector<Grouping::Group> RegroupBorrowed(const Leaf& a_leaf, const Grouping::Options& a_options)
		{
			if (a_leaf.borrowed && a_leaf.recipes.size() >= 2 * a_options.minGroupSize) {
				auto groups = Grouping::Split(a_leaf.recipes, a_options);
				if (groups.size() >= 2 && groups.size() < a_leaf.recipes.size()) {
					return groups;
				}
			}
			return { Grouping::Group{ a_leaf.name, a_leaf.recipes } };
		}

		struct LostMenu
		{
			std::string name;
			std::map<std::string, Leaf> leaves;  // lower-case name -> leaf
		};

		struct Bucket
		{
			std::string plugin;
			std::uint32_t loadOrderKey{ std::numeric_limits<std::uint32_t>::max() };
			std::map<std::string, LostMenu> menus;
			std::map<std::string, Leaf> leaves;
		};

		// --- The plan: which pool form gets which name and content ---------------

		class Planner
		{
		public:
			Planner(const OwnedForms& a_forms, const Settings& a_settings) :
				_forms(a_forms),
				_settings(a_settings)
			{
				_options.maxGroups = a_settings.maxGroups;
				_options.minGroupSize = a_settings.minGroupSize;
				_options.chunkSize = std::max<std::size_t>(a_settings.minGroupSize, a_settings.splitThreshold * 3 / 4);
			}

			RE::BGSListForm* AllocMenu(const std::string& a_name)
			{
				// The last menu of the pool is never used, so a folder is always possible.
				if (_menuIndex + 1 >= _forms.menus.size()) {
					return nullptr;
				}
				auto* menu = _forms.menus[_menuIndex];
				auto* title = _forms.menuTitles[_menuIndex];
				++_menuIndex;
				names.emplace_back(title, ShortName(a_name));
				contents[menu] = { title };
				menuOrder.push_back(menu);
				return menu;
			}

			RE::BGSKeyword* AllocLeaf(const std::string& a_name)
			{
				// The last leaf is reserved for the overflow category.
				if (_leafIndex + 1 >= _forms.leaves.size()) {
					return nullptr;
				}
				auto* leaf = _forms.leaves[_leafIndex++];
				names.emplace_back(leaf, ShortName(a_name));
				return leaf;
			}

			void Add(RE::BGSListForm* a_menu, RE::TESForm* a_child)
			{
				if (a_menu && a_child) {
					contents[a_menu].push_back(a_child);
				}
			}

			// A category: one leaf, or - if large - a sub menu of groups.
			RE::TESForm* BuildCategory(const std::string& a_name, const std::vector<RE::BGSConstructibleObject*>& a_recipes, const std::string& a_path, std::uint32_t a_depth = 0)
			{
				if (a_recipes.empty()) {
					return nullptr;
				}

				const auto path = a_path.empty() ? a_name : a_path + " > " + a_name;

				if (a_recipes.size() > _settings.splitThreshold && a_depth < MAX_SPLIT_DEPTH) {
					auto groups = Grouping::Split(a_recipes, _options);
					if (groups.size() >= 2) {
						if (auto* menu = AllocMenu(a_name)) {
							for (const auto& group : groups) {
								Add(menu, BuildCategory(group.label, group.recipes, path, a_depth + 1));
							}
							return menu;
						}
					}
				}

				auto* leaf = AllocLeaf(a_name);
				if (!leaf) {
					overflow.insert(overflow.end(), a_recipes.begin(), a_recipes.end());
					return nullptr;
				}
				for (auto* recipe : a_recipes) {
					tags[recipe] = leaf;
					placement[recipe] = { path, where, {} };
				}
				return leaf;
			}

			RE::TESForm* BuildMenu(const LostMenu& a_menu, const std::string& a_path, const std::string& a_name)
			{
				if (a_menu.leaves.size() == 1) {
					// A menu with a single category: no extra level.
					const auto& leaf = a_menu.leaves.begin()->second;
					return BuildCategory(a_name == leaf.name ? a_name : a_name + " - " + leaf.name, leaf.recipes, a_path);
				}

				auto* menu = AllocMenu(a_name);
				if (!menu) {
					for (const auto& [key, leaf] : a_menu.leaves) {
						overflow.insert(overflow.end(), leaf.recipes.begin(), leaf.recipes.end());
					}
					return nullptr;
				}
				const auto path = a_path.empty() ? a_name : a_path + " > " + a_name;
				for (const auto& [key, leaf] : a_menu.leaves) {
					Add(menu, BuildCategory(leaf.name, leaf.recipes, path));
				}
				return menu;
			}

			void Inject(RE::BGSListForm* a_target, RE::TESForm* a_node, const std::string& a_description)
			{
				if (a_target && a_node) {
					injections.emplace_back(a_target, a_node);
					injectionLog.push_back(a_description);
				}
			}

			[[nodiscard]] std::size_t Signature() const
			{
				std::size_t seed = 0;
				for (const auto& [form, name] : names) {
					seed = Mix(seed, form->GetFormID());
					seed = Mix(seed, std::hash<std::string>{}(name));
				}
				for (auto* menu : menuOrder) {
					seed = Mix(seed, menu->GetFormID());
					for (const auto* child : contents.at(menu)) {
						seed = Mix(seed, child->GetFormID());
					}
				}
				for (const auto& [target, node] : injections) {
					seed = Mix(seed, target->GetFormID());
					seed = Mix(seed, node->GetFormID());
				}
				std::vector<std::pair<std::uint32_t, std::uint32_t>> sortedTags;
				for (const auto& [recipe, leaf] : tags) {
					sortedTags.emplace_back(recipe->GetFormID(), leaf->GetFormID());
				}
				std::ranges::sort(sortedTags);
				for (const auto& [recipe, leaf] : sortedTags) {
					seed = Mix(seed, recipe);
					seed = Mix(seed, leaf);
				}
				return Mix(seed, overflow.size());
			}

			[[nodiscard]] std::size_t MenusUsed() const noexcept { return _menuIndex; }
			[[nodiscard]] std::size_t LeavesUsed() const noexcept { return _leafIndex; }

			std::vector<std::pair<RE::TESForm*, std::string>> names;
			std::unordered_map<RE::BGSListForm*, std::vector<RE::TESForm*>> contents;
			std::vector<RE::BGSListForm*> menuOrder;
			std::vector<std::pair<RE::BGSListForm*, RE::TESForm*>> injections;
			std::vector<std::string> injectionLog;
			std::unordered_map<RE::BGSConstructibleObject*, RE::BGSKeyword*> tags;
			std::unordered_map<const RE::BGSConstructibleObject*, Placed> placement;
			std::vector<RE::BGSConstructibleObject*> overflow;
			PlacedIn where{ PlacedIn::kTab };  // where the next categories go (for the logs)

		private:
			const OwnedForms& _forms;
			const Settings& _settings;
			Grouping::Options _options;
			std::size_t _menuIndex{ 0 };
			std::size_t _leafIndex{ 0 };
		};

		// Fallout 4 keeps the forms that scripts added to a list (Papyrus
		// FormList.AddForm, used by Settlement Menu Manager and many mods) at the
		// END of arrayOfForms; scriptAddedFormCount says how many. The save game
		// stores exactly that tail - the last scriptAddedFormCount entries. So
		// Lost Objects must never put anything behind them: the save would then
		// store our entry instead of the first script-added one, and that menu
		// would be gone after loading. Our entries always go in front of the tail.
		std::uint32_t ScriptTail(const RE::BGSListForm* a_list)
		{
			return std::min(a_list->scriptAddedFormCount, a_list->arrayOfForms.size());
		}

		// Replaces the entries of a form list. Returns true if anything changed.
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

		// Removes our forms from the script-added tail of a list and corrects
		// the count, so the next save stores the right forms. Returns true if
		// anything was removed.
		bool StripFromScriptTail(const OwnedForms& a_forms, RE::BGSListForm* a_list)
		{
			if (!a_list) {
				return false;
			}
			auto& array = a_list->arrayOfForms;
			const auto tailStart = array.size() - ScriptTail(a_list);
			std::vector<RE::TESForm*> kept;
			kept.reserve(array.size());
			std::uint32_t removed = 0;
			for (std::uint32_t i = 0; i < array.size(); ++i) {
				if (i >= tailStart && a_forms.IsOwned(array[i])) {
					++removed;
				} else {
					kept.push_back(array[i]);
				}
			}
			if (removed == 0) {
				return false;
			}
			SetListContents(a_list, kept);
			a_list->scriptAddedFormCount -= removed;
			a_list->AddChange(RE::BGSListForm::ChangeFlags::kAddedForm);
			REX::LogWarning("MenuBuilder: repaired {} - {} entry/entries of Lost Objects were stored among the script-added forms by an older version ({} script-added forms left)",
				Util::FormIDToString(a_list->GetFormID()), removed, a_list->scriptAddedFormCount);
			return true;
		}

		// Adds a_form to a list of another mod (or the base game), in front of
		// the forms that scripts added (see ScriptTail).
		void InsertBeforeScriptForms(RE::BGSListForm* a_list, RE::TESForm* a_form)
		{
			std::vector<RE::TESForm*> forms(a_list->arrayOfForms.begin(), a_list->arrayOfForms.end());
			const auto at = forms.size() - ScriptTail(a_list);
			forms.insert(forms.begin() + static_cast<std::ptrdiff_t>(at), a_form);
			SetListContents(a_list, forms);  // scriptAddedFormCount stays: the tail is still the same forms
		}

		// Puts a_node into a_list right behind a_after (if it is there), else at
		// the end - always in front of the forms that scripts added.
		void InsertAfter(RE::BGSListForm* a_list, RE::TESForm* a_node, const RE::TESForm* a_after)
		{
			std::vector<RE::TESForm*> forms(a_list->arrayOfForms.begin(), a_list->arrayOfForms.end());
			const auto tailStart = forms.size() - ScriptTail(a_list);
			auto at = tailStart;
			if (a_after) {
				for (std::size_t i = 0; i < tailStart; ++i) {
					if (forms[i] == a_after) {
						at = i + 1;
						break;
					}
				}
			}
			forms.insert(forms.begin() + static_cast<std::ptrdiff_t>(at), a_node);
			SetListContents(a_list, forms);  // scriptAddedFormCount stays: the tail is still the same forms
		}

		bool ListHas(const RE::BGSListForm* a_list, const RE::TESForm* a_node)
		{
			return std::ranges::find(a_list->arrayOfForms, a_node) != a_list->arrayOfForms.end();
		}

		std::vector<Bucket> CollectBuckets(const ScanResult& a_scan, const std::unordered_set<const RE::BGSConstructibleObject*>& a_sorted)
		{
			std::unordered_map<const RE::TESForm*, const MenuEntry*> entryByForm;
			for (const auto& entry : a_scan.entries) {
				entryByForm.emplace(entry.form, &entry);
			}

			std::map<std::string, Bucket> byPlugin;
			for (const auto& lost : a_scan.lostRecipes) {
				auto* recipe = lost.recipe;
				if (a_sorted.contains(recipe)) {
					continue;  // already sorted into an existing category
				}
				const auto plugin = Util::GetFileName(recipe, 0);
				auto& bucket = byPlugin[Util::ToLower(plugin)];
				bucket.plugin = plugin;
				bucket.loadOrderKey = std::min(bucket.loadOrderKey, Util::GetLoadOrderKey(recipe));

				// The first category that the scan knows about decides where it goes.
				const RE::BGSKeyword* category = nullptr;
				const MenuEntry* entry = nullptr;
				for (const auto* candidate : lost.categories) {
					const auto it = a_scan.entryOfCategory.find(candidate);
					if (it != a_scan.entryOfCategory.end()) {
						category = candidate;
						const auto e = entryByForm.find(it->second);
						entry = e != entryByForm.end() ? e->second : nullptr;
						break;
					}
				}

				const auto name = category ? CategoryName(category) : std::string(UNCATEGORIZED);
				const auto key = Util::ToLower(name);

				Leaf* leaf = nullptr;
				if (entry && entry->isMenu) {
					auto& menu = bucket.menus[Util::ToLower(entry->displayName)];
					menu.name = entry->displayName;
					leaf = &menu.leaves[key];
				} else {
					leaf = &bucket.leaves[key];
				}
				if (leaf->name.empty()) {
					leaf->name = name;
					leaf->borrowed = !category || Util::ToLower(Util::GetFileName(category, 0)) != Util::ToLower(plugin);
				}
				leaf->recipes.push_back(recipe);
			}

			std::vector<Bucket> buckets;
			for (auto& [key, bucket] : byPlugin) {
				buckets.push_back(std::move(bucket));
			}
			std::ranges::sort(buckets, [](const Bucket& a_lhs, const Bucket& a_rhs) {
				return a_lhs.loadOrderKey != a_rhs.loadOrderKey ? a_lhs.loadOrderKey < a_rhs.loadOrderKey : a_lhs.plugin < a_rhs.plugin;
			});
			return buckets;
		}

		std::string PathOf(const ScanResult& a_scan, const RE::BGSListForm* a_list)
		{
			const auto it = a_scan.menuPath.find(a_list);
			return it != a_scan.menuPath.end() ? it->second : Util::FormIDToString(a_list->GetFormID());
		}
	}

	bool MenuState::SameAs(const MenuState& a_other) const
	{
		const auto sorted = [](auto a_items) {
			std::ranges::sort(a_items, [](const auto& a_lhs, const auto& a_rhs) { return a_lhs.first->GetFormID() < a_rhs.first->GetFormID(); });
			return a_items;
		};
		const auto sortedPairs = [](auto a_items) {
			std::ranges::sort(a_items, [](const auto& a_lhs, const auto& a_rhs) {
				return a_lhs.first->GetFormID() != a_rhs.first->GetFormID() ? a_lhs.first->GetFormID() < a_rhs.first->GetFormID() : a_lhs.second->GetFormID() < a_rhs.second->GetFormID();
			});
			return a_items;
		};
		return tabVisible == a_other.tabVisible &&
		       restores == a_other.restores &&
		       merges == a_other.merges &&
		       sorted(names) == sorted(a_other.names) &&
		       sorted(contents) == sorted(a_other.contents) &&
		       sortedPairs(injections) == sortedPairs(a_other.injections) &&
		       sorted(tags) == sorted(a_other.tags);
	}

	RestoreSet MenuBuilder::RestoredPairs() const
	{
		RestoreSet pairs;
		for (const auto& restored : _insertedRestores) {
			pairs.emplace(restored.list, restored.node);
		}
		return pairs;
	}

	// Takes out again what we put back last time (only in front of the
	// forms that scripts added - we never put anything behind them).
	void MenuBuilder::RemoveRestores()
	{
		for (const auto& restored : _insertedRestores) {
			auto& array = restored.list->arrayOfForms;
			const auto tailStart = array.size() - ScriptTail(restored.list);
			std::vector<RE::TESForm*> kept;
			bool removed = false;
			for (std::uint32_t i = 0; i < array.size(); ++i) {
				if (!removed && i < tailStart && array[i] == restored.node) {
					removed = true;
				} else {
					kept.push_back(array[i]);
				}
			}
			if (removed) {
				SetListContents(restored.list, kept);
			}
		}
		_insertedRestores.clear();
	}

	// Merges menus of another mod: puts entries into a list and takes entries
	// out of a list - always only in front of the forms that scripts added, so
	// the save game never stores anything of this (see ScriptTail).
	void MenuBuilder::ApplyMerges(const std::vector<MergeOp>& a_merges)
	{
		for (const auto& op : a_merges) {
			auto& array = op.list->arrayOfForms;
			const auto tailStart = static_cast<std::size_t>(array.size() - ScriptTail(op.list));
			if (op.kind == MergeOp::Kind::kInsert) {
				if (!ListHas(op.list, op.node)) {
					InsertBeforeScriptForms(op.list, op.node);
					_appliedMerges.push_back({ op, 0 });
				}
				continue;
			}
			std::vector<RE::TESForm*> forms(array.begin(), array.end());
			for (std::size_t i = 0; i < tailStart; ++i) {
				if (forms[i] == op.node) {
					forms.erase(forms.begin() + static_cast<std::ptrdiff_t>(i));
					SetListContents(op.list, forms);  // scriptAddedFormCount stays: the tail is still the same forms
					_appliedMerges.push_back({ op, i });
					break;
				}
			}
		}
	}

	// Undoes what ApplyMerges changed, in reverse order.
	void MenuBuilder::RemoveMerges()
	{
		for (auto it = _appliedMerges.rbegin(); it != _appliedMerges.rend(); ++it) {
			const auto& op = it->op;
			auto& array = op.list->arrayOfForms;
			std::vector<RE::TESForm*> forms(array.begin(), array.end());
			const auto tailStart = forms.size() - static_cast<std::size_t>(ScriptTail(op.list));
			if (op.kind == MergeOp::Kind::kInsert) {
				for (std::size_t i = tailStart; i-- > 0;) {
					if (forms[i] == op.node) {
						forms.erase(forms.begin() + static_cast<std::ptrdiff_t>(i));
						SetListContents(op.list, forms);
						break;
					}
				}
			} else if (std::ranges::find(forms, op.node) == forms.end()) {
				const auto at = std::min(it->index, tailStart);
				forms.insert(forms.begin() + static_cast<std::ptrdiff_t>(at), op.node);
				SetListContents(op.list, forms);
			}
		}
		_appliedMerges.clear();
	}

	void MenuBuilder::SuspendMerges()
	{
		RemoveMerges();
	}

	void MenuBuilder::ResumeMerges()
	{
		if (_appliedMerges.empty()) {
			ApplyMerges(_state.merges);
		}
	}

	void MenuBuilder::Reset()
	{
		_originalKeywords.clear();
		_currentTags.clear();
		_injectedInto.clear();
		_insertedRestores.clear();
		_appliedMerges.clear();
		_previewGiven.clear();
		_state = {};
		_lastSignature = 0;
		_lastResult = {};
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

	// Items that were never meant for the build menu (scrap definitions of
	// clutter, containers, ...) often have no preview transform. The menu then
	// shows the 3D preview, but the thumbnail in the bar stays empty. They get
	// the transform that visible items of the same form type use most often.
	// Only items without any transform are touched; in memory only.
	std::uint32_t MenuBuilder::AddPreviewTransforms(const ScanResult& a_scan, const ApplyResult& a_result)
	{
		std::unordered_map<RE::FormType, std::unordered_map<RE::BGSTransform*, std::uint32_t>> byType;
		std::unordered_map<RE::BGSTransform*, std::uint32_t> overall;
		for (const auto& visible : a_scan.visible) {
			auto* item = RE::DynamicCast<RE::TESBoundObject*>(visible.recipe->createdItem);
			auto* transform = item ? item->previewTransform.transform : nullptr;
			if (transform) {
				++byType[item->GetFormType()][transform];
				++overall[transform];
			}
		}
		const auto mostCommon = [](const std::unordered_map<RE::BGSTransform*, std::uint32_t>& a_counts) -> RE::BGSTransform* {
			RE::BGSTransform* best = nullptr;
			std::uint32_t bestCount = 0;
			for (const auto& [transform, count] : a_counts) {
				if (count > bestCount || (count == bestCount && best && transform->GetFormID() < best->GetFormID())) {
					best = transform;
					bestCount = count;
				}
			}
			return best;
		};
		auto* fallback = mostCommon(overall);
		if (!fallback) {
			return 0;
		}

		std::uint32_t added = 0;
		for (const auto& [recipe, placed] : a_result.placement) {
			std::vector<const RE::TESForm*> variants;
			Util::CollectVariants(recipe->createdItem, variants);
			for (const auto* variant : variants) {
				auto* item = RE::DynamicCast<RE::TESBoundObject*>(const_cast<RE::TESForm*>(variant));
				if (!item || item->previewTransform.transform || _previewGiven.contains(item)) {
					continue;
				}
				const auto type = byType.find(item->GetFormType());
				auto* transform = type != byType.end() ? mostCommon(type->second) : nullptr;
				item->previewTransform.transform = transform ? transform : fallback;
				_previewGiven.emplace(item, item->previewTransform.transform);
				++added;
			}
		}
		if (added > 0) {
			REX::LogInformation("MenuBuilder: {} recovered items had no preview transform (empty thumbnail) and got one", added);
		}
		return added;
	}

	void MenuBuilder::RemoveInjections(const OwnedForms& a_forms)
	{
		const auto strip = [&a_forms](RE::BGSListForm* a_list) {
			StripFromScriptTail(a_forms, a_list);  // entries an older version left among the script-added forms

			auto& array = a_list->arrayOfForms;
			std::vector<RE::TESForm*> kept;
			kept.reserve(array.size());
			bool removed = false;
			for (auto* form : array) {
				if (a_forms.IsOwned(form)) {
					removed = true;
				} else {
					kept.push_back(form);
				}
			}
			if (removed) {
				SetListContents(a_list, kept);  // only entries in front of the tail are left to remove
			}
		};

		for (auto* list : _injectedInto) {
			strip(list);
		}
		_injectedInto.clear();
		strip(a_forms.workshopMenuMain);
	}

	ApplyResult MenuBuilder::Apply(const OwnedForms& a_forms, const ScanResult& a_scan, const Settings& a_settings)
	{
		if (!a_forms.IsValid() || !a_scan.valid) {
			return {};
		}

		// --- 1. Plan ------------------------------------------------------------
		Planner plan(a_forms, a_settings);
		const bool home = a_settings.placement == Placement::kHome;
		const auto rootName = std::string(Forms::RootName);

		// 1a. Sort into existing categories where the data clearly says so.
		std::unordered_set<const RE::BGSConstructibleObject*> sorted;
		std::unordered_map<const RE::BGSConstructibleObject*, std::string> notSortedWhy;
		if (home && a_settings.sortIntoCategories && !a_scan.lostRecipes.empty()) {
			const Matcher::Index index(a_scan);
			REX::LogInformation("Matcher: learned from {} recipes ({} visible, {} put back where their mod had them) in {} categories",
				index.ReferenceCount(), a_scan.visible.size(), a_scan.restoredVisible.size(), index.CategoryCount());
			for (const auto& lost : a_scan.lostRecipes) {
				auto match = index.Find(lost, a_settings.minConfidence);
				const auto pathOf = [&a_scan](const RE::BGSKeyword* a_category) {
					const auto it = a_scan.categoryPath.find(a_category);
					return it != a_scan.categoryPath.end() ? it->second : Util::FormIDToString(a_category->GetFormID());
				};
				if (match.category) {
					plan.tags[lost.recipe] = const_cast<RE::BGSKeyword*>(match.category);
					plan.placement[lost.recipe] = { pathOf(match.category), PlacedIn::kExistingCategory, std::move(match.evidence) };
					sorted.insert(lost.recipe);
				} else {
					notSortedWhy[lost.recipe] = match.guess ? std::format("{} - best guess: {}", match.evidence, pathOf(match.guess)) : match.evidence;
				}
			}
		}

		// 1b. The rest: a "Lost Objects" folder in the mod's own menu, or the tab.
		const auto buckets = CollectBuckets(a_scan, sorted);
		Grouping::Options groupOptions;
		groupOptions.maxGroups = a_settings.maxGroups;
		groupOptions.minGroupSize = a_settings.minGroupSize;
		groupOptions.chunkSize = std::max<std::size_t>(a_settings.minGroupSize, a_settings.splitThreshold * 3 / 4);

		struct TabItem
		{
			const Bucket* bucket;
			std::vector<std::pair<std::string, const LostMenu*>> menus;
			std::vector<const Leaf*> leaves;
		};
		std::vector<TabItem> tab;

		// Which plugins go into which menu. Several plugins can share one home
		// (an add-on uses the menu of its main mod): they share ONE "Lost
		// Objects" folder there; the menu's own plugin goes in directly, the
		// others get a sub folder named after their plugin.
		std::vector<std::pair<RE::BGSListForm*, std::vector<const Bucket*>>> homes;
		for (const auto& bucket : buckets) {
			RE::BGSListForm* homeMenu = nullptr;
			if (home) {
				const auto it = a_scan.homeOfPlugin.find(Util::ToLower(bucket.plugin));
				homeMenu = it != a_scan.homeOfPlugin.end() ? it->second : nullptr;
			}

			if (homeMenu) {
				auto existing = std::ranges::find(homes, homeMenu, &std::pair<RE::BGSListForm*, std::vector<const Bucket*>>::first);
				if (existing == homes.end()) {
					homes.emplace_back(homeMenu, std::vector<const Bucket*>{});
					existing = std::prev(homes.end());
				}
				existing->second.push_back(&bucket);
				continue;
			}

			TabItem item{ &bucket, {}, {} };
			for (const auto& [key, leaf] : bucket.leaves) {
				item.leaves.push_back(&leaf);
			}
			for (const auto& [key, menu] : bucket.menus) {
				item.menus.emplace_back(menu.name, &menu);
			}
			if (!item.leaves.empty() || !item.menus.empty()) {
				tab.push_back(std::move(item));
			}
		}

		plan.where = PlacedIn::kModFolder;
		for (const auto& [homeMenu, members] : homes) {
			const auto folderPath = PathOf(a_scan, homeMenu) + " > " + rootName;
			auto* folder = plan.AllocMenu(rootName);
			if (!folder) {
				for (const auto* bucket : members) {
					for (const auto& [key, leaf] : bucket->leaves) {
						plan.overflow.insert(plan.overflow.end(), leaf.recipes.begin(), leaf.recipes.end());
					}
					for (const auto& [key, menu] : bucket->menus) {
						for (const auto& [leafKey, leaf] : menu.leaves) {
							plan.overflow.insert(plan.overflow.end(), leaf.recipes.begin(), leaf.recipes.end());
						}
					}
				}
				continue;
			}

			const auto owner = Util::ToLower(Util::GetFileName(homeMenu, 0));
			std::size_t count = 0;
			std::string plugins;
			for (const auto* bucket : members) {
				RE::BGSListForm* target = folder;
				std::string path = folderPath;
				if (members.size() > 1 && Util::ToLower(bucket->plugin) != owner) {
					const auto stem = Util::StripExtension(bucket->plugin.empty() ? "Unknown"s : bucket->plugin);
					if (auto* sub = plan.AllocMenu(stem)) {
						plan.Add(folder, sub);
						target = sub;
						path = folderPath + " > " + stem;
					}
				}

				for (const auto& [key, leaf] : bucket->leaves) {
					for (const auto& group : RegroupBorrowed(leaf, groupOptions)) {
						plan.Add(target, plan.BuildCategory(group.label, group.recipes, path));
					}
					count += leaf.recipes.size();
				}
				for (const auto& [key, menu] : bucket->menus) {
					plan.Add(target, plan.BuildMenu(menu, path, menu.name));
					for (const auto& [leafKey, leaf] : menu.leaves) {
						count += leaf.recipes.size();
					}
				}
				plugins += (plugins.empty() ? "" : ", ") + bucket->plugin;
			}

			if (plan.contents[folder].size() > 1) {
				plan.Inject(homeMenu, folder, std::format("{} ({} recipes from {})", folderPath, count, plugins));
			}
		}

		// The tab.
		plan.where = PlacedIn::kTab;
		std::vector<RE::TESForm*> rootContents{ a_forms.rootKeyword };
		std::uint32_t folderCount = 0;

		for (const auto& item : tab) {
			const auto stem = Util::StripExtension(item.bucket->plugin.empty() ? "Unknown"s : item.bucket->plugin);

			// Borrowed categories (the base game's "Scrap" used by a DLC, ...) are
			// grouped by what their items have in common, like in the mod folders.
			std::vector<Grouping::Group> groups;
			for (const auto* leaf : item.leaves) {
				for (auto& group : RegroupBorrowed(*leaf, groupOptions)) {
					groups.push_back(std::move(group));
				}
			}
			const auto count = groups.size() + item.menus.size();
			const bool byMod = a_settings.sortMode == SortMode::kByMod;

			RE::BGSListForm* folder = nullptr;
			std::string path = rootName;
			std::string prefix;
			if (byMod && count > 1) {
				folder = plan.AllocMenu(stem);
				path = rootName + " > " + stem;
				if (folder) {
					rootContents.push_back(folder);
					++folderCount;
				}
			} else if (byMod) {
				prefix = stem + " - ";  // a single entry: no folder level, the name tells the origin
			}

			const auto place = [&](RE::TESForm* a_node) {
				if (!a_node) {
					return;
				}
				if (folder) {
					plan.Add(folder, a_node);
				} else {
					rootContents.push_back(a_node);
				}
			};

			for (const auto& group : groups) {
				place(plan.BuildCategory(prefix + group.label, group.recipes, path));
			}
			for (const auto& [name, menu] : item.menus) {
				place(plan.BuildMenu(*menu, path, prefix + name));
			}
		}

		// Recipes that did not fit into the pool.
		RE::BGSKeyword* overflowLeaf = nullptr;
		if (!plan.overflow.empty()) {
			overflowLeaf = a_forms.leaves.back();
			plan.names.emplace_back(overflowLeaf, std::string(OVERFLOW_NAME));
			for (auto* recipe : plan.overflow) {
				plan.tags[recipe] = overflowLeaf;
				plan.placement[recipe] = { rootName + " > " + std::string(OVERFLOW_NAME), PlacedIn::kTab, {} };
			}
			rootContents.push_back(overflowLeaf);
			REX::LogWarning("MenuBuilder: {} recipes did not fit into the menu pool and were put into '{}'", plan.overflow.size(), OVERFLOW_NAME);
		}

		std::size_t signature = plan.Signature();
		for (const auto* form : rootContents) {
			signature = Mix(signature, form->GetFormID());
		}
		for (const auto& restore : a_scan.restores) {
			signature = Mix(signature, restore.list->GetFormID());
			signature = Mix(signature, restore.node->GetFormID());
			signature = Mix(signature, restore.after ? restore.after->GetFormID() : 0);
		}
		for (const auto& merge : a_scan.mergeOps) {
			signature = Mix(signature, merge.kind == MergeOp::Kind::kInsert ? 1u : 2u);
			signature = Mix(signature, merge.list->GetFormID());
			signature = Mix(signature, merge.node->GetFormID());
		}

		ApplyResult result;
		result.tabVisible = rootContents.size() > 1;
		result.folderCount = folderCount;
		result.entryCount = static_cast<std::uint32_t>(plan.MenusUsed() + plan.LeavesUsed());
		result.recipeCount = static_cast<std::uint32_t>(a_scan.lostRecipes.size() + a_scan.restored.size());
		for (const auto& restored : a_scan.restored) {
			plan.placement[restored.recipe] = { restored.path, PlacedIn::kOriginalPlace, {} };
		}
		for (const auto& restore : a_scan.restores) {
			result.restores.push_back(std::format("{} - removed by {} ({} recipe(s))", restore.path, restore.removedBy, restore.recipeCount));
		}
		result.overflow = static_cast<std::uint32_t>(plan.overflow.size());
		result.placement = plan.placement;
		result.injections = plan.injectionLog;
		for (auto& [recipe, placed] : result.placement) {
			if (const auto why = notSortedWhy.find(recipe); why != notSortedWhy.end()) {
				placed.evidence = why->second;
			}
			switch (placed.where) {
			case PlacedIn::kOriginalPlace:
				++result.restoredInPlace;
				break;
			case PlacedIn::kExistingCategory:
				++result.sortedIn;
				break;
			case PlacedIn::kModFolder:
				++result.inModFolder;
				break;
			default:
				++result.inTab;
				break;
			}
		}

		// Another mod may have reset one of the menus we added to (Papyrus Revert()).
		// A save written by Lost Objects 0.10.0 or older can bring our entries
		// back as script-added forms (see ScriptTail). Take them out first.
		bool repaired = StripFromScriptTail(a_forms, a_forms.workshopMenuMain);
		for (const auto& [target, node] : plan.injections) {
			repaired = StripFromScriptTail(a_forms, target) || repaired;
		}
		for (auto* list : _injectedInto) {
			repaired = StripFromScriptTail(a_forms, list) || repaired;
		}

		const bool intact = !repaired && std::ranges::all_of(plan.injections, [](const auto& a_injection) {
			return a_injection.first->ContainsEditorForm(a_injection.second);
		}) && std::ranges::all_of(_insertedRestores, [](const MenuState::Restored& a_restored) {
			return ListHas(a_restored.list, a_restored.node);
		}) && std::ranges::all_of(_appliedMerges, [](const AppliedMerge& a_merge) {
			return ListHas(a_merge.op.list, a_merge.op.node) == (a_merge.op.kind == MergeOp::Kind::kInsert);
		}) && (!result.tabVisible || a_forms.workshopMenuMain->ContainsEditorForm(a_forms.rootMenu));

		MenuState state;
		state.names = plan.names;
		for (auto* menu : plan.menuOrder) {
			state.contents.emplace_back(menu, plan.contents.at(menu));
		}
		state.contents.emplace_back(a_forms.rootMenu, rootContents);
		state.injections = plan.injections;
		for (const auto& restore : a_scan.restores) {
			state.restores.push_back({ restore.list, restore.node, restore.after });
		}
		state.merges = a_scan.mergeOps;
		for (const auto& [recipe, tag] : plan.tags) {
			state.tags.emplace_back(recipe, tag);
		}
		std::ranges::sort(state.tags, [](const auto& a_lhs, const auto& a_rhs) { return a_lhs.first->GetFormID() < a_rhs.first->GetFormID(); });
		state.tabVisible = result.tabVisible;

		// Compared by content, not only by the signature: the menus put back
		// from the snapshot must count as "unchanged" when they are the same.
		const bool unchanged = _lastSignature != 0 && intact && state.SameAs(_state);
		if (!unchanged) {
			// --- 2. Apply ---------------------------------------------------------
			Execute(a_forms, state);
			AddPreviewTransforms(a_scan, result);
		}
		_lastSignature = signature;

		result.changed = !unchanged;
		result.previewsAdded = static_cast<std::uint32_t>(_previewGiven.size());
		_lastResult = result;
		return result;
	}

	void MenuBuilder::Execute(const OwnedForms& a_forms, const MenuState& a_state)
	{
		RemoveMerges();  // applied last, so taken out first
		RemoveRestores();
		RemoveInjections(a_forms);

		SetListContents(a_forms.rootMenu, { a_forms.rootKeyword });
		for (std::size_t i = 0; i < a_forms.menus.size(); ++i) {
			SetListContents(a_forms.menus[i], { a_forms.menuTitles[i] });
		}

		for (const auto& [form, name] : a_state.names) {
			if (!NameMap::SetName(form, name)) {
				REX::LogWarning("MenuBuilder: could not name {} '{}'", Util::FormIDToString(form->GetFormID()), name);
			}
		}

		for (const auto& [menu, children] : a_state.contents) {
			SetListContents(menu, children);
		}

		for (const auto& [target, node] : a_state.injections) {
			if (!target->ContainsEditorForm(node)) {
				InsertBeforeScriptForms(target, node);
			}
			_injectedInto.insert(target);
		}

		if (a_state.tabVisible) {
			InsertBeforeScriptForms(a_forms.workshopMenuMain, a_forms.rootMenu);
		}

		// Categories and menus of other mods, back where their plugins had them.
		for (const auto& restored : a_state.restores) {
			if (!ListHas(restored.list, restored.node)) {
				InsertAfter(restored.list, restored.node, restored.after);
				_insertedRestores.push_back(restored);
			}
		}

		// Menus of another mod with the same name and similar items, merged.
		ApplyMerges(a_state.merges);

		TagMap tags;
		for (const auto& [recipe, tag] : a_state.tags) {
			tags[recipe] = tag;
		}
		UpdateTags(tags);

		_state = a_state;
	}

	void MenuBuilder::Restore(const OwnedForms& a_forms, const MenuState& a_state, const PreviewMap& a_previews, std::size_t a_signature, const ApplyResult& a_summary)
	{
		if (!a_forms.IsValid()) {
			return;
		}

		Execute(a_forms, a_state);

		for (const auto& [item, transform] : a_previews) {
			if (item && transform && !item->previewTransform.transform) {
				item->previewTransform.transform = transform;
				_previewGiven.emplace(item, transform);
			}
		}

		_lastSignature = a_signature;
		_lastResult = a_summary;
		_lastResult.changed = true;
		_lastResult.previewsAdded = static_cast<std::uint32_t>(_previewGiven.size());
	}
}
