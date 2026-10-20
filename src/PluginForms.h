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
	inline constexpr std::uint32_t UncategorizedKeyword = 0x801;
	inline constexpr std::uint32_t RootMenu = 0x880;

	inline constexpr std::uint32_t SlotCount = 48;
	inline constexpr std::uint32_t FirstSlotKeyword = 0x802;
	inline constexpr std::uint32_t FirstSlotMenu = 0x881;
	inline constexpr std::uint32_t FirstSlotUncategorizedKeyword = 0x832;

	// Vanilla forms (Fallout4.esm).
	inline constexpr std::string_view BaseGameFile = "Fallout4.esm";
	inline constexpr std::uint32_t WorkshopMenuMain = 0x106DA2;
}
