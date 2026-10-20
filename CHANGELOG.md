# Changelog

All notable changes to Lost Objects. Format based on
[Keep a Changelog](https://keepachangelog.com/), versions follow
[Semantic Versioning](https://semver.org/).

## [0.1.0] - 2026-10-20

First test version.

### Added
- New workshop tab **Lost Objects** that collects every workshop recipe which cannot be
  reached from the main workshop menu - from mods, DLCs or the base game.
- Lost menus are restored with their original structure; lost categories keep their
  original names; recipes keep their costs and conditions.
- One folder per plugin file (`sSortMode=ByMod`, default) or a flat list (`sSortMode=Flat`).
- Lost recipes without a named category appear in "Uncategorized" inside their folder.
- Optional filters, all off by default: `bSkipUntouchedBaseGame`, `bSkipWithoutCategory`,
  `sIgnorePlugins`.
- Automatic scans after loading a save (with follow-ups for script-injected menus) and on
  entering workshop mode. The tab only appears when something is lost.
- Summary message on the first workshop visit after loading.
- AI disclosure message shown once per version.
- `LostObjects_Report.txt` with every recovered menu, category and recipe.
- INI settings: `bEnabled`, `sSortMode`, `bShowSummaryMessage`, `bBorrowMenuIcon`,
  `bWriteReport`, `bVerboseLog`, `bSkipUntouchedBaseGame`, `bSkipWithoutCategory`, `sIgnorePlugins`.
- One DLL for Fallout 4 1.10.163, 1.10.984 and 1.11.191+ (CommonLibF4 AV).
- Generated, reviewable light plugin (`tools/build_plugin.py`) with an independent checker.
- GitHub Actions build with automatic release archives.
