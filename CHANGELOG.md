# Changelog

All notable changes to Lost Objects. Format based on
[Keep a Changelog](https://keepachangelog.com/), versions follow
[Semantic Versioning](https://semver.org/).

## [0.13.1] - 2026-10-24 - public beta

### Changed
- **All filters are off by default.** `bSkipAlreadyBuildable` and `bSkipMissingModels`
  are now `0`: every lost recipe is shown. "The same object" only means the same form -
  its place in the menu, its set of variants, its costs and its conditions can differ,
  so the filter could hide items players miss (for example parts of Snap'n Build with
  PAN_SnB). If a choice on the holotape was saved in `LostObjects_Settings.ini`, that
  choice still counts - switch it off on the holotape or delete the file.
- **Every recipe a filter hides is listed** in `LostObjects_Report.txt` and
  `LostObjects.log`, with its own category and, for `bSkipAlreadyBuildable`, the place
  where the object can be built.
- **Thumbnails are framed by the item's own size.** Recovered items without a preview
  transform no longer get the one that items of the same form type use most often. They
  get a transform from a new pool of 768 transforms in `LostObjects.esp`, framed like the
  Lost Objects icon: the camera of the icon of the first sub menu of the main menu,
  scaled to the item's own size (its bounds). Nothing is taken from other items.
- **Name table:** two addresses (0x08 apart) are checked one after the other, each with
  the own keyword "Lost Objects" and fully guarded; only one that passes is used.
- **Documentation:** Lost Objects is tested on and supports only Fallout 4 1.11.240. The
  earlier statement that 1.10.984 and 1.10.163 are "supported but untested" was wrong and
  is removed. Other statements that were not backed by tests are corrected, among them
  what a save game contains (the holotape, the values of its option globals and script
  entries for the Lost Objects menu forms; the game drops them when the mod is removed).

## [0.13.0] - 2026-10-23 - public beta

### Added
- **Switch "Leave the menus of other mods untouched"** (INI `bOnlyOwnTab`, off by
  default, also on the settings holotape). When it is on, Lost Objects changes no menu of
  another mod: nothing is put back where its mod had it, nothing is sorted into
  categories of other mods, no "Lost Objects" folders are added to their menus and no
  menus are merged. Everything it thinks is lost goes into the "Lost Objects" tab of the
  main menu, one folder per mod. The plugin has one more global for it; the form ID of
  the holotape recipe changed (0xC8B -> 0xC8C), save games are not affected.

## [0.12.2] - 2026-10-23 - public beta

### Changed
- **Merging menus uses no fixed list any more.** 0.12.1 used the names of the base game
  files to tell a menu shared by many mods (like "Structures") from a mod's own menu.
  Now it is read from the forms: the upper menu and the menu it lies in must have been
  made by the same plugin file.

## [0.12.1] - 2026-10-22 - public beta

### Fixed
- **Merging menus with the same name went into neighbouring branches.** 0.12.0 also
  merged a menu into a same-named menu in another branch of the mod, for example the
  supports of "Concrete > Rusty" into "Wood > Supports" (Workshop Rearranged) or the
  wallpapers of the Log Building Set into the Steel Building Set (Yufarley). Now the
  upper menu must lie directly in one of the menus above the deeper one, on its own
  path up (like "Snap'n Build > Greenhouse" for "Snap'n Build > Real Estate > Yard >
  Greenhouse").

## [0.12.0] - 2026-10-22 - public beta

### Added
- **Menus of a mod with the same name and similar items are merged.** When a mod (or a
  patch for it) has the same set twice, one in a sub folder of the other - for example
  "Snap'n Build > Greenhouse" and "Snap'n Build > Real Estate > Yard > Greenhouse" -
  everything goes into the upper menu and the deeper entry disappears. Sub menus with
  the same name inside are merged too. Nothing is fixed for a mod: the two must belong
  to the same mod (a common parent menu that is not the base game's), and their items
  must be similar (at least one model file or model folder in common). Entries that
  scripts added are never moved. New INI option `bMergeSimilarMenus` (on). The report
  has a new section "Merged menus" with every merge and the reason, and the entries
  with the same name that were not merged and why; `LostObjects.log` has the same lines.

## [0.11.1] - 2026-10-22 - public beta

### Fixed
- **Items were not sorted into the categories of their own mod when those categories
  were just put back.** The matcher only learned from items that were visible before
  Lost Objects acted. When a mod's menu had been changed by another mod (for example
  Snap'n Build with PAN_SnB.esp), the categories that came back were unknown to it, so
  the rest of that mod's lost items went into a "Lost Objects" folder instead (for
  example 28 Snap'n Build stairs next to the stairs category). Now the recipes that are
  put back count like visible ones. Nothing is fixed for a single mod.

### Added
- Report section "Not sorted into an existing category (and why)": every item that went
  into a "Lost Objects" folder or the tab, with the reason and the best guess. The same
  lines are in `LostObjects.log` ("not sorted in"), plus a line saying how many recipes
  and categories the matcher learned from.

## [0.11.0] - 2026-10-22 - public beta

### Changed
- **Lost items go back exactly where their mod had them.** Lost Objects now reads the
  menus (form lists) of every plugin file in the load order, so it knows where each
  category and sub menu was before another mod changed that menu, and puts it back there:
  same name, same position, no "Lost Objects" folder, no guessing. Only items whose mod
  never put them into a menu are sorted into categories or folders as before. The report
  has a new section "Put back where their mod had them" with the mod that removed each
  entry. New INI option `bRestoreOriginalPlaces` (on).

## [0.10.2] - 2026-10-22 - public beta

### Added
- The settings holotape can be crafted at the chemistry station, free of charge. The
  workbench and its category are found from the game data (the station that makes chems
  and explosives), no fixed form IDs.

## [0.10.1] - 2026-10-21 - public beta

### Fixed
- **Menus of other mods could disappear from a save.** Fallout 4 keeps the entries that
  scripts add to a menu list (for example everything Settlement Menu Manager adds) at
  the end of the list, and the save game stores exactly the last ones of them. Lost
  Objects 0.10.0 and older appended its tab and folders behind them, so a save made
  while Lost Objects was running stored the Lost Objects entry instead of the first
  script-added menu (in the test: "Snappy Builds" and Snap'n Build's "Real Estate").
  After loading such a save those menus were gone, even with Lost Objects switched off.
  Lost Objects now always puts its entries in front of the script-added ones, and when
  it finds its own entries among them (in a save from an older version), it takes them
  out and corrects the count, so the next save is right again. A menu that is already
  missing in such a save comes back when its mod adds it again - see TROUBLESHOOTING.
- The snapshot was written but never used: reading it back failed for every entry
  ("unknown name entry"). Fixed.

## [0.10.0] - 2026-10-20 - public beta

### Fixed
- Recovered items were missing for the whole session when the workshop was opened
  within the first seconds after loading. The game builds its workshop menu the first
  time the workshop is opened and keeps it until the game is closed, so a late change
  was never shown - not even after leaving and entering the workshop again.

### Added
- **Snapshot**: Lost Objects saves the menus it built
  (`Documents\My Games\Fallout4\F4SE\LostObjects_Snapshot.txt`) and puts them back as
  soon as the game data is loaded - before a save is loaded and long before the workshop
  can be opened. After loading, the menus are checked as before; if nothing changed,
  nothing happens. The snapshot is only used when the plugins, their order, the version
  and the settings are exactly the same. Only settled results are saved.
- **Restart hint**: if the menus change after the workshop was already opened in this
  game session (first start, mods changed, an option switched), a short message says
  "still preparing". As soon as the result is saved, a message box asks for a restart if
  the Lost Objects tab or folders are missing - so a restart right after reading it never
  loses the result. Saving is not needed for Lost Objects.
- `docs/FILES_AND_PRIVACY.md`: every file the mod installs or writes, what is inside,
  and how to remove every trace. The log no longer contains full paths (so no Windows
  user name).
- **Settings holotape** "Lost Objects Settings", put into the inventory when a save is
  loaded (it comes back if it is lost): "Rebuild the workshop menus now" and switches for
  sorting into existing categories, hiding ghost objects, hiding items that can already
  be built, the summary message and the development mode. The choices are saved in
  `LostObjects_Settings.ini` next to the log and win over the INI.
  It uses one tiny Papyrus script that passes the selected menu item to the DLL and only
  runs when an item is selected. The scripts are compiled with the open source compiler
  Caprica at build time (`tools/papyrus/`), so the project still builds without the
  Creation Kit.

### Changed
- Follow-up scans every 2 seconds during the first 20 seconds after loading (before: 3,
  10 and 20 seconds), so the menus are ready earlier on the first start.
- Entering workshop mode no longer forces an early build when the menus are already in
  place (snapshot or earlier build): an early build could see the menu while other mods
  are still adding to it.
- Nexus page kit and header: "no scripts" replaced by "settings holotape".

## [0.9.0] - 2026-10-20 - public beta

First public release. Tested on Fallout 4 1.11.240 with F4SE 0.7.9 and a large
settlement load order (see `docs/TESTED_WITH.md`).

### Changed
- New setting `bDevelopmentMode` (default `0`): the large analysis files (inventory,
  patterns, complete menu, duplicates) are only written for testers. The report and
  the log are always written. The old name `bWriteInventory` still works.
- Lost Objects tab: borrowed categories (for example the base game's "Scrap" used by a
  DLC) are grouped by what their items have in common, like in the mod folders.
- Documentation brought up to date: README, installation, troubleshooting, tested
  configuration, Nexus page kit (`nexus/`), planned features (language packs).
- Build: GitHub Actions updated to the Node 24 versions.

## [0.4.6] - 2026-10-20

### Changed
- The menus are kept up to date again, but without flipping back and forth: Lost
  Objects builds when the workshop menu has settled after loading (`iSettleSeconds`,
  default 10, and two scans in a row with the same result) and updates whenever a
  later scan finds something new. During the first minute a change must be seen twice
  in a row (other mods are still adding their menus). This replaces the "build once"
  behaviour of 0.4.5. More follow-up scans (up to 180 s) and a scan every time you
  enter workshop mode.

## [0.4.5] - 2026-10-20

### Fixed
- Empty thumbnails in the build bar: recovered items without a preview transform get
  the transform that visible items of the same form type use most often.

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
