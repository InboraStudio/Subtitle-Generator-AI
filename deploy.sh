#!/usr/bin/env bash
# Makes the built app fully self-contained so it runs by double-click without
# any MSYS2/Qt directory on PATH. Run from an MSYS2 UCRT64 shell after building:
#
#     ./deploy.sh                       # defaults to build/bin/SubtitleGeneratorAI.exe
#     ./deploy.sh path/to/App.exe
#
# Why this exists: on MinGW/UCRT, windeployqt copies the Qt DLLs + plugins but
# NOT the compiler runtime (libstdc++, libgcc, libwinpthread) or Qt's own
# third-party dependencies (zstd, pcre2, icu, harfbuzz, ...). Without those the
# exe silently loads whatever Qt6Core.dll it finds on PATH, which triggers the
# "entry point qResourceFeatureZstd could not be located" crash on mismatch.
set -euo pipefail

EXE="${1:-build/bin/SubtitleGeneratorAI.exe}"
if [[ ! -f "$EXE" ]]; then
  echo "ERROR: executable not found: $EXE" >&2
  echo "Build first, or pass the exe path as an argument." >&2
  exit 1
fi
BIN_DIR="$(cd "$(dirname "$EXE")" && pwd)"
echo "Deploying into: $BIN_DIR"

# 1) Qt DLLs, platform plugins, imageformats, tls backends, etc.
WINDEPLOYQT="$(command -v windeployqt6 || command -v windeployqt)"
if [[ -z "$WINDEPLOYQT" ]]; then
  echo "ERROR: windeployqt not found on PATH. Run from an MSYS2 UCRT64 shell." >&2
  exit 1
fi
"$WINDEPLOYQT" --release --no-translations --no-system-d3d-compiler \
  --no-opengl-sw "$EXE"

# 2) Copy every remaining dependency that still resolves into the MSYS2 tree
#    (compiler runtime + Qt third-party libs) so nothing is pulled from PATH.
copy_deps() {
  ldd "$1" 2>/dev/null | grep -iE "/(ucrt64|mingw64|clang64)/bin/" \
    | awk '{print $3}' | sort -u | while read -r dep; do
      name="$(basename "$dep")"
      [[ -f "$BIN_DIR/$name" ]] || cp "$dep" "$BIN_DIR/"
    done
}
# Resolve transitively: scan the exe and each Qt DLL already placed in bin.
for f in "$EXE" "$BIN_DIR"/Qt6*.dll; do
  [[ -f "$f" ]] && copy_deps "$f"
done

remaining="$(ldd "$EXE" 2>/dev/null | grep -icE "/(ucrt64|mingw64|clang64)/bin/" || true)"
echo "External MSYS2 deps still referenced by the exe: $remaining (want 0)"
echo "Done. $BIN_DIR is now self-contained."
