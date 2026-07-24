/**
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "platform.h"
#include <QDebug>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QDir>
#include <dirent.h>

#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
#include <ApplicationServices/ApplicationServices.h>
#include <CoreGraphics/CoreGraphics.h>
#include <objc/objc.h>
#include <objc/message.h>
#endif

namespace ScreenCut {
namespace Platform {

#if !defined(Q_OS_WIN)

void excludeWindowFromCapture(WId winId) {
#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
  if (!winId) return;
  id nsView = reinterpret_cast<id>(winId);
  if (!nsView) return;
  id nsWindow = ((id (*)(id, SEL))objc_msgSend)(nsView, sel_registerName("window"));
  if (nsWindow) {
    // NSWindowSharingNone = 0 (excludes window from screen captures & recordings)
    ((void (*)(id, SEL, unsigned long))objc_msgSend)(nsWindow, sel_registerName("setSharingType:"), 0);
    qDebug() << "[Platform::excludeWindowFromCapture] Set NSWindow sharingType to NSWindowSharingNone (0) for winId:" << winId;
  }
#else
  Q_UNUSED(winId);
#endif
}

void setDarkTitlebar(WId winId) {
  // macOS / Linux: dark mode follows system or Qt palette theme natively
  Q_UNUSED(winId);
}

void elevateWindowAboveSystemBars(WId winId, bool joinAllSpaces) {
#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
  if (!winId) return;
  id nsView = reinterpret_cast<id>(winId);
  if (!nsView) return;
  id nsWindow = ((id (*)(id, SEL))objc_msgSend)(nsView, sel_registerName("window"));
  if (nsWindow) {
    // CGShieldingWindowLevel() returns the window level for shielding the screen (~1000),
    // placing our capture overlay cleanly above Apple's Top Menu Bar (level 24) and Dock (level 20).
    int level = CGShieldingWindowLevel();
    ((void (*)(id, SEL, long))objc_msgSend)(nsWindow, sel_registerName("setLevel:"), (long)level);
    if (joinAllSpaces) {
      unsigned long currentBehavior = ((unsigned long (*)(id, SEL))objc_msgSend)(nsWindow, sel_registerName("collectionBehavior"));
      unsigned long newBehavior = currentBehavior | (1 << 0); // NSWindowCollectionBehaviorCanJoinAllSpaces (1)
      ((void (*)(id, SEL, unsigned long))objc_msgSend)(nsWindow, sel_registerName("setCollectionBehavior:"), newBehavior);
    }
    qDebug() << "[Platform::elevateWindowAboveSystemBars] Elevated window level on winId:" << winId << "joinAllSpaces:" << joinAllSpaces;
  }
#else
  Q_UNUSED(winId);
  Q_UNUSED(joinAllSpaces);
#endif
}

void enableWindowAllSpacesAndMissionControl(WId winId) {
#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
  if (!winId) return;
  id nsView = reinterpret_cast<id>(winId);
  if (!nsView) return;
  id nsWindow = ((id (*)(id, SEL))objc_msgSend)(nsView, sel_registerName("window"));
  if (nsWindow) {
    // NSWindowCollectionBehaviorMoveToActiveSpace = 1 << 1 (2) -> Window exists on ONLY ONE space at a time (not on every space!), but when activated/clicked from Dock on another space, macOS moves the window onto the current space instead of switching spaces back!
    // NSWindowCollectionBehaviorManaged = 1 << 2 (4) -> Participates cleanly in Mission Control grid
    // NSWindowCollectionBehaviorParticipatesInCycle = 1 << 5 (32) -> Participates in Cmd+Tab / Exposé
    unsigned long currentBehavior = ((unsigned long (*)(id, SEL))objc_msgSend)(nsWindow, sel_registerName("collectionBehavior"));
    unsigned long newBehavior = (currentBehavior & ~(unsigned long)(1 << 0)) | (1 << 1) | (1 << 2) | (1 << 5);
    ((void (*)(id, SEL, unsigned long))objc_msgSend)(nsWindow, sel_registerName("setCollectionBehavior:"), newBehavior);
    qDebug() << "[Platform::enableWindowAllSpacesAndMissionControl] Enabled MoveToActiveSpace (2) + Managed (4) + Cycle (32), cleared CanJoinAllSpaces (1) on winId:" << winId;
  }
#else
  Q_UNUSED(winId);
#endif
}

QStringList getAudioInputArgs(const QString &micDeviceName) {
  QStringList args;
  if (micDeviceName.isEmpty())
    return args;
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

bool checkScreenCapturePermission() {
#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
    if (__builtin_available(macOS 11.0, *)) {
        return CGRequestScreenCaptureAccess();
    } else if (__builtin_available(macOS 10.15, *)) {
        return CGPreflightScreenCaptureAccess();
    }
#endif
    return true;
}

bool checkMicrophonePermission() {
#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
    id avCaptureDeviceClass = (id)objc_getClass("AVCaptureDevice");
    if (!avCaptureDeviceClass) return true;
    id mediaType = (id)CFStringCreateWithCString(NULL, "soun", kCFStringEncodingUTF8);
    long status = ((long (*)(id, SEL, id))objc_msgSend)(avCaptureDeviceClass, sel_registerName("authorizationStatusForMediaType:"), mediaType);
    CFRelease(mediaType);
    // 3 = AVAuthorizationStatusAuthorized
    return (status == 3);
#else
    return true;
#endif
}

bool checkDesktopFolderPermission() {
#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
    QString desktopPath = QDir::homePath() + "/Desktop";
    DIR* dir = opendir(desktopPath.toUtf8().constData());
    if (dir) {
        closedir(dir);
        return true;
    }
    return false;
#else
    return true;
#endif
}

bool checkDownloadFolderPermission() {
#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
    QString downloadsPath = QDir::homePath() + "/Downloads";
    DIR* dir = opendir(downloadsPath.toUtf8().constData());
    if (dir) {
        closedir(dir);
        return true;
    }
    return false;
#else
    return true;
#endif
}

void openSystemSettings(const QString& category) {
#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
    QString url = "x-apple.systempreferences:com.apple.preference.security?Privacy_" + category;
    QProcess::startDetached("open", {url});
#else
    Q_UNUSED(category);
#endif
}

#endif // !defined(Q_OS_WIN)

} // namespace Platform
} // namespace ScreenCut
