#include "Scanner.h"

#include "Grouping.h"
#include "ModelCheck.h"
#include "NameMap.h"
#include "PluginHistory.h"
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
		void CollectReachable(const RE::BGSListForm* a_main, const OwnedForms& a_forms, const RestoreSet& a_ours, KeywordSet& a_leaves, ListSet& a_lists)
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
					if (!form || a_forms.IsOwned(form) || a_ours.contains({ list, form })) {
						continue;  // our own tab and what we put back must never count as "reachable"
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

		MenuTreeInfo CollectMenuTree(const RE::BGSListForm* a_root, const OwnedForms& a_forms, const RestoreSet& a_ours)
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
					if (!form || a_forms.IsOwned(form) || a_ours.contains({ list, form })) {
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

	namespace
	{
		std::string NameOf(const RE::TESForm* a_form)
		{
			auto name = Util::GetFormName(a_form);
			if (name.empty()) {
				name = Util::GetEditorID(a_form);
			}
			return name.empty() && a_form ? Util::FormIDToString(a_form->GetFormID()) : name;
		}

		// Puts lost categories and sub menus back where the plugin files had
		// them (PluginHistory): into the newest version of a menu that listed
		// them, behind the entry that stood in front of them there. If that menu
		// cannot be reached either, it is put back the same way first.
		class Attacher
		{
		public:
			Attacher(const OwnedForms& a_forms, const KeywordSet& a_recipeFilters, const KeywordSet& a_leaves, const ListSet& a_lists,
				const std::unordered_map<const RE::BGSListForm*, std::string>& a_menuPath, const RestoreSet& a_ours, std::vector<Restore>& a_out) :
				_forms(a_forms),
				_recipeFilters(a_recipeFilters),
				_leaves(a_leaves),
				_lists(a_lists),
				_menuPath(a_menuPath),
				_ours(a_ours),
				_out(a_out),
				_history(PluginHistory::Get())
			{}

			// True if a_node can be reached (already, or through what was put back).
			bool Attach(RE::TESForm* a_node, std::uint32_t a_depth = 0)
			{
				if (!a_node || IsReachable(a_node)) {
					return a_node != nullptr;
				}
				if (a_depth > MAX_DEPTH || !_inProgress.insert(a_node).second) {
					return false;
				}
				bool attached = false;
				for (const auto& place : _history.PlacesOf(a_node->GetFormID())) {
					auto* listForm = RE::TESForm::FindFormByID(place.list);
					auto* list = listForm ? listForm->As<RE::BGSListForm>() : nullptr;
					if (!list || _forms.IsOwned(list)) {
						continue;
					}
					const bool isMain = list == _forms.workshopMenuMain;
					if (!isMain && (place.index == 0 || !IsMenuVersion(*place.version))) {
						continue;  // entry 0 is the name of a sub menu, not a category; or not a menu at all
					}
					if (!Attach(list, a_depth + 1)) {
						continue;
					}
					if (!Contains(list, a_node)) {
						_out.push_back({ list, a_node, InFrontOf(list, *place.version, place.index), PathOf(list, a_node), RemovedBy(list, a_node), 0 });
						_added[list].insert(a_node);
					}
					MarkReachable(list, a_node);
					attached = true;
					break;
				}
				_inProgress.erase(a_node);
				return attached;
			}

			[[nodiscard]] std::string PathOf(const RE::TESForm* a_node) const
			{
				const auto it = _nodePath.find(a_node);
				return it != _nodePath.end() ? it->second : NameOf(a_node);
			}

		private:
			static constexpr std::uint32_t MAX_DEPTH = 12;

			bool IsReachable(const RE::TESForm* a_node) const
			{
				if (const auto* keyword = a_node->As<RE::BGSKeyword>()) {
					return _leaves.contains(keyword) || _newLeaves.contains(keyword);
				}
				if (const auto* list = a_node->As<RE::BGSListForm>()) {
					return _lists.contains(list) || _newLists.contains(list);
				}
				return false;
			}

			// A version of a sub menu: entry 0 is a category keyword (its name).
			bool IsMenuVersion(const PluginHistory::Version& a_version) const
			{
				if (a_version.entries.empty()) {
					return false;
				}
				const auto* first = RE::TESForm::FindFormByID(a_version.entries.front());
				const auto* keyword = first ? first->As<RE::BGSKeyword>() : nullptr;
				return keyword && _recipeFilters.contains(keyword);
			}

			bool Contains(const RE::BGSListForm* a_list, const RE::TESForm* a_node) const
			{
				if (const auto it = _added.find(a_list); it != _added.end() && it->second.contains(a_node)) {
					return true;
				}
				for (const auto* form : a_list->arrayOfForms) {
					if (form == a_node && !_ours.contains({ a_list, a_node })) {
						return true;
					}
				}
				return false;
			}

			// The nearest entry in front of it in the plugin's version that is in the list now.
			RE::TESForm* InFrontOf(const RE::BGSListForm* a_list, const PluginHistory::Version& a_version, std::size_t a_index) const
			{
				for (std::size_t i = a_index; i-- > 0;) {
					auto* candidate = RE::TESForm::FindFormByID(a_version.entries[i]);
					if (candidate && Contains(a_list, candidate)) {
						return candidate;
					}
				}
				return nullptr;
			}

			std::string RemovedBy(const RE::BGSListForm* a_list, const RE::TESForm* a_node) const
			{
				const auto& versions = _history.VersionsOf(a_list->GetFormID());
				if (versions.empty()) {
					return "unknown";
				}
				const auto& last = versions.back();
				if (std::ranges::find(last.entries, a_node->GetFormID()) == last.entries.end()) {
					return last.file;
				}
				return "a script while the game was running";
			}

			std::string PathOf(const RE::BGSListForm* a_list, const RE::TESForm* a_node)
			{
				std::string parent;
				if (a_list != _forms.workshopMenuMain) {
					if (const auto it = _menuPath.find(a_list); it != _menuPath.end()) {
						parent = it->second;
					} else {
						parent = PathOf(a_list);
					}
				}
				const auto* title = a_node->As<RE::BGSListForm>() ? a_node->As<RE::BGSListForm>()->GetNthForm(0) : a_node;
				auto path = parent.empty() ? NameOf(title) : parent + " > " + NameOf(title);
				_nodePath[a_node] = path;
				return path;
			}

			// The node and - for a sub menu - everything in it is reachable now.
			void MarkReachable(const RE::BGSListForm* a_parent, RE::TESForm* a_node)
			{
				if (!_nodePath.contains(a_node)) {
					PathOf(a_parent, a_node);
				}
				if (auto* keyword = a_node->As<RE::BGSKeyword>()) {
					_newLeaves.insert(keyword);
					return;
				}
				auto* list = a_node->As<RE::BGSListForm>();
				if (!list || !_newLists.insert(list).second) {
					return;
				}
				const auto forms = list->GetAllForms();
				for (std::size_t i = 0; i < forms.size(); ++i) {
					auto* form = forms[i];
					if (!form || _forms.IsOwned(form) || _ours.contains({ list, form })) {
						continue;
					}
					if (i != 0 && form->As<RE::BGSKeyword>()) {
						_newLeaves.insert(form->As<RE::BGSKeyword>());
						_nodePath.try_emplace(form, PathOf(list) + " > " + NameOf(form));
					} else if (form->As<RE::BGSListForm>()) {
						MarkReachable(list, form);
					}
				}
			}

			const OwnedForms& _forms;
			const KeywordSet& _recipeFilters;
			const KeywordSet& _leaves;
			const ListSet& _lists;
			const std::unordered_map<const RE::BGSListForm*, std::string>& _menuPath;
			const RestoreSet& _ours;
			std::vector<Restore>& _out;
			const PluginHistory::History& _history;

			KeywordSet _newLeaves;
			ListSet _newLists;
			std::unordered_set<const RE::TESForm*> _inProgress;
			std::unordered_map<const RE::BGSListForm*, std::unordered_set<const RE::TESForm*>> _added;
			std::unordered_map<const RE::TESForm*, std::string> _nodePath;
		};
	}

	namespace
	{
		// "Green House", "greenhouse" and "GreenHouse:" are the same name.
		std::string NormalizedName(std::string_view a_name)
		{
			std::string result;
			for (const unsigned char c : a_name) {
				if (c >= 0x80) {
					result += static_cast<char>(c);
				} else if (std::isalnum(c) != 0) {
					result += static_cast<char>(std::tolower(c));
				}
			}
			return result;
		}

		// The name of a category, or of a sub menu (its entry 0).
		std::string TitleOf(const RE::TESForm* a_node)
		{
			const RE::TESForm* title = a_node;
			if (const auto* list = a_node ? a_node->As<RE::BGSListForm>() : nullptr) {
				title = list->GetNthForm(0);
			}
			if (!title) {
				return {};
			}
			auto name = Util::GetFormName(title);
			return name.empty() ? Util::GetEditorID(title) : name;
		}

		// Merges menus and categories of the same mod that have the same name and
		// similar items, where one is in a sub folder of the other (for example
		// "Snap'n Build > Greenhouse" and "Snap'n Build > Real Estate > Yard >
		// Greenhouse"). Everything of the deeper one goes into the upper menu, the
		// deeper entry is taken out of its folder, and sub menus with the same name
		// inside are merged the same way. Nothing is fixed for a mod:
		//   * on its own path: the upper menu lies directly in one of the menus above
		//     the deeper entry, and both were made by the same plugin file (read
		//     from the forms - no list of files or mods),
		//   * similar items: at least one model file or one model folder in common,
		//   * the upper one is a sub menu, the deeper one lies further down,
		//   * entries that scripts added are never moved (the save game stores them).
		class MergePlanner
		{
		public:
			MergePlanner(const OwnedForms& a_forms, const RestoreSet& a_ours, ScanResult& a_result,
				const std::unordered_map<const RE::BGSListForm*, const RE::BGSListForm*>& a_parentOf,
				const std::unordered_map<const RE::BGSListForm*, std::uint32_t>& a_depthOf,
				const std::unordered_map<const RE::BGSKeyword*, std::vector<const RE::BGSListForm*>>& a_menusOfCategory) :
				_forms(a_forms),
				_ours(a_ours),
				_result(a_result),
				_parentOf(a_parentOf),
				_depthOf(a_depthOf),
				_menusOfCategory(a_menusOfCategory)
			{
				const auto learn = [this](const VisibleRecipe& a_recipe) {
					for (const auto* category : a_recipe.categories) {
						_recipesOfCategory[category].push_back(a_recipe.recipe);
					}
				};
				for (const auto& visible : _result.visible) {
					learn(visible);
				}
				for (const auto& restored : _result.restoredVisible) {
					learn(restored);
				}
			}

			void Run()
			{
				// Every reachable sub menu and category, grouped by name.
				std::map<std::string, std::vector<Node>> byName;
				for (const auto& [list, path] : _result.menuPath) {
					const auto parent = _parentOf.find(list);
					const auto depth = _depthOf.find(list);
					if (parent == _parentOf.end() || depth == _depthOf.end()) {
						continue;
					}
					auto key = NormalizedName(TitleOf(list));
					if (!key.empty()) {
						byName[std::move(key)].push_back({ const_cast<RE::BGSListForm*>(list), const_cast<RE::BGSListForm*>(parent->second), depth->second, path });
					}
				}
				for (const auto& [keyword, lists] : _menusOfCategory) {
					auto key = NormalizedName(TitleOf(keyword));
					if (key.empty()) {
						continue;
					}
					for (const auto* list : lists) {
						const auto depth = _depthOf.find(list);
						const auto path = _result.menuPath.find(list);
						if (depth == _depthOf.end() || path == _result.menuPath.end()) {
							continue;
						}
						byName[key].push_back({ const_cast<RE::BGSKeyword*>(keyword), const_cast<RE::BGSListForm*>(list), depth->second + 1, path->second + " > " + TitleOf(keyword) });
					}
				}

				for (auto& entry : byName) {
					auto& nodes = entry.second;
					if (nodes.size() < 2) {
						continue;
					}
					std::ranges::sort(nodes, [](const Node& a_lhs, const Node& a_rhs) {
						if (a_lhs.depth != a_rhs.depth) {
							return a_lhs.depth < a_rhs.depth;
						}
						if (a_lhs.form->GetFormID() != a_rhs.form->GetFormID()) {
							return a_lhs.form->GetFormID() < a_rhs.form->GetFormID();
						}
						return a_lhs.parent->GetFormID() < a_rhs.parent->GetFormID();
					});
					// Each entry goes into the shallowest sub menu of the same name that
					// lies directly in a menu on its own path up.
					for (const auto& node : nodes) {
						for (const auto& target : nodes) {
							if (target.depth >= node.depth) {
								break;
							}
							if (!target.form->As<RE::BGSListForm>() || _mergedSources.contains(target.form) || !OnOwnPath(target, node)) {
								continue;
							}
							TryMerge(target, node);
							break;
						}
					}
				}
			}

		private:
			struct Node
			{
				RE::TESForm* form{ nullptr };       // sub menu or category
				RE::BGSListForm* parent{ nullptr };  // the menu it is listed in
				std::uint32_t depth{ 0 };
				std::string path;
			};

			struct Features
			{
				std::set<std::string> files;    // model file names ("greenhsfloor01foundation")
				std::set<std::string> folders;  // model folders ("snb\\greenh\\pipes")
			};

			static constexpr std::uint32_t MAX_DEPTH = 8;

			// The menu and all menus above it, up to the main menu.
			std::vector<const RE::BGSListForm*> Ancestors(const RE::BGSListForm* a_list) const
			{
				std::vector<const RE::BGSListForm*> chain;
				for (const auto* list = a_list; list && chain.size() < 64;) {
					chain.push_back(list);
					const auto it = _parentOf.find(list);
					list = it != _parentOf.end() ? it->second : nullptr;
				}
				return chain;
			}

			void AddFeatures(const RE::BGSKeyword* a_category, Features& a_features) const
			{
				const auto it = _recipesOfCategory.find(a_category);
				if (it == _recipesOfCategory.end()) {
					return;
				}
				for (const auto* recipe : it->second) {
					auto model = Util::ToLower(Grouping::ItemModel(recipe));
					std::ranges::replace(model, '/', '\\');
					if (model.starts_with("meshes\\")) {
						model.erase(0, 7);
					}
					if (model.empty()) {
						continue;
					}
					const auto slash = model.rfind('\\');
					auto file = slash == std::string::npos ? model : model.substr(slash + 1);
					if (file.ends_with(".nif")) {
						file.resize(file.size() - 4);
					}
					a_features.files.insert(std::move(file));
					if (slash != std::string::npos) {
						a_features.folders.insert(model.substr(0, slash));
					}
				}
			}

			Features FeaturesOf(const RE::TESForm* a_node) const
			{
				Features features;
				if (const auto* keyword = a_node->As<RE::BGSKeyword>()) {
					AddFeatures(keyword, features);
				} else if (const auto* list = a_node->As<RE::BGSListForm>()) {
					const auto tree = CollectMenuTree(list, _forms, _ours);
					for (const auto* leaf : tree.leaves) {
						AddFeatures(leaf, features);
					}
				}
				return features;
			}

			// Why the items are similar (empty: they are not).
			static std::string Similarity(const Features& a_lhs, const Features& a_rhs)
			{
				for (const auto& file : a_lhs.files) {
					if (a_rhs.files.contains(file)) {
						return std::format("same model file '{}'", file);
					}
				}
				for (const auto& folder : a_lhs.folders) {
					if (a_rhs.folders.contains(folder)) {
						return std::format("same model folder '{}'", folder);
					}
				}
				return {};
			}

			bool Contains(const RE::BGSListForm* a_list, const RE::TESForm* a_node) const
			{
				if (const auto it = _inserted.find(a_list); it != _inserted.end() && it->second.contains(a_node)) {
					return true;
				}
				return std::ranges::find(a_list->arrayOfForms, a_node) != a_list->arrayOfForms.end();
			}

			void Insert(RE::BGSListForm* a_list, RE::TESForm* a_node)
			{
				if (!Contains(a_list, a_node)) {
					_result.mergeOps.push_back({ MergeOp::Kind::kInsert, a_list, a_node });
					_inserted[a_list].insert(a_node);
				}
			}

			// Everything of menu a_from goes into menu a_into; sub menus with the
			// same name are merged in turn.
			void MergeInto(RE::BGSListForm* a_into, const RE::BGSListForm* a_from, std::uint32_t a_depth)
			{
				if (a_depth > MAX_DEPTH || a_into == a_from) {
					return;
				}
				const auto forms = a_from->GetAllForms();
				const auto into = a_into->GetAllForms();
				for (std::size_t i = 1; i < forms.size(); ++i) {
					auto* form = forms[i];
					if (!form || _forms.IsOwned(form)) {
						continue;
					}
					if (auto* subList = form->As<RE::BGSListForm>()) {
						const auto name = NormalizedName(TitleOf(subList));
						RE::BGSListForm* same = nullptr;
						for (std::size_t j = 1; j < into.size() && !name.empty(); ++j) {
							auto* candidate = into[j] ? into[j]->As<RE::BGSListForm>() : nullptr;
							if (candidate && candidate != subList && !_forms.IsOwned(candidate) && NormalizedName(TitleOf(candidate)) == name) {
								same = candidate;
								break;
							}
						}
						if (same) {
							MergeInto(same, subList, a_depth + 1);
							continue;
						}
					}
					Insert(a_into, form);
				}
			}

			// The target lies directly in one of the menus above the entry (on its
			// own path up), and that menu was made by the same plugin file as the
			// target (both read from the forms, no list of files). A menu with the
			// same name in a neighbouring branch ("Concrete > Supports" and "Wood >
			// Supports") is never a target.
			bool OnOwnPath(const Node& a_target, const Node& a_node) const
			{
				const auto* holder = a_target.parent;
				if (!holder || holder == _forms.workshopMenuMain) {
					return false;
				}
				if (Util::ToLower(Util::GetFileName(holder, 0)) != Util::ToLower(Util::GetFileName(a_target.form, 0))) {
					return false;  // the target was put into that menu by another plugin
				}
				const auto chain = Ancestors(a_node.parent);
				return std::ranges::find(chain, holder) != chain.end();
			}

			void TryMerge(const Node& a_target, const Node& a_node)
			{
				auto* targetList = a_target.form->As<RE::BGSListForm>();
				if (!targetList || a_node.form == a_target.form || _forms.IsOwned(a_node.parent)) {
					return;
				}

				// Already inside the upper menu, or the upper menu lies inside it.
				const auto nodeChain = Ancestors(a_node.parent);
				if (std::ranges::find(nodeChain, targetList) != nodeChain.end()) {
					return;
				}
				const auto targetChain = Ancestors(targetList);
				if (std::ranges::any_of(targetChain, [&a_node](const RE::BGSListForm* a_list) { return static_cast<const RE::TESForm*>(a_list) == a_node.form; })) {
					return;
				}
				// Inside something that is merged already: it moves along with it.
				for (const auto* list : nodeChain) {
					if (_mergedSources.contains(list)) {
						return;
					}
				}

				const auto similarity = Similarity(FeaturesOf(a_target.form), FeaturesOf(a_node.form));
				if (similarity.empty()) {
					_result.mergesSkipped.push_back(std::format("{} -> {}: the items are not similar (no model file or model folder in common)", a_node.path, a_target.path));
					return;
				}

				// Entries that scripts added stay where they are (the save stores them).
				const auto& array = a_node.parent->arrayOfForms;
				const auto tailStart = static_cast<std::size_t>(array.size() - std::min(a_node.parent->scriptAddedFormCount, array.size()));
				bool movable = false;
				for (std::size_t i = 0; i < tailStart; ++i) {
					if (array[static_cast<std::uint32_t>(i)] == a_node.form) {
						movable = true;
						break;
					}
				}
				if (!movable) {
					_result.mergesSkipped.push_back(std::format("{} -> {}: a script added it to its menu, it is left where it is", a_node.path, a_target.path));
					return;
				}

				_result.mergeOps.push_back({ MergeOp::Kind::kRemove, a_node.parent, a_node.form });
				if (auto* list = a_node.form->As<RE::BGSListForm>()) {
					MergeInto(targetList, list, 0);
				} else {
					Insert(targetList, a_node.form);
				}
				_mergedSources.insert(a_node.form);
				_result.merges.push_back(std::format("{} -> {} ({})", a_node.path, a_target.path, similarity));
			}

			const OwnedForms& _forms;
			const RestoreSet& _ours;
			ScanResult& _result;
			const std::unordered_map<const RE::BGSListForm*, const RE::BGSListForm*>& _parentOf;
			const std::unordered_map<const RE::BGSListForm*, std::uint32_t>& _depthOf;
			const std::unordered_map<const RE::BGSKeyword*, std::vector<const RE::BGSListForm*>>& _menusOfCategory;

			std::unordered_map<const RE::BGSKeyword*, std::vector<const RE::BGSConstructibleObject*>> _recipesOfCategory;
			std::unordered_map<const RE::BGSListForm*, std::unordered_set<const RE::TESForm*>> _inserted;
			std::unordered_set<const RE::TESForm*> _mergedSources;
		};
	}

	ScanResult Scanner::Run(const OwnedForms& a_forms, const Settings& a_settings, const KeywordMap& a_originalKeywords, const RestoreSet& a_ourRestores)
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
		CollectReachable(a_forms.workshopMenuMain, a_forms, a_ourRestores, reachableLeaves, reachableLists);

		// 1b) Depth and parent of every reachable sub menu (breadth first), the
		//     menus each category is shown in, and the plugins' own top menus.
		std::unordered_map<const RE::BGSListForm*, const RE::BGSListForm*> parentOf;
		std::unordered_map<const RE::BGSListForm*, std::uint32_t> depthOf;
		std::unordered_map<const RE::BGSKeyword*, std::vector<const RE::BGSListForm*>> menusOfCategory;
		{
			struct Node
			{
				RE::BGSListForm* list;
				std::uint32_t depth;
				std::string path;
			};

			std::unordered_map<std::string, std::pair<std::uint32_t, std::unordered_set<RE::BGSListForm*>>> topLists;
			std::unordered_set<const RE::BGSListForm*> seen{ a_forms.workshopMenuMain };
			std::vector<Node> queue{ { a_forms.workshopMenuMain, 0, std::string() } };

			for (std::size_t head = 0; head < queue.size(); ++head) {
				const auto node = queue[head];
				const bool isMainMenu = head == 0;
				const auto forms = node.list->GetAllForms();
				for (std::size_t i = 0; i < forms.size(); ++i) {
					auto* form = forms[i];
					if (!form || a_forms.IsOwned(form) || a_ourRestores.contains({ node.list, form })) {
						continue;
					}

					// Categories: the first (shallowest) path wins.
					if (const auto* keyword = form->As<RE::BGSKeyword>()) {
						if (!isMainMenu && i != 0) {
							menusOfCategory[keyword].push_back(node.list);
						}
						if ((isMainMenu || i != 0) && !result.categoryPath.contains(keyword)) {
							auto name = Util::GetFormName(keyword);
							if (name.empty()) {
								name = Util::GetEditorID(keyword);
							}
							result.categoryPath[keyword] = node.path.empty() ? name : node.path + " > " + name;
						}
						continue;
					}

					auto* subList = form->As<RE::BGSListForm>();
					if (!subList || !seen.insert(subList).second) {
						continue;
					}

					auto* title = subList->GetNthForm(0);
					auto titleName = Util::GetFormName(title);
					if (titleName.empty()) {
						titleName = Util::GetEditorID(title);
					}
					const auto path = node.path.empty() ? titleName : node.path + " > " + titleName;

					const Node child{ subList, node.depth + 1, path };
					queue.push_back(child);
					result.menuPath[subList] = path;
					parentOf[subList] = node.list;
					depthOf[subList] = child.depth;

					const auto file = Util::ToLower(Util::GetFileName(subList, 0));
					auto& [depth, lists] = topLists.try_emplace(file, child.depth, std::unordered_set<RE::BGSListForm*>{}).first->second;
					if (child.depth < depth) {
						depth = child.depth;
						lists.clear();
					}
					if (child.depth == depth) {
						lists.insert(subList);
					}
				}
			}

			for (const auto& [file, entry] : topLists) {
				if (entry.second.size() == 1) {
					result.homeOfPlugin[file] = *entry.second.begin();
				}
			}
		}

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
				VisibleRecipe visible{ recipe, {} };
				for (const auto* keyword : keywords) {
					if (reachableLeaves.contains(keyword)) {
						visible.categories.push_back(keyword);
					}
				}
				result.visible.push_back(std::move(visible));
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

		// Only workshop recipes count as "visible". Every item that can already
		// be built is remembered, with the place it is shown at.
		std::erase_if(result.visible, [&workshopBenches](const VisibleRecipe& a_visible) {
			return !workshopBenches.contains(a_visible.recipe->benchKeyword);
		});
		// Only recipes without conditions count: an item that needs a perk or a
		// quest elsewhere is not "already buildable" for everybody.
		std::unordered_map<const RE::TESForm*, const VisibleRecipe*> buildableItems;
		for (const auto& visible : result.visible) {
			if (visible.recipe->conditions) {
				continue;
			}
			buildableItems.try_emplace(visible.recipe->createdItem, &visible);
			if (visible.recipe->createdItem && visible.recipe->createdItem->As<RE::BGSListForm>()) {
				std::vector<const RE::TESForm*> variants;
				Util::CollectVariants(visible.recipe->createdItem, variants);
				for (const auto* variant : variants) {
					buildableItems.try_emplace(variant, &visible);
				}
			}
		}
		const auto shownAtOf = [&result](const VisibleRecipe* a_visible) {
			if (!a_visible->categories.empty()) {
				const auto path = result.categoryPath.find(a_visible->categories.front());
				if (path != result.categoryPath.end()) {
					return path->second;
				}
			}
			return std::string();
		};

		// Self-test of the model check with items that are visible right now.
		if (a_settings.skipMissingModels && !ModelCheck::IsActive()) {
			std::vector<std::string> sample;
			for (const auto& visible : result.visible) {
				auto model = Grouping::ItemModel(visible.recipe);
				if (!model.empty()) {
					sample.push_back(std::move(model));
				}
				if (sample.size() >= 300) {
					break;
				}
			}
			ModelCheck::SelfTest(sample);
		}

		// 3b) Plugins without a menu of their own (for example an add-on that
		//     puts its items into the menu of its main mod): their home is the
		//     shallowest menu that holds at least 80 % of their visible items,
		//     if that menu mostly shows items of this plugin or of the plugin
		//     that made the menu. Counted from the data only.
		{
			std::unordered_map<std::string, std::uint32_t> visibleOfPlugin;
			std::unordered_map<std::string, std::unordered_map<const RE::BGSListForm*, std::uint32_t>> pluginInMenu;
			std::unordered_map<const RE::BGSListForm*, std::uint32_t> menuTotal;

			for (const auto& visible : result.visible) {
				std::unordered_set<const RE::BGSListForm*> menus;
				for (const auto* category : visible.categories) {
					const auto it = menusOfCategory.find(category);
					if (it == menusOfCategory.end()) {
						continue;
					}
					for (const auto* menu : it->second) {
						for (auto* current = menu; current && current != a_forms.workshopMenuMain;) {
							if (!menus.insert(current).second) {
								break;  // the rest of the chain is already counted
							}
							const auto parent = parentOf.find(current);
							current = parent != parentOf.end() ? parent->second : nullptr;
						}
					}
				}
				const auto plugin = Util::ToLower(Util::GetFileName(visible.recipe, 0));
				++visibleOfPlugin[plugin];
				auto& inMenu = pluginInMenu[plugin];
				for (const auto* menu : menus) {
					++inMenu[menu];
					++menuTotal[menu];
				}
			}

			for (const auto& [plugin, total] : visibleOfPlugin) {
				if (result.homeOfPlugin.contains(plugin) || total == 0) {
					continue;
				}

				const RE::BGSListForm* best = nullptr;
				std::uint32_t bestDepth = std::numeric_limits<std::uint32_t>::max();
				std::uint32_t bestCount = 0;
				const auto& inMenu = pluginInMenu[plugin];
				for (const auto& [menu, count] : inMenu) {
					if (static_cast<double>(count) < 0.8 * static_cast<double>(total)) {
						continue;
					}
					// Does the menu belong to this plugin's "family"?
					const auto owner = Util::ToLower(Util::GetFileName(menu, 0));
					std::uint32_t family = count;
					if (owner != plugin) {
						if (const auto ownerIt = pluginInMenu.find(owner); ownerIt != pluginInMenu.end()) {
							if (const auto c = ownerIt->second.find(menu); c != ownerIt->second.end()) {
								family += c->second;
							}
						}
					}
					if (static_cast<double>(family) < 0.5 * static_cast<double>(menuTotal[menu])) {
						continue;  // a shared menu like "Structures"
					}
					const auto depth = depthOf[menu];
					if (!best || depth < bestDepth || (depth == bestDepth && count > bestCount)) {
						best = menu;
						bestDepth = depth;
						bestCount = count;
					}
				}
				if (best) {
					result.homeOfPlugin[plugin] = const_cast<RE::BGSListForm*>(best);
				}
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

		// Lost categories go back where their plugins had them, if the plugin
		// files say where that was.
		Attacher attacher(a_forms, recipeFilters, reachableLeaves, reachableLists, result.menuPath, a_ourRestores, result.restores);

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

			// The item itself can already be built through another recipe (for
			// example a "scrap return" recipe of a decoration mod). Showing it
			// again would only put the same item into the menu twice.
			// For a list of variants: only if EVERY variant can already be built
			// through its own recipe. Then nothing is lost, the items keep their
			// recipes where they are.
			if (a_settings.skipAlreadyBuildable) {
				if (const auto it = buildableItems.find(recipe->createdItem); it != buildableItems.end()) {
					result.alreadyBuildable.push_back({ LostRecipe{ recipe, usable.empty(), usable }, shownAtOf(it->second), "the same item is already in the menu" });
					continue;
				}
				if (recipe->createdItem->As<RE::BGSListForm>()) {
					std::vector<const RE::TESForm*> variants;
					Util::CollectVariants(recipe->createdItem, variants);
					const VisibleRecipe* first = nullptr;
					const bool all = !variants.empty() && std::ranges::all_of(variants, [&](const RE::TESForm* a_variant) {
						const auto found = buildableItems.find(a_variant);
						if (found == buildableItems.end()) {
							return false;
						}
						first = first ? first : found->second;
						return true;
					});
					if (all) {
						result.alreadyBuildable.push_back({ LostRecipe{ recipe, usable.empty(), usable }, shownAtOf(first), std::format("all {} variants are already in the menu", variants.size()) });
						continue;
					}
				}
			}

			// Ghost objects: nothing of the item can be seen in the game.
			if (a_settings.skipMissingModels) {
				if (auto problem = ModelCheck::FindProblem(recipe); !problem.empty()) {
					result.missingModel.push_back({ LostRecipe{ recipe, usable.empty(), usable }, std::string(), std::move(problem) });
					continue;
				}
			}

			if (a_settings.restoreOriginalPlaces && a_settings.UsesOtherMenus()) {
				const auto home = std::ranges::find_if(usable, [&attacher](RE::BGSKeyword* a_keyword) { return attacher.Attach(a_keyword); });
				if (home != usable.end()) {
					auto path = attacher.PathOf(*home);
					result.categoryPath.try_emplace(*home, path);
					result.restoredVisible.push_back({ recipe, { *home } });
					result.restored.push_back({ recipe, std::move(path) });
					continue;
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

			result.lostRecipes.push_back({ recipe, !hasUsableCategory, usable });
		}

		for (auto& restore : result.restores) {
			const auto prefix = restore.path;
			restore.recipeCount = static_cast<std::uint32_t>(std::ranges::count_if(result.restored, [&prefix](const RestoredRecipe& a_restored) {
				return a_restored.path == prefix || a_restored.path.starts_with(prefix + " > ");
			}));
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

			const auto tree = CollectMenuTree(list, a_forms, a_ourRestores);

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
					result.entryOfCategory[leaf] = list;
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

			result.entryOfCategory[keyword] = keyword;

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

		// 7) Menus of a mod with the same name and similar items: merged.
		if (a_settings.mergeSimilarMenus && !a_settings.onlyOwnTab) {
			MergePlanner(a_forms, a_ourRestores, result, parentOf, depthOf, menusOfCategory).Run();
		}

		result.valid = true;
		return result;
	}
}
