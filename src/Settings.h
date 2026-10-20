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

	// Values from Data/F4SE/Plugins/LostObjects.ini, then the choices made on the
	// settings holotape (Documents\My Games\Fallout4\F4SE\LostObjects_Settings.ini,
	// written by the DLL), which win. Every setting has a safe default, so a
	// missing or broken INI never stops the mod.
	class Settings
	{
	public:
		[[nodiscard]] static Settings& Get() noexcept;

		void Load();

		// Writes the settings that the holotape can change into the holotape
		// file, so they stay after a restart. The INI of the mod is never touched.
		void SaveHolotapeChoices() const;

		bool enabled{ true };
		bool liveUpdate{ false };            // true: also apply changes during the first minute without waiting for a second scan
		std::uint32_t settleSeconds{ 10 };   // earliest build after loading (two equal scans needed)
		Placement placement{ Placement::kHome };
		SortMode sortMode{ SortMode::kByMod };
		std::uint32_t splitThreshold{ 40 };  // categories with more recipes are split into groups
		std::uint32_t maxGroups{ 40 };
		std::uint32_t minGroupSize{ 3 };
		bool skipUntouchedBaseGame{ false };  // filter, off = show everything
		bool skipWithoutCategory{ false };    // filter, off = show everything
		bool skipAlreadyBuildable{ true };    // the item itself can already be built through another recipe
		bool skipMissingModels{ true };       // ghost objects: no model file the game can load
		bool sortIntoCategories{ true };      // put lost items into matching existing categories
		std::uint32_t minConfidence{ 30 };    // percent a match needs
		bool showSummaryMessage{ true };
		bool borrowMenuIcon{ true };   // give the tab and folders an icon
		bool customMenuIcon{ true };   // the Lost Objects icon instead of the vanilla "Decorations" icon
		bool writeReport{ true };
		bool writeInventory{ false };  // bDevelopmentMode: large analysis files
		bool verboseLog{ false };
		std::vector<std::string> ignoredPlugins;  // lower case file names

		[[nodiscard]] bool IsPluginIgnored(std::string_view a_fileName) const;

	private:
		Settings() = default;
	};
}
