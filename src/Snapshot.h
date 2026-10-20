#pragma once

#include "MenuBuilder.h"

// The snapshot: a copy of everything Lost Objects put into the menus, saved
// in Documents\My Games\Fallout4\F4SE\LostObjects_Snapshot.txt.
//
// Why: the game builds its workshop menu once, the first time the workshop
// is opened, and keeps that picture until the game is closed. Lost Objects
// has to wait until other mods have finished adding their menus after a
// save is loaded, which can take a few seconds - open the workshop before
// that and the recovered items are missing for the whole session.
//
// With the snapshot, the result of the last game is put back right when the
// game data is loaded, long before anyone can open the workshop. After
// loading, Lost Objects checks the menus as before; if the result is the
// same (the normal case) nothing changes. If it differs, the menus are
// updated and the snapshot is rewritten.
//
// The snapshot is only used when the plugins (and their order) and the
// settings that decide the menus are exactly the same as when it was written.
// Forms are stored as "plugin file|local form ID", so a different load order
// does not matter. Deleting the file is always safe.

namespace LO::Snapshot
{
	struct Data
	{
		MenuState state;
		PreviewMap previews;
		std::size_t signature{ 0 };
		ApplyResult summary;  // counts for the in-game summary message
	};

	// Fingerprint of everything the snapshot depends on.
	[[nodiscard]] std::uint64_t Fingerprint(const Settings& a_settings);

	// Reads the snapshot. Empty if there is none, it is outdated or a form
	// could not be found (then it is simply not used).
	[[nodiscard]] std::optional<Data> Load(const OwnedForms& a_forms, std::uint64_t a_fingerprint);

	// Writes the current state of the builder.
	void Save(const MenuBuilder& a_builder, const ApplyResult& a_summary, std::uint64_t a_fingerprint);
}
