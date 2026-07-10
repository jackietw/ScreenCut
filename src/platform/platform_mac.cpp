/**
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "platform.h"
#include <QDebug>

#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
#include <CoreGraphics/CoreGraphics.h>
#endif

namespace ScreenCut {
namespace Platform {

#if !defined(Q_OS_WIN)

void excludeWindowFromCapture(WId winId) {
    // macOS / Linux: window exclusion handled via native window levels or ScreenCaptureKit when applicable
    Q_UNUSED(winId);
}

void setDarkTitlebar(WId winId) {
    // macOS / Linux: dark mode follows system or Qt palette theme natively
    Q_UNUSED(winId);
}

QStringList getAudioInputArgs(const QString& micDeviceName) {
    QStringList args;
    if (micDeviceName.isEmpty()) return args;
#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
    // AVFoundation: -f avfoundation -i ":Device Name" (no video, just mic)
    args << "-f" << "avfoundation"
         << "-i" << QString("none:%1").arg(micDeviceName);
#else
    // Linux PulseAudio/ALSA: use pulse fallback
    args << "-f" << "pulse" << "-i" << "default";
#endif
    return args;
}

#endif // !defined(Q_OS_WIN)

} // namespace Platform
} // namespace ScreenCut
