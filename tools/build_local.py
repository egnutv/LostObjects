#!/usr/bin/env python3
"""
Lost Objects - one-click local build (Windows).

Started by build.cmd in the repository root. It:

  1. checks the required tools (Visual Studio 2026 with C++, CMake 4.3+, Git),
  2. sets up vcpkg (uses VCPKG_ROOT if set, otherwise a private copy in .vcpkg),
  3. configures and builds the DLL (same presets as the GitHub build),
  4. packages the mod into dist\\LostObjects-<version>.zip.

Every step prints what it does. If something is missing, it says what to
install and where to get it.

Usage:
  build.cmd               full build and package
  build.cmd --clean       delete the build folder first
  build.cmd --no-package  build the DLL only

Vibecoded by AI (Claude, Anthropic). License: PolyForm Noncommercial 1.0.0.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
BUILD_DIR = REPO_ROOT / "build" / "release"
OUTPUT_DIR = BUILD_DIR / "RelWithDebInfo"
LOCAL_VCPKG = REPO_ROOT / ".vcpkg"
MIN_CMAKE = (4, 3)


class BuildError(Exception):
    pass


def step(text: str) -> None:
    print()
    print(f"=== {text}")


def info(text: str) -> None:
    print(f"    {text}")


def run(command: list[str], env: dict[str, str] | None = None, cwd: Path = REPO_ROOT) -> None:
    info("> " + " ".join(f'"{c}"' if " " in c else c for c in command))
    result = subprocess.run(command, cwd=cwd, env=env)
    if result.returncode != 0:
        raise BuildError(f"command failed with exit code {result.returncode}")


def output(command: list[str]) -> str:
    try:
        return subprocess.run(command, capture_output=True, text=True, check=False).stdout.strip()
    except OSError:
        return ""


def read_version() -> str:
    manifest = json.loads((REPO_ROOT / "vcpkg.json").read_text(encoding="utf-8"))
    return manifest["version-semver"]


def read_vcpkg_baseline() -> str:
    manifest = json.loads((REPO_ROOT / "vcpkg.json").read_text(encoding="utf-8"))
    return manifest["builtin-baseline"]


# --- Tool checks -------------------------------------------------------------


def find_visual_studio() -> Path:
    vswhere = Path(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
    if not vswhere.is_file():
        raise BuildError(
            "Visual Studio was not found.\n"
            "    Install Visual Studio 2026 (the free Community edition is enough) from\n"
            "    https://visualstudio.microsoft.com/ and select the workload\n"
            "    \"Desktop development with C++\"."
        )

    path = output([
        str(vswhere), "-latest", "-products", "*",
        "-version", "18.0",
        "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
        "-property", "installationPath",
    ])
    if not path:
        raise BuildError(
            "Visual Studio 2026 with C++ was not found.\n"
            "    Open the Visual Studio Installer, choose \"Modify\" and select the workload\n"
            "    \"Desktop development with C++\" (Visual Studio 2026 / version 18 is required)."
        )
    return Path(path.splitlines()[0])


def cmake_version(cmake: str) -> tuple[int, int, int] | None:
    match = re.search(r"cmake version (\d+)\.(\d+)\.(\d+)", output([cmake, "--version"]))
    return tuple(int(x) for x in match.groups()) if match else None  # type: ignore[return-value]


def find_cmake(vs_path: Path) -> str:
    candidates = []
    on_path = shutil.which("cmake")
    if on_path:
        candidates.append(on_path)
    bundled = vs_path / "Common7" / "IDE" / "CommonExtensions" / "Microsoft" / "CMake" / "CMake" / "bin" / "cmake.exe"
    if bundled.is_file():
        candidates.append(str(bundled))

    found = []
    for candidate in candidates:
        version = cmake_version(candidate)
        if version:
            found.append(f"{candidate} ({'.'.join(map(str, version))})")
            if version[:2] >= MIN_CMAKE:
                return candidate

    details = ("\n    Found: " + ", ".join(found)) if found else ""
    raise BuildError(
        f"CMake {MIN_CMAKE[0]}.{MIN_CMAKE[1]} or newer was not found.{details}\n"
        "    Download the Windows installer from https://cmake.org/download/ and choose\n"
        "    \"Add CMake to the PATH\" during installation. Then start build.cmd again."
    )


def find_git() -> str:
    git = shutil.which("git")
    if not git:
        raise BuildError(
            "Git was not found.\n"
            "    Install it from https://git-scm.com/download/win (default options are fine)."
        )
    return git


def setup_vcpkg(git: str) -> Path:
    env_root = os.environ.get("VCPKG_ROOT")
    if env_root and (Path(env_root) / "scripts" / "buildsystems" / "vcpkg.cmake").is_file():
        info(f"using VCPKG_ROOT: {env_root}")
        root = Path(env_root)
    else:
        root = LOCAL_VCPKG
        if not (root / "scripts" / "buildsystems" / "vcpkg.cmake").is_file():
            info(f"downloading vcpkg into {root} (only the first time, a few hundred MB)")
            run([git, "clone", "https://github.com/microsoft/vcpkg.git", str(root)])
        else:
            info(f"using local vcpkg: {root}")
            # The manifest's baseline must exist in the local history.
            baseline = read_vcpkg_baseline()
            if subprocess.run([git, "-C", str(root), "cat-file", "-e", f"{baseline}^{{commit}}"], capture_output=True).returncode != 0:
                info("updating vcpkg (baseline not found locally)")
                run([git, "-C", str(root), "pull", "--ff-only"])

    if not (root / "vcpkg.exe").is_file():
        info("bootstrapping vcpkg")
        run([str(root / "bootstrap-vcpkg.bat"), "-disableMetrics"], cwd=root)

    return root


# --- Main --------------------------------------------------------------------


def main() -> int:
    parser = argparse.ArgumentParser(description="Local Windows build of Lost Objects.")
    parser.add_argument("--clean", action="store_true", help="delete the build folder first")
    parser.add_argument("--no-package", action="store_true", help="build the DLL only")
    args = parser.parse_args()

    if sys.platform != "win32":
        print("This builder is for Windows. On other systems use the GitHub build.")
        return 1

    version = read_version()
    print(f"Lost Objects {version} - local build")
    print("Vibecoded by AI (Claude, Anthropic). AI-generated code can contain mistakes.")

    try:
        step("1/5 Checking tools")
        vs_path = find_visual_studio()
        info(f"Visual Studio: {vs_path}")
        cmake = find_cmake(vs_path)
        info(f"CMake: {cmake}")
        git = find_git()
        info(f"Git: {git}")

        step("2/5 Setting up vcpkg")
        vcpkg_root = setup_vcpkg(git)

        env = dict(os.environ)
        env["VCPKG_ROOT"] = str(vcpkg_root)
        # Make sure CMake's FetchContent and vcpkg find the same Git.
        env["PATH"] = str(Path(git).parent) + os.pathsep + env.get("PATH", "")

        if args.clean and BUILD_DIR.exists():
            step("Cleaning the build folder")
            shutil.rmtree(BUILD_DIR)

        step("3/5 Configuring (the first run downloads and builds libraries, this takes a while)")
        run([cmake, "--preset", "release"], env=env)

        step("4/5 Building the DLL")
        run([cmake, "--build", "--preset", "release"], env=env)

        dll = OUTPUT_DIR / "LostObjects.dll"
        pdb = OUTPUT_DIR / "LostObjects.pdb"
        if not dll.is_file():
            raise BuildError(f"the build finished but {dll} is missing")
        info(f"DLL: {dll}")

        if args.no_package:
            step("Done (packaging skipped)")
            return 0

        step("5/5 Packaging")
        package_command = [sys.executable, str(REPO_ROOT / "tools" / "package.py"), "--dll", str(dll), "--out", str(REPO_ROOT / "dist")]
        if pdb.is_file():
            package_command += ["--pdb", str(pdb)]
        run(package_command)

        step("Done")
        info(f"Mod archive: {REPO_ROOT / 'dist' / f'LostObjects-{version}.zip'}")
        info("Install it with Vortex, the Nexus app or Mod Organizer 2.")
        return 0

    except BuildError as error:
        print()
        print("=== BUILD FAILED")
        info(str(error))
        return 1


if __name__ == "__main__":
    sys.exit(main())
