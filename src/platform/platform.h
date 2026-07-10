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
 * Build platform-specific audio input FFmpeg arguments for microphone capture.
 */
QStringList getAudioInputArgs(const QString& micDeviceName);

} // namespace Platform
} // namespace ScreenCut

#endif // SCREENCUT_PLATFORM_H
