#!/usr/bin/env python3
"""
Lost Objects - plugin file checker.

Reads data/LostObjects.esp with an independent parser and checks that it
is a well formed Fallout 4 light plugin that matches what the DLL expects:

  * TES4 header: light (ESL) flag, HEDR version 1.0, master Fallout4.esm,
    record + group count matches the file
  * only KYWD, FLST, ARTO, TRNS, GLOB, NOTE, TERM and COBJ records, all new form
    IDs inside 0x800-0xFFF
  * the settings holotape: NOTE (type terminal) -> TERM with the script and
    menu items whose conditions point to globals of this plugin
  * every keyword is a recipe filter (TNAM 9) with a name (FULL)
  * every form list entry points to a record inside this plugin
  * the form IDs in src/PluginForms.h exist with the right record type

Usage:  python tools/verify_plugin.py [path/to/LostObjects.esp]
Exit code 0 = everything fine.

Vibecoded by AI (Claude, Anthropic). License: PolyForm Noncommercial 1.0.0.
"""

from __future__ import annotations

import re
import struct
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_PLUGIN = REPO_ROOT / "data" / "LostObjects.esp"
HEADER = REPO_ROOT / "src" / "PluginForms.h"

errors: list[str] = []


def fail(message: str) -> None:
    errors.append(message)


def parse_subrecords(data: bytes) -> list[tuple[str, bytes]]:
    result = []
    offset = 0
    while offset < len(data):
        if offset + 6 > len(data):
            fail(f"truncated subrecord header at offset {offset}")
            break
        signature = data[offset : offset + 4].decode("ascii", "replace")
        (size,) = struct.unpack_from("<H", data, offset + 4)
        payload = data[offset + 6 : offset + 6 + size]
        if len(payload) != size:
            fail(f"truncated subrecord {signature}")
        result.append((signature, payload))
        offset += 6 + size
    return result


def zstring(data: bytes) -> str:
    if not data.endswith(b"\x00"):
        fail(f"string is not null-terminated: {data!r}")
    return data.rstrip(b"\x00").decode("ascii", "replace")


def main() -> int:
    path = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_PLUGIN
    blob = path.read_bytes()

    # --- TES4 header -------------------------------------------------------
    sig, size, flags, form_id, _vc, version, _unk = struct.unpack_from("<4sIIIIHH", blob, 0)
    if sig != b"TES4":
        fail("file does not start with TES4")
        return report()
    if not flags & 0x200:
        fail("TES4 is missing the light (ESL) flag 0x200")
    if flags & 0x1:
        fail("TES4 has the master flag set (should be a light .esp)")

    header = dict()
    masters = []
    for name, payload in parse_subrecords(blob[24 : 24 + size]):
        if name == "MAST":
            masters.append(zstring(payload))
        else:
            header[name] = payload

    hedr_version, hedr_count, next_id = struct.unpack("<fII", header["HEDR"])
    if abs(hedr_version - 1.0) > 1e-6:
        fail(f"HEDR version is {hedr_version}, expected 1.0")
    if masters != ["Fallout4.esm"]:
        fail(f"masters are {masters}, expected ['Fallout4.esm']")

    master_index = len(masters)

    # --- Groups and records -----------------------------------------------
    records: dict[int, tuple[str, dict[str, list[bytes]]]] = {}
    group_count = 0
    offset = 24 + size
    while offset < len(blob):
        gsig, gsize, label, gtype = struct.unpack_from("<4sI4sI", blob, offset)
        if gsig != b"GRUP" or gtype != 0:
            fail(f"expected a top level GRUP at offset {offset}")
            break
        group_count += 1
        end = offset + gsize
        inner = offset + 24
        while inner < end:
            rsig, rsize, rflags, rid, _vc, rversion, _unk = struct.unpack_from("<4sIIIIHH", blob, inner)
            rtype = rsig.decode("ascii")
            if rtype != label.decode("ascii"):
                fail(f"record {rtype} {rid:08X} inside group {label!r}")
            if rflags & 0x00040000:
                fail(f"record {rid:08X} is compressed (not expected)")
            fields: dict[str, list[bytes]] = {}
            for name, payload in parse_subrecords(blob[inner + 24 : inner + 24 + rsize]):
                fields.setdefault(name, []).append(payload)
            if rid in records:
                fail(f"duplicate form ID {rid:08X}")
            records[rid] = (rtype, fields)
            inner += 24 + rsize
        if inner != end:
            fail(f"group {label!r} size mismatch")
        offset = end

    if hedr_count != len(records) + group_count:
        fail(f"HEDR count {hedr_count} != records {len(records)} + groups {group_count}")

    header_text = HEADER.read_text(encoding="utf-8")

    def constant_text(name: str) -> str:
        match = re.search(rf'{name}\s*=\s*"([^"]*)"', header_text)
        return match.group(1) if match else ""

    # --- Record content ---------------------------------------------------
    for rid, (rtype, fields) in records.items():
        if rid >> 24 != master_index:
            fail(f"{rid:08X} is not a new record of this plugin (override?)")
        local = rid & 0xFFFFFF
        if not 0x800 <= local <= 0xFFF:
            fail(f"{rid:08X} is outside the light plugin range 0x800-0xFFF")
        if local >= next_id:
            fail(f"{rid:08X} is not below HEDR next object ID {next_id:X}")

        edid = zstring(fields.get("EDID", [b"\x00"])[0])
        if not edid.startswith("LO_"):
            fail(f"{rid:08X} editor ID '{edid}' does not start with LO_")

        if rtype == "KYWD":
            (tnam,) = struct.unpack("<I", fields["TNAM"][0])
            if tnam != 9:
                fail(f"keyword {edid} has type {tnam}, expected 9 (recipe filter)")
            if not zstring(fields.get("FULL", [b"\x00"])[0]):
                fail(f"keyword {edid} has no name (FULL)")
            if len(fields.get("CNAM", [b""])[0]) != 4:
                fail(f"keyword {edid} CNAM must be 4 bytes")
        elif rtype == "FLST":
            entries = [struct.unpack("<I", e)[0] for e in fields.get("LNAM", [])]
            if not entries:
                fail(f"form list {edid} is empty")
            for i, entry in enumerate(entries):
                if entry not in records:
                    fail(f"form list {edid} entry {i} -> {entry:08X} does not exist in this plugin")
                elif i == 0 and records[entry][0] != "KYWD":
                    fail(f"form list {edid} entry 0 must be a keyword")
        elif rtype == "TRNS":
            if len(fields.get("DATA", [b""])[0]) != 36:
                fail(f"transform {edid} DATA must be 36 bytes")
        elif rtype == "ARTO":
            ptrn = fields.get("PTRN")
            if not ptrn or struct.unpack("<I", ptrn[0])[0] not in records or records[struct.unpack("<I", ptrn[0])[0]][0] != "TRNS":
                fail(f"art object {edid} needs a preview transform (PTRN -> TRNS of this plugin)")
            model = zstring(fields.get("MODL", [b"\x00"])[0])
            if not model.lower().endswith(".nif"):
                fail(f"art object {edid} has no model (MODL)")
            if len(fields.get("OBND", [b""])[0]) != 12:
                fail(f"art object {edid} OBND must be 12 bytes")
            if len(fields.get("DNAM", [b""])[0]) != 4:
                fail(f"art object {edid} DNAM must be 4 bytes")
            if not (REPO_ROOT / "data" / "Meshes" / model.replace("\\", "/")).exists():
                fail(f"art object {edid}: mesh data/Meshes/{model} is missing")
        elif rtype == "GLOB":
            if fields.get("FNAM", [b""])[0] not in (b"s", b"l", b"f"):
                fail(f"global {edid} needs FNAM s, l or f")
            if len(fields.get("FLTV", [b""])[0]) != 4:
                fail(f"global {edid} FLTV must be 4 bytes")
        elif rtype == "NOTE":
            if fields.get("DNAM", [b""])[0] != b"\x03":
                fail(f"holotape {edid} must be of type terminal (DNAM 3)")
            if len(fields.get("DATA", [b""])[0]) != 8:
                fail(f"holotape {edid} DATA must be 8 bytes (value, weight)")
            snam = fields.get("SNAM", [b""])[0]
            if len(snam) != 4 or struct.unpack("<I", snam)[0] not in records or records[struct.unpack("<I", snam)[0]][0] != "TERM":
                fail(f"holotape {edid} SNAM must point to a TERM of this plugin")
            if not zstring(fields.get("FULL", [b"\x00"])[0]):
                fail(f"holotape {edid} has no name (FULL)")
        elif rtype == "TERM":
            vmad = fields.get("VMAD", [b""])[0]
            if len(vmad) < 6:
                fail(f"terminal {edid} has no script (VMAD)")
            else:
                version, object_format, script_count = struct.unpack_from("<hhH", vmad, 0)
                if version != 6 or object_format != 2 or script_count != 1:
                    fail(f"terminal {edid} VMAD header is {version}/{object_format}/{script_count}, expected 6/2/1")
                (name_length,) = struct.unpack_from("<H", vmad, 6)
                script = vmad[8 : 8 + name_length].decode("ascii", "replace")
                if script != constant_text("HolotapeScript"):
                    fail(f"terminal {edid} script is '{script}', expected the one in PluginForms.h")
                rest = vmad[8 + name_length :]
                # status, property count, fragments: version 3, empty script, flags, properties, count
                if rest != struct.pack("<BH", 0, 0) + struct.pack("<bH", 3, 0) + struct.pack("<BHH", 0, 0, 0):
                    fail(f"terminal {edid} VMAD tail is not the expected 'no properties, no fragments'")
            (isiz,) = struct.unpack("<I", fields.get("ISIZ", [b"\x00\x00\x00\x00"])[0])
            if isiz != len(fields.get("ITXT", [])) or isiz != len(fields.get("ITID", [])) or isiz != len(fields.get("ANAM", [])):
                fail(f"terminal {edid}: ISIZ {isiz} does not match the menu items")
            (bsiz,) = struct.unpack("<I", fields.get("BSIZ", [b"\x00\x00\x00\x00"])[0])
            if bsiz != len(fields.get("BTXT", [])):
                fail(f"terminal {edid}: BSIZ {bsiz} does not match the body texts")
            item_ids = [struct.unpack("<H", i)[0] for i in fields.get("ITID", [])]
            if len(set(item_ids)) != len(item_ids):
                fail(f"terminal {edid}: menu item IDs are not unique")
            for ctda in fields.get("CTDA", []):
                if len(ctda) != 32:
                    fail(f"terminal {edid}: CTDA must be 32 bytes")
                    continue
                function, = struct.unpack_from("<H", ctda, 8)
                param1, = struct.unpack_from("<I", ctda, 12)
                if function != 74 or param1 not in records or records[param1][0] != "GLOB":
                    fail(f"terminal {edid}: condition must be GetGlobalValue of a global of this plugin")
        elif rtype == "COBJ":
            cnam = fields.get("CNAM", [b""])[0]
            if len(cnam) != 4 or struct.unpack("<I", cnam)[0] not in records or records[struct.unpack("<I", cnam)[0]][0] != "NOTE":
                fail(f"recipe {edid} must create the holotape (CNAM -> NOTE of this plugin)")
            if "FVPA" in fields:
                fail(f"recipe {edid} must be free (no components)")
        else:
            fail(f"unexpected record type {rtype}")

    # --- Header constants ---------------------------------------------------

    def constant(name: str) -> int:
        match = re.search(rf"{name}\s*=\s*(0x[0-9A-Fa-f]+|\d+)", header_text)
        if not match:
            fail(f"constant {name} missing in PluginForms.h")
            return -1
        return int(match.group(1), 0)

    def expect(local: int, rtype: str, what: str) -> None:
        rid = (master_index << 24) | local
        if rid not in records:
            fail(f"{what} ({local:03X}) from PluginForms.h is missing in the plugin")
        elif records[rid][0] != rtype:
            fail(f"{what} ({local:03X}) is a {records[rid][0]}, expected {rtype}")

    expect(constant("RootKeyword"), "KYWD", "RootKeyword")
    expect(constant("RootMenu"), "FLST", "RootMenu")
    for i in range(max(constant("MenuCount"), 0)):
        expect(constant("FirstMenuTitle") + i, "KYWD", f"MenuTitle {i + 1}")
        expect(constant("FirstMenu") + i, "FLST", f"Menu {i + 1}")
    for i in range(max(constant("LeafCount"), 0)):
        expect(constant("FirstLeaf") + i, "KYWD", f"Leaf {i + 1}")
    expect(constant("MenuIcon"), "ARTO", "MenuIcon")
    expect(constant("MenuIconTransform"), "TRNS", "MenuIconTransform")
    expect(constant("Holotape"), "NOTE", "Holotape")
    expect(constant("HolotapeTerminal"), "TERM", "HolotapeTerminal")
    for i in range(max(constant("OptionCount"), 0)):
        expect(constant("FirstOptionGlobal") + i, "GLOB", f"Option {i + 1}")
    expect(constant("HolotapeRecipe"), "COBJ", "HolotapeRecipe")

    print(f"{path.name}: {len(records)} records in {group_count} groups, masters {masters}, light={bool(flags & 0x200)}")
    return report()


def report() -> int:
    if errors:
        for message in errors:
            print(f"ERROR: {message}")
        return 1
    print("Plugin check passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
