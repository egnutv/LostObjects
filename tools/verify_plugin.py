#!/usr/bin/env python3
"""
Lost Objects - plugin file checker.

Reads data/LostObjects.esp with an independent parser and checks that it
is a well formed Fallout 4 light plugin that matches what the DLL expects:

  * TES4 header: light (ESL) flag, HEDR version 1.0, master Fallout4.esm,
    record + group count matches the file
  * only KYWD and FLST records, all new form IDs inside 0x800-0xFFF
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
        else:
            fail(f"unexpected record type {rtype}")

    # --- Header constants ---------------------------------------------------
    header_text = HEADER.read_text(encoding="utf-8")

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
    expect(constant("UncategorizedKeyword"), "KYWD", "UncategorizedKeyword")
    expect(constant("RootMenu"), "FLST", "RootMenu")
    slots = constant("SlotCount")
    for i in range(max(slots, 0)):
        expect(constant("FirstSlotKeyword") + i, "KYWD", f"SlotKeyword {i + 1}")
        expect(constant("FirstSlotMenu") + i, "FLST", f"SlotMenu {i + 1}")
        expect(constant("FirstSlotUncategorizedKeyword") + i, "KYWD", f"SlotUncategorizedKeyword {i + 1}")

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
