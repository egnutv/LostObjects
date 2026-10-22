<p align="center">
  <img src="Logo/Logo.png" alt="Lost Objects logo" width="256">
</p>

# Lost Objects

**A Fallout 4 mod that finds every craftable workshop item you cannot reach in the build menu - from mods, DLCs or the base game - and puts it back into the menu, right where it belongs.**

> **Vibecoded by AI.** The code of this mod was written by Claude (Anthropic), an AI model, directed and tested by **egnutv**.
> AI-generated code can contain mistakes. Keep a backup save.

[![Build](https://github.com/egnutv/LostObjects/actions/workflows/build.yml/badge.svg)](https://github.com/egnutv/LostObjects/actions/workflows/build.yml)
![Game](https://img.shields.io/badge/Fallout%204-1.11.240-blue)
![F4SE](https://img.shields.io/badge/F4SE-0.7.9-blue)
![Status](https://img.shields.io/badge/status-public%20beta-orange)
![License](https://img.shields.io/badge/license-PolyForm%20Noncommercial%201.0.0-lightgrey)
[![Nexus Mods](https://img.shields.io/badge/Nexus%20Mods-Lost%20Objects-orange)](https://www.nexusmods.com/fallout4/mods/110356)

**Download:** [Lost Objects on Nexus Mods](https://www.nexusmods.com/fallout4/mods/110356) (or the archive in [`releases/`](releases/) of this repository).

---

## The problem

You install a settlement mod such as *Settlement Objects Expansion Pack* or a menu
reorganizer such as *Workshop Rearranged*, and some pieces never show up in the workshop
menu - sometimes new ones from the mod, sometimes vanilla or DLC pieces that another mod
pushed out of the menu. They are loaded, they have a recipe - there is just no way to
click on them. Usually the cause is a menu that was never connected, a menu replacer that
dropped a category, or two mods overwriting the same vanilla menu list. Finding and fixing
that by hand needs xEdit and a lot of knowledge.

## What this mod does

After you load a save and every time you enter workshop mode, Lost Objects walks through
the whole workshop menu tree exactly the way the game does and finds **every workshop
recipe that cannot be reached** - no matter whether it comes from a mod, a DLC or the base
game, and no matter what it builds. Then it puts each item back, in this order:

1. **Exactly where its mod had it.** Lost Objects reads the menus of every plugin file in
   your load order, so it knows where each category and sub menu was before another mod
   changed that menu - and puts it back there, with its own name, at its old position.
   The report says which mod removed it.
2. **Into the existing category it belongs to** - for items whose mod never put them into
   any menu. The category is found by comparing the item with what is already in the
   menu - model file, model folder, name, first word, `[Tag]`, category name.
3. **Into a "Lost Objects" folder inside the mod's own menu** if no category fits clearly
   (better there than in the wrong place).
4. **Into the "Lost Objects" tab** of the main workshop menu only if the item has no home
   at all (mostly base game and Creation Club pieces).

```
Workshop main menu
 +- Structures
 |   `- Snap'n Build
 |       +- ...                        <- lost stairs sorted in where they fit
 |       `- Lost Objects               <- the rest of that mod's lost pieces
 +- Snappy Builds
 |   `- Lost Objects
 |       +- Wallpapers, Walls, Roof    <- grouped by what the pieces have in common
 |       `- Snappy_DLC03               <- an add-on that shares the menu
 `- Lost Objects                       <- only what has no home anywhere
```

- Large groups are split automatically (tags, words, model folders).
- Items that can already be built through another recipe are not shown twice.
- "Ghost objects" whose model file does not exist are hidden.
- Items keep their original costs and requirements; conditions of their mod still apply.
- The menus are built once the workshop menu has settled after loading and are updated
  when something new turns up - they never flip back and forth.
- Lost Objects remembers its result (a snapshot file next to its log). At the next game
  start the menus are complete right away, even if you open the workshop immediately.
- A **settings holotape** ("Lost Objects Settings") in your inventory: rebuild the menus
  now, and switch the main options on or off - no text file needed.
- Lost Objects stores nothing in your save game. Only the holotape is a normal item in
  your inventory. Remove the mod and everything is as before; things you already built
  stay where they are.
- Only workshop (settlement) recipes are considered - chemistry, cooking, armor and weapon
  workbenches are not touched.

## Requirements

| | |
|---|---|
| Fallout 4 | **1.11.240** (current Steam version) - tested. 1.10.984 and 1.10.163 are supported by the DLL but untested. |
| [F4SE](https://www.nexusmods.com/fallout4/mods/42147) | **0.7.9** for 1.11.240 |
| [Address Library for F4SE Plugins](https://www.nexusmods.com/fallout4/mods/47327) | the file for your game version |

Tested with a large settlement load order (Workshop Framework, Settlement Menu Manager,
Workshop Rearranged, Homemaker, SOE, Snap'n Build, Snappy HouseKit, The Master Plan and
more): see [docs/TESTED_WITH.md](docs/TESTED_WITH.md).

## Installation

Download it from [Nexus Mods](https://www.nexusmods.com/fallout4/mods/110356) - with Vortex or the Nexus app simply "Mod Manager Download".

Install the archive with Vortex, the Nexus app or Mod Organizer 2 and enable
`LostObjects.esp` (it is a light plugin and does not use a regular plugin slot).
Details: [docs/INSTALLATION.md](docs/INSTALLATION.md).

On the first load you will see a message that the mod was made with AI. It is shown once
per version.

## Settings

The **settings holotape** ("Lost Objects Settings", it is put into your inventory
automatically and can be crafted for free at the chemistry station) has the everyday options: rebuild the menus now, sort into existing
categories, hide ghost objects, hide items that can already be built, the summary
message and the development mode. Your choices are saved by Lost Objects itself.

Everything else is in `Data/F4SE/Plugins/LostObjects.ini` - for example `sPlacement=Tab`
to put everything into the tab, `iMinConfidence=` to make the sorting stricter or
`sIgnorePlugins=` to hide a plugin's items. All options:
[docs/CONFIGURATION.md](docs/CONFIGURATION.md).

## Reporting problems

Lost Objects writes two files to `Documents\My Games\Fallout4\F4SE\`:

- `LostObjects.log` - what the mod did
- `LostObjects_Report.txt` - every recovered item and where it went

With `bDevelopmentMode=1` it also writes the full inventory, the complete workshop menu
and a list of everything that appears twice - very helpful for bug reports about the
sorting.

Please attach both when you open an [issue](https://github.com/egnutv/LostObjects/issues).
See [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md) first.

## Files and privacy

Lost Objects does not go online and collects no personal data. Besides the files your
mod manager installs, it writes a few small files of its own to
`Documents\My Games\Fallout4\F4SE\` (log, report, snapshot, holotape choices). A mod
manager does not remove them - delete every file starting with `LostObjects` there if
you want to remove every trace. The full list, with what is inside each file:
[docs/FILES_AND_PRIVACY.md](docs/FILES_AND_PRIVACY.md).

## Documentation

| Document | For |
|---|---|
| [INSTALLATION.md](docs/INSTALLATION.md) | players |
| [CONFIGURATION.md](docs/CONFIGURATION.md) | players |
| [TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md) | players |
| [FILES_AND_PRIVACY.md](docs/FILES_AND_PRIVACY.md) | players: every file the mod installs or writes, and how to remove it |
| [HOW_IT_WORKS.md](docs/HOW_IT_WORKS.md) | curious players, mod authors, developers |
| [TESTING.md](docs/TESTING.md) | testers |
| [BUILDING.md](docs/BUILDING.md) | developers |
| [RELEASING.md](docs/RELEASING.md) | maintainers |
| [TESTED_WITH.md](docs/TESTED_WITH.md) | everybody |

## Planned

- **Language packs**: translations of the few texts Lost Objects shows itself (the AI
  notice, the summary message, folder names like "Lost Objects" and "Other"). Item and
  category names already come from the game and the mods in your language.

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
