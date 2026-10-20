#include "Grouping.h"

#include "Util.h"

namespace LO::Grouping
{
	namespace
	{
		constexpr double TOO_COMMON = 0.5;  // features shared by more than half are too broad
		constexpr auto OTHER_LABEL = "Other"sv;

		enum class Kind : std::uint8_t
		{
			kTag = 0,  // strongest: the author grouped it on purpose
			kWord = 1,
			kFolder = 2
		};

		struct Feature
		{
			Kind kind;
			std::string key;  // lower case, unique per kind
		};

		bool IsLetter(unsigned char a_char)
		{
			return std::isalnum(a_char) != 0 || a_char >= 0x80;
		}

		std::string Capitalize(std::string a_text)
		{
			if (!a_text.empty() && static_cast<unsigned char>(a_text.front()) < 0x80) {
				a_text.front() = static_cast<char>(std::toupper(static_cast<unsigned char>(a_text.front())));
			}
			return a_text;
		}

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

		const RE::TESForm* FirstVariant(const RE::TESForm* a_item)
		{
			if (const auto* list = a_item ? a_item->As<RE::BGSListForm>() : nullptr) {
				return list->GetNthForm(0);
			}
			return a_item;
		}

		// Collects the features of one recipe. a_labels remembers the original
		// spelling of every key for the group names.
		std::vector<Feature> Features(const RE::BGSConstructibleObject* a_recipe, std::unordered_map<std::string, std::string>& a_labels)
		{
			std::vector<Feature> features;
			std::unordered_set<std::string> seen;

			const auto add = [&](Kind a_kind, std::string_view a_original) {
				if (a_original.empty()) {
					return;
				}
				auto key = std::format("{}:{}", static_cast<int>(a_kind), LowerAscii(a_original));
				if (seen.insert(key).second) {
					a_labels.try_emplace(key, std::string(a_original));
					features.push_back({ a_kind, std::move(key) });
				}
			};

			const auto name = Util::Trim(ItemName(a_recipe));

			// "[Tag] Name"
			if (name.starts_with('[')) {
				const auto close = name.find(']');
				if (close != std::string::npos && close > 1) {
					add(Kind::kTag, Util::Trim(std::string_view(name).substr(1, close - 1)));
				}
			}

			// Words of the name.
			std::string word;
			const auto flushWord = [&]() {
				const bool allDigits = std::ranges::all_of(word, [](unsigned char c) { return std::isdigit(c) != 0; });
				if (word.size() >= 3 && !allDigits) {
					add(Kind::kWord, word);
				}
				word.clear();
			};
			for (const unsigned char c : name) {
				if (IsLetter(c)) {
					word += static_cast<char>(c);
				} else {
					flushWord();
				}
			}
			flushWord();

			// Folders of the model path (without the file name).
			auto model = ItemModel(a_recipe);
			std::ranges::replace(model, '/', '\\');
			std::size_t start = 0;
			while (true) {
				const auto next = model.find('\\', start);
				if (next == std::string::npos) {
					break;  // the rest is the file name
				}
				const auto folder = std::string_view(model).substr(start, next - start);
				if (!folder.empty() && !Util::EqualsIgnoreCase(folder, "meshes")) {
					add(Kind::kFolder, folder);
				}
				start = next + 1;
			}

			return features;
		}

		std::vector<Group> SplitAlphabetically(std::vector<RE::BGSConstructibleObject*> a_recipes, const Options& a_options)
		{
			std::ranges::sort(a_recipes, [](const auto* a_lhs, const auto* a_rhs) {
				return Util::ToLower(ItemName(a_lhs)) < Util::ToLower(ItemName(a_rhs));
			});

			const auto count = a_recipes.size();
			const auto groups = std::clamp<std::size_t>((count + a_options.chunkSize - 1) / a_options.chunkSize, 1, a_options.maxGroups);
			const auto perGroup = (count + groups - 1) / groups;

			const auto initial = [](const RE::BGSConstructibleObject* a_recipe) {
				const auto name = Util::Trim(ItemName(a_recipe));
				if (name.empty()) {
					return std::string("?");
				}
				const auto c = static_cast<unsigned char>(name.front());
				return c < 0x80 ? std::string(1, static_cast<char>(std::toupper(c))) : name.substr(0, std::min<std::size_t>(2, name.size()));
			};

			std::vector<Group> result;
			for (std::size_t i = 0; i < count; i += perGroup) {
				const auto end = std::min(count, i + perGroup);
				Group group;
				group.recipes.assign(a_recipes.begin() + static_cast<std::ptrdiff_t>(i), a_recipes.begin() + static_cast<std::ptrdiff_t>(end));
				const auto first = initial(group.recipes.front());
				const auto last = initial(group.recipes.back());
				group.label = first == last ? first : first + " - " + last;
				result.push_back(std::move(group));
			}
			return result;
		}
	}

	std::string ItemName(const RE::BGSConstructibleObject* a_recipe)
	{
		const auto* item = a_recipe ? a_recipe->createdItem : nullptr;
		auto name = Util::GetFormName(item);
		if (name.empty()) {
			name = Util::GetFormName(FirstVariant(item));
		}
		return name;
	}

	std::string ItemModel(const RE::BGSConstructibleObject* a_recipe)
	{
		const auto* item = FirstVariant(a_recipe ? a_recipe->createdItem : nullptr);
		const auto* model = RE::DynamicCast<const RE::TESModel*>(item);
		const char* path = model ? model->GetModel() : nullptr;
		return path ? std::string(path) : std::string();
	}

	std::vector<Group> Split(const std::vector<RE::BGSConstructibleObject*>& a_recipes, const Options& a_options)
	{
		const auto total = a_recipes.size();
		if (total == 0) {
			return {};
		}

		// Features of every recipe.
		std::unordered_map<std::string, std::string> labels;
		std::vector<std::vector<Feature>> features;
		features.reserve(total);
		std::unordered_map<std::string, std::size_t> overall;
		std::unordered_map<std::string, Kind> kindOf;
		for (const auto* recipe : a_recipes) {
			features.push_back(Features(recipe, labels));
			for (const auto& feature : features.back()) {
				++overall[feature.key];
				kindOf[feature.key] = feature.kind;
			}
		}

		// Features (almost) everybody has say nothing ("HouseKit" in a HouseKit category).
		std::unordered_set<std::string> ignored;
		for (const auto& [key, count] : overall) {
			if (total >= 10 && static_cast<double>(count) > TOO_COMMON * static_cast<double>(total)) {
				ignored.insert(key);
			}
		}

		std::vector<bool> assigned(total, false);
		std::size_t remaining = total;
		std::vector<Group> groups;
		std::unordered_set<std::string> usedLabels;

		while (groups.size() + 1 < a_options.maxGroups && remaining > 0) {
			std::unordered_map<std::string, std::size_t> counts;
			for (std::size_t i = 0; i < total; ++i) {
				if (assigned[i]) {
					continue;
				}
				for (const auto& feature : features[i]) {
					if (!ignored.contains(feature.key)) {
						++counts[feature.key];
					}
				}
			}

			// Strongest kind first (tags, then words, then folders), then the
			// feature shared by most recipes.
			const std::string* best = nullptr;
			std::size_t bestCount = 0;
			for (const auto& [key, count] : counts) {
				if (count < a_options.minGroupSize) {
					continue;
				}
				const bool better = !best ||
				                    kindOf[key] < kindOf[*best] ||
				                    (kindOf[key] == kindOf[*best] && count > bestCount) ||
				                    (kindOf[key] == kindOf[*best] && count == bestCount && key < *best);
				if (better) {
					best = &key;
					bestCount = count;
				}
			}

			if (!best) {
				break;
			}

			const auto key = *best;
			auto label = Capitalize(labels[key]);

			// Never make one group out of everything that is left on the first pass,
			// and never use the same label twice.
			if ((groups.empty() && bestCount == remaining) || !usedLabels.insert(Util::ToLower(label)).second) {
				ignored.insert(key);
				continue;
			}

			Group group{ std::move(label), {} };
			for (std::size_t i = 0; i < total; ++i) {
				if (assigned[i]) {
					continue;
				}
				if (std::ranges::any_of(features[i], [&key](const Feature& a_feature) { return a_feature.key == key; })) {
					assigned[i] = true;
					--remaining;
					group.recipes.push_back(a_recipes[i]);
				}
			}
			groups.push_back(std::move(group));
		}

		// Nothing useful found: split alphabetically instead.
		if (groups.size() < 2) {
			return SplitAlphabetically(a_recipes, a_options);
		}

		std::ranges::sort(groups, [](const Group& a_lhs, const Group& a_rhs) {
			return Util::ToLower(a_lhs.label) < Util::ToLower(a_rhs.label);
		});

		if (remaining > 0) {
			Group other{ std::string(OTHER_LABEL), {} };
			for (std::size_t i = 0; i < total; ++i) {
				if (!assigned[i]) {
					other.recipes.push_back(a_recipes[i]);
				}
			}
			groups.push_back(std::move(other));
		}

		return groups;
	}
}
