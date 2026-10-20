#pragma once

namespace LO
{
	enum class Placement
	{
		kHome,  // put recovered things into the menu they belong to, if it can be found
		kTab    // everything into the "Lost Objects" tab
	};

	enum class SortMode
	{
		kByMod,  // "Lost Objects" -> one folder per plugin file (mod, DLC, base game) -> recovered menus
		kFlat    // "Lost Objects" -> recovered menus directly
	};

	// Values from Data/F4SE/Plugins/LostObjects.ini.
	// Every setting has a safe default, so a missing or broken INI never stops the mod.
	class Settings
	{
	public:
		[[nodiscard]] static Settings& Get() noexcept;

		void Load();

		bool enabled{ true };
		Placement placement{ Placement::kHome };
		SortMode sortMode{ SortMode::kByMod };
		std::uint32_t splitThreshold{ 40 };  // categories with more recipes are split into groups
		std::uint32_t maxGroups{ 40 };
		std::uint32_t minGroupSize{ 3 };
		bool skipUntouchedBaseGame{ false };  // filter, off = show everything
		bool skipWithoutCategory{ false };    // filter, off = show everything
		bool showSummaryMessage{ true };
		bool borrowMenuIcon{ true };
		bool writeReport{ true };
		bool writeInventory{ true };
		bool verboseLog{ false };
		std::vector<std::string> ignoredPlugins;  // lower case file names

		[[nodiscard]] bool IsPluginIgnored(std::string_view a_fileName) const;

	private:
		Settings() = default;
	};
}
