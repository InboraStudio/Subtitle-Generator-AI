@echo off
setlocal EnableDelayedExpansion

set "ISCC="
for %%P in (
    "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
    "C:\Program Files\Inno Setup 6\ISCC.exe"
    "C:\Program Files (x86)\Inno Setup 5\ISCC.exe"
    "C:\Program Files\Inno Setup 5\ISCC.exe"
) do (
    if exist %%P (
        set "ISCC=%%~P"
        goto :found
    )
)

echo.
echo  [ERROR] Inno Setup not found. Download it from:
echo          https://jrsoftware.org/isdl.php
echo          Then re-run this script.
echo.
pause
exit /b 1

:found
echo  [OK] Inno Setup: !ISCC!

if not exist "build\bin\SubtitleGeneratorAI.exe" (
    echo  [ERROR] build\bin\SubtitleGeneratorAI.exe not found.
    echo  Please build the project first.
    pause
    exit /b 1
)

echo  [INFO] Generating app icon ...
powershell -NoProfile -ExecutionPolicy Bypass -File "make_icon.ps1"

echo.
echo  [INFO] Compiling installer ...
"!ISCC!" "installer.iss"

if %ERRORLEVEL% == 0 (
    echo.
    echo  ====================================================
    echo   SUCCESS! Installer ready:
    echo   installer_output\SubtitleGeneratorAI_Setup.exe
    echo  ====================================================
) else (
    echo.
    echo  [ERROR] Installer compilation failed.
)

echo.
pause
