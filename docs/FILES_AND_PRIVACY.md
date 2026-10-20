# Files and privacy

## In short

- Lost Objects does **not** go online. It sends nothing anywhere.
- It collects **no personal data**. Its files contain only names of mods, plugins,
  workshop items and menus, and the settings of the mod.
- Every file it writes can be deleted at any time. Nothing breaks - Lost Objects simply
  builds its menus again the next time.
- Log files only name files, never full paths, so your Windows user name is not in them.
  Before you post a log anywhere, you can still open it and read it - it is plain text.

## 1. Files the mod manager installs

They are removed when you uninstall the mod with Vortex, the Nexus app or Mod Organizer 2.
If you installed by hand, delete them from `Fallout 4\Data\`:

| File | What it is |
|---|---|
| `LostObjects.esp` | the plugin (menu skeleton, menu icon, settings holotape) |
| `F4SE\Plugins\LostObjects.dll` | the program |
| `F4SE\Plugins\LostObjects.ini` | the settings |
| `F4SE\Plugins\LostObjects\` | readme, license, notices, changelog |
| `Meshes\LostObjects\MenuIcon.nif` | the menu icon |
| `Textures\LostObjects\MenuIcon_d.dds` | the menu icon picture |
| `Scripts\LostObjects.pex`, `Scripts\LostObjects_Holotape.pex` | the two holotape scripts |
| `Scripts\Source\User\LostObjects.psc`, `Scripts\Source\User\LostObjects_Holotape.psc` | their source code |

## 2. Files Lost Objects writes itself

All of them are in **`Documents\My Games\Fallout4\F4SE\`**. A mod manager does **not**
remove them, because the game writes them while you play. Delete them by hand if you
want to remove every trace:

| File | When | What is inside |
|---|---|---|
| `LostObjects.log` | always | what the mod did (for bug reports) |
| `LostObjects_Report.txt` | always (`bWriteReport=1`) | every recovered item and where it went |
| `LostObjects_State.ini` | always | the version whose AI notice you have seen |
| `LostObjects_Snapshot.txt` | always | the menus Lost Objects built, so they are complete at the next start |
| `LostObjects_Snapshot.txt.tmp` | for a moment while saving | the same, replaced at once |
| `LostObjects_Settings.ini` | only after you used the settings holotape | your choices on the holotape |
| `LostObjects_Inventory.tsv` | only in development mode | every recovered item in detail |
| `LostObjects_Patterns.txt` | only in development mode | common words and folders of the items |
| `LostObjects_Menu.tsv` | only in development mode | the complete workshop menu (several MB) |
| `LostObjects_Duplicates.txt` | only in development mode | everything that appears twice |

Quick way to remove them all: open the folder and delete every file whose name starts
with `LostObjects`.

## 3. In your save game

Lost Objects does not write its menus into the save. Only two small things of it end up
in a save, the same way as with any mod that adds an item:

- the settings holotape "Lost Objects Settings" in your inventory,
- the five values of the holotape options (on or off).

When you remove the mod, the game drops both when it loads the save. Things you built
stay where they are.

## 4. Building it yourself

If you compile the scripts yourself (see [BUILDING.md](BUILDING.md)), the Papyrus
compiler writes the name of your Windows user and of your computer into the `.pex`
files, as every Papyrus compiler does. The release on GitHub and Nexus is built by
GitHub Actions; its scripts contain only the build machine's names (`runneradmin`).
