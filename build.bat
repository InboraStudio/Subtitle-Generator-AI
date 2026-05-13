@echo off
setlocal enabledelayedexpansion

echo ============================================
echo  Subtitle Generator AI v2 - Build Script
echo ============================================
echo.

set BUILD_DIR=build
set CONFIG=Release

if "%1"=="Debug" set CONFIG=Debug
if "%1"=="clean" (
    echo Cleaning build directory...
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
    echo Done.
    exit /b 0
)

echo [1/3] Configuring CMake (%CONFIG%)...
cmake -B "%BUILD_DIR%" -G "MinGW Makefiles" ^
    -DCMAKE_BUILD_TYPE=%CONFIG% ^
    -DUSE_BUNDLED_WHISPER=OFF ^
    -DWITH_CUDA=OFF
if errorlevel 1 (
    echo.
    echo ERROR: CMake configuration failed.
    echo Make sure Qt6 and CMake are in your PATH.
    echo Set Qt6_DIR or CMAKE_PREFIX_PATH if needed.
    echo Example: set CMAKE_PREFIX_PATH=C:\Qt\6.8.0\mingw_64
    exit /b 1
)

echo.
echo [2/3] Building...
cmake --build "%BUILD_DIR%" --config %CONFIG% --parallel
if errorlevel 1 (
    echo.
    echo ERROR: Build failed. Check compiler output above.
    exit /b 1
)

echo.
echo [3/3] Deploying Qt DLLs...
set EXE_PATH=%BUILD_DIR%\bin\SubtitleGeneratorAI.exe
if exist "%EXE_PATH%" (
    windeployqt --no-quick-import --no-translations "%EXE_PATH%" 2>nul
    if errorlevel 1 (
        echo WARNING: windeployqt failed - run manually before distribution
    )
) else (
    echo WARNING: Executable not found at expected path
)

echo.
echo ============================================
echo  Build complete!
echo  Output: %BUILD_DIR%\bin\SubtitleGeneratorAI.exe
echo.
echo  NEXT STEPS:
echo   1. Place ffmpeg.exe and ffprobe.exe in bin\
echo   2. Place whisper model files (*.bin) in models\
echo   3. Run SubtitleGeneratorAI.exe
echo ============================================
