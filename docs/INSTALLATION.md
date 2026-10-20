# Installation

## Requirements

1. **Fallout 4** - Anniversary Edition 1.11.191 or newer (current target 1.11.240),
   Next-Gen 1.10.984 or Old-Gen 1.10.163.
2. **F4SE** that matches your game version
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
Data\F4SE\Plugins\LostObjects.dll
Data\F4SE\Plugins\LostObjects.ini
Data\F4SE\Plugins\LostObjects\   (readme, license, notices)
```

Then enable `LostObjects.esp` in `plugins.txt` or in the game's mod menu.

## Check that it works

1. Load a save. Once per version a message appears saying the mod was made with AI.
2. Open `Documents\My Games\Fallout4\F4SE\LostObjects.log`. It should contain
   `Lost Objects is ready`.
3. Enter workshop mode. If anything was lost, the main menu has a new tab
   **Lost Objects** and a short message tells you how many items were recovered.
   If nothing is lost, there is no tab - that is good news.

## Uninstall

Disable or remove the mod. Nothing is stored in your save game. Things you built from
the tab stay where they are; you just cannot build new ones from it anymore.

## Updating

Replace the files. After an update the AI message is shown once more.
