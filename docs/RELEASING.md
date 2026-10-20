# Releasing

## Version numbers

`MAJOR.MINOR.PATCH` (for example `0.1.0`). The version appears in three places and must
be changed together:

| File | Field |
|---|---|
| `CMakeLists.txt` | `project(... VERSION 0.1.0.0 ...)` |
| `vcpkg.json` | `"version-semver": "0.1.0"` |
| `CHANGELOG.md` | new section at the top |

A new version also shows the AI message in game once more.

## Steps

1. Update the three files above and commit.
2. Wait for the **Build** workflow to pass.
3. Create and push a tag:

   ```bat
   git tag v0.1.0
   git push origin v0.1.0
   ```

4. The workflow builds again and creates a **GitHub release** with
   `LostObjects-0.1.0.zip` and `LostObjects-0.1.0-pdb.zip`.
5. Upload `LostObjects-0.1.0.zip` to Nexus Mods as the main file
   (not the `-pdb` file).

## Nexus Mods page

- Description: copy `docs/NEXUS_PAGE.bbcode`.
- Main image: upload `Logo/Logo.png` (1000 x 1000) as the mod's primary image.
- Requirements: *Fallout 4 Script Extender (F4SE)* and *Address Library for F4SE Plugins*.
- Tags: Settlement, Utilities, F4SE.
- Permissions (matching `NOTICE.md`):
  - Upload to other sites: yes, with credit
  - Modification: yes
  - Conversion to other games: yes
  - Asset use: yes, with credit
  - Commercial use: **no** (the code is noncommercial; what players build is theirs)
- Mark the AI use in the description (already in the BBCode).
- File credits: "Vibecoded by AI (Claude, Anthropic). Project owner and tester: egnutv."

## Checklist before uploading

- [ ] `tools/build_plugin.py --check` and `tools/verify_plugin.py` pass (CI does this)
- [ ] Tested in game with [TESTING.md](TESTING.md)
- [ ] `CHANGELOG.md` updated
- [ ] AI notice text still correct (`src/Notice.cpp`)
