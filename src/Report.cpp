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
		file << "Mod folders in use         : " << a_apply.folderCount << "\n";
		file << "Tab visible                : " << (a_apply.tabVisible ? "yes" : "no (nothing lost)") << "\n\n";

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
			"Scan ({}): {} reachable recipes in {} categories / {} menus; {} lost recipes -> {} entries, {} uncategorized ({} skipped by filter), {} folders, tab {}",
			a_reason,
			a_scan.reachableRecipeCount,
			a_scan.reachableCategoryCount,
			a_scan.reachableMenuCount,
			a_scan.lostRecipes.size(),
			a_scan.entries.size(),
			a_scan.uncategorized.size(),
			a_scan.skippedWithoutCategory,
			a_apply.folderCount,
			a_apply.tabVisible ? "visible" : "hidden");

		for (const auto& bench : a_scan.workbenchKeywords) {
			REX::LogInformation("  workshop workbench keyword: {}", bench);
		}

		for (const auto& entry : a_scan.entries) {
			REX::LogInformation("  {}", DescribeEntry(entry));
		}

		if (a_verbose) {
			for (const auto& lost : a_scan.lostRecipes) {
				REX::LogInformation("    {}{}", lost.uncategorized ? "[uncategorized] " : "", DescribeRecipe(lost.recipe));
			}
		}
	}
}
