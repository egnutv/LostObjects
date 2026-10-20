# Building

You do not need to build anything yourself to play or test - GitHub builds every
commit automatically and stores the finished mod in `releases/` inside the repository (see [TESTING.md](TESTING.md#0-get-a-test-build)). This page is for
developers who want to build locally or continue the project.

## One-click build on your PC

Double-click `build.cmd` in the repository folder. It checks the tools, sets up
vcpkg, builds the DLL and packages the mod into `dist\LostObjects-<version>.zip`.
If a tool is missing it tells you what to install and where to get it.

Install once:

1. **Visual Studio 2026** (Community is free) with the workload
   "Desktop development with C++" - https://visualstudio.microsoft.com/
2. **CMake 4.3 or newer** - https://cmake.org/download/ (choose "Add CMake to the PATH")
3. **Git** - https://git-scm.com/download/win
4. **Python 3.10 or newer** - https://www.python.org/downloads/ (tick "Add python.exe to PATH")

The first build downloads vcpkg into the hidden folder `.vcpkg` and builds the
libraries; that takes a while. Later builds take a few minutes. Options:
`build.cmd --clean` (start from scratch), `build.cmd --no-package` (DLL only).

## Toolchain

| Tool | Version |
|---|---|
| Visual Studio 2026 | with "Desktop development with C++" (MSVC v145 or newer, Windows SDK) |
| CMake | 4.3 or newer (Visual Studio's bundled CMake works if new enough) |
| vcpkg | any recent version; set the environment variable `VCPKG_ROOT` to its folder |
| Git | for CMake to download CommonLibF4 |
| Python 3.10+ | for the tools in `tools/` (standard library only; the icon tools also need Pillow, see `tools/icon/requirements.txt`) |

Dependencies are resolved automatically:

- [CommonLibF4 AV](https://github.com/LucaDotGit/CommonLibF4) is downloaded by CMake
  (`FetchContent`) at the commit pinned in `CMakeLists.txt`.
- `fmt` and `spdlog` come from vcpkg (manifest `vcpkg.json`, pinned baseline).

## Build

```bat
git clone https://github.com/egnutv/LostObjects.git
cd LostObjects
cmake --preset release
cmake --build --preset release
```

The DLL is written to `build\release\RelWithDebInfo\LostObjects.dll`.

Optional: set `XSE_FO4_MODS_PATH` (e.g. your Mod Organizer 2 `mods` folder) or
`XSE_FO4_GAME_PATH` (the Fallout 4 folder) and the DLL is copied there after each build.

## Package

```bat
python tools\package.py --dll build\release\RelWithDebInfo\LostObjects.dll --pdb build\release\RelWithDebInfo\LostObjects.pdb
```

Creates `dist\LostObjects-<version>.zip` (the mod) and `dist\LostObjects-<version>-pdb.zip`
(debug symbols).

## The plugin file

`data/LostObjects.esp` is **generated**. Do not edit it in xEdit or the Creation Kit -
change `tools/build_plugin.py` and run:

```bat
python tools\build_plugin.py
python tools\verify_plugin.py
```

`build_plugin.py` also regenerates `src/PluginForms.h`, so the DLL and the plugin always
agree on form IDs. The CI fails if either file is out of date.

## Supported game versions

CommonLibF4 AV builds one DLL for Old-Gen (1.10.163), Next-Gen (1.10.984) and the
Anniversary Edition (1.11.191+). For the Anniversary Edition the DLL declares itself
version independent through the Address Library, so new game patches usually only need
a new Address Library file - not a new DLL. If a patch changes a structure the mod uses,
CommonLibF4 has to be updated: change `GIT_TAG` in `CMakeLists.txt` to a newer commit.

## Everything is reproducible

Every file in the mod archive can be rebuilt from this repository, and the CI checks it
on every push:

| File in the archive | Built from | Check |
|---|---|---|
| `F4SE/Plugins/LostObjects.dll` | `src/`, CommonLibF4 AV at a pinned commit, vcpkg at a pinned baseline | built by the CI |
| `LostObjects.esp` | `tools/build_plugin.py` | `build_plugin.py --check`, `verify_plugin.py` |
| `Meshes/LostObjects/MenuIcon.nif` | `tools/icon/make_icon_nif.cpp` | rebuilt and compared byte for byte |
| `Textures/LostObjects/MenuIcon_d.dds` | `tools/icon/make_icon_textures.py` from `Logo/Logo.png` | rebuilt and compared byte for byte |
| INI, readme, license files | `data/`, `docs/`, root | copied by `tools/package.py` |

No Creation Kit, xEdit, NifSkope or other closed tool is needed. Details for the icon:
[tools/icon/README.md](../tools/icon/README.md).

## Building in your own fork

Fork the repository on GitHub and push - the **Build** workflow runs in your fork as well
and stores the archive in `releases/` of your fork. Nothing in the workflow depends on
the original repository.

## Project layout

```
.github/        CI workflow and issue templates
Logo/           the logo (PNG, plus the Affinity and Paint.NET source files)
data/           files that go into the game's Data folder (esp, ini, mesh, texture)
docs/           documentation
nexus/          the Nexus Mods page kit
releases/       the latest mod archive, stored by the CI
src/            C++ source of the F4SE plugin
tools/          plugin generator and checker, packager, local build script
tools/icon/     generators for the menu icon mesh, its texture and the Nexus header
CMakeLists.txt  build definition
vcpkg.json      library dependencies
```

Code overview: [HOW_IT_WORKS.md](HOW_IT_WORKS.md#8-files-of-the-project).

## Publishing the repository for the first time

1. Create an empty repository on GitHub named `LostObjects` (no README, no license -
   both are already here).
2. In this folder:

   ```bat
   git init -b main
   git add .
   git commit -m "Lost Objects"
   git remote add origin https://github.com/<your-name>/LostObjects.git
   git push -u origin main
   ```

   Or with GitHub Desktop: *File -> Add local repository*, choose this folder, then
   *Publish repository*.
3. The **Actions** tab starts the first build automatically.

If your GitHub user name is not `egnutv`, replace `egnutv/LostObjects` in `README.md`,
`NOTICE.md` and `docs/` with your own path.
