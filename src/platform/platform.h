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
 * Check permissions (macOS specific, always return true on other platforms)
 */
bool checkScreenCapturePermission();
bool checkMicrophonePermission();
bool checkDesktopFolderPermission();
bool checkDownloadFolderPermission();

/**
 * Open system settings for a specific permission category (macOS specific)
 */
void openSystemSettings(const QString& category);

} // namespace Platform
} // namespace ScreenCut

#endif // SCREENCUT_PLATFORM_H
