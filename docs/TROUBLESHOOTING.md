# Troubleshooting

All files mentioned here are in `Documents\My Games\Fallout4\F4SE\`.

## The tab does not appear

1. **Is the mod loaded?** Open `LostObjects.log`.
   - No log file at all: F4SE did not load the DLL. Check that you start the game through
     F4SE, that F4SE and the Address Library match your game version, and look into
     `f4se.log` for a line about `LostObjects.dll`.
   - `'LostObjects.esp' is not active`: enable the plugin in your mod manager.
   - `does not match this DLL version`: the .esp and the .dll come from different
     versions. Reinstall the mod.
2. **Is anything lost?** Open `LostObjects_Report.txt`. If "Lost recipes recovered" is 0,
   there is nothing to show, so the tab stays hidden. The items you miss may be hidden by
   a **condition** of their mod (see below) or may not have a workshop recipe at all.
3. **Did the scan run?** The log has a `Scan (...)` line for every change. Enter workshop
   mode once, leave it and enter it again.

## Items appear twice

A mod injects its menu with a script some time after loading. Lost Objects saw the
menu as lost before the script ran. The next scan (at the latest when you enter workshop
mode again) notices that the menu is reachable now and removes it from the tab.

## An item is still missing

- **Conditions.** Some mods hide items until you enable them with a holotape, a perk or a
  setting. Lost Objects keeps those conditions, so the items stay hidden until the
  condition is met.
- **Not a workshop recipe.** The item may only exist for the chemistry station, cooking,
  armor or weapon workbenches, or it may have no recipe at all.
- **Ignored plugin.** Check `sIgnorePlugins` in the INI.
- **Filters.** Check `bSkipUntouchedBaseGame` and `bSkipWithoutCategory` in the INI
  (both are off by default; the report shows how many recipes a filter removed).

## The tab shows strange entries or is very large

The tab shows everything that is craftable but not listed - including recipes that
mods or the base game keep hidden on purpose, for example scrap-only recipes for
buildings and trees. To slim it down:

- `bSkipWithoutCategory=1` hides recipes without a named category,
- `bSkipUntouchedBaseGame=1` hides vanilla and DLC recipes that no mod changed,
- `sIgnorePlugins=` hides everything of the listed plugins.

Very large categories can make the workshop menu slow to open.

## Folder names are "Mod Slot 01"

Renaming the folders failed. Please report it with the log. As a workaround use
`sSortMode=Flat`.

## The tab icon is missing or wrong

Set `bBorrowMenuIcon=0`. The tab then shows without an icon; everything else works.

## Crash

1. Disable the mod and check whether the crash is gone.
2. If it is caused by Lost Objects, open an issue with:
   - `LostObjects.log` (with `bVerboseLog=1` if possible),
   - `LostObjects_Report.txt`,
   - the crash log (Buffout 4 or similar),
   - your game, F4SE and Address Library versions.

## Reporting a bug

Open an issue on [GitHub](https://github.com/egnutv/LostObjects/issues) and attach the
files listed above. Please remember that this is a hobby project without guaranteed support.
