# Workshop menu icon

The "Lost Objects" tab and folders show a small picture frame with the logo.

| File | Made by |
|---|---|
| `data/Meshes/LostObjects/MenuIcon.nif` | `make_icon_nif.cpp` |
| `data/Textures/LostObjects/MenuIcon_d.dds`, `MenuIcon_n.dds` | `make_icon_textures.py` (from `Logo/Logo.png`) |
| `LO_MenuIcon` (ARTO) in `LostObjects.esp` | `tools/build_plugin.py` |

The mesh is a 64 x 4 x 64 slab (bottom at Z = 0) with the logo on both faces and
the frame color on the edges, one `BSTriShape` with a `BSLightingShaderProperty`
(double sided) and a texture set pointing to the two textures.

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
