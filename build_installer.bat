@echo off
setlocal

rem Builds the complete Windows release (installer + portable zip) into dist\.
rem Needs MSYS2 with the UCRT64 toolchain + Qt 6, and Inno Setup 6:
rem   pacman -S mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-cmake ^
rem             mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-qt6-base ^
rem             mingw-w64-ucrt-x86_64-qt6-tools git
rem   https://jrsoftware.org/isdl.php

set "MSYS_ROOT=C:\msys64"
if not exist "%MSYS_ROOT%\usr\bin\bash.exe" (
    echo  [ERROR] MSYS2 not found at %MSYS_ROOT%. Install it from https://www.msys2.org
    pause
    exit /b 1
)

set MSYSTEM=UCRT64
rem A login shell starts in the MSYS2 home folder, so cd to this repo first.
"%MSYS_ROOT%\usr\bin\bash.exe" -lc "cd \"$(cygpath -u '%~dp0')\" && bash scripts/package-windows.sh"
if errorlevel 1 (
    echo.
    echo  [ERROR] Packaging failed - see the output above.
    pause
    exit /b 1
)

echo.
echo  Installer and portable zip are in the dist\ folder.
pause
