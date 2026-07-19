/**
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef SCREENCUT_PLATFORM_H
#define SCREENCUT_PLATFORM_H

#include <QPoint>
#include <QRect>
#include <QString>
#include <QStringList>
#include <qwindowdefs.h>

namespace ScreenCut {
struct WindowInfo;
}

namespace ScreenCut {
namespace Platform {

/**
 * Exclude the specified window handle from screen capture (if supported by OS).
 * - Windows: SetWindowDisplayAffinity(..., WDA_EXCLUDEFROMCAPTURE)
 * - macOS/Linux: No-op or native window level exclusion
 */
void excludeWindowFromCapture(WId winId);

/**
 * Apply dark title bar styling to a top-level window.
 * - Windows: DwmSetWindowAttribute(..., DWMWA_USE_IMMERSIVE_DARK_MODE, ...)
 * - macOS/Linux: Native or no-op
 */
void setDarkTitlebar(WId winId);

/**
 * Elevate window level above macOS Dock and Menu bar (CGShieldingWindowLevel / level 1000)
 * so that full-screen capture overlays can capture and receive mouse events over system bars.
 */
void elevateWindowAboveSystemBars(WId winId, bool joinAllSpaces = true);

/**
 * Configure macOS window collection behavior to participate in Mission Control (Managed)
 * while also joining and following across all virtual desktop spaces (CanJoinAllSpaces).
 */
void enableWindowAllSpacesAndMissionControl(WId winId);

/**
 * Build platform-specific audio input FFmpeg arguments for microphone capture.
 */
QStringList getAudioInputArgs(const QString& micDeviceName);

/**
 * Check whether the application has permission to record/capture the screen across all open windows.
 * - macOS (10.15+): Checks CGPreflightScreenCaptureAccess(). If false, returns only wallpaper when capturing.
 *   If requestIfMissing is true, calls CGRequestScreenCaptureAccess().
 *   If showPrompt is true, displays a clear GUI warning explaining why windows disappear and how to fix in System Settings.
 * - Windows/Linux: Always returns true.
 */
bool checkAndRequestScreenCapturePermission(bool requestIfMissing = true, bool showPrompt = true);

} // namespace Platform
} // namespace ScreenCut

#endif // SCREENCUT_PLATFORM_H
