# Workshop menu icon

The "Lost Objects" tab and folders show the logo as their icon - exactly the
transparent `Logo/Logo.png`, nothing added.

| File | Made by |
|---|---|
| `data/Meshes/LostObjects/MenuIcon.nif` | `make_icon_nif.cpp` |
| `data/Textures/LostObjects/MenuIcon_d.dds` | `make_icon_textures.py` (from `Logo/Logo.png`) |
| `LO_MenuIcon` (ARTO) in `LostObjects.esp` | `tools/build_plugin.py` |

The mesh is ONE 64 x 64 plane facing -Y, centered on the origin, like the flat
icons of other mods: one `BSTriShape` with an unlit `BSEffectShaderProperty`
(double sided, because the menu turns the selected icon) showing the texture as it
is, and a `NiAlphaProperty` (alpha test, threshold 128) that cuts out everything
transparent in the logo. `LO_MenuIcon` has a preview transform (`LO_MenuIconTransform`);
the DLL copies the vanilla "Decorations" icon's transform into it and scales it by
the ratio of the two icons' bounds, so the logo gets the same size as that icon.

## Rebuilding

The generated files are in the repository, so nobody has to rebuild them to build or
use the mod. The CI checks on every push that they can be reproduced byte for byte.

Textures (Python 3.10+ with the pinned Pillow version):

    pip install -r tools/icon/requirements.txt
    python tools/icon/make_icon_textures.py

Mesh (CMake 3.21+ and any C++17 compiler - Visual Studio, GCC or Clang; CMake
downloads [nifly](https://github.com/ousnius/nifly) (GPL-3.0) at a pinned commit, used
only by this build tool, not part of the mod):

    cmake -S tools/icon -B build/icon
    cmake --build build/icon --config Release
    build/icon/make_icon_nif data/Meshes/LostObjects/MenuIcon.nif                 (Linux, macOS)
    build\icon\Release\make_icon_nif.exe data\Meshes\LostObjects\MenuIcon.nif   (Windows)

`make_icon_nif --check <file.nif>` loads a mesh and prints what is in it.

Nexus header image (optional, `nexus/header.png`):

    python tools/icon/make_nexus_header.py
