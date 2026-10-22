#pragma once

// What the menus looked like in every plugin file, before the next one changed
// them.
//
// The game only keeps the final version of a form list (FLST) - the one of
// the last plugin in the load order that changes it. To know where a lost
// category or sub menu belongs, Lost Objects reads the form lists of every
// loaded plugin file itself, in load order, once per game start. Read-only;
// a broken or missing file only means its versions are missing (logged).

namespace LO::PluginHistory
{
	struct Version
	{
		std::uint32_t order{ 0 };  // position of the file in the load order
		std::string file;  // plugin file name
		std::vector<RE::TESFormID> entries;  // in the order of the plugin, as form IDs of the running game
	};

	// A place where a form was listed: list `list`, version `version` of it, at `index`.
	struct Place
	{
		RE::TESFormID list{ 0 };
		const Version* version{ nullptr };
		std::size_t index{ 0 };
	};

	class History
	{
	public:
		// Every place a form was listed in, newest version (latest plugin) first.
		[[nodiscard]] const std::vector<Place>& PlacesOf(RE::TESFormID a_form) const;

		// The versions of a form list, in load order (empty if never read).
		[[nodiscard]] const std::vector<Version>& VersionsOf(RE::TESFormID a_list) const;

		[[nodiscard]] bool Empty() const noexcept { return _lists.empty(); }
		[[nodiscard]] std::size_t ListCount() const noexcept { return _lists.size(); }

	private:
		friend const History& Get();
		std::unordered_map<RE::TESFormID, std::vector<Version>> _lists;
		std::unordered_map<RE::TESFormID, std::vector<Place>> _places;
	};

	// Reads all plugin files the first time it is called (main thread, game data loaded).
	[[nodiscard]] const History& Get();
}
