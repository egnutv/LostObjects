#!/usr/bin/env python3
"""
Lost Objects - release packager.

Builds the mod archive that goes to Nexus Mods / GitHub releases:

  LostObjects-<version>.zip
    LostObjects.esp
    Meshes/LostObjects/MenuIcon.nif          (workshop menu icon)
    Textures/LostObjects/MenuIcon_d.dds
    Scripts/LostObjects.pex                  (settings holotape, see tools/papyrus)
    Scripts/LostObjects_Holotape.pex
    Scripts/Source/User/*.psc                (their source code)
    F4SE/Plugins/LostObjects.dll
    F4SE/Plugins/LostObjects.ini
    F4SE/Plugins/LostObjects/README.txt
    F4SE/Plugins/LostObjects/LICENSE.txt
    F4SE/Plugins/LostObjects/NOTICE.txt
    F4SE/Plugins/LostObjects/CHANGELOG.txt
    F4SE/Plugins/LostObjects/THIRD_PARTY_NOTICES.txt

and, if a .pdb is given, LostObjects-<version>-pdb.zip (debug symbols for
crash logs; not needed by players).

The layout has no "Data" folder, so Vortex, the Nexus app and Mod Organizer 2
install it directly.

Usage:
  python tools/package.py --dll build/release/RelWithDebInfo/LostObjects.dll \
                          --pdb build/release/RelWithDebInfo/LostObjects.pdb \
                          --scripts build/papyrus/Scripts \
                          --out dist

Vibecoded by AI (Claude, Anthropic). License: PolyForm Noncommercial 1.0.0.
"""

from __future__ import annotations

import argparse
import json
import sys
import zipfile
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
DOCS_IN_ARCHIVE = {
    "README.txt": REPO_ROOT / "docs" / "PLAYER_README.txt",
    "LICENSE.txt": REPO_ROOT / "LICENSE",
    "NOTICE.txt": REPO_ROOT / "NOTICE.md",
    "CHANGELOG.txt": REPO_ROOT / "CHANGELOG.md",
    "THIRD_PARTY_NOTICES.txt": REPO_ROOT / "THIRD_PARTY_NOTICES.md",
}

SCRIPTS = ("LostObjects", "LostObjects_Holotape")
SCRIPT_SOURCE_DIR = REPO_ROOT / "data" / "Scripts" / "Source" / "User"


def read_version() -> str:
    manifest = json.loads((REPO_ROOT / "vcpkg.json").read_text(encoding="utf-8"))
    return manifest["version-semver"]


def add(archive: zipfile.ZipFile, source: Path, target: str) -> None:
    if not source.is_file():
        raise SystemExit(f"missing file for the package: {source}")
    archive.write(source, target)
    print(f"  + {target}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--dll", required=True, type=Path, help="path to LostObjects.dll")
    parser.add_argument("--pdb", type=Path, help="path to LostObjects.pdb (optional)")
    parser.add_argument("--scripts", required=True, type=Path, help="folder with the compiled scripts (*.pex), see tools/papyrus")
    parser.add_argument("--out", default=REPO_ROOT / "dist", type=Path, help="output folder")
    args = parser.parse_args()

    version = read_version()
    args.out.mkdir(parents=True, exist_ok=True)

    mod_zip = args.out / f"LostObjects-{version}.zip"
    print(f"Writing {mod_zip}")
    with zipfile.ZipFile(mod_zip, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        add(archive, REPO_ROOT / "data" / "LostObjects.esp", "LostObjects.esp")
        add(archive, args.dll, "F4SE/Plugins/LostObjects.dll")
        add(archive, REPO_ROOT / "data" / "F4SE" / "Plugins" / "LostObjects.ini", "F4SE/Plugins/LostObjects.ini")
        # Workshop menu icon (see tools/icon).
        for folder in ("Meshes", "Textures"):
            for source in sorted((REPO_ROOT / "data" / folder / "LostObjects").glob("*")):
                add(archive, source, f"{folder}/LostObjects/{source.name}")
        # Settings holotape scripts (see tools/papyrus) and their source code.
        for script in SCRIPTS:
            add(archive, args.scripts / f"{script}.pex", f"Scripts/{script}.pex")
            add(archive, SCRIPT_SOURCE_DIR / f"{script}.psc", f"Scripts/Source/User/{script}.psc")
        for name, source in DOCS_IN_ARCHIVE.items():
            add(archive, source, f"F4SE/Plugins/LostObjects/{name}")

    if args.pdb:
        pdb_zip = args.out / f"LostObjects-{version}-pdb.zip"
        print(f"Writing {pdb_zip}")
        with zipfile.ZipFile(pdb_zip, "w", compression=zipfile.ZIP_DEFLATED) as archive:
            add(archive, args.pdb, "LostObjects.pdb")

    print("Done.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
