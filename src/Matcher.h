#pragma once

#include "Scanner.h"

// Finds the existing category a lost recipe belongs to - only from what is
// already in the menu, no fixed lists.
//
// Every recipe is described by features: its model path, the model's folder
// and parent folder, the item name, a "[Tag]" at the start of the name, the
// single words of the name and the name of its (lost) category.
//
// For each feature of a lost recipe the index looks at where the visible
// recipes with the same feature are shown. A category that holds most of
// them gets most of the feature's weight. The category with the highest
// total wins if it is clearly ahead and the evidence is strong enough;
// otherwise the recipe is not sorted in (it goes into a "Lost Objects"
// folder instead - better there than in the wrong place).
//
// Candidates are the categories in which the recipe's own plugin already has
// visible items. If the plugin has none, all categories are candidates, but
// then a much stronger match is required.

namespace LO::Matcher
{
	struct Match
	{
		const RE::BGSKeyword* category{ nullptr };  // nullptr = no good match
		const RE::BGSKeyword* guess{ nullptr };     // best candidate, also when rejected (for the logs)
		std::uint32_t confidence{ 0 };              // percent
		std::string evidence;                       // why, for the logs
	};

	class Index
	{
	public:
		explicit Index(const ScanResult& a_scan);

		[[nodiscard]] Match Find(const LostRecipe& a_lost, std::uint32_t a_minConfidence) const;

	private:
		using Counts = std::unordered_map<const RE::BGSKeyword*, std::uint32_t>;
		using CategorySet = std::unordered_set<const RE::BGSKeyword*>;

		std::unordered_map<std::string, Counts> _features;            // feature key -> category -> visible recipes
		std::unordered_map<std::string, CategorySet> _pluginCategories;  // lower-case plugin -> categories with its visible items
		CategorySet _allCategories;
	};
}
