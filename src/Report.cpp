#include "Report.h"

#include "Util.h"

namespace LO::Report
{
	namespace
	{
		std::string DescribeRecipe(const RE::BGSConstructibleObject* a_recipe)
		{
			const auto* item = a_recipe->createdItem;
			auto itemName = Util::GetFormName(item);
			if (itemName.empty()) {
				itemName = "<no name>";
			}

			return std::format(
				"COBJ {} -> {} {} \"{}\" (recipe from {}, item from {})",
				Util::FormIDToString(a_recipe->GetFormID()),
				item ? item->GetFormTypeString() : "?"sv,
				item ? Util::FormIDToString(item->GetFormID()) : "?"s,
				itemName,
				Util::GetFileName(a_recipe, 0),
				Util::GetFileName(item, 0));
		}

		std::string DescribeEntry(const MenuEntry& a_entry)
		{
			return std::format(
				"{} {} \"{}\" from {} - {} recipe(s)",
				a_entry.isMenu ? "MENU    " : "CATEGORY",
				Util::FormIDToString(a_entry.form->GetFormID()),
				a_entry.displayName,
				a_entry.pluginFile,
				a_entry.recipeCount);
		}
	}

	void Write(const ScanResult& a_scan, const ApplyResult& a_apply, std::string_view a_reason)
	{
		const auto path = F4SE::GetLogDirectoryPath() / "LostObjects_Report.txt";

		std::ofstream file(path, std::ios::out | std::ios::trunc);
		if (!file.is_open()) {
			REX::LogWarning("Report: could not write {}", path.string());
			return;
		}

		file << "Lost Objects - report\n";
		file << "=====================\n\n";
		file << "Plugin version : " << F4SE::GetPluginVersion().ToString<char>() << "\n";
		file << "Game version   : " << F4SE::GetRuntimeVersion().ToString<char>() << "\n";
		file << "Scan reason    : " << a_reason << "\n\n";

		file << "Vibecoded by AI (Claude, Anthropic). AI-generated code can contain mistakes.\n";
		file << "If something looks wrong, please send this file and LostObjects.log.\n\n";

		file << "Summary\n-------\n";
		file << "Reachable workshop recipes : " << a_scan.reachableRecipeCount << "\n";
		file << "Reachable categories       : " << a_scan.reachableCategoryCount << "\n";
		file << "Reachable menus            : " << a_scan.reachableMenuCount << "\n";
		file << "Lost recipes recovered     : " << a_scan.lostRecipes.size() << "\n";
		file << "Recovered menus/categories : " << a_scan.entries.size() << "\n";
		file << "Uncategorized recipes      : " << a_scan.uncategorized.size() << "\n";
		file << "Skipped (no category)      : " << a_scan.skippedWithoutCategory << " (filter bSkipWithoutCategory)\n";
		file << "Put back where they belong : " << a_apply.placedHome << " recipes\n";
		file << "In the Lost Objects tab    : " << a_apply.placedTab << " recipes\n";
		file << "Did not fit (pool full)    : " << a_apply.overflow << " recipes\n";
		file << "Menus/categories created   : " << a_apply.entryCount << "\n";
		file << "Folders in the tab         : " << a_apply.folderCount << "\n";
		file << "Tab visible                : " << (a_apply.tabVisible ? "yes" : "no") << "\n\n";

		file << "Added to existing menus\n-----------------------\n";
		if (a_apply.injections.empty()) {
			file << "  (none)\n";
		}
		for (const auto& injection : a_apply.injections) {
			file << "  " << injection << "\n";
		}
		file << "\n";

		file << "Workbench keywords used by workshop recipes\n-------------------------------------------\n";
		for (const auto& bench : a_scan.workbenchKeywords) {
			file << "  " << bench << "\n";
		}
		file << "\n";

		file << "Recovered menus and categories\n------------------------------\n";
		if (a_scan.entries.empty()) {
			file << "  (none)\n";
		}
		for (const auto& entry : a_scan.entries) {
			file << "  " << DescribeEntry(entry) << "\n";
		}
		file << "\n";

		file << "Lost recipes\n------------\n";
		if (a_scan.lostRecipes.empty()) {
			file << "  (none)\n";
		}
		for (const auto& lost : a_scan.lostRecipes) {
			file << "  " << (lost.uncategorized ? "[uncategorized] " : "") << DescribeRecipe(lost.recipe) << "\n";
		}
	}

	void Log(const ScanResult& a_scan, const ApplyResult& a_apply, std::string_view a_reason, bool a_verbose)
	{
		REX::LogInformation(
			"Scan ({}): {} reachable recipes in {} categories / {} menus; {} lost recipes -> {} entries, {} uncategorized ({} skipped by filter), {} put back, {} in the tab, {} folders, tab {}",
			a_reason,
			a_scan.reachableRecipeCount,
			a_scan.reachableCategoryCount,
			a_scan.reachableMenuCount,
			a_scan.lostRecipes.size(),
			a_scan.entries.size(),
			a_scan.uncategorized.size(),
			a_scan.skippedWithoutCategory,
			a_apply.placedHome,
			a_apply.placedTab,
			a_apply.folderCount,
			a_apply.tabVisible ? "visible" : "hidden");

		for (const auto& bench : a_scan.workbenchKeywords) {
			REX::LogInformation("  workshop workbench keyword: {}", bench);
		}

		for (const auto& entry : a_scan.entries) {
			REX::LogInformation("  {}", DescribeEntry(entry));
		}
		for (const auto& injection : a_apply.injections) {
			REX::LogInformation("  added to: {}", injection);
		}

		if (a_verbose) {
			for (const auto& lost : a_scan.lostRecipes) {
				REX::LogInformation("    {}{}", lost.uncategorized ? "[uncategorized] " : "", DescribeRecipe(lost.recipe));
			}
		}
	}
}
