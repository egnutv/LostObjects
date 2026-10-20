# Changelog

All notable changes to Lost Objects. Format based on
[Keep a Changelog](https://keepachangelog.com/), versions follow
[Semantic Versioning](https://semver.org/).

## [0.4.4] - 2026-10-20

### Fixed
- The menu icon showed the logo twice on top of each other (two planes back to back,
  each double sided) and too dark. It is now one plane like the flat icons of other
  mods, with an unlit shader, so the logo keeps its own colours.
- The icon is scaled to the size of the vanilla "Decorations" icon (ratio of the
  two icons' bounds); before it was much smaller than the other icons.

### Removed
- `Textures/LostObjects/MenuIcon_n.dds` (not needed by the unlit shader).

## [0.4.3] - 2026-10-20

### Fixed
- The menu icon was invisible: workshop menu icons need a preview transform (TRNS)
  that tells the menu how to show the model. `LostObjects.esp` now has
  `LO_MenuIconTransform`; at runtime it gets the values of the vanilla "Decorations"
  icon, so the logo is framed like the other icons.

### Added
- The log lists how the icons of all main menu categories are set up (model,
  transform), to tune the icon if needed.

### Changed
- NOTICE.md: videos, streams, screenshots and reviews of the mod are explicitly
  allowed, also monetized; Bethesda's video policy still applies to game content.

## [0.4.2] - 2026-10-20

### Changed
- The menu icon is now just the logo, exactly as the transparent `Logo/Logo.png` -
  no frame, no background (alpha tested, two planes back to back).

## [0.4.1] - 2026-10-20

### Added
- Own workshop menu icon: a small picture frame with the Lost Objects logo
  (`LO_MenuIcon` in `LostObjects.esp`, `Meshes\LostObjects\MenuIcon.nif`,
  `Textures\LostObjects\MenuIcon_d.dds`). Built by `tools/icon` (mesh with the nifly
  library as a build tool, textures from `Logo/Logo.png`). `bCustomMenuIcon=0` or a
  missing mesh falls back to the vanilla "Decorations" icon.

### Fixed
- Two "Lost Objects" folders in the same menu when an add-on shares the menu of its
  main mod (Snappy HouseKit + Snappy_DLC03). Now there is one folder per menu; the
  add-on gets a sub folder named after its plugin.
- Variant lists whose every variant can already be built through its own recipe were
  shown again (duplicates in "Lost Objects > HouseKit"). Such a list is now skipped
  like other already buildable items; the variants keep their own recipes. A list is
  only skipped if every single variant is buildable, otherwise it is shown.
- "Already buildable" only counts recipes without conditions, so nothing that needs a
  perk or a quest elsewhere is hidden.
- NOTICE.md: the permission for player creations only covers our rights; Bethesda's
  terms for game content still apply.

## [0.4.0] - 2026-10-20

### Changed
- Sorting into existing categories is stricter and fairer: the share of a feature is
  now counted over the whole menu, so a folder that is everywhere ("setdressing",
  "props") no longer looks like a perfect match. New feature: the first word of the
  name. The winner must share the model, the model folder, the name, the first word, a
  tag or the category name with the lost item - words and parent folders alone never
  decide. This removes wrong placements like a film projector in "Floor coverings".
  `iMinConfidence` default is now 30 % (the numbers are lower because of the fairer
  counting).
- Plugins without a menu of their own (add-ons like `Snappy_DLC03.esp`) now get the
  menu that holds at least 80 % of their visible items as home, if that menu mostly
  belongs to them or to the plugin that made it. Their "Lost Objects" folder goes there
  instead of the tab.
- In a "Lost Objects" folder, items of a borrowed category (for example the base game's
  "Scrap" used by a mod) are grouped by what they have in common ("Wallpapers",
  "Roof", ...) instead of all landing in "Scrap".

### Added
- Ghost objects are hidden (`bSkipMissingModels=1`): recipes whose item, or every
  variant of it, points to a model file that does not exist. Checked through the
  game's file system (loose files and archives) after a self-test; listed in the
  report and the inventory ("skipped, model file missing").

## [0.3.0] - 2026-10-20

### Changed
- Recovered items are sorted into the **existing category they belong to** (new
  `bSortIntoCategories=1`). The category is found by comparing the item with what is
  already in the menu: model file, model folder, parent folder, name, `[Tag]`, words and
  the name of its lost category. Candidates are the categories where the item's plugin
  already has visible items. Only clear matches are used (`iMinConfidence`, default 45 %).
- What cannot be sorted in goes into a **"Lost Objects" folder inside the mod's own top
  menu** instead of a loose entry next to the author's folders.
- Removed: putting a lost category loosely into a sub menu with the same title.
- New filter `bSkipAlreadyBuildable=1`: lost recipes whose item can already be built
  through a reachable recipe are skipped (for example about 980 "scrap return" recipes of
  OCDecorator), so items no longer appear twice.
- Inventory: new columns "Status" and "Why" (what happened to each item, which features
  matched, confidence, best guess when not sorted in); skipped items are listed too.
- Report: summary per placement and a list of everything sorted into existing categories.
- `LostObjects_Menu.tsv`: new column "Model".
- The summary message also counts the items sorted into existing categories.

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
