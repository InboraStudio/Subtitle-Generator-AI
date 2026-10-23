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

# The OpenSSL TLS backend loads libssl/libcrypto at runtime (invisible to ldd),
# so on a clean PC it would fail. Windows' own Schannel backend is bundled
# too and needs nothing extra, so ship only that one.
rm -f "$BIN_DIR"/tls/qopensslbackend*.dll

# 2) Copy every remaining dependency that still resolves into the MSYS2 tree
#    (compiler runtime + Qt third-party libs) so nothing is pulled from PATH.
#    Plugins in subfolders count too (imageformats, tls, styles...), and new
#    DLLs can pull in more, so repeat until a pass copies nothing.
msys_deps() {
  ldd "$1" 2>/dev/null | grep -iE "/(ucrt64|mingw64|clang64)/bin/" \
    | awk '{print $3}' | sort -u
}
while :; do
  copied=0
  while read -r f; do
    while read -r dep; do
      [[ -n "$dep" ]] || continue
      name="$(basename "$dep")"
      if [[ ! -f "$BIN_DIR/$name" ]]; then
        cp "$dep" "$BIN_DIR/"
        echo "  + $name"
        copied=1
      fi
    done < <(msys_deps "$f")
  done < <(find "$BIN_DIR" -iname '*.exe' -o -iname '*.dll')
  [[ $copied -eq 0 ]] && break
done

echo "Done. $BIN_DIR is now self-contained."
