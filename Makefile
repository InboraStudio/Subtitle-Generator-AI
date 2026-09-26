# macOS build helper for SubtitleGeneratorAI.
#
#     make deps      # brew install cmake ninja qt ffmpeg
#     make           # fetch whisper.cpp, configure and build
#     make run
#
# Builds natively for the machine's CPU even when invoked from a Rosetta
# terminal, so Apple Silicon gets an arm64 binary with Metal acceleration.
# Linux and Windows builds use CMake directly (see README.md).

ifneq ($(shell uname -s),Darwin)
$(error This Makefile is for macOS only; see README.md for Linux/Windows)
endif

# Apple Silicon reports hw.optional.arm64=1 even under Rosetta, unlike uname -m.
ifeq ($(shell sysctl -n hw.optional.arm64 2>/dev/null),1)
ARCH ?= arm64
else
ARCH ?= x86_64
endif

ifeq ($(ARCH),arm64)
BREW ?= /opt/homebrew/bin/brew
else
BREW ?= /usr/local/bin/brew
endif

BUILD_DIR  ?= build
BUILD_TYPE ?= Release
JOBS       ?= $(shell sysctl -n hw.ncpu)

WHISPER_DIR    := third_party/whisper.cpp
WHISPER_REPO   := https://github.com/ggml-org/whisper.cpp.git
# Must match the gitlink recorded in the repo (git ls-tree HEAD third_party/)
WHISPER_COMMIT := 642b5d3260e020c2fc6f34a9569d10ddd7672963

BREW_PREFIX := $(shell $(BREW) --prefix 2>/dev/null)
QT_PREFIX   := $(shell $(BREW) --prefix qt 2>/dev/null)

# Run every tool under the target arch with Homebrew first on PATH, so a
# Rosetta shell or an active conda env (which ships Qt 5) can't leak in.
ENV   := env PATH="$(BREW_PREFIX)/bin:/usr/bin:/bin:/usr/sbin:/sbin"
RUN   := arch -$(ARCH) $(ENV)
CMAKE := $(RUN) $(BREW_PREFIX)/bin/cmake

APP := $(BUILD_DIR)/bin/SubtitleGeneratorAI

.PHONY: all help deps whisper configure build run doctor clean distclean

all: build

help:
	@echo "Targets:"
	@echo "  deps       Install cmake, ninja, qt and ffmpeg via Homebrew ($(BREW))"
	@echo "  whisper    Fetch whisper.cpp @ $(WHISPER_COMMIT) into $(WHISPER_DIR)"
	@echo "  configure  Run CMake ($(BUILD_TYPE), $(ARCH)) into $(BUILD_DIR)/"
	@echo "  build      Build the app (default target)"
	@echo "  run        Build and launch the app"
	@echo "  doctor     Show the detected toolchain"
	@echo "  clean      Remove $(BUILD_DIR)/"
	@echo "  distclean  Also remove the fetched whisper.cpp sources"
	@echo "Variables: ARCH=$(ARCH) BUILD_TYPE=$(BUILD_TYPE) BUILD_DIR=$(BUILD_DIR) JOBS=$(JOBS)"

deps:
	@test -x "$(BREW)" || { echo "Homebrew not found at $(BREW)." \
	  "Install it from https://brew.sh (natively, not under Rosetta)."; exit 1; }
	arch -$(ARCH) $(BREW) install cmake ninja qt ffmpeg

whisper: $(WHISPER_DIR)/CMakeLists.txt

# The repo records whisper.cpp as a gitlink without a .gitmodules entry, so
# `git submodule update` can't fetch it; pull the pinned commit directly.
$(WHISPER_DIR)/CMakeLists.txt:
	rm -rf $(WHISPER_DIR)
	git init -q $(WHISPER_DIR)
	git -C $(WHISPER_DIR) remote add origin $(WHISPER_REPO)
	git -C $(WHISPER_DIR) fetch --depth 1 origin $(WHISPER_COMMIT)
	git -C $(WHISPER_DIR) checkout -q FETCH_HEAD

configure: $(BUILD_DIR)/build.ninja

$(BUILD_DIR)/build.ninja: CMakeLists.txt $(WHISPER_DIR)/CMakeLists.txt
	@test -n "$(QT_PREFIX)" -a -d "$(QT_PREFIX)" || { echo "Qt 6 not found; run 'make deps'."; exit 1; }
	$(CMAKE) -S . -B $(BUILD_DIR) -G Ninja \
	  -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
	  -DCMAKE_OSX_ARCHITECTURES=$(ARCH) \
	  -DCMAKE_PREFIX_PATH="$(QT_PREFIX)" \
	  -DCMAKE_IGNORE_PREFIX_PATH=/opt/anaconda3
	@touch $@

build: configure
	$(CMAKE) --build $(BUILD_DIR) -j $(JOBS)
	@echo "Built $(APP) ($$(lipo -archs $(APP)))"

run: build
	$(RUN) ./$(APP)

doctor:
	@echo "ARCH        $(ARCH) (shell translated by Rosetta: $$(sysctl -n sysctl.proc_translated 2>/dev/null || echo 0))"
	@echo "brew        $(BREW) -> $(or $(BREW_PREFIX),MISSING)"
	@echo "cmake       $$(test -x $(BREW_PREFIX)/bin/cmake && $(BREW_PREFIX)/bin/cmake --version | head -1 || echo MISSING)"
	@echo "ninja       $$(test -x $(BREW_PREFIX)/bin/ninja && $(BREW_PREFIX)/bin/ninja --version || echo MISSING)"
	@echo "qt          $$(test -d '$(QT_PREFIX)' && echo $(QT_PREFIX) || echo MISSING)"
	@echo "ffmpeg      $$(test -x $(BREW_PREFIX)/bin/ffmpeg && echo $(BREW_PREFIX)/bin/ffmpeg || echo MISSING)"
	@echo "whisper.cpp $$(test -f $(WHISPER_DIR)/CMakeLists.txt && git -C $(WHISPER_DIR) rev-parse --short HEAD || echo 'not fetched')"

clean:
	rm -rf $(BUILD_DIR)

distclean: clean
	rm -rf $(WHISPER_DIR)
	mkdir -p $(WHISPER_DIR)
