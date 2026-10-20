# Installation

## Requirements

1. **Fallout 4 1.11.240** (the current Steam version) - tested. The DLL also supports
   1.10.984 (Next-Gen) and 1.10.163 (Old-Gen), but these were not tested.
2. **F4SE 0.7.9** for 1.11.240, or the F4SE that matches your game version
   ([Nexus](https://www.nexusmods.com/fallout4/mods/42147)). Start the game through F4SE.
3. **Address Library for F4SE Plugins** for your game version
   ([Nexus](https://www.nexusmods.com/fallout4/mods/47327)).

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
Data\F4SE\Plugins\LostObjects.dll
Data\F4SE\Plugins\LostObjects.ini
Data\F4SE\Plugins\LostObjects\   (readme, license, notices)
```

Then enable `LostObjects.esp` in `plugins.txt` or in the game's mod menu.

## Check that it works

1. Load a save. Once per version a message appears saying the mod was made with AI.
2. Open `Documents\My Games\Fallout4\F4SE\LostObjects.log`. It should contain
   `Lost Objects is ready`.
3. Wait about ten seconds (other mods add their menus after loading), then enter
   workshop mode. A short message tells you how many items were recovered and how many
   of them were sorted into existing categories. Look for "Lost Objects" folders at the
   end of your mods' menus and, if anything has no home, the **Lost Objects** tab in the
   main menu. If nothing is lost, there is nothing to see - that is good news.

## Uninstall

Disable or remove the mod. Nothing is stored in your save game. Things you built stay
where they are; you just cannot build new lost items anymore.

## Updating

Replace the files. After an update the AI message is shown once more.
