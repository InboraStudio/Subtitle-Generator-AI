cmake_minimum_required(VERSION 3.25)
project(SubtitleGeneratorAI VERSION 2.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

# GPU acceleration backend for whisper.cpp/ggml. Vulkan is the recommended
# choice on AMD and Intel: it needs no vendor SDK, and on RDNA/Arc hardware ggml
# picks up cooperative-matrix (tensor core) paths through it.
set(GPU_BACKEND "none" CACHE STRING
    "GPU backend for whisper.cpp: none, vulkan, cuda or hip")
set_property(CACHE GPU_BACKEND PROPERTY STRINGS none vulkan cuda hip)

# Superseded by GPU_BACKEND. Kept for backwards-compatibility.
option(WITH_CUDA "Deprecated: use -DGPU_BACKEND=cuda" OFF)

option(USE_BUNDLED_WHISPER "Use bundled whisper.cpp sources" ON)

string(TOLOWER "${GPU_BACKEND}" _gpu_backend)

if(WITH_CUDA)
    if(_gpu_backend STREQUAL "none")
        message(WARNING "WITH_CUDA is deprecated; prefer -DGPU_BACKEND=cuda")
        set(_gpu_backend "cuda")
    elseif(NOT _gpu_backend STREQUAL "cuda")
        message(FATAL_ERROR
            "WITH_CUDA=ON conflicts with GPU_BACKEND=${GPU_BACKEND}. "
            "Set only GPU_BACKEND.")
    endif()
endif()

# Turn everything off first to reset cached variables when switching backends
set(GGML_VULKAN OFF CACHE BOOL "" FORCE)
set(GGML_CUDA OFF CACHE BOOL "" FORCE)
set(GGML_HIP OFF CACHE BOOL "" FORCE)

# These have to be set before whisper.cpp is added, so ggml sees them.
if(_gpu_backend STREQUAL "vulkan")
    set(GGML_VULKAN ON CACHE BOOL "" FORCE)
elseif(_gpu_backend STREQUAL "cuda")
    set(GGML_CUDA ON CACHE BOOL "" FORCE)
elseif(_gpu_backend STREQUAL "hip")
    set(GGML_HIP ON CACHE BOOL "" FORCE)
elseif(NOT _gpu_backend STREQUAL "none")
    message(FATAL_ERROR
        "Unknown GPU_BACKEND '${GPU_BACKEND}' (expected none, vulkan, cuda or hip)")
endif()

if(_gpu_backend STREQUAL "none")
    message(STATUS "GPU backend: none (CPU inference only)")
else()
    message(STATUS "GPU backend: ${_gpu_backend}")
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Concurrent Network)

set(SOURCES
    src/main.cpp
    src/core/AudioExtractor.cpp
    src/core/FfmpegLocator.cpp
    src/core/TranscriptionEngine.cpp
    src/core/TranslationEngine.cpp
    src/core/SubtitleWriter.cpp
    src/core/VideoEmbedder.cpp
    src/pipeline/JobQueue.cpp
    src/pipeline/BatchProcessor.cpp
    src/pipeline/ProcessingWorker.cpp
    src/pipeline/FileScanner.cpp
    src/monitor/SystemMonitor.cpp
    src/monitor/MonitorWorker.cpp
    src/logging/Logger.cpp
    src/logging/LogModel.cpp
    src/ui/MainWindow.cpp
    src/ui/SidebarWidget.cpp
    src/ui/panels/InputPanel.cpp
    src/ui/panels/SettingsPanel.cpp
    src/ui/panels/ProgressPanel.cpp
    src/ui/widgets/CircularProgressWidget.cpp
    src/ui/widgets/SystemStatsWidget.cpp
    src/ui/widgets/LogPanel.cpp
    src/ui/widgets/JobListWidget.cpp
    src/ui/widgets/ToggleSwitch.cpp
    src/ui/widgets/ModelDownloadDialog.cpp
)

set(HEADERS
    src/core/AudioExtractor.h
    src/core/FfmpegLocator.h
    src/core/TranscriptionEngine.h
    src/core/TranslationEngine.h
    src/core/SubtitleWriter.h
    src/core/VideoEmbedder.h
    src/pipeline/Job.h
    src/pipeline/JobQueue.h
    src/pipeline/BatchProcessor.h
    src/pipeline/ProcessingWorker.h
    src/pipeline/FileScanner.h
    src/monitor/SystemMonitor.h
    src/monitor/MonitorWorker.h
    src/logging/Logger.h
    src/logging/LogModel.h
    src/ui/MainWindow.h
    src/ui/SidebarWidget.h
    src/ui/panels/InputPanel.h
    src/ui/panels/SettingsPanel.h
    src/ui/panels/ProgressPanel.h
    src/ui/widgets/CircularProgressWidget.h
    src/ui/widgets/SystemStatsWidget.h
    src/ui/widgets/LogPanel.h
    src/ui/widgets/JobListWidget.h
    src/ui/widgets/ToggleSwitch.h
    src/ui/widgets/ModelDownloadDialog.h
)

if(USE_BUNDLED_WHISPER)
    set(WHISPER_BUILD_TESTS OFF CACHE BOOL "Disable whisper tests" FORCE)
    set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "Disable whisper examples" FORCE)
    set(WHISPER_BUILD_SERVER OFF CACHE BOOL "Disable whisper server" FORCE)
    
    add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/whisper.cpp)
    set(WHISPER_AVAILABLE TRUE)
else()
    find_library(WHISPER_LIB whisper)
    if(WHISPER_LIB)
        set(WHISPER_AVAILABLE TRUE)
    else()
        set(WHISPER_AVAILABLE FALSE)
        message(WARNING "whisper.cpp not found - transcription will be disabled")
    endif()
endif()

qt_add_resources(RESOURCES resources/app.qrc)

if(WIN32)
    set(APP_SOURCES ${SOURCES} ${HEADERS} ${RESOURCES} resources/app.rc)
    add_executable(SubtitleGeneratorAI WIN32 ${APP_SOURCES})
else()
    add_executable(SubtitleGeneratorAI ${SOURCES} ${HEADERS} ${RESOURCES})
endif()

target_include_directories(SubtitleGeneratorAI PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party/whisper.cpp/include
    ${CMAKE_SOURCE_DIR}/third_party/whisper.cpp/ggml/include
)

target_link_libraries(SubtitleGeneratorAI PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Concurrent
    Qt6::Network
)

if(WHISPER_AVAILABLE)
    if(USE_BUNDLED_WHISPER)
        target_link_libraries(SubtitleGeneratorAI PRIVATE whisper)
    else()
        target_link_libraries(SubtitleGeneratorAI PRIVATE ${WHISPER_LIB})
    endif()
    target_compile_definitions(SubtitleGeneratorAI PRIVATE WHISPER_AVAILABLE)
endif()

if(WIN32)
    target_link_libraries(SubtitleGeneratorAI PRIVATE
        pdh
        psapi
        winmm
    )
endif()

if(MSVC)
    target_compile_options(SubtitleGeneratorAI PRIVATE /W3 /O2)
else()
    target_compile_options(SubtitleGeneratorAI PRIVATE -Wall -O2)
endif()

set_target_properties(SubtitleGeneratorAI PROPERTIES
    OUTPUT_NAME "SubtitleGeneratorAI"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

install(TARGETS SubtitleGeneratorAI RUNTIME DESTINATION .)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/bin/" DESTINATION bin)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/models/" DESTINATION models)
