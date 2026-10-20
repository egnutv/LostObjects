#pragma once

namespace LO
{
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
		SortMode sortMode{ SortMode::kByMod };
		bool skipUntouchedBaseGame{ false };  // filter, off = show everything
		bool skipWithoutCategory{ false };    // filter, off = show everything
		bool showSummaryMessage{ true };
		bool borrowMenuIcon{ true };
		bool writeReport{ true };
		bool verboseLog{ false };
		std::vector<std::string> ignoredPlugins;  // lower case file names

		[[nodiscard]] bool IsPluginIgnored(std::string_view a_fileName) const;

	private:
		Settings() = default;
	};
}
