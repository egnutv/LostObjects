#include "Settings.h"

#include "Util.h"

namespace LO
{
	namespace
	{
		constexpr auto INI_PATH = "Data/F4SE/Plugins/LostObjects.ini"sv;

		// Minimal INI reader: [Section] lines and key=value pairs.
		// Comments start with ';' or '#'. Keys are case-insensitive.
		using IniData = std::map<std::string, std::string>;  // "section.key" -> value

		IniData ReadIni(const std::filesystem::path& a_path)
		{
			IniData data;

			std::ifstream file(a_path);
			if (!file.is_open()) {
				return data;
			}

			std::string section;
			std::string line;
			while (std::getline(file, line)) {
				const auto trimmed = Util::Trim(line);
				if (trimmed.empty() || trimmed.front() == ';' || trimmed.front() == '#') {
					continue;
				}

				if (trimmed.front() == '[') {
					const auto close = trimmed.find(']');
					section = Util::ToLower(Util::Trim(trimmed.substr(1, close == std::string::npos ? std::string::npos : close - 1)));
					continue;
				}

				const auto equals = trimmed.find('=');
				if (equals == std::string::npos) {
					continue;
				}

				auto key = Util::ToLower(Util::Trim(trimmed.substr(0, equals)));
				auto value = Util::Trim(trimmed.substr(equals + 1));

				// Allow trailing comments: "bEnabled=1 ; comment"
				const auto comment = value.find_first_of(";#");
				if (comment != std::string::npos) {
					value = Util::Trim(value.substr(0, comment));
				}

				data[section + "." + key] = value;
			}

			return data;
		}

		bool GetBool(const IniData& a_data, std::string_view a_key, bool a_default)
		{
			const auto it = a_data.find(std::string(a_key));
			if (it == a_data.end()) {
				return a_default;
			}

			const auto value = Util::ToLower(it->second);
			if (value == "1" || value == "true" || value == "yes" || value == "on") {
				return true;
			}
			if (value == "0" || value == "false" || value == "no" || value == "off") {
				return false;
			}

			REX::LogWarning("Settings: invalid value '{}' for '{}', using default", it->second, a_key);
			return a_default;
		}

		std::uint32_t GetUInt(const IniData& a_data, std::string_view a_key, std::uint32_t a_default, std::uint32_t a_min, std::uint32_t a_max)
		{
			const auto it = a_data.find(std::string(a_key));
			if (it == a_data.end()) {
				return a_default;
			}
			std::uint32_t value = 0;
			const auto* begin = it->second.data();
			const auto* end = begin + it->second.size();
			const auto [ptr, ec] = std::from_chars(begin, end, value);
			if (ec != std::errc() || ptr != end) {
				REX::LogWarning("Settings: invalid number '{}' for '{}', using default", it->second, a_key);
				return a_default;
			}
			return std::clamp(value, a_min, a_max);
		}

		std::string GetString(const IniData& a_data, std::string_view a_key, std::string_view a_default)
		{
			const auto it = a_data.find(std::string(a_key));
			return it == a_data.end() ? std::string(a_default) : it->second;
		}

		std::vector<std::string> SplitList(std::string_view a_text)
		{
			std::vector<std::string> result;

			std::size_t start = 0;
			while (start <= a_text.size()) {
				const auto comma = a_text.find(',', start);
				const auto end = comma == std::string_view::npos ? a_text.size() : comma;
				auto item = Util::ToLower(Util::Trim(a_text.substr(start, end - start)));
				if (!item.empty()) {
					result.push_back(std::move(item));
				}
				if (comma == std::string_view::npos) {
					break;
				}
				start = comma + 1;
			}

			return result;
		}
	}

	Settings& Settings::Get() noexcept
	{
		static Settings singleton;
		return singleton;
	}

	void Settings::Load()
	{
		const std::filesystem::path path(INI_PATH);
		const auto data = ReadIni(path);

		if (data.empty()) {
			REX::LogWarning("Settings: '{}' not found or empty, using defaults", INI_PATH);
		}

		enabled = GetBool(data, "general.benabled", true);
		liveUpdate = GetBool(data, "general.bliveupdate", false);
		settleSeconds = GetUInt(data, "general.isettleseconds", 10, 0, 600);
		showSummaryMessage = GetBool(data, "general.bshowsummarymessage", true);
		borrowMenuIcon = GetBool(data, "general.bborrowmenuicon", true);
		customMenuIcon = GetBool(data, "general.bcustommenuicon", true);
		writeReport = GetBool(data, "general.bwritereport", true);
		// Development mode: the large analysis files (inventory, patterns, full
		// menu, duplicates). Off for players; bWriteInventory is the old name.
		writeInventory = GetBool(data, "general.bdevelopmentmode", GetBool(data, "general.bwriteinventory", false));
		verboseLog = GetBool(data, "general.bverboselog", false);

		const auto sortText = Util::ToLower(GetString(data, "general.ssortmode", "ByMod"));
		if (sortText == "flat") {
			sortMode = SortMode::kFlat;
		} else {
			if (sortText != "bymod") {
				REX::LogWarning("Settings: unknown sSortMode '{}', using ByMod", sortText);
			}
			sortMode = SortMode::kByMod;
		}

		skipUntouchedBaseGame = GetBool(data, "filters.bskipuntouchedbasegame", false);
		skipWithoutCategory = GetBool(data, "filters.bskipwithoutcategory", false);
		skipAlreadyBuildable = GetBool(data, "filters.bskipalreadybuildable", true);
		skipMissingModels = GetBool(data, "filters.bskipmissingmodels", true);
		sortIntoCategories = GetBool(data, "general.bsortintocategories", true);
		minConfidence = GetUInt(data, "general.iminconfidence", 30, 1, 100);
		const auto placementText = Util::ToLower(GetString(data, "general.splacement", "Home"));
		if (placementText == "tab") {
			placement = Placement::kTab;
		} else {
			if (placementText != "home") {
				REX::LogWarning("Settings: unknown sPlacement '{}', using Home", placementText);
			}
			placement = Placement::kHome;
		}

		splitThreshold = GetUInt(data, "grouping.isplitthreshold", 40, 5, 10000);
		maxGroups = GetUInt(data, "grouping.imaxgroups", 40, 2, 100);
		minGroupSize = GetUInt(data, "grouping.imingroupsize", 3, 2, 1000);

		ignoredPlugins = SplitList(GetString(data, "filters.signoreplugins", ""));

		REX::LogInformation(
			"Settings: enabled={} liveUpdate={} settle={}s placement={} sortIntoCategories={} minConfidence={} skipAlreadyBuildable={} skipMissingModels={} split>{} maxGroups={} minGroup={} sortMode={} skipUntouchedBaseGame={} skipWithoutCategory={} summary={} borrowIcon={} report={} developmentMode={} verbose={} ignoredPlugins={}",
			enabled,
			liveUpdate,
			settleSeconds,
			placement == Placement::kTab ? "Tab" : "Home",
			sortIntoCategories,
			minConfidence,
			skipAlreadyBuildable,
			skipMissingModels,
			splitThreshold,
			maxGroups,
			minGroupSize,
			sortMode == SortMode::kFlat ? "Flat" : "ByMod",
			skipUntouchedBaseGame,
			skipWithoutCategory,
			showSummaryMessage,
			borrowMenuIcon,
			writeReport,
			writeInventory,
			verboseLog,
			ignoredPlugins.size());
	}

	bool Settings::IsPluginIgnored(std::string_view a_fileName) const
	{
		if (a_fileName.empty() || ignoredPlugins.empty()) {
			return false;
		}

		const auto lower = Util::ToLower(a_fileName);
		return std::ranges::find(ignoredPlugins, lower) != ignoredPlugins.end();
	}
}
