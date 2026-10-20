# Tested with

Lost Objects 0.9.0 was tested by egnutv on **20 October 2026** with this setup.
Other setups should work, but they were not tested - reports are welcome.

## Game

| | Version |
|---|---|
| Fallout 4 (Steam) | **1.11.240** (current version, with the Anniversary / Creation Club content) |
| Fallout 4 Script Extender (F4SE) | **0.7.9** |
| Address Library for F4SE Plugins (All In One) | **1.11.240** |
| Mod manager | Vortex 1.15.2 |

The DLL is also built for the older game versions 1.10.984 (Next-Gen) and 1.10.163
(Old-Gen), but it was **only tested on 1.11.240**.

## Mods in the tested load order

| Mod | Version |
|---|---|
| Achievements Mods Enabler | 1.41 |
| Mod Configuration Menu (MCM) | 1.11.240 |
| Workshop Framework | 2.6.1 |
| Workshop Rearranged | 0.6.13 |
| Settlement Menu Manager | 0.1.7 |
| Settlement Menu Manager Patches (SMM) | 6.0 |
| Addictol | 1.7.1 |
| Homemaker | 1.79.6 (NG Hotfix 6) |
| Snap'n Build | 2.0 |
| Settlement Objects Expansion Pack - All DLC | 1.6.7 |
| Settlement Objects Expansion Pack - Vanilla Menu Fixer | 1.6.1 |
| Snappy HouseKit | 9.2.2 |
| Snappy_DLC03 | 2.2.0 |
| Siles - Connectable Electricpoles | 1.1.0 |
| Place Everywhere (for 1.11.240) | 1.22.1 |
| Scrap Everything | 2.5.5 |
| SOU All in one | 1.3 |
| Settlement Height 9000 - AIO | 3.1 |
| Workshop Plus | 1.0.15 |
| SettlementOfUnlimitedSize AE | 0.6 |
| BNS Landscape | 3.0 |
| BNS Trees | 3.0 |
| Renovated Furniture | 5.1 |
| Renovated Decorations | 3.6 |
| Settlement Keywords Expanded | 1.60 |
| The Master Plan | 2.3 |
| Atom Greatness | 1.1 |
| Box Houses | 1.1 |
| Castle In The Sky | 1.2 |
| Cabin In The Woods | 2.0 |
| Vanilla Extensions | 2.6.0 |
| Red Rocket Reborn | 1.8.0 |
| Red Rocket Reborn - Expanded | 0.6.0 |
| Hamed Bridge | 1.2 |
| Wasteland Billboards | 1.2 |
| Wireless Living | 1.1.0 |
| OC Decorator (+ DLC) | - |
| Chuck Yufarley Collection (Diner, Greenhouse, Log, Security Wall sets) | - |
| NPC Spawner | - |
| Lore Friendly Posters | - |
| Settlement Supplies Expanded (SSEX) | - |

Plus the Creation Club content of the current game version (Halloween, Holiday,
Mid-Century Modern, Slocum's Joe, VR Workshops, Neon Flats, Vault Homes, Manor and
others) and all six official DLCs.

## What was checked

- Lost recipes are found and placed; nothing appears twice (`LostObjects_Duplicates.txt`).
- Sorting into existing categories (77 items) checked by hand - no wrong placements.
- "Lost Objects" folders inside the mods' own menus, one per menu.
- Ghost objects (model file missing) are hidden; thumbnails show for recovered items.
- The menus are built once the workshop menu has settled after loading and stay stable.
- The menu icon (the logo) shows in the main workshop menu.
