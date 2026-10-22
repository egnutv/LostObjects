#include "Crafting.h"

namespace LO::Crafting
{
	namespace
	{
		struct Counts
		{
			std::uint32_t chems{ 0 };
			std::uint32_t explosives{ 0 };
		};
	}

	Spot FindChemistryStation()
	{
		Spot spot;
		auto* dataHandler = RE::TESDataHandler::GetSingleton();
		if (!dataHandler) {
			return spot;
		}

		// The chemistry station makes chems (ALCH) and explosives (WEAP).
		// Cooking stations only make ALCH; weapon and armor workbenches make mods.
		std::unordered_map<RE::BGSKeyword*, Counts> counts;
		for (auto* recipe : dataHandler->GetFormArray<RE::BGSConstructibleObject>()) {
			if (!recipe || !recipe->benchKeyword || !recipe->createdItem) {
				continue;
			}
			const auto type = recipe->createdItem->GetFormType();
			if (type == RE::FormType::kPotion) {
				++counts[recipe->benchKeyword].chems;
			} else if (type == RE::FormType::kWeapon) {
				++counts[recipe->benchKeyword].explosives;
			}
		}
		std::uint32_t bestScore = 0;
		for (const auto& [bench, count] : counts) {
			const auto score = std::min(count.chems, count.explosives);
			if (score > bestScore) {
				bestScore = score;
				spot.bench = bench;
			}
		}
		if (!spot.bench) {
			return spot;
		}

		// The category: the one of the station's recipes that make neither chems
		// nor explosives (utility items); else the station's most used category.
		std::unordered_map<RE::BGSKeyword*, std::uint32_t> utility;
		std::unordered_map<RE::BGSKeyword*, std::uint32_t> any;
		for (auto* recipe : dataHandler->GetFormArray<RE::BGSConstructibleObject>()) {
			if (!recipe || recipe->benchKeyword != spot.bench || !recipe->createdItem) {
				continue;
			}
			const auto type = recipe->createdItem->GetFormType();
			for (auto* keyword : recipe->GetRecipeKeywords()) {
				if (!keyword) {
					continue;
				}
				++any[keyword];
				if (type != RE::FormType::kPotion && type != RE::FormType::kWeapon) {
					++utility[keyword];
				}
			}
		}
		const auto mostUsed = [](const std::unordered_map<RE::BGSKeyword*, std::uint32_t>& a_counts) -> RE::BGSKeyword* {
			RE::BGSKeyword* best = nullptr;
			std::uint32_t bestCount = 0;
			for (const auto& [keyword, count] : a_counts) {
				if (count > bestCount || (count == bestCount && best && keyword->GetFormID() < best->GetFormID())) {
					best = keyword;
					bestCount = count;
				}
			}
			return best;
		};
		spot.category = mostUsed(utility);
		if (!spot.category) {
			spot.category = mostUsed(any);
		}
		return spot;
	}

	bool MakeCraftable(RE::BGSConstructibleObject* a_recipe, const Spot& a_spot)
	{
		if (!a_recipe || !a_spot.bench) {
			return false;
		}
		a_recipe->benchKeyword = a_spot.bench;
		if (a_spot.category) {
			std::array<RE::BGSKeyword*, 1> categories{ a_spot.category };
			a_recipe->SetRecipeKeywords(categories);
		}
		return true;
	}
}
