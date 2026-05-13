cmake_minimum_required(VERSION 3.25)
project(SubtitleGeneratorAI VERSION 2.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

option(WITH_CUDA "Enable CUDA acceleration for whisper.cpp" OFF)
option(USE_BUNDLED_WHISPER "Use bundled whisper.cpp sources" ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Concurrent Network)

set(SOURCES
    src/main.cpp
    src/core/AudioExtractor.cpp
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
