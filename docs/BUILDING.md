# Building

You do not need to build anything yourself to play or test - GitHub builds every
commit automatically (see [TESTING.md](TESTING.md#0-get-a-test-build)). This page is for
developers who want to build locally or continue the project.

## Toolchain

| Tool | Version |
|---|---|
| Visual Studio 2026 | with "Desktop development with C++" (MSVC v145 or newer, Windows SDK) |
| CMake | 4.3 or newer (Visual Studio's bundled CMake works if new enough) |
| vcpkg | any recent version; set the environment variable `VCPKG_ROOT` to its folder |
| Git | for CMake to download CommonLibF4 |
| Python 3.10+ | only for the tools in `tools/` |

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

## Project layout

```
.github/        CI workflow and issue templates
data/           files that go into the game's Data folder
docs/           documentation
src/            C++ source of the F4SE plugin
tools/          plugin generator, plugin checker, packager
CMakeLists.txt  build definition
vcpkg.json      library dependencies
```

Code overview: [HOW_IT_WORKS.md](HOW_IT_WORKS.md#7-files-of-the-project).

## Publishing the repository for the first time

1. Create an empty repository on GitHub named `LostObjects` (no README, no license -
   both are already here).
2. In this folder:

   ```bat
   git init -b main
   git add .
   git commit -m "Lost Objects 0.1.0"
   git remote add origin https://github.com/<your-name>/LostObjects.git
   git push -u origin main
   ```

   Or with GitHub Desktop: *File -> Add local repository*, choose this folder, then
   *Publish repository*.
3. The **Actions** tab starts the first build automatically.

If your GitHub user name is not `egnutv`, replace `egnutv/LostObjects` in `README.md`,
`NOTICE.md` and `docs/` with your own path.
