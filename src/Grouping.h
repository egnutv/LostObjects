#pragma once

// Splits a large set of recipes into smaller groups, based only on what the
// recipes themselves have in common - no fixed lists of words.
//
// Every recipe is described by "features":
//   * a tag in brackets at the start of the name   "[Alcohol] Beer"   -> Alcohol
//   * the words of the item name                   "HouseKit Wall"    -> HouseKit, Wall
//   * the folders of the item's model path         "...\Flora\Fern.nif" -> Flora
//
// Features shared by more than half of the recipes are too broad and ignored.
// Then, strongest kind first (tags, words, folders), the feature shared by the
// most remaining recipes becomes a group, again and again, until the groups get
// too small. What is left goes to "Other". If nothing useful is found, the
// recipes are split alphabetically. Large groups can be split again.

namespace LO::Grouping
{
	struct Group
	{
		std::string label;
		std::vector<RE::BGSConstructibleObject*> recipes;
	};

	struct Options
	{
		std::size_t maxGroups{ 40 };  // including "Other"
		std::size_t minGroupSize{ 3 };
		std::size_t chunkSize{ 30 };  // alphabetical fallback
	};

	[[nodiscard]] std::vector<Group> Split(const std::vector<RE::BGSConstructibleObject*>& a_recipes, const Options& a_options);

	// Name and model of what a recipe builds. If it builds a form list (a set
	// of variants), the list's name and the first variant's model are used.
	[[nodiscard]] std::string ItemName(const RE::BGSConstructibleObject* a_recipe);
	[[nodiscard]] std::string ItemModel(const RE::BGSConstructibleObject* a_recipe);
}
