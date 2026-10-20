<p align="center">
  <img src="Logo/Logo.png" alt="Lost Objects logo" width="256">
</p>

# Lost Objects

**A Fallout 4 mod that finds every craftable workshop item you cannot reach in the build menu - from mods, DLCs or the base game - and puts it into a new tab called "Lost Objects".**

> **Vibecoded by AI.** The code of this mod was written by Claude (Anthropic), an AI model, directed and tested by **egnutv**.
> AI-generated code can contain mistakes. Keep a backup save.

[![Build](https://github.com/egnutv/LostObjects/actions/workflows/build.yml/badge.svg)](https://github.com/egnutv/LostObjects/actions/workflows/build.yml)
![Game](https://img.shields.io/badge/Fallout%204-1.10.163%20%7C%201.10.984%20%7C%201.11.191%2B-blue)
![License](https://img.shields.io/badge/license-PolyForm%20Noncommercial%201.0.0-lightgrey)

---

## The problem

You install a settlement mod such as *Settlement Objects Expansion Pack* or a menu
reorganizer such as *Workshop Rearranged*, and some pieces never show up in the workshop
menu - sometimes new ones from the mod, sometimes vanilla or DLC pieces that another mod
pushed out of the menu. They are loaded, they have a recipe - there is just no way to
click on them. Usually the cause is a menu that was never connected (for example a broken
Settlement Menu Manager setup), a menu replacer that dropped a category, or two mods
overwriting the same vanilla menu list. Finding and fixing that by hand needs xEdit and a
lot of knowledge.

## What this mod does

Every time you enter workshop mode (and a few times after loading a save), Lost Objects
walks through the whole workshop menu tree, exactly the way the game does, and finds
**every workshop recipe that cannot be reached from the main menu** - no matter whether it
comes from a mod, a DLC or the base game, and no matter what it builds: walls, furniture,
buildings, trees, clutter. If it is craftable in the workshop and not listed, it goes into
a new tab **Lost Objects** in the main workshop menu:

```
Workshop main menu
 +- Structures
 +- Furniture
 +- ...
 `- Lost Objects                      <- new, only visible if something was lost
     +- Fallout4                      <- one folder per plugin file (can be turned off)
     |   `- Uncategorized             <- vanilla items without a category
     +- SettlementObjectsExpansion
     |   +- SOE High Tech             <- the mod's own lost menus and categories,
     |   +- SOE Walls                    with their original names
     |   `- Uncategorized
     `- WorkshopRearranged
         `- ...
```

- Whole lost menus are restored with their original structure, names and icons.
- Single lost categories are added with their original names.
- Lost items without any category go into "Uncategorized" inside their folder.
- Items keep their original costs and requirements.
- Nothing is written into your save game. Remove the mod and everything is as before;
  things you already built stay where they are.
- Only workshop (settlement) recipes are considered - chemistry, cooking, armor and weapon
  workbenches are not touched.
- Items hidden by a condition of their mod stay hidden, and objects without any recipe are
  not part of this mod.
- Optional filters in the INI can hide untouched vanilla recipes, recipes without a
  category or whole plugins. All filters are off by default.

## Requirements

| | |
|---|---|
| Fallout 4 | Anniversary Edition 1.11.191 or newer (tested target: **1.11.240**), Next-Gen 1.10.984 or Old-Gen 1.10.163 |
| [F4SE](https://www.nexusmods.com/fallout4/mods/42147) | the version that matches your game (0.7.9 for 1.11.240) |
| [Address Library for F4SE Plugins](https://www.nexusmods.com/fallout4/mods/47327) | the file for your game version |

One DLL supports all three game versions.

## Installation

Install the archive with Vortex, the Nexus app or Mod Organizer 2 and enable
`LostObjects.esp` (it is a light plugin and does not use a regular plugin slot).
Details: [docs/INSTALLATION.md](docs/INSTALLATION.md).

On the first load you will see a message that the mod was made with AI. It is shown once
per version.

## Where recovered items go

By default Lost Objects sorts recovered items **into the existing category they belong
to**, right next to their siblings. It finds that category by comparing the item with
what is already in the menu (model folder, name, words, tags) - no fixed lists. What it
cannot place with confidence goes into a **"Lost Objects" folder inside the mod's own
menu**, so you can see what the mod author made and what Lost Objects added. Only what
has no home at all ends up in the "Lost Objects" tab. Large categories are split into
groups automatically. Items that can already be built through another recipe are not
shown twice, and "ghost objects" whose model file does not exist are hidden. `sPlacement=Tab` puts everything into the tab instead.

## Settings

`Data/F4SE/Plugins/LostObjects.ini` - for example `sSortMode=Flat` to drop the
folders, `bSkipWithoutCategory=1` to hide scrap-only recipes, or `sIgnorePlugins=` to hide
a plugin's items.
All options: [docs/CONFIGURATION.md](docs/CONFIGURATION.md).

## Reporting problems

Lost Objects writes two files to `Documents\My Games\Fallout4\F4SE\`:

- `LostObjects.log` - what the mod did
- `LostObjects_Report.txt` - a readable list of every recovered menu, category and item

Please attach both when you open an [issue](https://github.com/egnutv/LostObjects/issues).
See [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md) first.

## Documentation

| Document | For |
|---|---|
| [INSTALLATION.md](docs/INSTALLATION.md) | players |
| [CONFIGURATION.md](docs/CONFIGURATION.md) | players |
| [TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md) | players |
| [HOW_IT_WORKS.md](docs/HOW_IT_WORKS.md) | curious players, mod authors, developers |
| [TESTING.md](docs/TESTING.md) | testers |
| [BUILDING.md](docs/BUILDING.md) | developers |
| [RELEASING.md](docs/RELEASING.md) | maintainers |

## Credits

- **egnutv** - idea, project owner, tester
- **Claude (Anthropic)** - code and documentation (vibecoded by AI)
- [CommonLibF4 AV](https://github.com/LucaDotGit/CommonLibF4) by LucaDotGit, based on work by
  Ryan McKenzie and alandtse
- The [F4SE](https://f4se.silverlock.org/) team and the authors of the Address Library for F4SE Plugins
- Third-party licenses: [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)

## License

[PolyForm Noncommercial 1.0.0](LICENSE) with additional permissions in [NOTICE.md](NOTICE.md):

- the code may be used, changed and shared for **noncommercial** purposes only,
- **videos, streams, screenshots and reviews** of the mod are allowed, also monetized
  (YouTube, Twitch, ...), and everything you build with it is yours,
- anyone may fork and continue the project without asking,
- no warranty and no support obligation.
