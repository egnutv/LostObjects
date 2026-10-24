# Installation

## Requirements

1. **Fallout 4 1.11.240** (the current Steam version) - the only version Lost Objects is
   tested on and supports.
2. **F4SE 0.7.9** for 1.11.240
   ([Nexus](https://www.nexusmods.com/fallout4/mods/42147)). Start the game through F4SE.
3. **Address Library for F4SE Plugins** for 1.11.240
   ([Nexus](https://www.nexusmods.com/fallout4/mods/47327)).

## Download

[Lost Objects on Nexus Mods](https://www.nexusmods.com/fallout4/mods/110356). The same archive is also in `releases/` of the
[GitHub repository](https://github.com/egnutv/LostObjects).

## With a mod manager (recommended)

Vortex, the Nexus app or Mod Organizer 2:

1. Install the `LostObjects-<version>.zip` archive like any other mod.
2. Make sure `LostObjects.esp` is enabled. It is a light plugin (ESL flagged), so it
   does not count against the plugin limit. Load order does not matter.
3. Start the game through F4SE.

## By hand

Copy the contents of the archive into `Fallout 4\Data\`:

```
Data\LostObjects.esp
Data\Meshes\LostObjects\MenuIcon.nif
Data\Textures\LostObjects\MenuIcon_d.dds
Data\Scripts\LostObjects.pex
Data\Scripts\LostObjects_Holotape.pex
Data\Scripts\Source\User\   (source code of the two scripts)
Data\F4SE\Plugins\LostObjects.dll
Data\F4SE\Plugins\LostObjects.ini
Data\F4SE\Plugins\LostObjects\   (readme, license, notices)
```

Then enable `LostObjects.esp` in `plugins.txt` or in the game's mod menu.

## Check that it works

1. Load a save. Once per version a message appears saying the mod was made with AI,
   and the settings holotape "Lost Objects Settings" is put into your inventory.
2. Open `Documents\My Games\Fallout4\F4SE\LostObjects.log`. It should contain
   `Lost Objects is ready`.
3. **The very first time** with Lost Objects (or after you changed your mods), wait
   about 20 seconds after loading before you enter workshop mode - other mods add their
   menus after loading and Lost Objects waits for them. If you are faster, a message
   asks you to restart the game once; nothing is lost. From then on the menus are
   complete right away.
4. In workshop mode a short message tells you how many items were recovered and how many
   of them were sorted into existing categories. Look for "Lost Objects" folders at the
   end of your mods' menus and, if anything has no home, the **Lost Objects** tab in the
   main menu. If nothing is lost, there is nothing to see - that is good news.

## Uninstall

1. Disable or remove the mod in your mod manager. Things you built stay where they are;
   you just cannot build new lost items anymore. The settings holotape disappears from
   your inventory.
2. **Files that stay behind:** the mod manager does not remove the files Lost Objects
   wrote while you played. They are in `Documents\My Games\Fallout4\F4SE\` - delete
   every file whose name starts with `LostObjects`. They are harmless if you keep them.

Every single file is listed in [FILES_AND_PRIVACY.md](FILES_AND_PRIVACY.md).

## Updating

Replace the files. After an update the AI message is shown once more.
