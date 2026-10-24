#pragma once

// Access to the game's "sparse name map": the table that stores the display
// names of forms without their own name field (keywords, form lists, statics).
//
// Version 0.1.0 crashed on Fallout 4 1.11.240 because CommonLibF4 AV points to
// the wrong address for this table on the Anniversary Edition. Lost Objects
// therefore tries two addresses (0x08 apart) and checks each before it is
// used: it looks up one of its own keywords and compares the name with the one
// stored in LostObjects.esp, under structured exception handling. Only an
// address that passes is used. If none passes, names are simply not used and
// the rest of the mod keeps working.

namespace LO::NameMap
{
	// Resolves and verifies the table. a_probe must be a keyword whose name is
	// a_expectedName (taken from LostObjects.esp).
	bool Initialize(const RE::BGSKeyword* a_probe, std::string_view a_expectedName);

	[[nodiscard]] bool IsAvailable() noexcept;

	// Name of a form: its own name field if it has one, otherwise the verified
	// name table. Empty if unknown.
	[[nodiscard]] std::string GetName(const RE::TESForm* a_form);

	// Renames a form that already has an entry in the table.
	bool SetName(const RE::TESForm* a_form, std::string_view a_name);
}
