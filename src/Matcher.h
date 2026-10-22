#pragma once

#include "Scanner.h"

// Finds the existing category a lost recipe belongs to - only from what is
// already in the menu, no fixed lists.
//
// Every recipe is described by features: its model path, the model's folder
// and parent folder, the item name, its first word, a "[Tag]" at the start of
// the name, the single words of the name and the name of its (lost) category.
//
// For each feature of a lost recipe the index looks at where ALL visible
// recipes with the same feature are shown in the whole menu. A category that
// holds most of them gets most of the feature's weight; a feature that is
// everywhere ("setdressing", "box") gives each category only a tiny part.
// The category with the highest total wins if
//   * it shares at least the model, the model folder, the name, the first
//     word, a tag or the category name with the lost item (words and parent
//     folders alone never decide),
//   * the confidence reaches iMinConfidence and
//   * it is clearly ahead of the runner-up.
// Otherwise the recipe is not sorted in (it goes into a "Lost Objects" folder
// instead - better there than in the wrong place).
//
// Candidates are the categories in which the recipe's own plugin already has
// visible items. If the plugin has none, all categories are candidates, but
// then at least 60 % confidence is required.
//
// Recipes that Lost Objects puts back where their plugin had them (see
// PluginHistory) count like visible recipes: they show where the mod keeps
// its items, even if that menu was not reachable before.

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

		// Recipes the index learned from (visible + put back), categories it knows.
		[[nodiscard]] std::size_t ReferenceCount() const noexcept { return _referenceCount; }
		[[nodiscard]] std::size_t CategoryCount() const noexcept { return _allCategories.size(); }

	private:
		std::size_t _referenceCount{ 0 };
		using Counts = std::unordered_map<const RE::BGSKeyword*, std::uint32_t>;
		using CategorySet = std::unordered_set<const RE::BGSKeyword*>;

		std::unordered_map<std::string, Counts> _features;            // feature key -> category -> visible recipes
		std::unordered_map<std::string, CategorySet> _pluginCategories;  // lower-case plugin -> categories with its visible items
		CategorySet _allCategories;
	};
}
