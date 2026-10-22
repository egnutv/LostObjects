#pragma once

// Where the settings holotape is crafted: at the chemistry station, found
// from the game data (no fixed form IDs), free of charge.

namespace LO::Crafting
{
	struct Spot
	{
		RE::BGSKeyword* bench{ nullptr };  // the chemistry station's workbench keyword
		RE::BGSKeyword* category{ nullptr };  // its category for utility items
	};

	[[nodiscard]] Spot FindChemistryStation();

	// Puts a recipe (from our plugin, no components = free) at that station.
	bool MakeCraftable(RE::BGSConstructibleObject* a_recipe, const Spot& a_spot);
}
