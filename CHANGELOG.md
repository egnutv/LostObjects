# Changelog

All notable changes to Lost Objects. Format based on
[Keep a Changelog](https://keepachangelog.com/), versions follow
[Semantic Versioning](https://semver.org/).

## [0.2.0] - 2026-10-20

### Changed
- Recovered items are put back where they belong (`sPlacement=Home`, default): into the
  sub menu whose title is their category, or into the plugin's own top menu. Only what
  has no home goes into the "Lost Objects" tab. A plugin with a single entry in the tab
  gets no extra folder level.
- Every recovered recipe is now tagged with exactly one category from a pool in
  `LostObjects.esp`. This fixes duplicates: before, a whole foreign category was shown,
  including recipes that were reachable elsewhere.
- Large categories are split automatically into groups by tags in names (`[Alcohol]`),
  words and model folders - derived from the data, no fixed lists. Settings in the new
  `[Grouping]` section: `iSplitThreshold`, `iMaxGroups`, `iMinGroupSize`.
- The menus are only rebuilt when the result changes; entries removed by other mods are
  added again on the next scan.
- Inventory column "Shown at" now contains the real place in the menu.
- The duplicate report detects Lost Objects entries by their forms, also inside other
  mods' menus.

## [0.1.4] - 2026-10-20

### Added
- `LostObjects_Menu.tsv`: the complete workshop menu as the game sees it (every path and
  every recipe each category shows, including the Lost Objects tab).
- `LostObjects_Duplicates.txt`: recipes shown in several places, items buildable through
  several recipes, categories and sub menus reachable through several paths, menu paths
  with the same name, and a self-check that nothing in Lost Objects is also in the normal
  menu. Both written together with the inventory (`bWriteInventory`).

## [0.1.3] - 2026-10-20

### Added
- `LostObjects_Inventory.tsv`: one line per recovered recipe with folder, entry, recipe and
  item IDs, plugins, item name and type, model path, size, categories, workbench,
  components, conditions and item keywords. Opens in Excel or LibreOffice.
- `LostObjects_Patterns.txt`: frequency statistics over the inventory (folders, entries,
  plugins, form types, sizes, model folders, words in names, keywords) as a basis for
  better automatic categories.
- INI setting `bWriteInventory` (default on).

## [0.1.2] - 2026-10-20

### Fixed
- The in-game notice still had the old title "LOST MOD OBJECTS".
- Single lost categories were filed under the plugin that defined the category keyword,
  not the plugin the recipes come from. Example: scrap recipes added by Scrap Everything
  use the vanilla "Scrap" keyword and ended up in the folder "Fallout4". A category now
  goes into the folder of the plugin that contributes most of its lost recipes.

## [0.1.1] - 2026-10-20

### Fixed
- Crash after loading a save on Fallout 4 1.11.240. CommonLibF4 AV points to the wrong
  address for the game's name table (names of keywords and form lists) on the Anniversary
  Edition. Lost Objects now resolves this address itself and verifies it with one of its
  own keywords before using it. If the check fails, names are not used and the mod keeps
  working (folders fall back to `sSortMode=Flat`).

### Added
- Safety net: if a scan ever fails with an internal error, Lost Objects switches itself
  off for the session and shows a message instead of crashing the game.
- More detailed progress lines in `LostObjects.log` for the first scans after loading.

## [0.1.0] - 2026-10-20

First test version.

### Added
- New workshop tab **Lost Objects** that collects every workshop recipe which cannot be
  reached from the main workshop menu - from mods, DLCs or the base game.
- Lost menus are restored with their original structure; lost categories keep their
  original names; recipes keep their costs and conditions.
- One folder per plugin file (`sSortMode=ByMod`, default) or a flat list (`sSortMode=Flat`).
- Lost recipes without a named category appear in "Uncategorized" inside their folder.
- Optional filters, all off by default: `bSkipUntouchedBaseGame`, `bSkipWithoutCategory`,
  `sIgnorePlugins`.
- Automatic scans after loading a save (with follow-ups for script-injected menus) and on
  entering workshop mode. The tab only appears when something is lost.
- Summary message on the first workshop visit after loading.
- AI disclosure message shown once per version.
- `LostObjects_Report.txt` with every recovered menu, category and recipe.
- INI settings: `bEnabled`, `sSortMode`, `bShowSummaryMessage`, `bBorrowMenuIcon`,
  `bWriteReport`, `bVerboseLog`, `bSkipUntouchedBaseGame`, `bSkipWithoutCategory`, `sIgnorePlugins`.
- One DLL for Fallout 4 1.10.163, 1.10.984 and 1.11.191+ (CommonLibF4 AV).
- Generated, reviewable light plugin (`tools/build_plugin.py`) with an independent checker.
- GitHub Actions build with automatic release archives.
