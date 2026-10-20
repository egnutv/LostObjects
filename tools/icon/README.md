# Workshop menu icon

The "Lost Objects" tab and folders show the logo as their icon - exactly the
transparent `Logo/Logo.png`, nothing added.

| File | Made by |
|---|---|
| `data/Meshes/LostObjects/MenuIcon.nif` | `make_icon_nif.cpp` |
| `data/Textures/LostObjects/MenuIcon_d.dds`, `MenuIcon_n.dds` | `make_icon_textures.py` (from `Logo/Logo.png`) |
| `LO_MenuIcon` (ARTO) in `LostObjects.esp` | `tools/build_plugin.py` |

The mesh is two 64 x 64 planes back to back (bottom at Z = 0, the back one with
mirrored UVs so the logo reads correctly from both sides): one `BSTriShape` with a
`BSLightingShaderProperty` (double sided), a texture set pointing to the two
textures and a `NiAlphaProperty` (alpha test, threshold 128) that cuts out
everything transparent in the logo.

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
