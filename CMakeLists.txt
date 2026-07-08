cmake_minimum_required(VERSION 3.20)

# Multi-Platform Compiler & Toolchain Setup:
# 1. Windows: MSVC (msvc2022_64 / cl.exe) for best Windows system compatibility and UCRT/MSVCRT alignment.
# 2. macOS: Apple Clang (MACOSX_BUNDLE / macdeployqt) for native Apple Silicon & Intel Universal binary support.
# 3. Linux: GNU GCC / LLVM Clang.

project(ScreenCut VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Add Qt search path automatically if not specified
if(WIN32 AND NOT DEFINED CMAKE_PREFIX_PATH AND NOT DEFINED Qt6_DIR)
    list(APPEND CMAKE_PREFIX_PATH "C:/Qt/6.8.3/msvc2022_64" "C:/Qt/6.8.0/msvc2022_64")
elseif(APPLE AND NOT DEFINED CMAKE_PREFIX_PATH AND NOT DEFINED Qt6_DIR)
    list(APPEND CMAKE_PREFIX_PATH "$ENV{HOME}/Qt/6.8.1/macos" "$ENV{HOME}/Qt/6.8.0/macos" "$ENV{HOME}/Qt/6.7.2/macos" "/opt/homebrew/opt/qt" "/usr/local/opt/qt")
endif()

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Multimedia Network Svg)

# Shared headers and sources
set(SHARED_SOURCES
    src/version.h
    src/config.h
    src/config.cpp
    src/core/common_project.h
    src/widgets/common_pin.h
    src/widgets/common_pin.cpp
    src/widgets/common_switch.h
    src/widgets/common_switch.cpp
    src/resources/IconUtils.h
    src/resources/IconUtils.cpp
)

# Capture application sources
set(CAPTURE_SOURCES
    src/main.cpp
    src/core/capture_engine.h
    src/core/capture_engine.cpp
    src/widgets/capture_toolbar.h
    src/widgets/capture_toolbar.cpp
    src/capture/capture_window.h
    src/capture/capture_window.cpp
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
    add_executable(${PROJECT_NAME} ${CAPTURE_SOURCES})
    add_executable(${PROJECT_NAME}Editor ${EDITOR_SOURCES})
elseif(APPLE)
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${CAPTURE_SOURCES})
    add_executable(${PROJECT_NAME}Editor MACOSX_BUNDLE ${EDITOR_SOURCES})
    set_target_properties(${PROJECT_NAME} ${PROJECT_NAME}Editor PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.screencut.app"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    )
else()
    add_executable(${PROJECT_NAME} ${CAPTURE_SOURCES})
    add_executable(${PROJECT_NAME}Editor ${EDITOR_SOURCES})
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_include_directories(${PROJECT_NAME}Editor PRIVATE src)

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
target_link_libraries(${PROJECT_NAME}Editor PRIVATE ${QT_LIBRARIES})

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE dwmapi user32)
    target_link_libraries(${PROJECT_NAME}Editor PRIVATE dwmapi user32)
    if(MINGW)
        target_link_options(${PROJECT_NAME} PRIVATE -Wl,--allow-multiple-definition)
        target_link_options(${PROJECT_NAME}Editor PRIVATE -Wl,--allow-multiple-definition)
    endif()
    
    # Automatically deploy Qt DLLs and platform plugins after build (Windows MSVC)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "C:/Qt/6.8.1/msvc2022_64/bin" "C:/Qt/6.8.0/msvc2022_64/bin" "C:/Qt/6.7.2/msvc2022_64/bin" "${Qt6_DIR}/../../../bin")
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --no-opengl-sw --compiler-runtime "$<TARGET_FILE:${PROJECT_NAME}>"
            COMMENT "Deploying Qt runtime DLLs and platform plugins for Capture app (Windows)..."
        )
        add_custom_command(TARGET ${PROJECT_NAME}Editor POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --no-opengl-sw --compiler-runtime "$<TARGET_FILE:${PROJECT_NAME}Editor>"
            COMMENT "Deploying Qt runtime DLLs and platform plugins for Editor app (Windows)..."
        )
    endif()
elseif(APPLE)
    # Automatically deploy Qt Frameworks and plugins for macOS application bundles (Apple Clang)
    find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "$ENV{HOME}/Qt/6.8.1/macos/bin" "$ENV{HOME}/Qt/6.8.0/macos/bin" "$ENV{HOME}/Qt/6.7.2/macos/bin" "/opt/homebrew/opt/qt/bin" "/usr/local/opt/qt/bin" "${Qt6_DIR}/../../../bin")
    if(MACDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${MACDEPLOYQT_EXECUTABLE}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../.." -always-overwrite
            COMMENT "Deploying Qt Frameworks and Bundles for Capture app (Apple Clang / macOS)..."
        )
        add_custom_command(TARGET ${PROJECT_NAME}Editor POST_BUILD
            COMMAND "${MACDEPLOYQT_EXECUTABLE}" "$<TARGET_FILE_DIR:${PROJECT_NAME}Editor>/../.." -always-overwrite
            COMMENT "Deploying Qt Frameworks and Bundles for Editor app (Apple Clang / macOS)..."
        )
    endif()
endif()

# Compiler warnings & optimization flags
if(MSVC)
    # /utf-8: Ensure source files and string literals (emojis/Chinese text) are treated as UTF-8
    # /Zc:__cplusplus: Report correct C++17 standard macro
    # /permissive-: Enforce standard C++ conformance
    # Note: /O2 is only applied in non-Debug builds to avoid error D8016 conflict with /RTC1 in Debug
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 "$<$<NOT:$<CONFIG:Debug>>:/O2>" /utf-8 /Zc:__cplusplus /permissive-)
    target_compile_options(${PROJECT_NAME}Editor PRIVATE /W4 "$<$<NOT:$<CONFIG:Debug>>:/O2>" /utf-8 /Zc:__cplusplus /permissive-)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN)
    target_compile_definitions(${PROJECT_NAME}Editor PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN)
elseif(APPLE)
    # Apple Clang specific optimizations and symbol visibility
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra "$<$<NOT:$<CONFIG:Debug>>:-O3>" -fvisibility=hidden)
    target_compile_options(${PROJECT_NAME}Editor PRIVATE -Wall -Wextra "$<$<NOT:$<CONFIG:Debug>>:-O3>" -fvisibility=hidden)
else()
    # GNU GCC / LLVM Clang on Linux
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra "$<$<NOT:$<CONFIG:Debug>>:-O3>")
    target_compile_options(${PROJECT_NAME}Editor PRIVATE -Wall -Wextra "$<$<NOT:$<CONFIG:Debug>>:-O3>")
endif()


