# Testing guide

Lost Objects is written by an AI that cannot start the game. Every in-game check
is done by a human tester. This guide lists what to test and what to send back.

## 0. Get a test build

After every successful build of the `main` branch, GitHub puts the finished mod into the
repository itself:

```
releases/LostObjects-<version>.zip
```

Open the folder `releases` on the repository page (tab **Code**), click the zip and
then **Download** (the button with the arrow at the top right of the file view). In
GitHub Desktop, click **Fetch origin** and then **Pull origin** to get it into your local
folder. This zip is the mod archive - install it with your mod manager.

The same file is also available in the tab **Actions** -> newest run -> **Artifacts**
(GitHub wraps it in another zip there).

If the run has a red cross, open it, click the failed step and copy the error text.

## 1. Before you start

- Make a **backup save** (or use a test character).
- Note your versions: game (main menu, bottom left), F4SE (`f4se.log`), Address Library file name.
- Install the archive with your mod manager and enable `LostObjects.esp`.
- Delete old copies of `LostObjects.log`, `LostObjects_Report.txt` and
  `LostObjects_State.ini` in `Documents\My Games\Fallout4\F4SE\` so the test starts clean.

## 2. Test cases

Mark each case with OK / FAIL / NOT TESTED.

| # | Test | Expected result |
|---|---|---|
| T1 | Start the game, load a save | Message box about AI-made code appears **once**. |
| T2 | Look at `LostObjects.log` | Contains `Lost Objects is ready` and `Events: registered`. No `ERROR`. |
| T3 | Load the save again (or another one) | The AI message does **not** appear again. |
| T4 | Enter workshop mode in a settlement | If items are lost: tab **Lost Objects** at the end of the main menu, and a short message "... item(s) recovered". |
| T5 | Open the tab | Folders named after plugin files (mods, DLCs, `Fallout4`). Inside: menus/categories with their original names, plus "Uncategorized" where items have no category. |
| T6 | Compare with `LostObjects_Report.txt` | Every entry in the report can be found in the tab. |
| T7 | Build one item from the tab | Costs as expected, item can be placed, moved, stored and scrapped. |
| T8 | Save, quit to desktop, load again | Built item is still there. Tab is rebuilt. |
| T9 | Items you missed before | Are they in the tab now? Which ones are still missing? |
| T10 | Duplicates | Is something shown both in its normal place and in the tab? (Wait 2 minutes after loading, then re-enter workshop mode.) |
| T11 | `sSortMode=Flat` in the INI, restart | No folders, recovered menus directly in the tab. |
| T12 | `sIgnorePlugins=<one of the mods in the tab>`, restart | That mod's entries are gone. |
| T12b | `bSkipWithoutCategory=1`, restart | The "Uncategorized" entries are gone. Were they useful or junk (scrap recipes)? |
| T12c | `bSkipUntouchedBaseGame=1`, restart | Vanilla/DLC items that no mod changed are gone from the tab. |
| T13 | `bBorrowMenuIcon=0`, restart | Tab works, maybe without icon. Did the icon look OK with `1`? |
| T14 | Delete `LostObjects_State.ini`, load a save | The AI message appears again. |
| T15 | Remove the mod, load the save from T8 | Save loads, built items are still there. |
| T16 | General feel | Any stutter when entering workshop mode or opening big categories? Any crash? |

## 3. What to send back

- The table above with your results.
- `LostObjects.log`, `LostObjects_Report.txt`, `LostObjects_Inventory.tsv`, `LostObjects_Patterns.txt`, `LostObjects_Menu.tsv` and `LostObjects_Duplicates.txt` (for a full picture set
  `bVerboseLog=1` once).
- Screenshots of the tab (main menu with tab, one folder opened).
- For a crash: the crash log (Buffout 4 or similar) and `f4se.log`.
- Your versions (game, F4SE, Address Library) and the names of the settlement mods involved.

## 4. Known things to watch

- **Folder names.** If folders are called "Mod Slot 01" instead of mod names, renaming
  does not work on your game version - report it.
- **Icon.** The icon comes from the vanilla "Decorations" menu. If it is missing or
  misplaced, report it and use `bBorrowMenuIcon=0`.
- **First open after loading.** Menus that mods add by script appear a few seconds after
  loading. If the tab shows such a menu right after loading and it disappears later,
  that is expected.
