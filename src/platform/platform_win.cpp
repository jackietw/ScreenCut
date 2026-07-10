/**
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "platform.h"
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")

#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#endif

namespace ScreenCut {
namespace Platform {

void excludeWindowFromCapture(WId winId) {
#ifdef Q_OS_WIN
    if (!winId) return;
    BOOL res = SetWindowDisplayAffinity(reinterpret_cast<HWND>(winId), WDA_EXCLUDEFROMCAPTURE);
    if (!res) {
        qDebug() << "[Platform::excludeWindowFromCapture] SetWindowDisplayAffinity failed for winId:" << winId;
    }
#else
    Q_UNUSED(winId);
#endif
}

void setDarkTitlebar(WId winId) {
#ifdef Q_OS_WIN
    if (!winId) return;
    BOOL value = TRUE;
    DwmSetWindowAttribute(reinterpret_cast<HWND>(winId), DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
#else
    Q_UNUSED(winId);
#endif
}

QStringList getAudioInputArgs(const QString& micDeviceName) {
    QStringList args;
    if (micDeviceName.isEmpty()) return args;
#ifdef Q_OS_WIN
    // DirectShow: -f dshow -i audio="Device Name"
    args << "-f" << "dshow"
         << "-i" << QString("audio=\"%1\"").arg(micDeviceName);
#else
    Q_UNUSED(micDeviceName);
#endif
    return args;
}

} // namespace Platform
} // namespace ScreenCut
