#!/usr/bin/env bash
# Builds a self-contained Windows release and the one-click installer.
# Run from an MSYS2 UCRT64 shell at the repo root (CI uses this same script):
#
#     scripts/package-windows.sh            # version from CMakeLists
#     VERSION=2.1.0 scripts/package-windows.sh
#
# Output:
#     dist/SubtitleGeneratorAI-Setup-<version>.exe       (installer)
#     dist/SubtitleGeneratorAI-Windows-<version>-portable.zip
#
# The result needs nothing pre-installed on the user's PC: Qt, the compiler
# runtime, whisper.cpp (static) and FFmpeg are all bundled.
set -euo pipefail
cd "$(dirname "$0")/.."

VERSION="${VERSION:-$(sed -n 's/^project(SubtitleGeneratorAI VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)}"
BUILD_DIR="${BUILD_DIR:-build-release}"
BIN="$BUILD_DIR/bin"
CACHE="${CACHE_DIR:-.cache}"
echo "==> Subtitle Generator AI $VERSION"

step() { echo; echo "==> $*"; }

# ---------------------------------------------------------------- whisper.cpp
# third_party/whisper.cpp is a gitlink without a .gitmodules entry, so fetch
# the exact pinned commit when the folder is empty.
if [[ ! -f third_party/whisper.cpp/CMakeLists.txt ]]; then
  # MSYS2 often has no git of its own; Git for Windows works just as well.
  GIT="$(command -v git || true)"
  [[ -n "$GIT" ]] || GIT="/c/Program Files/Git/cmd/git.exe"
  COMMIT="$("$GIT" ls-tree HEAD third_party/whisper.cpp | awk '{print $3}')"
  [[ -n "$COMMIT" ]] || { echo "ERROR: cannot read the pinned whisper.cpp commit" >&2; exit 1; }
  step "Fetching whisper.cpp @ $COMMIT"
  mkdir -p "$CACHE" third_party/whisper.cpp
  TARBALL="$CACHE/whisper-$COMMIT.tar.gz"
  [[ -s "$TARBALL" ]] || curl -fL --retry 3 -o "$TARBALL" \
    "https://github.com/ggml-org/whisper.cpp/archive/$COMMIT.tar.gz"
  tar -xzf "$TARBALL" -C third_party/whisper.cpp --strip-components=1
fi

# ---------------------------------------------------------------------- build
step "Configuring"
# GGML_NATIVE=OFF: never tune for the build machine's CPU, or the app dies
# with "illegal instruction" on users' older processors.
cmake -S . -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DUSE_BUNDLED_WHISPER=ON \
  -DGGML_NATIVE=OFF \
  -DBUILD_SHARED_LIBS=OFF \
  -DAPP_VERSION="$VERSION"

step "Building"
cmake --build "$BUILD_DIR" -j

# --------------------------------------------------------------------- deploy
step "Bundling Qt and runtime DLLs"
bash ./deploy.sh "$BIN/SubtitleGeneratorAI.exe"

# --------------------------------------------------------------------- ffmpeg
# The app looks for ffmpeg/ffprobe in <app>/bin first (FfmpegLocator).
step "Bundling FFmpeg"
mkdir -p "$CACHE" "$BIN/bin"
FFZIP="${FFMPEG_ZIP:-}"
if [[ -z "$FFZIP" ]]; then
  FFZIP="$CACHE/ffmpeg-win64-shared.zip"
  if [[ ! -s "$FFZIP" ]]; then
    # Newest stable-branch GPL build (names rotate with each release). The
    # "shared" flavour keeps the codecs in DLLs used by both ffmpeg and
    # ffprobe, about half the size of two static executables.
    AUTH=()
    [[ -n "${GH_TOKEN:-}" ]] && AUTH=(-H "Authorization: Bearer $GH_TOKEN")
    URL="$(curl -fsSL "${AUTH[@]}" https://api.github.com/repos/BtbN/FFmpeg-Builds/releases/latest \
            | grep -oE 'https://[^"]*ffmpeg-n[0-9.]+-latest-win64-gpl-shared-[0-9.]+\.zip' \
            | sort -V | tail -1 || true)"
    [[ -n "$URL" ]] || URL="https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-essentials.zip"
    echo "    $URL"
    curl -fL --retry 3 -o "$FFZIP.part" "$URL"
    mv "$FFZIP.part" "$FFZIP"
  fi
fi
FFTMP="$CACHE/ffmpeg-extract"
rm -rf "$FFTMP" && mkdir -p "$FFTMP"
# MSYS2 has no unzip by default; PowerShell's archiver is always there.
powershell.exe -NoProfile -Command \
  "Expand-Archive -LiteralPath '$(cygpath -w "$FFZIP")' -DestinationPath '$(cygpath -w "$FFTMP")' -Force"
rm -rf "$BIN/bin" && mkdir -p "$BIN/bin"
for tool in ffmpeg ffprobe; do
  src="$(find "$FFTMP" -type f -iname "$tool.exe" | head -1)"
  [[ -n "$src" ]] || { echo "ERROR: $tool.exe not in $FFZIP" >&2; exit 1; }
  cp "$src" "$BIN/bin/"
done
# Codec DLLs of a shared build (a static build simply has none).
find "$(dirname "$src")" -maxdepth 1 -iname '*.dll' -exec cp {} "$BIN/bin/" \;
lic="$(find "$FFTMP" -maxdepth 3 -type f -iname 'LICENSE*' | head -1)"
[[ -n "$lic" ]] && cp "$lic" "$BIN/bin/FFMPEG-LICENSE.txt"
rm -rf "$FFTMP"

# ------------------------------------------------------------- self-check
step "Checking the bundle runs without MSYS2"
# ldd resolves through this shell's PATH, so a DLL counts as missing only if
# it lives in MSYS2 and no copy with that name ships next to the exe.
leaks="$(find "$BIN" -iname '*.exe' -o -iname '*.dll' | while read -r f; do
           ldd "$f" 2>/dev/null | grep -iE '/(ucrt64|mingw64|clang64)/bin/' \
             | awk '{print $1}' || true
         done | sort -u | while read -r name; do
           [[ -f "$BIN/$name" ]] || echo "$name"
         done)"
if [[ -n "$leaks" ]]; then
  echo "ERROR: these DLLs would be missing on a clean PC:" >&2
  echo "$leaks" >&2
  exit 1
fi
echo "    OK - every dependency is inside $BIN"

# ------------------------------------------------------------- packages
mkdir -p dist
step "Portable zip"
ZIP="dist/SubtitleGeneratorAI-Windows-$VERSION-portable.zip"
rm -f "$ZIP"
powershell.exe -NoProfile -Command \
  "Compress-Archive -Path '$(cygpath -w "$BIN")\\*' -DestinationPath '$(cygpath -w "$PWD/$ZIP")' -Force"

step "Installer"
ISCC="${ISCC:-}"
for c in "/c/Program Files (x86)/Inno Setup 6/ISCC.exe" "/c/Program Files/Inno Setup 6/ISCC.exe"; do
  [[ -z "$ISCC" && -x "$c" ]] && ISCC="$c"
done
if [[ -z "$ISCC" ]]; then
  echo "Inno Setup 6 not found - skipping installer (https://jrsoftware.org/isdl.php)"
  exit 0
fi
"$ISCC" //Q "//DMyAppVersion=$VERSION" "//DMyAppSourceDir=$BUILD_DIR\\bin" installer.iss
mv -f "installer_output/SubtitleGeneratorAI_Setup.exe" "dist/SubtitleGeneratorAI-Setup-$VERSION.exe"

echo
echo "Done:"
ls -lh dist/
