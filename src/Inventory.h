#pragma once

#include "MenuBuilder.h"
#include "Scanner.h"

namespace LO::Inventory
{
	// Writes two files to "Documents/My Games/Fallout4/F4SE/":
	//
	//   LostObjects_Inventory.tsv  one line per recovered recipe with everything
	//                              known about it (item, model path, size,
	//                              categories, components, keywords, ...).
	//                              Tab separated, opens in Excel/LibreOffice.
	//   LostObjects_Patterns.txt   frequency statistics over the inventory:
	//                              common words in names, model folders, form
	//                              types, plugins. Used to design better
	//                              automatic categories.
	void Write(const ScanResult& a_scan, const ApplyResult& a_apply, std::string_view a_reason);
}
