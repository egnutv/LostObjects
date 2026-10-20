#!/usr/bin/env python3
"""
Lost Objects - plugin file generator.

Generates two files from one single source of truth (this script):

  data/LostObjects.esp   The ESL-flagged plugin that provides the menu
                            skeleton (keywords and form lists).
  src/PluginForms.h         The matching form IDs for the F4SE plugin.

Why a generator instead of the Creation Kit?
  The plugin only contains simple records (KYWD, FLST and one ARTO for the
  menu icon). Generating it
  makes every byte reviewable in git, keeps form IDs in sync with the C++
  code and lets anyone rebuild it without the Creation Kit.

Usage:
  python tools/build_plugin.py            (writes both files)
  python tools/build_plugin.py --check    (fails if the files are outdated)

Only the Python standard library is used.

Vibecoded by AI (Claude, Anthropic). Project owner and tester: egnutv.
License: PolyForm Noncommercial 1.0.0 (see LICENSE).
"""

from __future__ import annotations

import argparse
import struct
import sys
from dataclasses import dataclass, field
from pathlib import Path

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

PLUGIN_FILE = "LostObjects.esp"
AUTHOR = "egnutv"
DESCRIPTION = (
    "Lost Objects: adds a workshop menu tab that collects every craftable "
    "workshop item that is not reachable in the build menu. Requires LostObjects.dll "
    "(F4SE). Vibecoded by AI - AI-generated code can contain mistakes."
)
MASTERS = ["Fallout4.esm"]

# Fallout 4 plugin format details.
TES4_FLAG_LIGHT = 0x00000200  # ESL flag ("ESPFE")
HEDR_VERSION = 1.0  # 1.0 allows light plugin form IDs 0x800-0xFFF
FORM_VERSION = 131  # current Fallout 4 form version
KEYWORD_TYPE_RECIPE_FILTER = 9  # KYWD TNAM value for workshop categories

# Layout of the menu skeleton. Lost Objects builds its menus at runtime from a
# pool of generic, renamable menus and categories:
#   * MENU_COUNT sub menus (form list + title keyword) for folders and groups
#   * LEAF_COUNT categories (recipe filter keywords) that lost recipes are tagged with
MENU_COUNT = 192
LEAF_COUNT = 768
FIRST_LOCAL_ID = 0x800
LAST_LOCAL_ID = 0xFFF

ROOT_NAME = "Lost Objects"
MENU_ICON_MODEL = "LostObjects\\MenuIcon.nif"  # relative to Data\Meshes, see tools/icon
MENU_ICON_BOUNDS = (-32, -2, 0, 32, 2, 64)  # OBND of the icon mesh (x, y, z min / max)
MENU_NAME_TEMPLATE = "Menu {:03d}"  # renamed at runtime
LEAF_NAME_TEMPLATE = "Group {:03d}"  # renamed at runtime

REPO_ROOT = Path(__file__).resolve().parent.parent
PLUGIN_PATH = REPO_ROOT / "data" / PLUGIN_FILE
HEADER_PATH = REPO_ROOT / "src" / "PluginForms.h"


# ---------------------------------------------------------------------------
# Binary helpers
# ---------------------------------------------------------------------------


def zstring(text: str) -> bytes:
    """Null-terminated string. Only ASCII is used to avoid code page issues."""
    text.encode("ascii")  # raises if a non-ASCII character slipped in
    return text.encode("ascii") + b"\x00"


def subrecord(signature: str, data: bytes) -> bytes:
    if len(data) > 0xFFFF:
        raise ValueError(f"subrecord {signature} too large ({len(data)} bytes)")
    return signature.encode("ascii") + struct.pack("<H", len(data)) + data


def record(signature: str, form_id: int, data: bytes, flags: int = 0) -> bytes:
    # Fallout 4 record header (24 bytes):
    # type, data size, flags, form ID, version control info, form version, unknown
    header = struct.pack(
        "<4sIIIIHH",
        signature.encode("ascii"),
        len(data),
        flags,
        form_id,
        0,
        FORM_VERSION,
        0,
    )
    return header + data


def group(label: str, content: bytes) -> bytes:
    # Top level group header (24 bytes): "GRUP", total size (including this
    # header), label, group type 0 (top), stamp, unknown, version, unknown.
    header = struct.pack(
        "<4sI4sIHHHH",
        b"GRUP",
        24 + len(content),
        label.encode("ascii"),
        0,
        0,
        0,
        0,
        0,
    )
    return header + content


# ---------------------------------------------------------------------------
# Plugin content
# ---------------------------------------------------------------------------


@dataclass
class Keyword:
    local_id: int
    editor_id: str
    name: str


@dataclass
class FormList:
    local_id: int
    editor_id: str
    entries: list[int] = field(default_factory=list)  # local IDs


def full_id(local_id: int) -> int:
    """Form ID as stored inside the plugin: master count in the high byte."""
    return (len(MASTERS) << 24) | local_id


def build_layout() -> tuple[list[Keyword], list[FormList], dict[str, int]]:
    ids: dict[str, int] = {}
    next_id = FIRST_LOCAL_ID

    def take(name: str) -> int:
        nonlocal next_id
        if next_id > LAST_LOCAL_ID:
            raise ValueError("ran out of light plugin form IDs")
        ids[name] = next_id
        next_id += 1
        return ids[name]

    keywords: list[Keyword] = []
    formlists: list[FormList] = []

    keywords.append(Keyword(take("RootKeyword"), "LO_RootKeyword", ROOT_NAME))
    for i in range(1, MENU_COUNT + 1):
        keywords.append(Keyword(take(f"MenuTitle{i:03d}"), f"LO_MenuTitle{i:03d}", MENU_NAME_TEMPLATE.format(i)))
    for i in range(1, LEAF_COUNT + 1):
        keywords.append(Keyword(take(f"Leaf{i:03d}"), f"LO_Leaf{i:03d}", LEAF_NAME_TEMPLATE.format(i)))

    formlists.append(FormList(take("RootMenu"), "LO_RootMenu", [ids["RootKeyword"]]))
    for i in range(1, MENU_COUNT + 1):
        formlists.append(FormList(take(f"Menu{i:03d}"), f"LO_Menu{i:03d}", [ids[f"MenuTitle{i:03d}"]]))

    take("MenuIcon")

    ids["NextObjectID"] = next_id
    return keywords, formlists, ids


def build_plugin(keywords: list[Keyword], formlists: list[FormList], ids: dict[str, int]) -> bytes:
    next_object_id = ids["NextObjectID"]
    keyword_records = b"".join(
        record(
            "KYWD",
            full_id(k.local_id),
            subrecord("EDID", zstring(k.editor_id))
            + subrecord("CNAM", bytes((0, 0, 0, 0)))  # editor colour, unused
            + subrecord("TNAM", struct.pack("<I", KEYWORD_TYPE_RECIPE_FILTER))
            + subrecord("FULL", zstring(k.name)),
        )
        for k in keywords
    )

    formlist_records = b"".join(
        record(
            "FLST",
            full_id(f.local_id),
            subrecord("EDID", zstring(f.editor_id))
            + b"".join(subrecord("LNAM", struct.pack("<I", full_id(e))) for e in f.entries),
        )
        for f in formlists
    )

    # The workshop menu icon: an art object with our own mesh. The DLL links
    # the tab's and folders' title keywords to it at runtime.
    art_records = record(
        "ARTO",
        full_id(ids["MenuIcon"]),
        subrecord("EDID", zstring("LO_MenuIcon"))
        + subrecord("OBND", struct.pack("<6h", *MENU_ICON_BOUNDS))
        + subrecord("MODL", zstring(MENU_ICON_MODEL))
        + subrecord("DNAM", struct.pack("<I", 0)),  # art type: none / magic casting (unused for menu icons)
    )

    # HEDR "number of records" counts records *and* groups (as the Creation Kit does).
    group_count = 3
    record_count = len(keywords) + len(formlists) + 1 + group_count

    header_data = subrecord("HEDR", struct.pack("<fII", HEDR_VERSION, record_count, next_object_id))
    header_data += subrecord("CNAM", zstring(AUTHOR))
    header_data += subrecord("SNAM", zstring(DESCRIPTION))
    for master in MASTERS:
        header_data += subrecord("MAST", zstring(master))
        header_data += subrecord("DATA", struct.pack("<Q", 0))

    tes4 = record("TES4", 0, header_data, flags=TES4_FLAG_LIGHT)

    # Top level groups in the order the Creation Kit writes them (KYWD, FLST, ARTO).
    return tes4 + group("KYWD", keyword_records) + group("FLST", formlist_records) + group("ARTO", art_records)


def build_header(ids: dict[str, int]) -> str:
    lines = [
        "#pragma once",
        "",
        "// -----------------------------------------------------------------------------",
        "// GENERATED FILE - do not edit by hand.",
        "// Source: tools/build_plugin.py (run it to regenerate this file and the .esp).",
        "// -----------------------------------------------------------------------------",
        "",
        "#include <cstdint>",
        "#include <string_view>",
        "",
        "namespace LO::Forms",
        "{",
        f'\tinline constexpr std::string_view PluginFile = "{PLUGIN_FILE}";',
        f'\tinline constexpr std::string_view RootName = "{ROOT_NAME}";  // FULL of RootKeyword',
        "",
        f"\tinline constexpr std::uint32_t RootKeyword = 0x{ids['RootKeyword']:03X};",
        f"\tinline constexpr std::uint32_t RootMenu = 0x{ids['RootMenu']:03X};",
        "",
        f"\tinline constexpr std::uint32_t MenuCount = {MENU_COUNT};",
        f"\tinline constexpr std::uint32_t FirstMenuTitle = 0x{ids['MenuTitle001']:03X};",
        f"\tinline constexpr std::uint32_t FirstMenu = 0x{ids['Menu001']:03X};",
        "",
        f"\tinline constexpr std::uint32_t LeafCount = {LEAF_COUNT};",
        f"\tinline constexpr std::uint32_t FirstLeaf = 0x{ids['Leaf001']:03X};",
        "",
        f"\tinline constexpr std::uint32_t MenuIcon = 0x{ids['MenuIcon']:03X};  // ARTO with Meshes\\{MENU_ICON_MODEL}",
        f'\tinline constexpr std::string_view MenuIconModel = "{MENU_ICON_MODEL.replace(chr(92), chr(92)*2)}";',
        "",
        "\t// Vanilla forms (Fallout4.esm).",
        '\tinline constexpr std::string_view BaseGameFile = "Fallout4.esm";',
        "\tinline constexpr std::uint32_t WorkshopMenuMain = 0x106DA2;",
        "}",
        "",
    ]
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--check", action="store_true", help="only verify that the generated files are up to date")
    args = parser.parse_args()

    keywords, formlists, ids = build_layout()
    plugin_bytes = build_plugin(keywords, formlists, ids)
    header_text = build_header(ids)

    if args.check:
        ok = True
        if not PLUGIN_PATH.exists() or PLUGIN_PATH.read_bytes() != plugin_bytes:
            print(f"OUTDATED: {PLUGIN_PATH.relative_to(REPO_ROOT)}")
            ok = False
        if not HEADER_PATH.exists() or HEADER_PATH.read_text(encoding="utf-8") != header_text:
            print(f"OUTDATED: {HEADER_PATH.relative_to(REPO_ROOT)}")
            ok = False
        if ok:
            print("Generated files are up to date.")
        return 0 if ok else 1

    PLUGIN_PATH.parent.mkdir(parents=True, exist_ok=True)
    PLUGIN_PATH.write_bytes(plugin_bytes)
    HEADER_PATH.write_text(header_text, encoding="utf-8", newline="\n")
    print(f"Wrote {PLUGIN_PATH.relative_to(REPO_ROOT)} ({len(plugin_bytes)} bytes, {len(keywords) + len(formlists) + 1} records)")
    print(f"Wrote {HEADER_PATH.relative_to(REPO_ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
