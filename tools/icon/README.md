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

Textures (Python 3 with Pillow):

    python tools/icon/make_icon_textures.py

Mesh (needs [nifly](https://github.com/ousnius/nifly), GPL-3.0, used only as a
build tool - it is not part of the mod and not shipped):

    git clone https://github.com/ousnius/nifly
    cmake -S nifly -B nifly/build -DBUILD_TESTING=OFF && cmake --build nifly/build
    g++ -std=c++17 -Inifly/include -Inifly/external tools/icon/make_icon_nif.cpp nifly/build/src/libnifly.a -o make_icon_nif
    ./make_icon_nif data/Meshes/LostObjects/MenuIcon.nif

`make_icon_nif --check <file.nif>` loads a mesh and prints what is in it.
