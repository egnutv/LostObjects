#pragma once

#include "Forms.h"
#include "Settings.h"

// The settings holotape ("Lost Objects Settings").
//
// It is a terminal holotape from LostObjects.esp. Its menu has the item
// "Rebuild the workshop menus now" and one switch per option. The tiny script
// on the terminal (data/Scripts/Source/User/LostObjects_Holotape.psc) passes
// the selected item ID to the native function LostObjects.HolotapeItem,
// which this DLL provides. Everything else happens here, on the main thread.
//
// Each option has a global in the plugin (1 = on) that the menu items check,
// so the holotape shows [X] or [ ]. The DLL keeps the globals in sync with the
// settings. The choices are saved in LostObjects_Settings.ini (see Settings).
//
// The holotape is put into the player's inventory after a save is loaded if
// it is not there - so a lost or sold holotape comes back.

namespace LO::Holotape
{
	// Order of the options = order of the globals and menu items in the plugin
	// (tools/build_plugin.py, HOLOTAPE_OPTIONS).
	inline constexpr std::array<std::string_view, 5> OPTION_NAMES{
		"Sort lost items into existing categories",
		"Hide ghost objects",
		"Hide items that can already be built",
		"Summary message",
		"Development mode",
	};
	inline constexpr std::size_t OPTION_SUMMARY = 3;
	inline constexpr std::size_t OPTION_DEVELOPMENT = 4;
	static_assert(OPTION_NAMES.size() == Forms::OptionCount, "options in Holotape.h and the plugin differ");

	// F4SE plugin load: provide LostObjects.HolotapeItem to Papyrus.
	void RegisterPapyrus();

	// Game data loaded: give the holotape a model and set the globals.
	void Setup(const OwnedForms& a_forms);

	// Sets the option globals from the settings.
	void SyncGlobals(const OwnedForms& a_forms);

	// After a save was loaded: put the holotape into the inventory if it is missing.
	void GiveIfMissing(const OwnedForms& a_forms);

	// The setting behind option a_index.
	[[nodiscard]] bool& OptionValue(Settings& a_settings, std::size_t a_index);
}
