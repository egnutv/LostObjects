#pragma once

#include "MenuBuilder.h"
#include "Scanner.h"

namespace LO::Report
{
	// Writes "Documents/My Games/Fallout4/F4SE/LostObjects_Report.txt".
	// A human readable list of everything that was recovered - the most useful
	// file for testing and bug reports.
	void Write(const ScanResult& a_scan, const ApplyResult& a_apply, std::string_view a_reason);

	// Writes the same information to the log (short form, or full with bVerboseLog).
	void Log(const ScanResult& a_scan, const ApplyResult& a_apply, std::string_view a_reason, bool a_verbose);
}
