# ScreenCut (Native C++ / Qt 6 Edition)

[![License: LGPL v2.1](https://img.shields.io/badge/License-LGPL_v2.1-blue.svg)](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17%2F20-blue.svg)](https://isocpp.org/)
[![Framework: Qt 6](https://img.shields.io/badge/GUI-Qt%206%20(Native)-green.svg)](https://www.qt.io/)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20macOS%20%7C%20Linux-lightgrey.svg)

> [!IMPORTANT]
> **【Next-Gen Architecture: Native C++ / Qt 6 High-Performance Engine】**
> This project is a complete native rewrite and evolution of the original `ScreenCut-Prototype` Python/PySide6 proof-of-concept.
> Built with 100% native **C++17 / Qt 6**, ScreenCut eliminates virtual machine runtime overhead to achieve **0ms instant shutter response** and an **ultra-low 15~30MB memory footprint**, delivering a cross-platform, industrial-grade screenshot, cropping, and annotation powerhouse comparable to Snagit, ShareX, and OBS!

---

## Why Pivot to Native C++ / Qt 6?

1. **Zero-Latency Shutter Response (0ms)**: Direct invocation of native OS windowing and display graphics APIs (Win32 API / DXGI / macOS Cocoa). Freezes your screen the exact millisecond you hit the hotkey!
2. **Ultra-Low Memory Footprint**: Without Python interpreter overhead or garbage collection pauses, background memory usage in the system tray stays between **15MB and 30MB**, outperforming electron and interpreted alternatives.
3. **Seamless Blueprint Inheritance**: Transitioned from PySide6 to native C++ Qt 6 while preserving the proven architecture, intuitive UI/UX workflows, annotation business logic, and significantly boosting 2D vector rendering performance.

---

## 🛠️ Key Features

* **Multi-Mode Capture & Scrolling Screenshots**:
  * **Region Selection Capture**: Freely select any screen area with custom aspect ratios and dimensions.
  * **Smart Window Detection**: Automatically detects and highlights window boundaries under your cursor for one-click precision capture.
  * **Scrolling Screenshot**: Automated page scrolling and feature stitching that seamlessly converts long webpages, documents, or chat logs into a single ultra-high-resolution image.
  * **Multi-Monitor & High-DPI Adaptation**: Flawlessly supports multi-display setups, 4K resolution, and Retina displays.

* **Professional Non-Destructive Image Editor**:
  * **Rich Annotation Suite**: Sleek arrows, rectangular/circular borders, freehand draw, highlighter pens, and text boxes.
  * **Step Sequence Markers (1, 2, 3...)**: Click anywhere on the image to drop auto-incrementing numbered markers—perfect for technical documentation and tutorial guides!
  * **Privacy Shaders**: Built-in **Mosaic Pixelation** and **Gaussian Blur** to quickly censor sensitive or confidential information.
  * **Unlimited Undo / Redo**: Full history stack (`Ctrl+Z` / `Ctrl+Y`), proportional cropping, and aspect ratio locking.

* **Instant Export & Sharing**:
  * **One-Click Clipboard Copy** (`Ctrl+C` / `Enter`): Instantly paste into Slack, Teams, GitHub, or Word documents.
  * **Multi-Format Export** (`Ctrl+S`): Support for high-resolution PNG, JPG, and WEBP formats.
  * **Dedicated Project Library**: All auto-saves, captures, and `.scut` project files are cleanly organized and stored in your OS Documents folder under `~/Documents/My ScreenCut Library/`.

---

## Project Structure

```text
ScreenCut/
├── CMakeLists.txt            # CMake build configuration (Qt6 Core/Gui/Widgets/Multimedia/Network)
├── README.md                 # Project documentation
├── src/
│   ├── main.cpp              # Capture application entry point (`ScreenCut.exe` / `ScreenCut.app`)
│   ├── version.h             # Version definitions and compilation macros (global)
│   ├── config.h/.cpp         # Global application configuration and settings manager
│   ├── capture/              # [prefix: capture_] Screen capture specific windows and logic
│   │   ├── capture_window.h/.cpp      # Main overlay window and region selection canvas
│   │   ├── capture_preferences.h/.cpp # Preferences and settings dialog
│   │   └── capture_permissions.h/.cpp # Screen recording permission checks (macOS)
│   ├── core/                 # Core engine and data structures
│   │   ├── capture_engine.h/.cpp      # 0ms screen caching, window boundary detection, and scroll stitching
│   │   ├── migration_manager.h/.cpp   # Handles legacy config and data migrations
│   │   └── common_project.h           # Dedicated library directory and .scut project JSON schema
│   ├── editor/               # [prefix: editor_] Non-destructive annotation editor (`SCEditor.exe` / bundled in macOS App)
│   │   ├── editor_main.cpp            # Editor application internal entry point
│   │   ├── editor_window.h/.cpp       # Editor main window, Undo/Redo history stack, cropping, and exporting
│   │   └── editor_models.h/.cpp       # Core annotation data models
│   ├── platform/             # OS-specific native API implementations
│   │   ├── platform.h                 # Cross-platform interface
│   │   ├── platform_win.cpp           # Windows (Win32 API) implementations
│   │   └── platform_mac.cpp           # macOS (Cocoa / Objective-C++) implementations
│   ├── resources/            # Application assets and resources
│   │   ├── IconUtils.h/.cpp           # SVG rendering utilities and icon constants
│   │   └── resources.qrc              # Qt resource bundle
│   └── widgets/              # Reusable UI widgets
│       ├── tools/                     # Editor annotation tool logic (arrow, shape, text, freehand, step, blur, select)
│       ├── capture_*                  # Capture widgets (toolbar, countdown, pin, scroll, status)
│       ├── editor_*                   # Editor widgets (canvas, props, colorpicker, shadowpicker, thumbs)
│       └── common_*                   # Shared generic widgets (switch, notification)
```

---

## Building & Running

### Prerequisites

* **CMake** 3.20 or later
* **C++ Compiler** (Multi-Platform Native):
  * **Windows**: **MSVC 2022 / 2019** (Microsoft Visual C++ `cl.exe`, UTF-8 enabled with `/utf-8` and `/permissive-`)
  * **macOS**: **Apple Clang** (Xcode / Command Line Tools, Universal Apple Silicon & Intel)
  * **Linux**: **GCC 9+** or **LLVM Clang**
* **Qt 6** (Qt 6.11.2 required):
  * Required components: `Core`, `Gui`, `Widgets`, `Multimedia`, `Network`, and `Svg`
  * Automatically deploys runtime dependencies via `windeployqt` (Windows) or `macdeployqt` (macOS App Bundle).
  * **Note**: By default, `CMakeLists.txt` looks for Qt 6.11.2 in standard paths (`C:/Qt/6.11.2/msvc2022_64` on Windows, `~/Qt/6.11.2/macos` on macOS). If your installation path differs, simply update the `# --- Custom Path Configurations ---` section at the top of `CMakeLists.txt`.
* **FFmpeg** (Runtime Requirement for Video Recording):
  * ScreenCut requires an external `ffmpeg` executable in the system `PATH` or the application directory to perform hardware-accelerated video recording (H.264/MP4).
  * **macOS**: `brew install ffmpeg`
  * **Windows**: Download `ffmpeg.exe` and add it to your `PATH`, or place it in the same directory as `ScreenCut.exe`.

### Command-Line Build (CLI)

#### Windows (MSVC 2022 / Visual Studio)

```powershell
# 1. Open "x64 Native Tools Command Prompt for VS 2022" or normal Terminal
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64

# 2. Compile project (Release build with windeployqt auto-deployment)
cmake --build . --config Release

# 3. Run standalone applications
.\Release\ScreenCut.exe         # Background Capture Engine & Tray
.\Release\SCEditor.exe          # Standalone Annotation Studio
```

#### macOS (Apple Clang)

```bash
# 1. Create build directory and configure CMake
mkdir build && cd build
cmake ..

# 2. Compile project (Automatically packages .app bundles with macdeployqt)
cmake --build . --config Release

# 3. Run application bundles
open ScreenCut.app              # Launch Capture Tray App (Editor is bundled inside)
```

---

## License

This project is open-sourced under the **GNU Lesser General Public License v2.1 or later (LGPL-2.1-or-later)**. See the [LICENSE](LICENSE) file for full license terms.

### Third-Party Licenses
* **Qt 6**: This software dynamically links against the Qt 6 Framework, licensed under the **GNU Lesser General Public License v3.0 (LGPL-3.0)**. Qt is a registered trademark of The Qt Company Ltd.
* **FFmpeg**: Video recording functionality utilizes FFmpeg (https://ffmpeg.org/), which is licensed under the **LGPLv2.1 / GPLv3** (depending on compilation flags). ScreenCut communicates with FFmpeg purely via external command-line process invocation (`QProcess` / standard I/O pipes) and does not statically or dynamically link against FFmpeg libraries. As such, ScreenCut constitutes a separate work and is not a derivative work of FFmpeg. Users must obtain or install FFmpeg independently, or comply with FFmpeg's distribution terms if packaging it alongside this application.
