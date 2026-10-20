#pragma once

#include "Forms.h"

namespace LO::MenuDump
{
	// Walks the complete workshop menu as the game sees it right now
	// (including the "Lost Objects" tab) and writes to
	// "Documents/My Games/Fallout4/F4SE/":
	//
	//   LostObjects_Menu.tsv        every path in the menu and every recipe that
	//                               a category shows (conditions not evaluated).
	//   LostObjects_Duplicates.txt  everything that appears more than once:
	//                               recipes in several places, items buildable
	//                               through several recipes, categories and sub
	//                               menus reachable through several paths, and
	//                               menu paths with the same name.
	void Write(const OwnedForms& a_forms, std::string_view a_reason);
}
