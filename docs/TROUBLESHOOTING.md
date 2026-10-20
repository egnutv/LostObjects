# Troubleshooting

All files mentioned here are in `Documents\My Games\Fallout4\F4SE\`.

## I see no recovered items

1. **Is the mod loaded?** Open `LostObjects.log`.
   - No log file at all: F4SE did not load the DLL. Check that you start the game through
     F4SE, that F4SE and the Address Library match your game version, and look into
     `f4se.log` for a line about `LostObjects.dll`.
   - `'LostObjects.esp' is not active`: enable the plugin in your mod manager.
   - `does not match this DLL version`: the .esp and the .dll come from different
     versions. Reinstall the mod.
2. **Is anything lost?** Open `LostObjects_Report.txt`. If "Lost recipes recovered" is 0,
   there is nothing to show. The items you miss may be hidden by a **condition** of their
   mod (see below) or may not have a workshop recipe at all.
3. **Where did they go?** Most items are sorted into an existing category ("Sorted into
   existing categories" in the report lists them) or into a "Lost Objects" folder at the
   end of their mod's own menu. Only items without any home are in the Lost Objects tab.
4. **Was the menu built?** Right after loading, the log says `waiting until the menu has
   settled`. About ten seconds later it says `built`. Entering workshop mode builds at
   once.

## Items appear twice

Lost Objects never shows an item that can already be built through another recipe. If
you still see something twice, a mod probably added its menu very late. The next scan
(at the latest when you enter workshop mode again) notices it and updates the menus.
If it stays, please report it with `bDevelopmentMode=1` - `LostObjects_Duplicates.txt`
shows what appears twice and why.

## An item is in the wrong category

The sorting compares each lost item with what is already in the menu. If it chose the
wrong category, the inventory (`bDevelopmentMode=1`, `LostObjects_Inventory.tsv`, column
"Why") says which features matched. Raise `iMinConfidence` to sort in only clear
matches, or set `bSortIntoCategories=0` to put everything into the "Lost Objects"
folders. Please report it - it helps to improve the matching.

## An item is still missing

- **Conditions.** Some mods hide items until you enable them with a holotape, a perk or a
  setting. Lost Objects keeps those conditions, so the items stay hidden until the
  condition is met.
- **Not a workshop recipe.** The item may only exist for the chemistry station, cooking,
  armor or weapon workbenches, or it may have no recipe at all.
- **Ignored plugin.** Check `sIgnorePlugins` in the INI.
- **Filters.** Check `bSkipUntouchedBaseGame` and `bSkipWithoutCategory` in the INI
  (both are off by default; the report shows how many recipes a filter removed).

## There are strange items

Lost Objects shows everything that is craftable but not listed - including recipes that
mods or the base game keep hidden on purpose, for example scrap recipes for clutter,
buildings and trees. To slim it down:

- `bSkipWithoutCategory=1` hides recipes without a named category,
- `bSkipUntouchedBaseGame=1` hides vanilla and DLC recipes that no mod changed,
- `sIgnorePlugins=` hides everything of the listed plugins.

## The numbers next to the items are 999

That is not the cost. "Wood 999/30" means: you have 999 (or more), it costs 30. The number
behind the name is how many you could build; the game shows at most 999. Lost Objects
never changes costs.

## Folder names are "Menu 001" or "Group 001"

Renaming the folders failed. Please report it with the log. As a workaround use
`sSortMode=Flat`.

## The icon is missing or wrong

Set `bCustomMenuIcon=0` to use the vanilla "Decorations" icon instead of the logo, or
`bBorrowMenuIcon=0` for no icon at all; everything else works.

## "Lost Objects stopped after an error"

A scan failed with an internal error. Lost Objects switched itself off for this session
so the game keeps running. Please report it with `LostObjects.log` - the last lines show
where it stopped.

## Crash

1. Disable the mod and check whether the crash is gone.
2. If it is caused by Lost Objects, open an issue with:
   - `LostObjects.log` (with `bVerboseLog=1` and `bDevelopmentMode=1` if possible),
   - `LostObjects_Report.txt`,
   - the crash log (Buffout 4 or similar),
   - your game, F4SE and Address Library versions.

## Reporting a bug

Open an issue on [GitHub](https://github.com/egnutv/LostObjects/issues) and attach the
files listed above. Please remember that this is a hobby project without guaranteed support.
