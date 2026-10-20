#pragma once

namespace LO::Notice
{
	// Shows the AI disclosure once per plugin version (after the first save is
	// loaded or a new game is started). The acknowledged version is stored in
	// "Documents/My Games/Fallout4/F4SE/LostObjects_State.ini".
	void ShowIfNeeded();

	// The text of the disclosure (also used in the log).
	[[nodiscard]] std::string_view GetText() noexcept;
}
