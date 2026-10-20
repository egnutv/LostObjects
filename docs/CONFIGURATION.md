# Configuration

Settings are in `Data/F4SE/Plugins/LostObjects.ini` (in Mod Organizer 2: inside the
mod's folder under `F4SE/Plugins`). Changes take effect the next time the game starts.
If the file is missing or a value is invalid, the default is used and a warning is
written to the log.

`1` = on, `0` = off (`true`/`false`, `yes`/`no`, `on`/`off` also work).

## [General]

| Setting | Default | Meaning |
|---|---|---|
| `bEnabled` | `1` | Master switch. `0` = the mod does nothing at all. |
| `sPlacement` | `Home` | `Home`: sort recovered items into an existing category that fits; what does not fit goes into a "Lost Objects" folder inside the mod's own menu; only what has no home goes into the tab. `Tab`: everything into the "Lost Objects" tab, nothing is added to other menus. |
| `bSortIntoCategories` | `1` | `0`: do not sort into existing categories, use the "Lost Objects" folder of the mod (or the tab) for everything. Only with `sPlacement=Home`. |
| `iMinConfidence` | `30` | How sure (in percent) the match into an existing category must be. Higher = fewer items sorted in, but fewer mistakes. The inventory shows the confidence of every item. |
| `sSortMode` | `ByMod` | `ByMod`: one folder per plugin file (mod, DLC or base game) inside the tab. `Flat`: all recovered menus directly in the tab. |
| `bShowSummaryMessage` | `1` | Short message the first time you enter workshop mode after loading, e.g. "Lost Objects: 37 workshop item(s) recovered." |
| `bBorrowMenuIcon` | `1` | Gives the tab and its folders the icon of the vanilla "Decorations" menu. |
| `bWriteReport` | `1` | Writes `Documents\My Games\Fallout4\F4SE\LostObjects_Report.txt` whenever the result changes. |
| `bWriteInventory` | `1` | Writes `LostObjects_Inventory.tsv` (one line per recovered item: name, model path, size, categories, components, keywords; opens in Excel) and `LostObjects_Patterns.txt` (common words, model folders, types) to the same folder. Also writes the complete workshop menu to `LostObjects_Menu.tsv` and everything that appears more than once (recipes, items, categories, sub menus, menu names) to `LostObjects_Duplicates.txt`. Used to improve the automatic sorting. |
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
