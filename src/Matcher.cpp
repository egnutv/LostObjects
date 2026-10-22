#include "Matcher.h"

#include "Grouping.h"
#include "Util.h"

namespace LO::Matcher
{
	namespace
	{
		enum class Kind : std::uint8_t
		{
			kModel = 0,  // same model file
			kFolder,     // same model folder
			kParent,     // same parent folder of the model folder (only a hint)
			kName,       // same item name
			kPrefix,     // same first word of the item name
			kTag,        // same "[Tag]" at the start of the name
			kCategory,   // category with the same name as the lost category
			kWord,       // a word of the item name (only a hint)
			kCount
		};

		constexpr std::array<double, static_cast<std::size_t>(Kind::kCount)> WEIGHTS{ 4.0, 3.0, 1.5, 3.0, 2.0, 2.0, 2.0, 1.0 };
		constexpr std::array<std::string_view, static_cast<std::size_t>(Kind::kCount)> LABELS{ "model", "model folder", "parent folder", "name", "first word", "tag", "category", "word" };

		constexpr double MIN_LEAD = 1.5;                     // the winner needs 1.5x the score of the runner-up
		constexpr std::uint32_t MIN_CONFIDENCE_GLOBAL = 60;  // when the plugin has no visible items at all

		std::uint32_t Percent(double a_fraction) { return static_cast<std::uint32_t>(100.0 * a_fraction + 0.5); }

		double Weight(Kind a_kind) { return WEIGHTS[static_cast<std::size_t>(a_kind)]; }
		std::string_view Label(Kind a_kind) { return LABELS[static_cast<std::size_t>(a_kind)]; }

		// The winner must have at least one of these in common with the lost item.
		// Words and parent folders alone are too broad ("box", "setdressing").
		bool IsSpecific(Kind a_kind) { return a_kind != Kind::kWord && a_kind != Kind::kParent; }

		struct Feature
		{
			Kind kind;
			std::string key;
			std::string value;
		};

		std::string LowerAscii(std::string_view a_text)
		{
			std::string result(a_text);
			for (auto& c : result) {
				if (static_cast<unsigned char>(c) < 0x80) {
					c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
				}
			}
			return result;
		}

		class FeatureList
		{
		public:
			void Add(Kind a_kind, std::string_view a_value)
			{
				if (a_value.empty()) {
					return;
				}
				auto value = LowerAscii(a_value);
				auto key = std::format("{}:{}", static_cast<int>(a_kind), value);
				if (_seen.insert(key).second) {
					features.push_back({ a_kind, std::move(key), std::move(value) });
				}
			}

			std::vector<Feature> features;

		private:
			std::unordered_set<std::string> _seen;
		};

		std::string CategoryName(const RE::BGSKeyword* a_keyword)
		{
			auto name = Util::GetFormName(a_keyword);
			return name.empty() ? Util::GetEditorID(a_keyword) : name;
		}

		void AddItemFeatures(const RE::BGSConstructibleObject* a_recipe, FeatureList& a_list)
		{
			const auto name = Util::Trim(Grouping::ItemName(a_recipe));
			a_list.Add(Kind::kName, name);

			if (name.starts_with('[')) {
				const auto close = name.find(']');
				if (close != std::string::npos && close > 1) {
					a_list.Add(Kind::kTag, Util::Trim(std::string_view(name).substr(1, close - 1)));
				}
			}

			std::vector<std::string> words;
			std::string word;
			const auto flush = [&]() {
				const bool allDigits = std::ranges::all_of(word, [](unsigned char c) { return std::isdigit(c) != 0; });
				if (word.size() >= 3 && !allDigits) {
					words.push_back(word);
				}
				word.clear();
			};
			for (const unsigned char c : name) {
				if (std::isalnum(c) != 0 || c >= 0x80) {
					word += static_cast<char>(c);
				} else {
					flush();
				}
			}
			flush();
			for (const auto& w : words) {
				a_list.Add(Kind::kWord, w);
			}
			if (!words.empty()) {
				a_list.Add(Kind::kPrefix, words.front());
			}

			auto model = LowerAscii(Grouping::ItemModel(a_recipe));
			std::ranges::replace(model, '/', '\\');
			if (model.starts_with("meshes\\")) {
				model.erase(0, 7);
			}
			if (model.empty()) {
				return;
			}
			a_list.Add(Kind::kModel, model);

			const auto slash = model.rfind('\\');
			if (slash == std::string::npos) {
				return;
			}
			const auto folder = model.substr(0, slash);
			a_list.Add(Kind::kFolder, folder);

			const auto parentSlash = folder.rfind('\\');
			if (parentSlash != std::string::npos) {
				a_list.Add(Kind::kParent, folder.substr(0, parentSlash));
			}
		}
	}

	Index::Index(const ScanResult& a_scan)
	{
		// Visible recipes and the recipes that are put back where their plugin had
		// them: both show where a mod keeps its items.
		const auto learn = [this](const VisibleRecipe& a_reference) {
			if (a_reference.categories.empty()) {
				return;
			}

			FeatureList list;
			AddItemFeatures(a_reference.recipe, list);

			auto& pluginCategories = _pluginCategories[Util::ToLower(Util::GetFileName(a_reference.recipe, 0))];
			for (const auto* category : a_reference.categories) {
				pluginCategories.insert(category);
				_allCategories.insert(category);
				for (const auto& feature : list.features) {
					++_features[feature.key][category];
				}
			}
			++_referenceCount;
		};
		for (const auto& visible : a_scan.visible) {
			learn(visible);
		}
		for (const auto& restored : a_scan.restoredVisible) {
			learn(restored);
		}

		// The name of every category that shows something.
		for (const auto* category : _allCategories) {
			FeatureList list;
			list.Add(Kind::kCategory, CategoryName(category));
			for (const auto& feature : list.features) {
				_features[feature.key][category] = 1;
			}
		}
	}

	Match Index::Find(const LostRecipe& a_lost, std::uint32_t a_minConfidence) const
	{
		Match match;

		FeatureList list;
		AddItemFeatures(a_lost.recipe, list);
		for (const auto* category : a_lost.categories) {
			list.Add(Kind::kCategory, CategoryName(category));
		}

		const CategorySet* candidates = nullptr;
		if (const auto it = _pluginCategories.find(Util::ToLower(Util::GetFileName(a_lost.recipe, 0))); it != _pluginCategories.end() && !it->second.empty()) {
			candidates = &it->second;
		}
		const bool global = candidates == nullptr;
		if (global) {
			candidates = &_allCategories;
		}

		struct Score
		{
			double total{ 0.0 };
			double specific{ 0.0 };
		};
		std::unordered_map<const RE::BGSKeyword*, Score> scores;
		double present = 0.0;  // weight of all features that exist anywhere in the menu

		struct Share
		{
			const Feature* feature;
			std::unordered_map<const RE::BGSKeyword*, double> byCategory;
		};
		std::vector<Share> shares;

		for (const auto& feature : list.features) {
			const auto it = _features.find(feature.key);
			if (it == _features.end()) {
				continue;
			}

			// Share of the WHOLE menu: a folder that is everywhere ("setdressing")
			// gives every single category only a small part.
			std::uint32_t total = 0;
			for (const auto& [category, count] : it->second) {
				total += count;
			}
			if (total == 0) {
				continue;
			}

			const auto weight = Weight(feature.kind);
			present += weight;

			Share share{ &feature, {} };
			for (const auto& [category, count] : it->second) {
				if (!candidates->contains(category)) {
					continue;
				}
				const auto part = static_cast<double>(count) / static_cast<double>(total);
				auto& score = scores[category];
				score.total += weight * part;
				if (IsSpecific(feature.kind)) {
					score.specific += weight * part;
				}
				share.byCategory[category] = part;
			}
			shares.push_back(std::move(share));
		}

		if (scores.empty() || present <= 0.0) {
			match.evidence = "nothing in common with any visible item";
			return match;
		}

		// Best and runner-up (ties: lower form ID, so the result is stable).
		const RE::BGSKeyword* best = nullptr;
		double bestScore = 0.0;
		double secondScore = 0.0;
		for (const auto& [category, score] : scores) {
			const bool better = !best || score.total > bestScore || (score.total == bestScore && category->GetFormID() < best->GetFormID());
			if (better) {
				if (best) {
					secondScore = std::max(secondScore, bestScore);
				}
				best = category;
				bestScore = score.total;
			} else {
				secondScore = std::max(secondScore, score.total);
			}
		}

		const auto& bestDetail = scores.at(best);
		match.guess = best;
		match.confidence = Percent(bestScore / present);

		// Why: the strongest contributions to the winner.
		std::vector<std::pair<double, std::string>> reasons;
		for (const auto& share : shares) {
			const auto it = share.byCategory.find(best);
			if (it != share.byCategory.end()) {
				reasons.emplace_back(Weight(share.feature->kind) * it->second, std::format("{} '{}' {}%", Label(share.feature->kind), share.feature->value, Percent(it->second)));
			}
		}
		std::ranges::sort(reasons, [](const auto& a_lhs, const auto& a_rhs) { return a_lhs.first > a_rhs.first; });

		std::string evidence = std::format("{}% confidence", match.confidence);
		for (std::size_t i = 0; i < reasons.size() && i < 3; ++i) {
			evidence += ", " + reasons[i].second;
		}

		const auto minConfidence = global ? std::max(a_minConfidence, MIN_CONFIDENCE_GLOBAL) : a_minConfidence;
		std::string rejected;
		if (bestDetail.specific <= 0.0) {
			rejected = "no model, folder, name, first word, tag or category in common";
		} else if (match.confidence < minConfidence) {
			rejected = std::format("below {}%", minConfidence);
		} else if (secondScore > 0.0 && bestScore < MIN_LEAD * secondScore) {
			rejected = "no clear winner";
		}

		if (rejected.empty()) {
			match.category = best;
			match.evidence = std::move(evidence);
		} else {
			match.evidence = std::format("not sorted in ({}): {}", rejected, evidence);
		}
		return match;
	}
}
