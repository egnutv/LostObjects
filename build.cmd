@echo off
rem ===========================================================================
rem  Lost Objects - one-click local build
rem  Builds the DLL and packages the mod into dist\LostObjects-<version>.zip.
rem  Needs: Visual Studio 2026 with C++, CMake 4.3+, Git, Python 3.10+.
rem  Details: docs\BUILDING.md
rem  Vibecoded by AI (Claude, Anthropic). AI-generated code can contain mistakes.
rem ===========================================================================
setlocal
cd /d "%~dp0"

set "PY="
where py >nul 2>nul && set "PY=py -3"
if not defined PY where python >nul 2>nul && set "PY=python"
if not defined PY (
    echo ERROR: Python was not found.
    echo Install Python 3 from https://www.python.org/downloads/ and tick
    echo "Add python.exe to PATH" during installation. Then start build.cmd again.
    pause
    exit /b 1
)

%PY% tools\build_local.py %*
set "RC=%ERRORLEVEL%"

echo.
if "%RC%"=="0" (
    echo Build finished. The mod archive is in the "dist" folder.
) else (
    echo Build FAILED. Please send the messages above.
)
pause
exit /b %RC%
