cmake_minimum_required(VERSION 3.20)

# Multi-Platform Compiler & Toolchain Setup:
# 1. Windows: MSVC (msvc2022_64 / cl.exe) for best Windows system compatibility and UCRT/MSVCRT alignment.
# 2. macOS: Apple Clang (MACOSX_BUNDLE / macdeployqt) for native Apple Silicon & Intel Universal binary support.
# 3. Linux: GNU GCC / LLVM Clang.

file(STRINGS "src/version.h" _ver_major_line REGEX "^#define SCREENCUT_VERSION_MAJOR [0-9]+")
string(REGEX REPLACE "^#define SCREENCUT_VERSION_MAJOR ([0-9]+)" "\\1" SCREENCUT_VER_MAJOR "${_ver_major_line}")

file(STRINGS "src/version.h" _ver_minor_line REGEX "^#define SCREENCUT_VERSION_MINOR [0-9]+")
string(REGEX REPLACE "^#define SCREENCUT_VERSION_MINOR ([0-9]+)" "\\1" SCREENCUT_VER_MINOR "${_ver_minor_line}")

file(STRINGS "src/version.h" _ver_patch_line REGEX "^#define SCREENCUT_VERSION_PATCH [0-9]+")
string(REGEX REPLACE "^#define SCREENCUT_VERSION_PATCH ([0-9]+)" "\\1" SCREENCUT_VER_PATCH "${_ver_patch_line}")

set(SCREENCUT_VERSION "${SCREENCUT_VER_MAJOR}.${SCREENCUT_VER_MINOR}.${SCREENCUT_VER_PATCH}")
project(ScreenCut VERSION ${SCREENCUT_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(APPLE)
    set(CMAKE_BUILD_RPATH "@executable_path/../Frameworks")
endif()

# Add Qt search path automatically if not specified
if(WIN32 AND NOT DEFINED CMAKE_PREFIX_PATH AND NOT DEFINED Qt6_DIR)
    list(APPEND CMAKE_PREFIX_PATH "C:/Qt/6.8.3/msvc2022_64" "C:/Qt/6.8.0/msvc2022_64")
elseif(APPLE AND NOT DEFINED CMAKE_PREFIX_PATH AND NOT DEFINED Qt6_DIR)
    list(APPEND CMAKE_PREFIX_PATH "$ENV{HOME}/Qt/6.8.3/macos" "$ENV{HOME}/Qt/6.8.1/macos" "$ENV{HOME}/Qt/6.8.0/macos" "$ENV{HOME}/Qt/6.7.2/macos" "/opt/homebrew/opt/qt" "/usr/local/opt/qt")
endif()

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Multimedia Network Svg)

# Shared headers and sources
set(SHARED_SOURCES
    src/version.h
    src/config.h
    src/config.cpp
    src/core/common_project.h
    src/platform/platform.h
    src/widgets/common_switch.h
    src/widgets/common_switch.cpp
    src/widgets/common_notification.h
    src/widgets/common_notification.cpp
    src/resources/IconUtils.h
    src/resources/IconUtils.cpp
)

if(WIN32)
    list(APPEND SHARED_SOURCES src/platform/platform_win.cpp)
else()
    list(APPEND SHARED_SOURCES src/platform/platform_mac.cpp)
endif()

list(APPEND SHARED_SOURCES src/resources/resources.qrc)

# Capture application sources
set(CAPTURE_SOURCES
    src/main.cpp
    src/core/capture_engine.h
    src/core/capture_engine.cpp
    src/widgets/capture_toolbar.h
    src/widgets/capture_toolbar.cpp
    src/widgets/capture_countdown.h
    src/widgets/capture_countdown.cpp
    src/widgets/capture_pin.h
    src/widgets/capture_pin.cpp
    src/widgets/capture_scroll.h
    src/widgets/capture_scroll.cpp
    src/widgets/capture_status.h
    src/widgets/capture_status.cpp
    src/capture/capture_window.h
    src/capture/capture_window.cpp
    src/capture/capture_preferences.h
    src/capture/capture_preferences.cpp
    src/capture/capture_permissions.h
    src/capture/capture_permissions.cpp
    ${SHARED_SOURCES}
)

# Editor application sources
set(EDITOR_SOURCES
    src/editor/editor_main.cpp
    src/editor/editor_models.h
    src/editor/editor_models.cpp
    src/editor/editor_window.h
    src/editor/editor_window.cpp
    ${SHARED_SOURCES}
)

# Platform specific configurations
if(WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${CAPTURE_SOURCES})
    add_executable(SCEditor WIN32 ${EDITOR_SOURCES})
elseif(APPLE)
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${CAPTURE_SOURCES})
    add_executable(SCEditor MACOSX_BUNDLE ${EDITOR_SOURCES})
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "ScreenCut"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.screencut.app"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        # macOS 10.15+ : provide NSScreenCaptureDescription & NSMicrophoneUsageDescription
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/platform/macos/Info.plist"
    )
    set_target_properties(SCEditor PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "SCEditor"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.screencut.editor"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/platform/macos/Info.plist"
    )
    # Link Apple system frameworks required for screen capture, window enumeration, and cursor rendering
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework Cocoa"
        "-framework AppKit"
        "-framework CoreGraphics"
        "-framework CoreMedia"
        "-framework CoreVideo"
        "-framework IOKit"
        "-framework AVFoundation"
    )
    # ScreenCaptureKit is only available from macOS 12.3+; link conditionally
    if(CMAKE_OSX_DEPLOYMENT_TARGET VERSION_GREATER_EQUAL "12.3")
        target_link_libraries(${PROJECT_NAME} PRIVATE "-framework ScreenCaptureKit")
    endif()
else()
    add_executable(${PROJECT_NAME} ${CAPTURE_SOURCES})
    add_executable(SCEditor ${EDITOR_SOURCES})
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_include_directories(SCEditor PRIVATE src)

# Link Qt libraries
set(QT_LIBRARIES
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Network
    Qt6::Svg
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${QT_LIBRARIES})
target_link_libraries(SCEditor PRIVATE ${QT_LIBRARIES})

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE dwmapi user32)
    target_link_libraries(SCEditor PRIVATE dwmapi user32)
    if(MINGW)
        target_link_options(${PROJECT_NAME} PRIVATE -Wl,--allow-multiple-definition)
        target_link_options(SCEditor PRIVATE -Wl,--allow-multiple-definition)
    endif()
    
    # Automatically deploy Qt DLLs and platform plugins after build (Windows MSVC)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "C:/Qt/6.8.1/msvc2022_64/bin" "C:/Qt/6.8.0/msvc2022_64/bin" "C:/Qt/6.7.2/msvc2022_64/bin" "${Qt6_DIR}/../../../bin")
    if(WINDEPLOYQT_EXECUTABLE)
        if(FFMPEG_EXECUTABLE)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${FFMPEG_EXECUTABLE}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/ffmpeg.exe"
                COMMENT "Bundling ffmpeg.exe into target directory..."
            )
        endif()
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --no-opengl-sw --compiler-runtime "$<TARGET_FILE:${PROJECT_NAME}>"
            COMMENT "Deploying Qt runtime DLLs and platform plugins for Capture app (Windows)..."
        )
        add_custom_command(TARGET SCEditor POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --no-opengl-sw --compiler-runtime "$<TARGET_FILE:SCEditor>"
            COMMENT "Deploying Qt runtime DLLs and platform plugins for Editor app (Windows)..."
        )
    endif()
elseif(APPLE)
    # Automatically deploy Qt Frameworks and plugins for macOS application bundles (Apple Clang)
    # Hints cover: Qt Installer default, Apple Silicon Homebrew, Intel Homebrew, and fallback from Qt6_DIR
    find_program(MACDEPLOYQT_EXECUTABLE macdeployqt
        HINTS
            "$ENV{HOME}/Qt/6.8.1/macos/bin"
            "$ENV{HOME}/Qt/6.8.0/macos/bin"
            "$ENV{HOME}/Qt/6.7.2/macos/bin"
            "/opt/homebrew/opt/qt/bin"    # Apple Silicon Homebrew
            "/usr/local/opt/qt/bin"       # Intel x86 Homebrew
            "/usr/local/opt/qt@6/bin"     # Intel x86 Homebrew (versioned)
            "${Qt6_DIR}/../../../bin"
    )
    # FFmpeg: used at runtime for video recording (not linked at compile-time, spawned as subprocess)
    # Detect brew-installed ffmpeg for developers to verify PATH at configure time
    find_program(FFMPEG_EXECUTABLE ffmpeg
        HINTS
            "/opt/homebrew/bin"           # Apple Silicon Homebrew
            "/usr/local/bin"              # Intel x86 Homebrew / manual install
    )
    if(FFMPEG_EXECUTABLE)
        message(STATUS "[ScreenCut] ffmpeg found at: ${FFMPEG_EXECUTABLE} (will be used at runtime for video recording)")
    else()
        message(WARNING "[ScreenCut] ffmpeg NOT found. Video recording will not work. Install via: brew install ffmpeg")
    endif()

    # Determine signing identity from environment, default to ad-hoc ("-")
    set(MAC_SIGN_IDENTITY "-")
    if(DEFINED ENV{APPLE_TEAM_ID})
        set(MAC_SIGN_IDENTITY "$ENV{APPLE_TEAM_ID}")
        message(STATUS "[ScreenCut] Using Apple Team ID for signing: ${MAC_SIGN_IDENTITY}")
    endif()

    if(MACDEPLOYQT_EXECUTABLE)
        if(FFMPEG_EXECUTABLE)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -DAPP_DIR="${CMAKE_BINARY_DIR}/ScreenCut.app" -DMACDEPLOYQT="${MACDEPLOYQT_EXECUTABLE}" -P "${CMAKE_SOURCE_DIR}/platform/macos/deploy_mac.cmake"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${FFMPEG_EXECUTABLE}" "${CMAKE_BINARY_DIR}/ScreenCut.app/Contents/MacOS/ffmpeg"
                COMMAND codesign --force --deep --sign "${MAC_SIGN_IDENTITY}"
                        --entitlements "${CMAKE_SOURCE_DIR}/platform/macos/ScreenCut.entitlements"
                        "${CMAKE_BINARY_DIR}/ScreenCut.app"
                COMMENT "Deploying Qt Frameworks, bundling ffmpeg, and code-signing ScreenCut.app (Identity: ${MAC_SIGN_IDENTITY})..."
            )
        else()
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -DAPP_DIR="${CMAKE_BINARY_DIR}/ScreenCut.app" -DMACDEPLOYQT="${MACDEPLOYQT_EXECUTABLE}" -P "${CMAKE_SOURCE_DIR}/platform/macos/deploy_mac.cmake"
                COMMAND codesign --force --deep --sign "${MAC_SIGN_IDENTITY}"
                        --entitlements "${CMAKE_SOURCE_DIR}/platform/macos/ScreenCut.entitlements"
                        "${CMAKE_BINARY_DIR}/ScreenCut.app"
                COMMENT "Deploying Qt Frameworks and code-signing ScreenCut.app (Identity: ${MAC_SIGN_IDENTITY})..."
            )
        endif()
        add_custom_command(TARGET SCEditor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -DAPP_DIR="${CMAKE_BINARY_DIR}/SCEditor.app" -DMACDEPLOYQT="${MACDEPLOYQT_EXECUTABLE}" -P "${CMAKE_SOURCE_DIR}/platform/macos/deploy_mac.cmake"
            COMMAND codesign --force --deep --sign "${MAC_SIGN_IDENTITY}"
                    "${CMAKE_BINARY_DIR}/SCEditor.app"
            COMMENT "Deploying Qt Frameworks and code-signing SCEditor.app (Identity: ${MAC_SIGN_IDENTITY})..."
        )
        # Prevent concurrent macdeployqt runs when building targets in parallel (e.g. make -j)
        add_dependencies(SCEditor ${PROJECT_NAME})
    endif()
endif()

# Compiler warnings & optimization flags
if(MSVC)
    # /utf-8: Ensure source files and string literals (emojis/Chinese text) are treated as UTF-8
    # /Zc:__cplusplus: Report correct C++17 standard macro
    # /permissive-: Enforce standard C++ conformance
    # Note: /O2 is only applied in non-Debug builds to avoid error D8016 conflict with /RTC1 in Debug
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 "$<$<NOT:$<CONFIG:Debug>>:/O2>" /utf-8 /Zc:__cplusplus /permissive-)
    target_compile_options(SCEditor PRIVATE /W4 "$<$<NOT:$<CONFIG:Debug>>:/O2>" /utf-8 /Zc:__cplusplus /permissive-)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN)
    target_compile_definitions(SCEditor PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN)
elseif(APPLE)
    # Apple Clang specific optimizations and symbol visibility
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra "$<$<NOT:$<CONFIG:Debug>>:-O3>" -fvisibility=hidden)
    target_compile_options(SCEditor PRIVATE -Wall -Wextra "$<$<NOT:$<CONFIG:Debug>>:-O3>" -fvisibility=hidden)
else()
    # GNU GCC / LLVM Clang on Linux
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra "$<$<NOT:$<CONFIG:Debug>>:-O3>")
    target_compile_options(SCEditor PRIVATE -Wall -Wextra "$<$<NOT:$<CONFIG:Debug>>:-O3>")
endif()


