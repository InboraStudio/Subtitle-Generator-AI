# Subtitle Generator AI v2

A professional Windows desktop application for automated video subtitle generation. Extracts audio via FFmpeg, transcribes locally using **whisper.cpp**, optionally translates via LibreTranslate, and outputs `.srt` files or directly embeds subtitles into video.

---

## Features

- **Batch processing** — queue multiple videos or scan folders recursively
- **Local transcription** — whisper.cpp (no cloud, no API key needed)
- **Multi-language** — auto-detect language, optional translation
- **SRT output or burn-in** — soft subtitles (MKV stream) or hard-coded subtitles
- **Parallel jobs** — configurable thread count
- **System monitoring** — live CPU/RAM/GPU gauges
- **Toggleable console** — scrollable, color-coded log panel
- **Dark professional UI** — frameless Qt6 window with orange accent theme

---

## Requirements

| Component | Version |
|-----------|---------|
| Qt        | 6.5+    |
| CMake     | 3.25+   |
| Compiler  | MinGW-w64 13+ or MSVC 2022 |
| whisper.cpp | (optional, bundled or external) |
| FFmpeg    | Pre-built Windows binary |

---

## Build Instructions

### 1. Prerequisites

Install the following:

- **Qt 6.5+**: https://www.qt.io/download-qt-installer (select `Qt 6.x.x > MinGW 13.1.0 64-bit`)
- **CMake 3.25+**: https://cmake.org/download/
- **MinGW-w64**: included with Qt installer
- **FFmpeg**: https://www.gyan.dev/ffmpeg/builds/ → download `ffmpeg-release-essentials.zip`

### 2. Set Up FFmpeg

Extract and copy `ffmpeg.exe` and `ffprobe.exe` into:
```
Subtitle-Generator-AI v2\bin\ffmpeg.exe
Subtitle-Generator-AI v2\bin\ffprobe.exe
```

### 3. Get a Whisper Model

Download a pre-converted GGUF/bin model from:
- https://huggingface.co/ggerganov/whisper.cpp

Recommended starting model: `ggml-base.en.bin` (fast, English only)
Accurate model: `ggml-medium.bin` (multilingual)

Place model files in:
```
Subtitle-Generator-AI v2\models\ggml-base.en.bin
```

### 4. (Optional) whisper.cpp Source

For full transcription support, clone whisper.cpp into `third_party/`:
```powershell
mkdir third_party
cd third_party
git clone https://github.com/ggerganov/whisper.cpp
```

Then enable in CMake:
```
-DUSE_BUNDLED_WHISPER=ON
```

### 5. Build

**Quick build (MinGW):**
```batch
# Set Qt path (adjust to your install)
set CMAKE_PREFIX_PATH=C:\Qt\6.8.0\mingw_64

build.bat
```

**Manual CMake (MSVC):**
```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_PREFIX_PATH="C:\Qt\6.8.0\msvc2022_64" `
    -DUSE_BUNDLED_WHISPER=OFF

cmake --build build --config Release
```

### 6. Deploy Qt DLLs

After building, run:
```batch
windeployqt build\bin\SubtitleGeneratorAI.exe
```

This copies all required Qt DLLs next to the executable.

---

## Project Structure

```
Subtitle-Generator-AI v2/
├── CMakeLists.txt
├── build.bat                   ← Quick build script
├── src/
│   ├── main.cpp
│   ├── core/                   ← Audio/Transcription/Translation/Subtitle/VideoEmbed engines
│   ├── pipeline/               ← Job queue, batch processor, file scanner, worker
│   ├── monitor/                ← CPU/RAM/GPU system monitor
│   ├── logging/                ← Thread-safe logger + QAbstractListModel
│   └── ui/                     ← Qt6 widgets, panels, MainWindow
├── resources/
│   ├── app.qrc
│   ├── styles/dark.qss         ← Full dark theme stylesheet
│   └── icons/                  ← SVG icon set
├── bin/                        ← Place ffmpeg.exe + ffprobe.exe here
├── models/                     ← Place .bin/.gguf whisper models here
└── third_party/
    └── whisper.cpp/            ← Optional: clone whisper.cpp here
```

---

## Usage

1. **Input tab** — Add video files or folders. Supports: `.mp4 .mkv .avi .mov .webm .flv .wmv .m4v .ts .mpg`
2. **Settings tab** — Select model, output directory, language, translation, parallel job count
3. Click **Start Processing** — switches to Progress tab automatically
4. Output `.srt` files are saved to your chosen directory
5. Toggle **Console** in the title bar to view detailed processing logs

---

## Translation (Optional)

Translation uses [LibreTranslate](https://libretranslate.com/).

**Offline setup:**
```bash
pip install libretranslate
libretranslate --host 0.0.0.0 --port 5000
```

Then in Settings → API Endpoint: `http://localhost:5000`

---

## GPU Acceleration

Enable CUDA in CMake:
```batch
cmake -B build -G "MinGW Makefiles" -DWITH_CUDA=ON
```
Requires NVIDIA GPU + CUDA Toolkit 11+.

---

## Dependencies Summary

| Library | Purpose | Source |
|---------|---------|--------|
| Qt 6.5+ | UI + threading + network | Qt installer |
| whisper.cpp | Local speech-to-text | github.com/ggerganov/whisper.cpp |
| FFmpeg | Audio extraction + subtitle embedding | gyan.dev/ffmpeg/builds |
| Windows PDH | CPU metrics | Windows SDK (built-in) |
| LibreTranslate | Translation REST API | libretranslate.com |

---

## License

MIT
