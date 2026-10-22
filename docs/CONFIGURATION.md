# Configuration

## The settings holotape

"Lost Objects Settings" is put into your inventory when you load a save (if you lose or
sell it, it comes back the next time you load). You can also craft it at any chemistry
station, free of charge. Play it in the Pip-Boy:

| Menu item | What it does |
|---|---|
| Rebuild the workshop menus now | Scans and builds at once, without waiting. A message tells you whether anything changed. |
| Sort lost items into existing categories | = `bSortIntoCategories` |
| Hide ghost objects (no model file) | = `bSkipMissingModels` |
| Hide items that can already be built | = `bSkipAlreadyBuildable` |
| Show the summary message | = `bShowSummaryMessage` |
| Development mode (analysis files) | = `bDevelopmentMode` |

`[X]` = on, `[ ]` = off. Select an item to switch it, then go back to see the new state.
Lost Objects saves your choices in `Documents\My Games\Fallout4\F4SE\LostObjects_Settings.ini`;
they win over the INI below. Delete that file to go back to the INI values.

If you already opened the workshop in this game session, the game shows the new menu
only after a restart of the game (it builds its workshop menu once per session). Lost
Objects tells you when that is the case. You do not need to save for Lost Objects.

## The INI

All settings are in `Data/F4SE/Plugins/LostObjects.ini` (in Mod Organizer 2: inside the
mod's folder under `F4SE/Plugins`). Changes take effect the next time the game starts.
If the file is missing or a value is invalid, the default is used and a warning is
written to the log.

`1` = on, `0` = off (`true`/`false`, `yes`/`no`, `on`/`off` also work).

## [General]

| Setting | Default | Meaning |
|---|---|---|
| `bEnabled` | `1` | Master switch. `0` = the mod does nothing at all. |
| `bLiveUpdate` | `0` | Lost Objects checks the menu again and again and updates whenever something new turns up. During the first minute after loading a change must be seen twice in a row (other mods are still adding their menus). `1`: do not wait for the second scan (only for testing). |
| `iSettleSeconds` | `10` | Earliest moment (seconds after loading) to build the menus. Two scans in a row must also agree. |
| `sPlacement` | `Home` | `Home`: sort recovered items into an existing category that fits; what does not fit goes into a "Lost Objects" folder inside the mod's own menu; only what has no home goes into the tab. `Tab`: everything into the "Lost Objects" tab, nothing is added to other menus. |
| `bMergeSimilarMenus` | `1` | Merge menus and categories of a mod that have the same name and similar items (at least one model file or model folder in common), when the upper one lies directly in a menu on the path of the deeper one (for example "Snap'n Build > Greenhouse" and "Snap'n Build > Real Estate > Yard > Greenhouse"; never between neighbouring branches like "Concrete > Supports" and "Wood > Supports"). Everything goes into the upper menu, the deeper entry disappears, sub menus with the same name inside are merged too. Menus of different mods and entries that scripts added are never touched. `0`: leave the menus of other mods as they are. |
| `bRestoreOriginalPlaces` | `1` | Put lost categories and menus back exactly where their mod had them, read from the plugin files of your load order. `0`: do not use the plugin files. Only with `sPlacement=Home`. |
| `bSortIntoCategories` | `1` | `0`: do not sort into existing categories, use the "Lost Objects" folder of the mod (or the tab) for everything. Only with `sPlacement=Home`. |
| `iMinConfidence` | `30` | How sure (in percent) the match into an existing category must be. Higher = fewer items sorted in, but fewer mistakes. The inventory shows the confidence of every item. |
| `sSortMode` | `ByMod` | `ByMod`: one folder per plugin file (mod, DLC or base game) inside the tab. `Flat`: all recovered menus directly in the tab. |
| `bShowSummaryMessage` | `1` | Short message the first time you enter workshop mode after loading, e.g. "Lost Objects: 37 workshop item(s) recovered." |
| `bBorrowMenuIcon` | `1` | Gives the tab and the "Lost Objects" folders an icon in the workshop menu. |
| `bCustomMenuIcon` | `1` | `1`: the Lost Objects icon (the logo, `Meshes\LostObjects\MenuIcon.nif`). `0`: the icon of the vanilla "Decorations" menu. If the mesh is missing, the vanilla icon is used automatically. |
| `bWriteReport` | `1` | Writes `Documents\My Games\Fallout4\F4SE\LostObjects_Report.txt` whenever the result changes. |
| `bDevelopmentMode` | `0` | For testers: also writes `LostObjects_Inventory.tsv` (every recovered item: name, model path, size, categories, components, status and why it was put where it is; opens in Excel), `LostObjects_Patterns.txt` (common words, model folders, types), `LostObjects_Menu.tsv` (the complete workshop menu, several MB) and `LostObjects_Duplicates.txt` (everything that appears more than once). The old name `bWriteInventory` still works. |
| `bVerboseLog` | `0` | Writes every single recovered recipe into `LostObjects.log` and enables debug messages. Useful for bug reports. |

## [Grouping]

| Setting | Default | Meaning |
|---|---|---|
| `iSplitThreshold` | `40` | Categories with more recipes are split into groups (by tags, words in names and model folders). |
| `iMaxGroups` | `40` | Maximum number of groups per split (including "Other"). |
| `iMinGroupSize` | `3` | A group needs at least this many recipes. |

## [Filters]

By default Lost Objects shows **everything** that has a workshop recipe and cannot be
reached in the build menu. The only filters that are on by default are
`bSkipAlreadyBuildable` and `bSkipMissingModels`, because they remove nothing you could
build or see anyway. Both are listed in the inventory with the reason.

| Setting | Default | Meaning |
|---|---|---|
| `bSkipUntouchedBaseGame` | `0` | `1` hides vanilla and DLC recipes that no mod changed. The base game hides a few recipes on purpose (for example a duplicate mattress). |
| `bSkipAlreadyBuildable` | `1` | Skips lost recipes whose item can already be built through a reachable recipe (for example "scrap return" recipes of decoration mods). They would only show the same item twice. `0` shows them anyway. |
| `bSkipMissingModels` | `1` | Hides "ghost objects": recipes whose item (or every variant of it) points to a model file that does not exist, so the build menu only shows an empty slot. Checked through the game's own file system (loose files and archives) after a self-test. `0` shows them anyway. |
| `bSkipWithoutCategory` | `0` | `1` hides recipes that have no named category. Many mods create such recipes only to define scrap results, often hundreds of buildings, trees and clutter. |
| `sIgnorePlugins` | *(empty)* | Comma separated plugin file names. Recipes defined or last changed in these files never appear in the tab. Example: `sIgnorePlugins=SomeMod.esp, AnotherMod.esl` |

## Examples

Hide the items of two mods:

```ini
[Filters]
sIgnorePlugins=SomeMod.esp, AnotherMod.esl
```

Fewer folders:

```ini
[General]
sSortMode=Flat
```

Collecting information for a bug report:

```ini
[General]
bWriteReport=1
bVerboseLog=1
```
