#pragma once
#include <QString>

// Locates the ffmpeg / ffprobe executables across a wide range of install
// layouts so the app works out-of-the-box for source builds (MSYS2/UCRT64),
// package-manager installs (winget, Chocolatey, Scoop) and bundled releases.
//
// Search order (first match wins):
//   1. A binary bundled next to the app: <appdir>/bin/ffmpeg[.exe], <appdir>/ffmpeg[.exe]
//   2. The system PATH (QStandardPaths::findExecutable)
//   3. Well-known install locations for the current platform
//
// Results are cached after the first successful lookup. Returns an empty string
// when nothing is found, so callers can present an actionable error.
namespace FfmpegLocator {

// Absolute path to ffmpeg, or an empty string if it cannot be found.
QString ffmpegPath();

// Absolute path to ffprobe, or an empty string if it cannot be found.
QString ffprobePath();

// True when a usable ffmpeg executable is available.
bool isAvailable();

// A human-readable, actionable message explaining how to install ffmpeg on the
// current platform. Shown to the user when ffmpeg is missing.
QString installHint();

} // namespace FfmpegLocator
