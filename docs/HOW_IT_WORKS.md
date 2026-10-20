# How Lost Objects works

This document explains the workshop menu, what "lost" means and exactly what the
mod changes in memory. It is written for players who want to understand the mod,
for mod authors and for developers who want to continue the project.

## 1. How the workshop menu is built

The workshop menu is not a hard coded list. It is a tree of form lists (`FLST`)
that starts at one list in `Fallout4.esm`:

```
WorkshopMenuMain  [FLST:00106DA2]
 +- WorkshopRecipeFilterQuest     (keyword = a category directly in the main menu)
 +- Structures   [FLST:00106DA3]  (form list = a sub menu)
 |   +- entry 0: keyword  -> only gives the sub menu its NAME and ICON
 |   +- entry 1: keyword  -> category "Wood"
 |   +- entry 2: FLST     -> another sub menu
 |   `- ...
 +- Furniture    [FLST:001B4AAE]
 `- ...
```

Rules the game follows:

1. A form list inside the tree is a **sub menu**.
2. **Entry 0** of every sub menu is a keyword that only provides the sub menu's
   name and icon. It is not a category. (The main menu itself has no entry 0 rule.)
3. Every other **keyword** in a form list is a **category**. The keyword must be of
   type *Recipe Filter*.
4. A **recipe** (`COBJ`) appears in every category whose keyword is in its
   *Category* field (`FNAM`).
5. Icons are art objects (`ARTO`) that list the keyword in their keyword field.

Mods add their items by adding keywords and form lists to this tree - either by
editing vanilla form lists in their plugin, or with a Papyrus script at runtime
(for example through Settlement Menu Manager).

## 2. What "lost" means

A recipe is **lost** when none of its categories can be reached by walking the
tree from `WorkshopMenuMain`. Common reasons:

- A mod's menu was supposed to be added by a script, but the script never ran
  (missing Settlement Menu Manager, script lag, an uninstalled holotape, ...).
- Two mods edit the same vanilla form list and the one that loads later removes
  the other's entries.
- A menu replacer reorganizes the vanilla menus and does not know about the
  categories of a mod that came later.
- A mod author put recipes into the "entry 0" keyword of a sub menu (rule 2), so
  the game never shows them.

## 3. The scan

The scan is read-only. It runs on the main thread and takes a few milliseconds.

1. **Reachable tree.** Walk from `WorkshopMenuMain` through all form lists
   (including entries added by scripts) and collect every reachable category
   keyword and sub menu. The mod's own tab is skipped, otherwise everything it
   shows would count as reachable.
2. **Recipes.** Every `COBJ` that is not deleted and creates something is checked.
   If one of its categories is reachable, it is reachable.
3. **Which recipes are workshop recipes?** Instead of hard coding a keyword, the
   mod looks at the workbench keywords (`BNAM`) of the reachable recipes. Every
   workbench keyword used by at least 2 % of them (minimum 5 recipes) is treated
   as "workshop". Chemistry, cooking, armor and weapon recipes therefore never
   show up.
4. **Lost recipes.** Every unreachable workshop recipe is lost - from mods, DLCs
   or the base game, whatever it builds. Only the optional filters remove some:
   - recipes from plugins in `sIgnorePlugins`,
   - vanilla and DLC recipes that no mod changed (`bSkipUntouchedBaseGame=1`),
   - recipes without a named category (`bSkipWithoutCategory=1`).
5. **Lost whole menus.** Form lists that look like a sub menu (entry 0 is a recipe
   filter keyword), are not reachable and are not inside another such list are
   candidates. If at least half of the categories below one of them are lost and
   it has a name, the **whole menu** is restored with its original structure.
6. **Lost single categories.** All other lost categories that have a name are
   restored one by one.
7. **Uncategorized.** Lost recipes whose categories have no name or are not
   recipe filter keywords are shown in an "Uncategorized" category inside the
   folder of the file that defined the recipe (in `Flat` mode: one
   "Uncategorized" entry in the tab).

## 4. What the mod changes

All changes are made **in memory only**. None of them is stored in the save game,
because form list contents that come from plugins are never saved and the mod does
not use Papyrus.

| What | Change | Undone when |
|---|---|---|
| `WorkshopMenuMain` | `LO_RootMenu` is appended | nothing is lost any more, or the game is restarted without the mod |
| `LO_RootMenu` (own) | filled with the folders or recovered menus | every scan |
| `LO_SlotMenuNN` (own) | filled with one mod's recovered menus | every scan |
| `LO_SlotKeywordNN` (own) | renamed to the mod's file name | every scan |
| Lost recipes **without a usable category** | one of the keywords `LO_SlotUncategorizedKeywordNN` (or `LO_UncategorizedKeyword` in `Flat` mode) is added to the recipe | as soon as the recipe is reachable again |
| One vanilla icon art object | the mod's keywords are added to its keyword list so the tab gets an icon | game restart (`bBorrowMenuIcon=0` to disable) |

Lost menus and categories of other mods are **not modified**. They are only
referenced from the new tab, so they appear there with their original names,
icons, costs and conditions.

## 5. When the scan runs

| Moment | Why |
|---|---|
| Game data loaded | find the forms of `LostObjects.esp` |
| Save loaded / new game | first scan |
| 3, 10, 30 and 90 seconds later | menus added by scripts appear a few seconds after loading |
| Entering workshop mode | catch everything else |
| Workshop menu closed | if a scan was skipped because the menu was open |

The menu is never changed while it is on screen.

## 6. Folders per plugin file ("ByMod")

`LostObjects.esp` contains 48 folder form lists, each with its own name keyword and
its own "Uncategorized" keyword. At runtime the name keywords are renamed to the file
name of the plugin that defined the content (a mod, a DLC or `Fallout4`) through the
game's name table for keywords. With more than 48 files, the last folder becomes
"Other". With `sSortMode=Flat` the folders are not used.

## 7. Files of the project

| File | Purpose |
|---|---|
| `src/Main.cpp` | F4SE entry point, game messages |
| `src/Recovery.*` | when to scan, events, icon, summary message |
| `src/Scanner.*` | the read-only analysis described above |
| `src/MenuBuilder.*` | writes the result into the menu forms |
| `src/Report.*` | log output and `LostObjects_Report.txt` |
| `src/Notice.*` | the AI disclosure shown once per version |
| `src/Settings.*` | INI file |
| `src/Forms.*` | looks up the forms of `LostObjects.esp` |
| `src/PluginForms.h` | form IDs, generated by `tools/build_plugin.py` |
| `tools/build_plugin.py` | generates `data/LostObjects.esp` and `src/PluginForms.h` |
| `tools/verify_plugin.py` | independent check of the plugin file |
| `tools/package.py` | builds the release archive |

## 8. Known limits

- Recipes hidden by **conditions** (for example "only show if a holotape option is
  on") stay hidden, because the mod keeps conditions untouched. A future version may
  offer an option to show them anyway.
- The icon borrowing relies on the game resolving menu icons lazily. If the tab
  shows no icon, nothing else is affected.
- Items that were never meant to be buildable (no recipe at all) and recipes hidden
  by conditions are not part of this mod. That idea ("Anarchy Objects") may become a
  separate mod.
- Showing everything can create very large categories (for example hundreds of
  scrap-only recipes). Large categories make the workshop menu slower; use the
  filters if that bothers you.
