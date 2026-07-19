/**
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "platform.h"
#include <QDebug>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>

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

bool checkAndRequestScreenCapturePermission(bool requestIfMissing,
                                            bool showPrompt) {
#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
  static bool s_forceAllowSession = false;
  if (s_forceAllowSession) {
    return true;
  }

  if (__builtin_available(macOS 10.15, *)) {
    bool authorized = CGPreflightScreenCaptureAccess();
    if (!authorized) {
      qWarning() << "[Platform::checkAndRequestScreenCapturePermission] macOS "
                    "Screen Recording access NOT authorized!";
      if (requestIfMissing) {
        CGRequestScreenCaptureAccess();
      }
      if (showPrompt) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle(QObject::tr("需要螢幕錄製權限 / 權限驗證提示"));
        msgBox.setText(QObject::tr(
            "【偵測到尚未獲得或需更新 macOS 螢幕錄製授權】\n\n"
            "因為 ScreenCut 重新編譯或首次執行，macOS "
            "隱私機制會校驗簽章。若系統判定尚未獲得『螢幕錄製』授權，截圖時會自"
            "動隱藏所有第三方應用程式視窗（只拍到桌面背景桌布）。\n\n"
            "👉 解決方案 A（推薦，正常截取所有視窗）：\n"
            "點擊『前往設定授權』，在「隱私權與安全性 -> 螢幕錄製」中找到 "
            "ScreenCut（或啟動它的終端機 / IDE）。如果原本已勾選，請【先按 - "
            "刪除它，再重新加入並勾選】，最後重新啟動 ScreenCut "
            "即可永久生效！\n\n"
            "👉 解決方案 B（立即測試/略過警告）：\n"
            "點擊『強制繼續截圖』，本程式在這次執行期間將不再提示並直接為您截圖"
            "。"));
        QPushButton *btnSettings = msgBox.addButton(QObject::tr("前往設定授權"),
                                                    QMessageBox::ActionRole);
        QPushButton *btnForce = msgBox.addButton(QObject::tr("強制繼續截圖"),
                                                 QMessageBox::AcceptRole);
        QPushButton *btnCancel =
            msgBox.addButton(QObject::tr("Cancel"), QMessageBox::RejectRole);
        msgBox.setDefaultButton(btnSettings);

        msgBox.exec();

        if (msgBox.clickedButton() == btnSettings) {
          qDebug()
              << "[Platform::checkAndRequestScreenCapturePermission] Opening "
                 "macOS Privacy & Security -> Screen Recording settings...";
          QProcess::startDetached(
              "open", {"x-apple.systempreferences:com.apple.preference."
                       "security?Privacy_ScreenCapture"});
          return false;
        } else if (msgBox.clickedButton() == btnForce) {
          qDebug() << "[Platform::checkAndRequestScreenCapturePermission] User "
                      "chose to force continue capture session.";
          s_forceAllowSession = true;
          return true;
        } else if (msgBox.clickedButton() == btnCancel) {
          qDebug() << "[Platform::checkAndRequestScreenCapturePermission] User "
                      "cancelled permission dialog.";
          return false;
        } else {
          return false;
        }
      }
      return false;
    }
    return true;
  }
#endif
  Q_UNUSED(requestIfMissing);
  Q_UNUSED(showPrompt);
  return true;
}

#endif // !defined(Q_OS_WIN)

} // namespace Platform
} // namespace ScreenCut
