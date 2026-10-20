#pragma once

// -----------------------------------------------------------------------------
// GENERATED FILE - do not edit by hand.
// Source: tools/build_plugin.py (run it to regenerate this file and the .esp).
// -----------------------------------------------------------------------------

#include <cstdint>
#include <string_view>

namespace LO::Forms
{
	inline constexpr std::string_view PluginFile = "LostObjects.esp";
	inline constexpr std::string_view RootName = "Lost Objects";  // FULL of RootKeyword

	inline constexpr std::uint32_t RootKeyword = 0x800;
	inline constexpr std::uint32_t RootMenu = 0xBC1;

	inline constexpr std::uint32_t MenuCount = 192;
	inline constexpr std::uint32_t FirstMenuTitle = 0x801;
	inline constexpr std::uint32_t FirstMenu = 0xBC2;

	inline constexpr std::uint32_t LeafCount = 768;
	inline constexpr std::uint32_t FirstLeaf = 0x8C1;

	inline constexpr std::uint32_t MenuIcon = 0xC82;  // ARTO with Meshes\LostObjects\MenuIcon.nif
	inline constexpr std::uint32_t MenuIconTransform = 0xC83;  // TRNS, preview transform of MenuIcon
	inline constexpr std::string_view MenuIconModel = "LostObjects\\MenuIcon.nif";

	// Vanilla forms (Fallout4.esm).
	inline constexpr std::string_view BaseGameFile = "Fallout4.esm";
	inline constexpr std::uint32_t WorkshopMenuMain = 0x106DA2;
}
