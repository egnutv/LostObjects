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

## 4. Where recovered recipes go

Every recovered recipe gets exactly one extra category, so it is shown exactly once.
With `sPlacement=Home` (the default) the order is:

1. **Into an existing category that fits** (`bSortIntoCategories=1`). Lost Objects
   looks at what is already in the menu. For every feature of the lost item - its
   model file, model folder and parent folder, its name, the first word of the name,
   a `[Tag]` in the name, the other words of the name and the name of its lost
   category - it checks in which categories ALL visible items with the same feature
   are shown, across the whole menu. A category that holds most of them gets most of
   the feature's weight; a feature that is everywhere ("setdressing", "box") gives
   every category only a tiny part. The category with the highest total wins if
   - it shares at least the model, the model folder, the name, the first word, a tag
     or the category name with the lost item (words and parent folders alone never
     decide),
   - the confidence reaches `iMinConfidence` (default 30 %), and
   - it is clearly ahead of the runner-up (1.5 times its score).

   Candidates are the categories in which the item's own plugin already has visible
   items; if it has none, all categories are candidates, but then at least 60 % are
   required. Nothing in this is a fixed list: the mod only compares features of the
   lost item with the same features of the items that are already visible.
2. **Into a "Lost Objects" folder inside the plugin's own top menu.** The top menu is
   the plugin's own sub menu if it has exactly one (for example "Snappy Builds").
   A plugin without a menu of its own (an add-on such as `Snappy_DLC03.esp`) gets the
   shallowest menu that holds at least 80 % of its visible items, if that menu mostly
   shows items of this plugin or of the plugin that made the menu - so a shared menu
   like "Structures" never counts. Inside the folder the items are sorted by their
   original category. If that category comes from another plugin (for example the
   base game's "Scrap", which many mods borrow), its name says nothing, so the items
   are grouped by what they have in common themselves ("Wallpapers", "Roof",
   "Fungus", ...). Lost whole menus keep their name and categories. Like this
   everybody can see what the mod author made and what Lost Objects added.
3. **Into the "Lost Objects" tab** otherwise: one folder per plugin, or a single
   entry named "Plugin - Category" if the plugin has only one thing to show.

With `sPlacement=Tab` everything goes into the tab and nothing is added to other menus.

Lost recipes whose item can **already be built** through a reachable recipe (for
example the "scrap return" recipes of decoration mods) are skipped
(`bSkipAlreadyBuildable=1`), because they would only show the same item twice. They are
still listed in the inventory with the place where the item can be built.

**Ghost objects** are skipped too (`bSkipMissingModels=1`): recipes whose item - or,
for a list of variants, every single variant - points to a model file that does not
exist, so the build menu would only show an empty slot. Lost Objects asks the game's
own file system (loose files in `Data` and all loaded `.ba2` archives). Forms that
have no model by nature (an NPC from a spawner mod) are never hidden. The check only
runs after a self-test: the model files of items that are visible right now must be
found; otherwise it stays off and the report says so ("Model check").

The inventory and the report say for every item where it went and why (which features
matched and how strongly, or why it was not sorted in and what the best guess was).

### Large categories are split automatically

A category with more than `iSplitThreshold` recipes (default 40) becomes a sub menu
of groups. The groups are found in the data itself - no fixed word lists:

- a tag in brackets at the start of the item name (`[Alcohol] Beer` -> "Alcohol"),
- the words of the item names ("Stairs", "Concrete", ...),
- the folders of the model paths ("Flora", ...).

Features shared by more than half of the recipes are too broad and ignored. Then,
tags first, words second, folders last, the feature shared by the most remaining
recipes becomes a group (at least `iMinGroupSize`, at most `iMaxGroups` groups). The
rest goes to "Other", and large groups are split again (up to three levels). If no
useful feature exists, the recipes are split alphabetically ("A - F", ...).

## 5. What the mod changes

What you build in your settlements is saved by the game as always - Lost Objects does
not change that, and nothing you built disappears. The changes Lost Objects makes to
the menus are made **in memory only** and are not stored in the save game: they are
checked again every time a save is loaded, so they always match the mods that are
installed right now, and no stale entries can end up in a save. The only thing of Lost
Objects in a save is the settings holotape in your inventory - a normal item that
disappears when the mod is removed.

Lost Objects keeps two files of its own in `Documents\My Games\Fallout4\F4SE\`:
`LostObjects_Snapshot.txt` (the menus it built, see section 6) and
`LostObjects_Settings.ini` (the choices made on the holotape).

| What | Change | Undone when |
|---|---|---|
| Recovered recipes | one category is added (an existing one that fits, or one from the pool) | the recipe is reachable again, or on restart |
| Pool menus and categories of `LostObjects.esp` | named and filled | every rebuild |
| Top menus of other mods | a "Lost Objects" folder (pool menu) is appended | every rebuild (removed and added again), or on restart |
| `WorkshopMenuMain` | the "Lost Objects" tab is appended if needed | every rebuild |
| The menu icon | the tab's and the pool menus' title keywords are added to `LO_MenuIcon` (our logo) or, as fallback, to the vanilla "Decorations" icon | restart (`bBorrowMenuIcon=0` to disable, `bCustomMenuIcon=0` for the vanilla icon) |

The menus are only rebuilt when the result of a scan changes, and never while the
workshop menu is open. If another mod resets one of the menus (Papyrus `Revert()`),
the next scan adds the entries again.

## 6. When the menus are built

| Moment | What happens |
|---|---|
| Game data loaded | find the forms of `LostObjects.esp`, set up the icon and the holotape, put the menus of the snapshot back |
| Save loaded / new game | first scan; the holotape is added to the inventory if it is missing |
| every 2 seconds up to 20 seconds, then 25, 30, 45, 60, 90 and 180 seconds later | scans again: menus added by scripts (for example through Settlement Menu Manager) appear a few seconds after loading |
| Entering workshop mode | scans again |
| Holotape "Rebuild the workshop menus now" or an option switched | scans and builds at once |

**Why a snapshot?** The game builds its workshop menu the first time the workshop is
opened and keeps that picture until the game is closed - opening and closing the
workshop again does not read the menus again. So everything Lost Objects adds must be
in place before the workshop is opened for the first time. Lost Objects saves what it
built in `LostObjects_Snapshot.txt` and puts it back as soon as the game data is loaded,
long before a save is loaded. After loading, it checks the menus as described below; if
the result is the same - the normal case - nothing changes. The snapshot is only used
if the plugins (and their order), the Lost Objects version and the settings that decide
the menus are exactly the same as when it was written; forms are stored as "plugin
file + form ID". Only a settled result is saved, never the half-finished menu right
after loading.

If the menus change after the workshop was already opened in this game session (the
very first start, mods changed, an option switched on the holotape), the change is made
anyway, saved, and a message says that the game shows it after a restart.

**The menus always show the best current state.** Right after loading, other mods
are still adding their menus. So Lost Objects first waits until the menu has settled:
at least `iSettleSeconds` (default 10) after loading, and two scans in a row must find
exactly the same lost recipes. Then it builds its menus. Every later scan checks again;
if something new turns up (a mod injected its menu late, an item became reachable
elsewhere), the menus are updated. During the first minute a change must be seen twice
in a row, so the menus do not flip back and forth while mods are loading; after that,
changes are applied at once. A scan that finds nothing new changes nothing (it only
re-adds entries another mod removed).

If you open the workshop before the menu has settled and there is no snapshot yet,
Lost Objects builds right away and asks for a restart (see above).

The menu is never changed while it is on screen.

Items that were never meant for the build menu (scrap definitions of clutter,
containers, ...) often have no preview transform, so their thumbnail in the bar
stays empty. Recovered items without one get the transform that visible items of the
same form type use most often (in memory only).

## 7. Names

Menu and category names are written into the game's name table for keywords. Lost
Objects resolves the address of that table itself and verifies it at startup by
looking up its own keyword `LO_RootKeyword`, whose name must be "Lost Objects". If the
check fails, names are not read or written.

## 8. Files of the project

| File | Purpose |
|---|---|
| `src/Main.cpp` | F4SE entry point, game messages |
| `src/Recovery.*` | when to scan, events, icon, summary message, restart hint |
| `src/Snapshot.*` | saves the built menus and puts them back at the next game start |
| `src/Holotape.*` | the settings holotape: native Papyrus function, options, inventory |
| `src/Scanner.*` | the read-only analysis described above |
| `src/MenuBuilder.*` | decides where recovered recipes go and builds the menus |
| `src/Matcher.*` | finds the existing category a lost item belongs to |
| `src/Grouping.*` | splits large categories into groups by common features |
| `src/ModelCheck.*` | finds ghost objects (model file missing) |
| `src/Util.h` | small helpers (names, files, variants of a form list) |
| `src/Inventory.*`, `src/MenuDump.*` | the inventory, pattern, menu and duplicate logs |
| `src/NameMap.*` | verified access to the game's name table |
| `src/Report.*` | log output and `LostObjects_Report.txt` |
| `src/Notice.*` | the AI disclosure shown once per version |
| `src/Settings.*` | INI file and the holotape choices |
| `src/Forms.*` | looks up the forms of `LostObjects.esp` |
| `src/PluginForms.h` | form IDs, generated by `tools/build_plugin.py` |
| `tools/build_plugin.py` | generates `data/LostObjects.esp` and `src/PluginForms.h` |
| `tools/verify_plugin.py` | independent check of the plugin file |
| `tools/package.py` | builds the release archive |
| `tools/build_local.py`, `build.cmd` | one-click build on Windows |
| `tools/icon/` | generators for the menu icon mesh and texture, and the Nexus header |
| `tools/papyrus/` | builds the Papyrus compiler Caprica and compiles the holotape scripts; compile-time stubs of the base game scripts |
| `data/Scripts/Source/User/` | source code of the two holotape scripts |
| `data/` | esp, ini, icon mesh and texture as they go into the game's Data folder |
| `nexus/` | the Nexus Mods page kit |

## 9. Known limits

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
