/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "capture_engine.h"
#include "../capture/capture_window.h"
#include <QGuiApplication>
#include <QApplication>
#include <QScreen>
#include <QPainterPath>
#include <QStyle>
#include <QFontDatabase>
#include <QDebug>
#include <QFileDialog>
#include <QDateTime>
#include <QMessageBox>
#include <QFrame>
#include <QPushButton>
#include <QHBoxLayout>
#include "../resources/IconUtils.h"
#include "../config.h"
#include <algorithm>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#endif

#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
#include <CoreGraphics/CoreGraphics.h>
#include <unistd.h>  // getpid()
#endif

#include "../widgets/capture_toolbar.h"
#include "../widgets/capture_pin.h"
#include <QClipboard>
#include <QFileDialog>
#include "common_project.h"
#include "../platform/platform.h"
#include "../widgets/capture_countdown.h"
#include "../widgets/capture_scroll.h"
#include <QDir>
#include <QThreadPool>
#include <QMutex>
#include <QMutexLocker>
#include "../widgets/common_notification.h"
#include "../widgets/capture_status.h"
#include <QMediaDevices>
#include <QAudioDevice>

namespace ScreenCut {

CaptureEngine* CaptureEngine::s_instance = nullptr;

CaptureEngine* CaptureEngine::instance() {
    if (!s_instance) {
        s_instance = new CaptureEngine();
        qDebug() << "[CaptureEngine] Singleton instance initialized.";
    }
    return s_instance;
}

CaptureEngine::CaptureEngine(QObject* parent) : QObject(parent) {
    m_videoRecorder = new VideoRecorder(this);
    connect(m_videoRecorder, &VideoRecorder::recordingFinished, this, [this](const QString& path) {
        qDebug() << "[CaptureEngine] MP4 video recording successfully saved:" << path;
        emit captureSaved(path);
        Notification::showMessage(QString("🎬 Video Saved to: %1").arg(path), 5000, false);
    });
    connect(m_videoRecorder, &VideoRecorder::recordingError, this, [this](const QString& err) {
        Notification::showMessage(QString("⚠️ Recording Error: %1").arg(err), 6000, true);
    });
    VideoRecorder::detectAvailableHwEncodersAsync();
}

CaptureEngine::~CaptureEngine() {
    if (m_videoRecorder && m_videoRecorder->isRecording()) {
        m_videoRecorder->stop();
    }
    if (m_overlayWidget) {
        m_overlayWidget->deleteLater();
    }
}

void CaptureEngine::initSessionStateFromConfig() {
    if (CaptureMainWindow::instance()) {
        m_sessionCursorEnabled = CaptureMainWindow::instance()->isSettingEnabled("Video_Capture Cursor");
        m_sessionMicEnabled = CaptureMainWindow::instance()->isSettingEnabled("Record Microphone");
        m_sessionSysAudioEnabled = CaptureMainWindow::instance()->isSettingEnabled("Record System Audio");
        m_sessionEditorEnabled = CaptureMainWindow::instance()->isSettingEnabled("Preview in Editor");
        m_sessionClipboardEnabled = CaptureMainWindow::instance()->isSettingEnabled("Copy to Clipboard");
        m_sessionImgCursorEnabled = CaptureMainWindow::instance()->isSettingEnabled("Image_Capture Cursor");
    } else {
        m_sessionCursorEnabled = Config::getValue("Video_Capture Cursor", true).toBool();
        m_sessionMicEnabled = Config::getValue("Record Microphone", true).toBool();
        m_sessionSysAudioEnabled = Config::getValue("Record System Audio", true).toBool();
        m_sessionEditorEnabled = Config::getValue("Preview in Editor", true).toBool();
        m_sessionClipboardEnabled = Config::getValue("Copy to Clipboard", true).toBool();
        m_sessionImgCursorEnabled = Config::getValue("Image_Capture Cursor", false).toBool();
    }
}

void CaptureEngine::setSessionCursorEnabled(bool enabled) {
    m_sessionCursorEnabled = enabled;
    if (m_videoRecorder && m_videoRecorder->worker()) {
        QMetaObject::invokeMethod(m_videoRecorder->worker(), [w = m_videoRecorder->worker(), enabled]() {
            w->setCaptureCursor(enabled);
        });
    }
}

void CaptureEngine::setSessionMicEnabled(bool enabled) {
    m_sessionMicEnabled = enabled;
}

void CaptureEngine::setSessionSysAudioEnabled(bool enabled) {
    m_sessionSysAudioEnabled = enabled;
}

void CaptureEngine::startVideoRecord(const QRect& rect) {
    if (m_videoRecorder && !m_videoRecorder->isRecording()) {
        m_videoRecorder->start(rect);
    }
}

void CaptureEngine::stopVideoRecord() {
    if (m_videoRecorder && m_videoRecorder->isRecording()) {
        m_videoRecorder->stop();
    }
}

void CaptureEngine::drawCursorOnRegion(QPixmap& target, const QPoint& targetTopLeftGlobal, const QPoint& cursorGlobalPos) {
    if (target.isNull()) return;
    QPoint localPos = cursorGlobalPos - targetTopLeftGlobal;

    if (localPos.x() < -50 || localPos.x() > target.width() + 50 ||
        localPos.y() < -50 || localPos.y() > target.height() + 50) {
        if (target.rect().contains(localPos)) {
            // inside, okay
        } else {
            QPoint curPos = QCursor::pos() - targetTopLeftGlobal;
            if (target.rect().contains(curPos)) {
                localPos = curPos;
            } else {
                return;
            }
        }
    }

#ifdef Q_OS_WIN
    CURSORINFO ci = { sizeof(CURSORINFO) };
    if (GetCursorInfo(&ci) && ci.hCursor) {
        ICONINFO ii = { 0 };
        if (GetIconInfo(ci.hCursor, &ii)) {
            QPoint hotspot(ii.xHotspot, ii.yHotspot);
            if (ii.hbmMask) DeleteObject(ii.hbmMask);
            if (ii.hbmColor) DeleteObject(ii.hbmColor);

            int w = GetSystemMetrics(SM_CXCURSOR);
            int h = GetSystemMetrics(SM_CYCURSOR);
            if (w <= 0) w = 32;
            if (h <= 0) h = 32;

            BITMAPINFO bmi = { 0 };
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = w;
            bmi.bmiHeader.biHeight = -h; // Top-down DIB
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;

            HDC hdcScreen = GetDC(NULL);
            HDC hdcMem = CreateCompatibleDC(hdcScreen);
            void* pBits = nullptr;
            HBITMAP hDib = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
            if (hDib) {
                HGDIOBJ hOld = SelectObject(hdcMem, hDib);
                DrawIconEx(hdcMem, 0, 0, ci.hCursor, w, h, 0, NULL, DI_NORMAL);

                QImage cursorImg(static_cast<const uchar*>(pBits), w, h, QImage::Format_ARGB32_Premultiplied);
                QPixmap cursorPix = QPixmap::fromImage(cursorImg.copy());

                SelectObject(hdcMem, hOld);
                DeleteObject(hDib);
                DeleteDC(hdcMem);
                ReleaseDC(NULL, hdcScreen);

                QPainter painter(&target);
                if (CaptureEngine::instance()->isSessionHighlightEnabled()) {
                    painter.setRenderHint(QPainter::Antialiasing, true);
                    painter.setPen(Qt::NoPen);
                    painter.setBrush(QColor(255, 235, 59, 140));
                    painter.drawEllipse(localPos, 22, 22);
                }
                if (CaptureEngine::instance()->isSessionAnimationEnabled()) {
                    painter.setRenderHint(QPainter::Antialiasing, true);
                    int ringRadius = 26 + ((QDateTime::currentMSecsSinceEpoch() / 80) % 8);
                    painter.setPen(QPen(QColor(0, 168, 255, 170), 2));
                    painter.setBrush(Qt::NoBrush);
                    painter.drawEllipse(localPos, ringRadius, ringRadius);
                }
                painter.drawPixmap(localPos - hotspot, cursorPix);
                painter.end();
                return;
            }
            DeleteDC(hdcMem);
            ReleaseDC(NULL, hdcScreen);
        }
    }
#endif

    QPainter painter(&target);
    painter.setRenderHint(QPainter::Antialiasing, true);
    if (CaptureEngine::instance()->isSessionHighlightEnabled()) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 235, 59, 140));
        painter.drawEllipse(localPos, 22, 22);
    }
    if (CaptureEngine::instance()->isSessionAnimationEnabled()) {
        int ringRadius = 26 + ((QDateTime::currentMSecsSinceEpoch() / 80) % 8);
        painter.setPen(QPen(QColor(0, 168, 255, 170), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(localPos, ringRadius, ringRadius);
    }
    QPainterPath path;
    path.moveTo(localPos.x(), localPos.y());
    path.lineTo(localPos.x() + 0, localPos.y() + 18);
    path.lineTo(localPos.x() + 5, localPos.y() + 14);
    path.lineTo(localPos.x() + 9, localPos.y() + 22);
    path.lineTo(localPos.x() + 12, localPos.y() + 20);
    path.lineTo(localPos.x() + 8, localPos.y() + 12);
    path.lineTo(localPos.x() + 14, localPos.y() + 12);
    path.closeSubpath();

    painter.setPen(QPen(Qt::black, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::white);
    painter.drawPath(path);
    painter.end();
}

void CaptureEngine::drawCursorOnSnapshot(QPixmap& snapshot) {
    if (snapshot.isNull()) return;
    QList<QScreen*> screens = QGuiApplication::screens();
    QRect virtualGeometry;
    for (QScreen* screen : screens) {
        virtualGeometry = virtualGeometry.united(screen->geometry());
    }
    drawCursorOnRegion(snapshot, virtualGeometry.topLeft(), QCursor::pos());
}

void CaptureEngine::startCapture(CaptureMode mode) {
    qDebug() << "[CaptureEngine] Starting capture. Mode:" << static_cast<int>(mode);
    if (m_isPendingCapture) {
        qWarning() << "[CaptureEngine] Capture already pending, ignoring request.";
        return;
    }
    m_currentMode = mode;
    m_isPendingCapture = true;

    bool wasVisible = CaptureMainWindow::instance() && CaptureMainWindow::instance()->isVisible();
    if (wasVisible) {
        qDebug() << "[CaptureEngine] Hiding visible CaptureMainWindow before capture starts...";
        CaptureMainWindow::instance()->hide();
    }

    bool delay5s = CaptureMainWindow::instance() && CaptureMainWindow::instance()->isSettingEnabled("5 Second Delay");
    if (delay5s) {
        qDebug() << "[CaptureEngine] 5 Second Delay enabled. Showing top-right 5s countdown overlay...";
        CountdownWidget* countdown = new CountdownWidget(5, nullptr);
        connect(countdown, &CountdownWidget::cancelled, this, [this]() {
            qDebug() << "[CaptureEngine] 5s countdown cancelled by user clicking box.";
            m_isPendingCapture = false;
            emit captureCancelled();
        });
        connect(countdown, &CountdownWidget::completed, this, [this, mode]() {
            qDebug() << "[CaptureEngine] 5s countdown completed. Taking snapshot...";
            doActualCapture(mode);
        });
        countdown->startCountdown();
    } else if (wasVisible) {
        QTimer::singleShot(200, this, [this, mode]() {
            doActualCapture(mode);
        });
    } else {
        doActualCapture(mode);
    }
}

void CaptureEngine::doActualCapture(CaptureMode mode) {
    m_isPendingCapture = false;
    initSessionStateFromConfig();
    m_fullDesktopSnapshot = captureAllScreens();
    if (mode == CaptureMode::FullScreen && isSessionImgCursorEnabled()) {
        drawCursorOnSnapshot(m_fullDesktopSnapshot);
    }
    qDebug() << "[CaptureEngine] Desktop snapshot captured. Size:" << m_fullDesktopSnapshot.size();

    if (m_currentMode == CaptureMode::FullScreen) {
        qDebug() << "[CaptureEngine] FullScreen mode completed immediately.";
        emit captureCompleted(m_fullDesktopSnapshot);
        return;
    }

    if (m_overlayWidget) {
        m_overlayWidget->deleteLater();
        m_overlayWidget = nullptr;
    }

    m_overlayWidget = new RegionSelectWidget(m_fullDesktopSnapshot, m_currentMode);

    auto startScrollCaptureFunc = [this](const QRect& rect) {
        qDebug() << "[CaptureEngine] Region selected for Scrolling capture. Launching ScrollCaptureManager on rect:" << rect;
        if (m_overlayWidget) {
            m_overlayWidget->close();
            m_overlayWidget->deleteLater();
            m_overlayWidget = nullptr;
        }
        ScrollCaptureManager* scrollMgr = new ScrollCaptureManager(rect, nullptr);
        connect(scrollMgr, &ScrollCaptureManager::completed, this, [this](const QPixmap& stitchedPix) {
            qDebug() << "[CaptureEngine] Scrolling capture completed. Stitched size:" << stitchedPix.size();
            emit captureCompleted(stitchedPix);
        });
        connect(scrollMgr, &ScrollCaptureManager::cancelled, this, [this]() {
            qDebug() << "[CaptureEngine] Scrolling capture cancelled by user.";
            emit captureCancelled();
        });
        scrollMgr->show();
    };

    connect(m_overlayWidget, &RegionSelectWidget::confirmRequested, this, [this, startScrollCaptureFunc](const QRect& rect) {
        if (m_currentMode == CaptureMode::Scrolling) {
            startScrollCaptureFunc(rect);
            return;
        }
        if (m_currentMode == CaptureMode::VideoRegion) {
            qDebug() << "[CaptureEngine] Video recording finished/confirmed for region:" << rect;
            if (CaptureEngine::instance()->videoRecorder() && CaptureEngine::instance()->videoRecorder()->isRecording()) {
                CaptureEngine::instance()->stopVideoRecord();
            }
            if (m_overlayWidget) {
                m_overlayWidget->close();
                m_overlayWidget->deleteLater();
                m_overlayWidget = nullptr;
            }
            if (CaptureMainWindow::instance()) {
                CaptureMainWindow::instance()->show();
                CaptureMainWindow::instance()->raise();
                CaptureMainWindow::instance()->activateWindow();
            }
            return;
        }
        QPixmap cropped = captureRect(rect);
        if (m_overlayWidget) {
            m_overlayWidget->close();
            m_overlayWidget->deleteLater();
            m_overlayWidget = nullptr;
        }
        emit captureCompleted(cropped);
    });

    connect(m_overlayWidget, &RegionSelectWidget::regionSelected, this, [this, startScrollCaptureFunc](const QRect& rect) {
        if (m_currentMode == CaptureMode::Scrolling) {
            startScrollCaptureFunc(rect);
            return;
        }
        QPixmap cropped = captureRect(rect);
        if (m_overlayWidget) {
            m_overlayWidget->close();
            m_overlayWidget->deleteLater();
            m_overlayWidget = nullptr;
        }
        emit captureEdited(cropped);
    });

    connect(m_overlayWidget, &RegionSelectWidget::copyRequested, this, [this, startScrollCaptureFunc](const QRect& rect) {
        if (m_currentMode == CaptureMode::Scrolling) {
            startScrollCaptureFunc(rect);
            return;
        }
        QPixmap cropped = captureRect(rect);
        if (m_overlayWidget) {
            m_overlayWidget->close();
            m_overlayWidget->deleteLater();
            m_overlayWidget = nullptr;
        }
        emit captureCopied(cropped);
    });

    connect(m_overlayWidget, &RegionSelectWidget::saveRequested, this, [this, startScrollCaptureFunc](const QRect& rect) {
        if (m_currentMode == CaptureMode::Scrolling) {
            startScrollCaptureFunc(rect);
            return;
        }
        QPixmap cropped = captureRect(rect);
        if (m_overlayWidget) {
            m_overlayWidget->close();
            m_overlayWidget->deleteLater();
            m_overlayWidget = nullptr;
        }
        QString defaultPath = ScutProject::getLibraryDir() + "/" + 
            QString("Capture_%1.scut").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz"));
        QString fileName = QFileDialog::getSaveFileName(nullptr, "Save ScreenCut Capture / Project",
            defaultPath,
            "ScreenCut Project (*.scut);;PNG Image (*.png);;JPEG Image (*.jpg);;WebP Image (*.webp)");
        if (!fileName.isEmpty()) {
            bool saved = false;
            if (fileName.endsWith(".scut", Qt::CaseInsensitive)) {
                saved = ScutProject::saveImageAsScut(cropped, fileName);
            } else {
                saved = cropped.save(fileName);
            }
            if (saved) {
                emit captureSaved(fileName);
            } else {
                QMessageBox::critical(nullptr, "Save Error", "Failed to save capture to specified location.");
            }
        } else {
            emit captureCancelled();
        }
    });

    connect(m_overlayWidget, &RegionSelectWidget::cancelled, this, [this]() {
        if (m_overlayWidget) {
            m_overlayWidget->close();
            m_overlayWidget->deleteLater();
            m_overlayWidget = nullptr;
        }
        emit captureCancelled();
    });

    emit captureStarted();
    QList<QScreen*> screens = QGuiApplication::screens();
    if (screens.size() > 1) {
        QRect virtualGeometry;
        for (QScreen* screen : screens) {
            virtualGeometry = virtualGeometry.united(screen->geometry());
        }
        m_overlayWidget->setGeometry(virtualGeometry);
        m_overlayWidget->show();
    } else {
        m_overlayWidget->showFullScreen();
    }
    m_overlayWidget->activateWindow();
    m_overlayWidget->setFocus();
}

void CaptureEngine::startRegionCapture() {
    startCapture(CaptureMode::Region);
}

void CaptureEngine::startVideoRegionCapture() {
    startCapture(CaptureMode::VideoRegion);
}

void CaptureEngine::startWindowCapture() {
    startCapture(CaptureMode::Window);
}

void CaptureEngine::startScrollingCapture() {
    // For Scrolling capture, we initiate with Region capture to let user select the scrollable viewport
    startCapture(CaptureMode::Scrolling);
}

void CaptureEngine::startFullScreenCapture() {
    startCapture(CaptureMode::FullScreen);
}

QPixmap CaptureEngine::captureRect(const QRect& rect) {
    if (m_fullDesktopSnapshot.isNull()) {
        m_fullDesktopSnapshot = captureAllScreens();
    }
    
    // Ensure rect is within bounds
    QRect boundedRect = rect.intersected(m_fullDesktopSnapshot.rect());
    if (boundedRect.isEmpty()) {
        return m_fullDesktopSnapshot;
    }
    QPixmap cropped = m_fullDesktopSnapshot.copy(boundedRect);

    if (isSessionImgCursorEnabled()) {
        QPoint cursorPos = QCursor::pos();
        if (m_overlayWidget && !m_overlayWidget->lastSelectionMousePos().isNull()) {
            cursorPos = m_overlayWidget->lastSelectionMousePos();
        }
        drawCursorOnRegion(cropped, boundedRect.topLeft(), cursorPos);
    }
    return cropped;
}

QPixmap CaptureEngine::captureScreen(QScreen* screen) {
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    if (!screen) {
        return QPixmap();
    }
    return screen->grabWindow(0);
}

QPixmap CaptureEngine::captureAllScreens() {
    QList<QScreen*> screens = QGuiApplication::screens();
    qDebug() << "[CaptureEngine] captureAllScreens across" << screens.size() << "monitor(s).";
    if (screens.isEmpty()) {
        qWarning() << "[CaptureEngine] No screens found!";
        return QPixmap();
    }
    if (screens.size() == 1) {
        QPixmap pix = screens.first()->grabWindow(0);
        qDebug() << "[CaptureEngine] Single screen grab size:" << pix.size();
        return pix;
    }

    // Calculate bounding geometry across all monitors
    QRect virtualGeometry;
    for (QScreen* screen : screens) {
        virtualGeometry = virtualGeometry.united(screen->geometry());
    }
    qDebug() << "[CaptureEngine] Multi-monitor virtual geometry:" << virtualGeometry;

    qreal maxDpr = 1.0;
    for (QScreen* screen : screens) {
        if (screen->devicePixelRatio() > maxDpr) maxDpr = screen->devicePixelRatio();
    }
    QSize physSize = virtualGeometry.size() * maxDpr;
    QPixmap combined(physSize);
    combined.setDevicePixelRatio(maxDpr);
    combined.fill(Qt::black);

    QPainter painter(&combined);
    for (QScreen* screen : screens) {
        QPixmap screenPixmap = screen->grabWindow(0);
        QPoint offset = screen->geometry().topLeft() - virtualGeometry.topLeft();
        painter.drawPixmap(offset, screenPixmap);
    }
    painter.end();

    return combined;
}

#ifdef Q_OS_WIN
struct FindWindowContext {
    POINT pt;
    DWORD currentPid;
    HWND bestHwnd;
    RECT bestRect;
};

static BOOL CALLBACK EnumWindowsCallbackProc(HWND hwnd, LPARAM lParam) {
    FindWindowContext* context = reinterpret_cast<FindWindowContext*>(lParam);
    if (!IsWindowVisible(hwnd) || IsIconic(hwnd)) {
        return TRUE;
    }
    DWORD windowPid = 0;
    GetWindowThreadProcessId(hwnd, &windowPid);
    if (windowPid == context->currentPid) {
        return TRUE;
    }
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TRANSPARENT) {
        return TRUE;
    }
    wchar_t className[256] = {0};
    GetClassNameW(hwnd, className, 255);
    if (wcscmp(className, L"Progman") == 0 || wcscmp(className, L"WorkerW") == 0) {
        return TRUE;
    }
    RECT rect;
    HRESULT hr = DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rect, sizeof(rect));
    if (FAILED(hr)) {
        if (!GetWindowRect(hwnd, &rect)) return TRUE;
    }
    if (context->pt.x >= rect.left && context->pt.x < rect.right &&
        context->pt.y >= rect.top && context->pt.y < rect.bottom) {
        if ((rect.right - rect.left) > 10 && (rect.bottom - rect.top) > 10) {
            context->bestHwnd = hwnd;
            context->bestRect = rect;
            return FALSE;
        }
    }
    return TRUE;
}
#endif

WindowInfo CaptureEngine::findWindowAt(const QPoint& globalPos) {
    WindowInfo info;
#ifdef Q_OS_WIN
    QScreen* screen = QGuiApplication::screenAt(globalPos);
    if (!screen) screen = QGuiApplication::primaryScreen();
    qreal dpr = screen ? screen->devicePixelRatio() : 1.0;
    if (dpr <= 0) dpr = 1.0;

    POINT physPt = { qRound(globalPos.x() * dpr), qRound(globalPos.y() * dpr) };
    POINT cursorPos;
    if (GetCursorPos(&cursorPos)) {
        if (qAbs(globalPos.x() - qRound(cursorPos.x / dpr)) <= 5 && qAbs(globalPos.y() - qRound(cursorPos.y / dpr)) <= 5) {
            physPt = cursorPos;
        }
    }

    FindWindowContext ctx = { physPt, GetCurrentProcessId(), nullptr, {0, 0, 0, 0} };
    EnumWindows(&EnumWindowsCallbackProc, reinterpret_cast<LPARAM>(&ctx));

    if (!ctx.bestHwnd) {
        return info;
    }

    int physLeft = ctx.bestRect.left;
    int physTop = ctx.bestRect.top;
    int physWidth = ctx.bestRect.right - ctx.bestRect.left;
    int physHeight = ctx.bestRect.bottom - ctx.bestRect.top;

    int logLeft = qRound(physLeft / dpr);
    int logTop = qRound(physTop / dpr);
    int logWidth = qRound(physWidth / dpr);
    int logHeight = qRound(physHeight / dpr);

    QList<QScreen*> screens = QGuiApplication::screens();
    QRect virtualGeometry;
    for (QScreen* s : screens) {
        virtualGeometry = virtualGeometry.united(s->geometry());
    }
    int relLeft = logLeft - virtualGeometry.topLeft().x();
    int relTop = logTop - virtualGeometry.topLeft().y();

    info.rect = QRect(relLeft, relTop, logWidth, logHeight);
    wchar_t titleBuf[512] = {0};
    GetWindowTextW(ctx.bestHwnd, titleBuf, 511);
    info.title = QString::fromWCharArray(titleBuf);
    info.isValid = !info.rect.isEmpty() && info.rect.width() > 10 && info.rect.height() > 10;
#else
// -----------------------------------------------------------------------
// macOS fallback: use CGWindowListCopyWindowInfo via CoreGraphics to
// enumerate visible windows and find the topmost window under globalPos.
// -----------------------------------------------------------------------
#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
    // Convert Qt logical coords → CoreGraphics screen coords (CG origin = top-left of primary)
    QScreen* primaryScreen = QGuiApplication::primaryScreen();
    qreal dpr = primaryScreen ? primaryScreen->devicePixelRatio() : 1.0;

    // Build virtual geometry (may span multiple screens)
    QList<QScreen*> screens = QGuiApplication::screens();
    QRect virtualGeometry;
    for (QScreen* s : screens) virtualGeometry = virtualGeometry.united(s->geometry());

    // CG coords: origin = top-left of primary screen, Y increases downward, in physical pixels
    CGFloat cgX = globalPos.x() * dpr;
    CGFloat cgY = globalPos.y() * dpr;

    CFArrayRef windowList = CGWindowListCopyWindowInfo(
        kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements,
        kCGNullWindowID);
    if (windowList) {
        CFIndex count = CFArrayGetCount(windowList);
        for (CFIndex i = 0; i < count; ++i) {
            CFDictionaryRef dict = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(windowList, i));
            if (!dict) continue;

            // Skip windows from this process
            CFNumberRef pidRef = static_cast<CFNumberRef>(CFDictionaryGetValue(dict, kCGWindowOwnerPID));
            if (pidRef) {
                pid_t wPid = 0;
                CFNumberGetValue(pidRef, kCFNumberIntType, &wPid);
                if (wPid == getpid()) continue;
            }

            // Get window bounds in CG coords
            CFDictionaryRef boundsDict = static_cast<CFDictionaryRef>(CFDictionaryGetValue(dict, kCGWindowBounds));
            if (!boundsDict) continue;
            CGRect bounds;
            CGRectMakeWithDictionaryRepresentation(boundsDict, &bounds);

            if (cgX >= bounds.origin.x && cgX < bounds.origin.x + bounds.size.width &&
                cgY >= bounds.origin.y && cgY < bounds.origin.y + bounds.size.height &&
                bounds.size.width > 10 && bounds.size.height > 10) {

                // Convert CG physical → Qt logical coords
                int logLeft   = qRound(bounds.origin.x / dpr) - virtualGeometry.left();
                int logTop    = qRound(bounds.origin.y / dpr) - virtualGeometry.top();
                int logWidth  = qRound(bounds.size.width  / dpr);
                int logHeight = qRound(bounds.size.height / dpr);
                info.rect = QRect(logLeft, logTop, logWidth, logHeight);

                // Window title
                CFStringRef nameRef = static_cast<CFStringRef>(CFDictionaryGetValue(dict, kCGWindowName));
                if (nameRef) {
                    char buf[512] = {};
                    CFStringGetCString(nameRef, buf, sizeof(buf), kCFStringEncodingUTF8);
                    info.title = QString::fromUtf8(buf);
                }
                info.isValid = true;
                break; // Top-most matching window found (list is front-to-back)
            }
        }
        CFRelease(windowList);
    }
#else
    // Linux / other: return full desktop as fallback
    info.rect = QRect(0, 0, m_fullDesktopSnapshot.width(), m_fullDesktopSnapshot.height());
    info.isValid = false;
#endif
#endif
    return info;
}

std::vector<WindowInfo> CaptureEngine::getAllWindows() {
    std::vector<WindowInfo> windows;
    // Can be expanded with EnumWindows on Win32 or CGWindowList on macOS
    return windows;
}

// ============================================================================
// ============================================================================
// VideoBorderOverlay Implementation
// ============================================================================

class VideoBorderOverlay : public QWidget {
public:
    explicit VideoBorderOverlay(const QRect& logicalRect, QWidget* parent = nullptr)
        : QWidget(parent), m_rect(logicalRect) {
        setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::WindowTransparentForInput | Qt::NoDropShadowWindowHint | Qt::Tool);
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);

        Platform::excludeWindowFromCapture(winId());

        // Position slightly outside the recording rect so the 2px red border wraps tightly around m_selectedRect
        setGeometry(logicalRect.left() - 2, logicalRect.top() - 2, logicalRect.width() + 4, logicalRect.height() + 4);
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        QPen borderPen(QColor(235, 50, 50), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(borderPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(1, 1, width() - 2, height() - 2);
    }

private:
    QRect m_rect;
};

// ============================================================================
// RegionSelectWidget Implementation
// ============================================================================

RegionSelectWidget::RegionSelectWidget(const QPixmap& desktopSnapshot, CaptureMode mode, QWidget* parent)
    : QWidget(parent), m_desktopSnapshot(desktopSnapshot), m_mode(mode) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_NoSystemBackground);
    setCursor(Qt::CrossCursor);
    setMouseTracking(true);

    if (!m_desktopSnapshot.isNull()) {
        setGeometry(m_desktopSnapshot.rect());
    }
    updateHoveredWindow(mapFromGlobal(QCursor::pos()));
}

RegionSelectWidget::~RegionSelectWidget() {
    if (m_countdownTimer) {
        m_countdownTimer->stop();
    }
    if (m_borderOverlay) {
        m_borderOverlay->close();
        m_borderOverlay->deleteLater();
        m_borderOverlay = nullptr;
    }
    if (m_toolbar) {
        m_toolbar->deleteLater();
        m_toolbar = nullptr;
    }
    if (m_statusCard) {
        m_statusCard->deleteLater();
        m_statusCard = nullptr;
    }
}

void RegionSelectWidget::setVideoRecordingActive(bool active) {
    m_isVideoRecordingActive = active;
    if (active) {
        if (!m_borderOverlay) {
            m_borderOverlay = new VideoBorderOverlay(m_selectedRect);
            m_borderOverlay->show();
        }
        if (m_toolbar) {
            m_toolbar->setParent(nullptr);
            m_toolbar->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
            m_toolbar->setAttribute(Qt::WA_TranslucentBackground, true);
            updateToolbarPosition();
            m_toolbar->show();
            m_toolbar->raise();
        }
        if (m_statusCard) {
            m_statusCard->hide();
        }
        hide(); // Hide full-screen selection widget so clicks pass directly to desktop and separate border overlay shows the red border
    } else {
        if (m_borderOverlay) {
            m_borderOverlay->close();
            m_borderOverlay->deleteLater();
            m_borderOverlay = nullptr;
        }
        show();
        raise();
    }
    update();
}

void RegionSelectWidget::startVideoCountdown() {
    if (m_toolbar) m_toolbar->hide();
    if (m_statusCard) m_statusCard->hide();
    m_isCountingDown = true;
    m_countdownValue = 3;
    update();

    if (!m_countdownTimer) {
        m_countdownTimer = new QTimer(this);
        connect(m_countdownTimer, &QTimer::timeout, this, [this]() {
            m_countdownValue--;
            if (m_countdownValue <= 0) {
                m_countdownTimer->stop();
                m_isCountingDown = false;
                setVideoRecordingActive(true);
                CaptureEngine::instance()->startVideoRecord(m_selectedRect);
            }
            update();
        });
    }
    m_countdownTimer->start(1000);
}

void RegionSelectWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    if (m_isVideoRecordingActive) {
        QPen borderPen(QColor(235, 50, 50), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(borderPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(m_selectedRect);
        return;
    }

    // 1. Draw base desktop snapshot
    painter.drawPixmap(0, 0, m_desktopSnapshot);

    // 2. Draw semi-transparent dark mask over the entire screen
    QColor maskColor(0, 0, 0, 120);
    
    QRect highlightRect;
    if (m_selectionConfirmed && m_selectedRect.isValid()) {
        highlightRect = m_selectedRect;
    } else if (m_isSelecting) {
        QRect dragRect = QRect(m_startPoint, m_currentMousePos).normalized();
        if ((m_currentMousePos - m_startPoint).manhattanLength() > 10 && (dragRect.width() > 10 || dragRect.height() > 10)) {
            highlightRect = dragRect;
        } else if (m_hoveredWindow.isValid && !m_hoveredWindow.rect.isEmpty()) {
            highlightRect = m_hoveredWindow.rect;
        } else {
            highlightRect = rect();
        }
    } else if (m_hoveredWindow.isValid && !m_hoveredWindow.rect.isEmpty()) {
        highlightRect = m_hoveredWindow.rect;
    } else {
        highlightRect = rect();
    }

    if (highlightRect.isValid() && !highlightRect.isEmpty()) {
        // Draw mask excluding the highlighted region using QPainterPath
        QPainterPath path;
        path.addRect(rect());
        path.addRect(highlightRect);
        painter.fillPath(path, maskColor);

        QColor themeColor = (m_mode == CaptureMode::VideoRegion) ? QColor(235, 50, 50) : QColor(0, 168, 255);

        // 3. Draw vibrant border around highlighted area
        QPen borderPen(themeColor, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(borderPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(highlightRect);

        if (m_isCountingDown && highlightRect == m_selectedRect) {
            // 紅框內變暗
            painter.fillRect(highlightRect, QColor(0, 0, 0, 150));
            
            // Draw countdown number 3 -> 2 -> 1
            painter.setPen(Qt::white);
            QFont font = painter.font();
            font.setPointSize(80);
            font.setBold(true);
            painter.setFont(font);
            painter.drawText(highlightRect, Qt::AlignCenter, QString::number(m_countdownValue));
        } else if (m_selectionConfirmed) {
            drawResizeHandles(painter, highlightRect);
        } else {
            // Draw corner dots
            int handleSize = 6;
            painter.setBrush(themeColor);
            painter.drawRect(highlightRect.left() - handleSize/2, highlightRect.top() - handleSize/2, handleSize, handleSize);
            painter.drawRect(highlightRect.right() - handleSize/2, highlightRect.top() - handleSize/2, handleSize, handleSize);
            painter.drawRect(highlightRect.left() - handleSize/2, highlightRect.bottom() - handleSize/2, handleSize, handleSize);
            painter.drawRect(highlightRect.right() - handleSize/2, highlightRect.bottom() - handleSize/2, handleSize, handleSize);
        }

        // 4. Draw dimensions tooltip
        if (!(m_selectionConfirmed && m_mode == CaptureMode::VideoRegion)) {
            drawDimensionsTooltip(painter, highlightRect);
        }
    } else {
        // No selection yet, fill entire screen with mask
        painter.fillRect(rect(), maskColor);
    }

    // 5. Draw magnifier loupe near mouse cursor during aiming or resizing/selecting
    if (!m_isCountingDown && (!m_selectionConfirmed || m_isSelecting || m_activeHandle != 0)) {
        drawMagnifier(painter, m_currentMousePos);
    }
}

void RegionSelectWidget::drawDimensionsTooltip(QPainter& painter, const QRect& rect) {
    QString dimText = QString("%1 × %2").arg(rect.width()).arg(rect.height());
    QFont font = painter.font();
    font.setPointSize(10);
    font.setBold(true);
    painter.setFont(font);

    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(dimText);
    int textHeight = fm.height();
    
    QRect tooltipRect(rect.left(), rect.top() - textHeight - 12, textWidth + 16, textHeight + 8);
    if (tooltipRect.top() < 0) {
        tooltipRect.moveTop(rect.top() + 8);
    }
    if (tooltipRect.right() > width()) {
        tooltipRect.moveRight(rect.right());
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(25, 28, 36, 230));
    painter.drawRoundedRect(tooltipRect, 4, 4);

    painter.setPen(Qt::white);
    painter.drawText(tooltipRect, Qt::AlignCenter, dimText);
}

void RegionSelectWidget::drawMagnifier(QPainter& painter, const QPoint& pos) {
    // Draw full screen crosshair guidelines
    painter.save();
    QColor themeColor = (m_mode == CaptureMode::VideoRegion) ? QColor(235, 50, 50, 180) : QColor(0, 168, 255, 180);
    painter.setPen(QPen(themeColor, 1, Qt::DashLine));
    painter.drawLine(0, pos.y(), width(), pos.y());
    painter.drawLine(pos.x(), 0, pos.x(), height());
    painter.restore();

    int loupeSize = 120;
    int zoomFactor = 6;
    
    QPoint loupePos = pos + QPoint(20, 20);
    if (loupePos.x() + loupeSize > width()) {
        loupePos.setX(pos.x() - loupeSize - 20);
    }
    if (loupePos.y() + loupeSize > height()) {
        loupePos.setY(pos.y() - loupeSize - 20);
    }

    painter.save();
    QPainterPath clipPath;
    clipPath.addRoundedRect(loupePos.x(), loupePos.y(), loupeSize, loupeSize, loupeSize/2, loupeSize/2);
    painter.setClipPath(clipPath);

    qreal dpr = m_desktopSnapshot.devicePixelRatioF();
    if (dpr <= 0) dpr = 1.0;

    QRectF targetRect(loupePos.x(), loupePos.y(), loupeSize, loupeSize);
    qreal logicalSourceSize = loupeSize / (qreal)zoomFactor;
    QRectF sourceRect((pos.x() - logicalSourceSize / 2.0) * dpr,
                      (pos.y() - logicalSourceSize / 2.0) * dpr,
                      logicalSourceSize * dpr,
                      logicalSourceSize * dpr);

    // Disable smooth pixmap transform for crisp pixelation inside loupe
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.drawPixmap(targetRect, m_desktopSnapshot, sourceRect);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // Draw center pixel box and crosshairs inside loupe
    qreal logicalPixelSize = loupeSize / logicalSourceSize;
    if (logicalPixelSize < 1.0) logicalPixelSize = 1.0;
    QRectF centerBox(loupePos.x() + (loupeSize - logicalPixelSize) / 2.0,
                     loupePos.y() + (loupeSize - logicalPixelSize) / 2.0,
                     logicalPixelSize, logicalPixelSize);
    painter.setPen(QPen(themeColor, 1.5));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(centerBox);

    painter.setPen(QPen(QColor(255, 255, 255, 180), 1));
    painter.drawLine(loupePos.x() + loupeSize/2, loupePos.y(), loupePos.x() + loupeSize/2, qRound(centerBox.top()));
    painter.drawLine(loupePos.x() + loupeSize/2, qRound(centerBox.bottom()), loupePos.x() + loupeSize/2, loupePos.y() + loupeSize);
    painter.drawLine(loupePos.x(), loupePos.y() + loupeSize/2, qRound(centerBox.left()), loupePos.y() + loupeSize/2);
    painter.drawLine(qRound(centerBox.right()), loupePos.y() + loupeSize/2, loupePos.x() + loupeSize, loupePos.y() + loupeSize/2);
    painter.restore();

    // Draw border around loupe
    painter.setPen(QPen(QColor(255, 255, 255, 220), 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(loupePos.x(), loupePos.y(), loupeSize, loupeSize, loupeSize/2, loupeSize/2);

    // Draw RGB hex tooltip below loupe
    QImage img = m_desktopSnapshot.toImage();
    int physX = qBound(0, qRound(pos.x() * dpr), img.width() - 1);
    int physY = qBound(0, qRound(pos.y() * dpr), img.height() - 1);
    QRgb rgb = img.pixel(physX, physY);
    QString hexColor = QString("#%1%2%3")
        .arg(qRed(rgb), 2, 16, QChar('0'))
        .arg(qGreen(rgb), 2, 16, QChar('0'))
        .arg(qBlue(rgb), 2, 16, QChar('0')).toUpper();

    QRect colorRect(loupePos.x(), loupePos.y() + loupeSize + 4, loupeSize, 22);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(25, 28, 36, 230));
    painter.drawRoundedRect(colorRect, 4, 4);

    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPointSize(8);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(colorRect, Qt::AlignCenter, hexColor);
}

void RegionSelectWidget::mousePressEvent(QMouseEvent* event) {
    if (m_isCountingDown || m_isVideoRecordingActive || (CaptureEngine::instance()->videoRecorder() && CaptureEngine::instance()->videoRecorder()->isRecording())) {
        return;
    }
    if (event->button() == Qt::LeftButton) {
        m_lastSelectionMousePos = event->pos();
        if (m_selectionConfirmed) {
            m_activeHandle = hitTestHandle(event->pos());
            if (m_activeHandle != 0) {
                m_dragStartPos = event->pos();
                m_dragStartRect = m_selectedRect;
                return;
            } else {
                // Clicking outside: reset confirmed selection and start selecting anew
                m_selectionConfirmed = false;
                if (m_toolbar) m_toolbar->hide();
                m_isSelecting = true;
                m_startPoint = event->pos();
                m_currentMousePos = event->pos();
                updateHoveredWindow(event->pos());
                update();
                return;
            }
        }
        if (m_mode == CaptureMode::Window && m_hoveredWindow.isValid) {
            m_selectedRect = m_hoveredWindow.rect;
            m_selectionConfirmed = true;
            ensureToolbarAndShow();
            return;
        }
        m_isSelecting = true;
        m_startPoint = event->pos();
        m_currentMousePos = event->pos();
        updateHoveredWindow(event->pos());
        update();
    } else if (event->button() == Qt::RightButton) {
        if (m_selectionConfirmed) {
            m_selectionConfirmed = false;
            if (m_toolbar) m_toolbar->hide();
            update();
        } else {
            emit cancelled();
        }
    }
}

void RegionSelectWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_isCountingDown || m_isVideoRecordingActive || (CaptureEngine::instance()->videoRecorder() && CaptureEngine::instance()->videoRecorder()->isRecording())) {
        setCursor(Qt::ArrowCursor);
        return;
    }
    m_currentMousePos = event->pos();
    if (m_isSelecting || (m_mode == CaptureMode::Window || m_mode == CaptureMode::Region || m_mode == CaptureMode::VideoRegion)) {
        m_lastSelectionMousePos = event->pos();
    }
    if (m_selectionConfirmed && m_activeHandle != 0) {
        QPoint delta = event->pos() - m_dragStartPos;
        QRect newRect = m_dragStartRect;
        switch (m_activeHandle) {
            case 9: newRect.translate(delta); break;
            case 1: newRect.setTopLeft(m_dragStartRect.topLeft() + delta); break;
            case 2: newRect.setTopRight(m_dragStartRect.topRight() + delta); break;
            case 3: newRect.setBottomLeft(m_dragStartRect.bottomLeft() + delta); break;
            case 4: newRect.setBottomRight(m_dragStartRect.bottomRight() + delta); break;
            case 5: newRect.setTop(m_dragStartRect.top() + delta.y()); break;
            case 6: newRect.setBottom(m_dragStartRect.bottom() + delta.y()); break;
            case 7: newRect.setLeft(m_dragStartRect.left() + delta.x()); break;
            case 8: newRect.setRight(m_dragStartRect.right() + delta.x()); break;
            default: break;
        }
        m_selectedRect = newRect.normalized();
        updateToolbarPosition();
        update();
        return;
    } else if (m_selectionConfirmed) {
        int h = hitTestHandle(event->pos());
        updateCursorForHandle(h);
    } else if (m_isSelecting) {
        m_endPoint = event->pos();
        m_selectedRect = QRect(m_startPoint, m_endPoint).normalized();
        updateHoveredWindow(event->pos());
    } else if (m_mode == CaptureMode::Window || m_mode == CaptureMode::Region || m_mode == CaptureMode::VideoRegion) {
        updateHoveredWindow(event->pos());
    }
    update();
}

void RegionSelectWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_lastSelectionMousePos = event->pos();
        if (m_isSelecting) {
            m_isSelecting = false;
            m_endPoint = event->pos();
            QRect draggedRect = QRect(m_startPoint, m_endPoint).normalized();
            if ((m_endPoint - m_startPoint).manhattanLength() > 10 && (draggedRect.width() > 10 || draggedRect.height() > 10)) {
                // 點一下並拖曳 為自訂大小
                m_selectedRect = draggedRect;
            } else if (m_hoveredWindow.isValid && !m_hoveredWindow.rect.isEmpty()) {
                // 點一下選取範圍 若有視窗自動取取視窗尺寸
                m_selectedRect = m_hoveredWindow.rect;
            } else {
                // 若無視窗 則全螢幕
                m_selectedRect = rect();
            }
            m_selectionConfirmed = true;
            ensureToolbarAndShow();
        } else if (m_selectionConfirmed && m_activeHandle != 0) {
            m_activeHandle = 0;
            updateToolbarPosition();
            update();
        }
    }
}

void RegionSelectWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_selectionConfirmed && m_selectedRect.contains(event->pos())) {
        m_lastSelectionMousePos = event->pos();
        executeConfirm();
    }
}

void RegionSelectWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        emit cancelled();
    } else if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) && m_selectionConfirmed) {
        executeConfirm();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void RegionSelectWidget::updateHoveredWindow(const QPoint& pos) {
    QPoint globalPos = mapToGlobal(pos);
    m_hoveredWindow = CaptureEngine::instance()->findWindowAt(globalPos);
}

int RegionSelectWidget::hitTestHandle(const QPoint& pos) const {
    if (!m_selectionConfirmed) return 0;
    const int s = 10;
    QRect r = m_selectedRect;
    if (QRect(r.topLeft() - QPoint(s/2, s/2), QSize(s, s)).contains(pos)) return 1;
    if (QRect(r.topRight() - QPoint(s/2, s/2), QSize(s, s)).contains(pos)) return 2;
    if (QRect(r.bottomLeft() - QPoint(s/2, s/2), QSize(s, s)).contains(pos)) return 3;
    if (QRect(r.bottomRight() - QPoint(s/2, s/2), QSize(s, s)).contains(pos)) return 4;
    if (QRect(r.left() + s, r.top() - s/2, r.width() - 2*s, s).contains(pos)) return 5;
    if (QRect(r.left() + s, r.bottom() - s/2, r.width() - 2*s, s).contains(pos)) return 6;
    if (QRect(r.left() - s/2, r.top() + s, s, r.height() - 2*s).contains(pos)) return 7;
    if (QRect(r.right() - s/2, r.top() + s, s, r.height() - 2*s).contains(pos)) return 8;
    if (r.contains(pos)) return 9;
    return 0;
}

void RegionSelectWidget::updateCursorForHandle(int handle) {
    switch (handle) {
        case 1: case 4: setCursor(Qt::SizeFDiagCursor); break;
        case 2: case 3: setCursor(Qt::SizeBDiagCursor); break;
        case 5: case 6: setCursor(Qt::SizeVerCursor); break;
        case 7: case 8: setCursor(Qt::SizeHorCursor); break;
        case 9: setCursor(Qt::SizeAllCursor); break;
        default: setCursor(Qt::ArrowCursor); break;
    }
}

void RegionSelectWidget::drawResizeHandles(QPainter& painter, const QRect& rect) {
    if (!m_selectionConfirmed) return;
    painter.save();
    QColor handleColor = (m_mode == CaptureMode::VideoRegion) ? QColor(235, 50, 50) : QColor("#00a8ff");
    painter.setBrush(handleColor);
    painter.setPen(QPen(Qt::white, 1));
    const int s = 8;
    painter.drawRect(rect.left() - s/2, rect.top() - s/2, s, s);
    painter.drawRect(rect.right() - s/2, rect.top() - s/2, s, s);
    painter.drawRect(rect.left() - s/2, rect.bottom() - s/2, s, s);
    painter.drawRect(rect.right() - s/2, rect.bottom() - s/2, s, s);
    painter.drawRect(rect.center().x() - s/2, rect.top() - s/2, s, s);
    painter.drawRect(rect.center().x() - s/2, rect.bottom() - s/2, s, s);
    painter.drawRect(rect.left() - s/2, rect.center().y() - s/2, s, s);
    painter.drawRect(rect.right() - s/2, rect.center().y() - s/2, s, s);
    painter.restore();
}

void RegionSelectWidget::updateToolbarPosition() {
    if (!m_toolbar || !m_selectionConfirmed) return;
    m_toolbar->updateTargetDimensions(m_selectedRect.width(), m_selectedRect.height());
    m_toolbar->adjustSize();
    int tbWidth = m_toolbar->width();
    int tbHeight = m_toolbar->height();

    int x = m_selectedRect.right() - tbWidth + 1;
    int y = m_selectedRect.bottom() + 8;

    // 1. If placing below m_selectedRect goes past screen bottom minus margin (e.g., touching taskbar/system bar),
    // try placing above m_selectedRect.
    if (y + tbHeight > height() - 8) {
        if (m_selectedRect.top() - tbHeight - 8 >= 8) {
            y = m_selectedRect.top() - tbHeight - 8;
        } else {
            // 2. If neither above nor below fits outside m_selectedRect (e.g. full screen or window touching top & bottom),
            // place inside the bottom-right of m_selectedRect.
            y = m_selectedRect.bottom() - tbHeight - 8;
        }
    }

    // 3. Absolute clamp against screen bounds so toolbar never disappears off screen.
    if (y + tbHeight > height() - 8) {
        y = height() - tbHeight - 8;
    }
    if (y < 8) {
        y = 8;
    }

    if (x + tbWidth > width() - 8) {
        x = width() - tbWidth - 8;
    }
    if (x < 8) {
        x = 8;
    }

    if (m_toolbar->parentWidget() == nullptr) {
        m_toolbar->move(mapToGlobal(QPoint(x, y)));
    } else {
        m_toolbar->move(x, y);
    }
    m_toolbar->raise();

    if (m_statusCard && m_mode == CaptureMode::VideoRegion) {
        m_statusCard->adjustSize();
        int scW = m_statusCard->width();
        int scH = m_statusCard->height();
        
        int scX = m_selectedRect.center().x() - scW / 2;
        int scY = m_selectedRect.bottom() - scH - 15;
        if (scY < m_selectedRect.top() + 5) {
            scY = m_selectedRect.center().y() - scH / 2;
            if (scY < m_selectedRect.top() + 5) scY = m_selectedRect.top() + 5;
        }
        if (scX < m_selectedRect.left() + 5) scX = m_selectedRect.left() + 5;

        if (m_statusCard->parentWidget() == nullptr) {
            m_statusCard->move(mapToGlobal(QPoint(scX, scY)));
        } else {
            m_statusCard->move(scX, scY);
        }
        m_statusCard->show();
        m_statusCard->raise();
    }
}

void RegionSelectWidget::executeEdit() {
    if (m_selectedRect.isValid() && !m_selectedRect.isEmpty()) {
        emit regionSelected(m_selectedRect);
    }
}

void RegionSelectWidget::executeCopy() {
    if (m_selectedRect.isValid() && !m_selectedRect.isEmpty()) {
        emit copyRequested(m_selectedRect);
    }
}

void RegionSelectWidget::executeConfirm() {
    if (m_selectedRect.isValid() && !m_selectedRect.isEmpty()) {
        emit confirmRequested(m_selectedRect);
    }
}

void RegionSelectWidget::ensureToolbarAndShow() {
    if (!m_toolbar) {
        m_toolbar = new CaptureToolBarWidget(this, m_mode);
        connect(m_toolbar, &CaptureToolBarWidget::actionConfirm, this, &RegionSelectWidget::executeConfirm);
        connect(m_toolbar, &CaptureToolBarWidget::actionEdit, this, &RegionSelectWidget::executeEdit);
        connect(m_toolbar, &CaptureToolBarWidget::actionCopy, this, &RegionSelectWidget::executeCopy);
        connect(m_toolbar, &CaptureToolBarWidget::actionSave, this, &RegionSelectWidget::executeSave);
        connect(m_toolbar, &CaptureToolBarWidget::actionPin, this, &RegionSelectWidget::executePin);
        connect(m_toolbar, &CaptureToolBarWidget::actionCancel, this, &RegionSelectWidget::cancelled);
        connect(m_toolbar, &CaptureToolBarWidget::actionVideoRecordStateChanged, this, [this](bool rec) {
            if (rec) {
                startVideoCountdown();
            } else {
                CaptureEngine::instance()->stopVideoRecord();
                if (m_statusCard) m_statusCard->refreshStatus();
            }
        });
        connect(m_toolbar, &CaptureToolBarWidget::actionVideoStop, this, [this]() {
            executeConfirm();
        });
        connect(m_toolbar, &CaptureToolBarWidget::cursorToggled, this, [this](bool checked) {
            CaptureEngine::instance()->setSessionCursorEnabled(checked);
            if (m_statusCard) m_statusCard->refreshStatus();
        });
        connect(m_toolbar, &CaptureToolBarWidget::micToggled, this, [this](bool checked) {
            CaptureEngine::instance()->setSessionMicEnabled(checked);
            if (m_statusCard) m_statusCard->refreshStatus();
        });
        connect(m_toolbar, &CaptureToolBarWidget::sysAudioToggled, this, [this](bool checked) {
            CaptureEngine::instance()->setSessionSysAudioEnabled(checked);
            if (m_statusCard) m_statusCard->refreshStatus();
        });
    }

    if (m_mode == CaptureMode::VideoRegion && !m_statusCard) {
        m_statusCard = new CaptureStatusWidget(this, false);
    }

    m_toolbar->updateTargetDimensions(m_selectedRect.width(), m_selectedRect.height());
    updateToolbarPosition();
    m_toolbar->show();
    m_toolbar->raise();

    if (m_statusCard && m_mode == CaptureMode::VideoRegion) {
        m_statusCard->refreshStatus();
        m_statusCard->show();
        m_statusCard->raise();
    }
    update();
}

void RegionSelectWidget::executeSave() {
    if (m_selectedRect.isValid() && !m_selectedRect.isEmpty()) {
        emit saveRequested(m_selectedRect);
    }
}

void RegionSelectWidget::executePin() {
    if (m_selectedRect.isValid() && !m_selectedRect.isEmpty()) {
        QPixmap cropped = m_desktopSnapshot.copy(m_selectedRect);
        if (CaptureEngine::instance()->isSessionImgCursorEnabled()) {
            QPoint cursorPos = m_lastSelectionMousePos.isNull() ? QCursor::pos() : m_lastSelectionMousePos;
            CaptureEngine::instance()->drawCursorOnRegion(cropped, m_selectedRect.topLeft(), cursorPos);
        }
        PinWidget* pin = new PinWidget(cropped);
        pin->move(m_selectedRect.topLeft());
        pin->show();
        emit cancelled();
    }
}

// ============================================================================
// VideoCaptureWorker & VideoRecorder Implementation (Integrated into CaptureEngine)
// ============================================================================

static QList<HwEncoderInfo> s_cachedHwEncoders;
static bool s_hwDetected = false;
static bool s_hwDetecting = false;
static QMutex s_hwMutex;
static QList<std::function<void(const QList<HwEncoderInfo>&)>> s_detectCallbacks;

QString VideoRecorder::getFFmpegPath() {
    QString appDirExe = QApplication::applicationDirPath() + "/ffmpeg.exe";
    if (QFile::exists(appDirExe)) return appDirExe;

    QString pathExe = QStandardPaths::findExecutable("ffmpeg.exe");
    if (!pathExe.isEmpty()) return pathExe;

    // Check common local cache directories or python environments
    QString localAppData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QDir appDataDir(localAppData + "/imageio_ffmpeg/binaries");
    if (appDataDir.exists()) {
        QStringList entries = appDataDir.entryList(QStringList() << "*ffmpeg*", QDir::Files);
        if (!entries.isEmpty()) {
            return appDataDir.absoluteFilePath(entries.first());
        }
    }

    return "ffmpeg";
}

QList<HwEncoderInfo> VideoRecorder::detectAvailableHwEncoders(bool forceRefresh) {
    s_hwMutex.lock();
    if (s_hwDetected && !forceRefresh) {
        QList<HwEncoderInfo> copy = s_cachedHwEncoders;
        s_hwMutex.unlock();
        return copy;
    }
    s_hwMutex.unlock();

    QList<HwEncoderInfo> tempEncoders;
    QString exe = getFFmpegPath();

#ifdef Q_OS_WIN
    struct Candidate { QString codec; QString name; };
    QList<Candidate> candidates = {
        {"h264_nvenc", "NVIDIA NVENC (GPU) - h264_nvenc"},
        {"h264_amf", "AMD Radeon AMF (GPU) - h264_amf"},
        {"h264_qsv", "Intel QuickSync (GPU) - h264_qsv"},
        {"h264_mf", "Windows MediaFoundation (DXVA) - h264_mf"}
    };
#else
    struct Candidate { QString codec; QString name; };
    QList<Candidate> candidates = {
        {"h264_videotoolbox", "Apple VideoToolbox (GPU) - h264_videotoolbox"},
        {"h264_vaapi", "VAAPI Hardware Acceleration - h264_vaapi"}
    };
#endif

    qDebug() << "[VideoRecorder] Detecting supported hardware encoders using FFmpeg:" << exe;

    for (const auto& cand : candidates) {
        QStringList args;
        args << "-v" << "error"
             << "-f" << "lavfi"
             << "-i" << "nullsrc=s=1280x720:d=0.05"
             << "-pix_fmt" << "yuv420p";

        if (cand.codec == "h264_videotoolbox") {
            args << "-c:v" << cand.codec << "-allow_sw" << "1" << "-f" << "null" << "-";
        } else {
            args << "-c:v" << cand.codec << "-f" << "null" << "-";
        }

        QProcess process;
        process.setProcessChannelMode(QProcess::MergedChannels); // Capture stdout/stderr internally to keep console clean
        process.start(exe, args);
        bool finished = process.waitForFinished(3000);
        int ret = (finished && process.exitStatus() == QProcess::NormalExit) ? process.exitCode() : -1;

        if (ret == 0) {
            qDebug() << "[VideoRecorder] Hardware encoder supported:" << cand.codec << "(" << cand.name << ")";
            tempEncoders.append(HwEncoderInfo{cand.codec, cand.name});
        } else {
            qDebug() << "[VideoRecorder] Hardware encoder not supported on this PC (filtered out):" << cand.codec;
        }
    }

    s_hwMutex.lock();
    s_cachedHwEncoders = tempEncoders;
    s_hwDetected = true;
    s_hwMutex.unlock();

    return tempEncoders;
}

bool VideoRecorder::isHwDetected() {
    QMutexLocker locker(&s_hwMutex);
    return s_hwDetected;
}

void VideoRecorder::detectAvailableHwEncodersAsync(std::function<void(const QList<HwEncoderInfo>&)> callback, bool forceRefresh) {
    s_hwMutex.lock();
    if (s_hwDetected && !forceRefresh) {
        QList<HwEncoderInfo> copy = s_cachedHwEncoders;
        s_hwMutex.unlock();
        if (callback) {
            callback(copy);
        }
        return;
    }

    if (callback) {
        s_detectCallbacks.append(callback);
    }

    if (s_hwDetecting) {
        s_hwMutex.unlock();
        return;
    }

    s_hwDetecting = true;
    s_hwMutex.unlock();

    qDebug() << "[VideoRecorder] Starting asynchronous GPU hardware encoder probing in background thread...";

    QThreadPool::globalInstance()->start([forceRefresh]() {
        QList<HwEncoderInfo> list = detectAvailableHwEncoders(forceRefresh);

        QMetaObject::invokeMethod(qApp, [list]() {
            QMutexLocker l(&s_hwMutex);
            s_hwDetecting = false;
            auto cbs = s_detectCallbacks;
            s_detectCallbacks.clear();
            l.unlock();

            for (auto& cb : cbs) {
                if (cb) cb(list);
            }
        }, Qt::QueuedConnection);
    });
}

VideoCaptureWorker::VideoCaptureWorker(const QRect& rect, const QString& outputPath, bool captureCursor, QObject* parent)
    : QObject(parent), m_rect(rect), m_outputPath(outputPath), m_captureCursor(captureCursor)
{
}

VideoCaptureWorker::~VideoCaptureWorker() {
    if (m_isRunning) {
        stopRecording();
    }
}

QString VideoCaptureWorker::getFFmpegPath() const {
    return VideoRecorder::getFFmpegPath();
}

QString VideoCaptureWorker::selectVideoCodec(bool& outIsHw) const {
    bool hwEnabled = Config::getValue("hw_accel", true).toBool();
    QString hwEncoder = Config::getValue("hw_encoder", "").toString();

    if (!hwEnabled) {
        outIsHw = false;
        return "libx264";
    }

    outIsHw = true;
    if (!hwEncoder.isEmpty() && hwEncoder != "libx264") {
        return hwEncoder;
    }
    // Default recommended Windows hardware encoder: check available or h264_mf
    QList<HwEncoderInfo> avail = VideoRecorder::detectAvailableHwEncoders();
    if (!avail.isEmpty()) {
        return avail.first().codec;
    }
#ifdef Q_OS_WIN
    return "h264_mf";
#else
    return "libx264";
#endif
}

void VideoCaptureWorker::startRecording() {
    if (m_isRunning) return;

    bool limit1080p = Config::getValue("limit_1080p", false).toBool();
    int targetW = m_rect.width();
    int targetH = m_rect.height();
    if (limit1080p && (targetW > 1920 || targetH > 1080)) {
        QSize scaled = QSize(targetW, targetH).scaled(1920, 1080, Qt::KeepAspectRatio);
        targetW = scaled.width();
        targetH = scaled.height();
        qDebug() << "[VideoRecorder] Record Limit 1080p active. Downscaling resolution from" << m_rect.size() << "to" << targetW << "x" << targetH;
    }
    // Ensure even dimensions for yuv420p H.264 video encoding
    targetW = (targetW + 1) & ~1;
    targetH = (targetH + 1) & ~1;
    m_targetSize = QSize(targetW, targetH);

    QString ffmpeg = getFFmpegPath();
    bool isHw = false;
    QString codec = selectVideoCodec(isHw);

    qDebug() << "[VideoRecorder] Starting MP4 recording using ffmpeg:" << ffmpeg << "Codec:" << codec << "(HW acceleration:" << isHw << ") TargetSize:" << m_targetSize;

    // Resolve microphone device for audio capture
    bool micEnabled = CaptureEngine::instance()->isSessionMicEnabled();
    QString micDeviceName; // FFmpeg device name string (empty = default or disabled)
    if (micEnabled) {
        QByteArray savedMicId = Config::getValue("mic_device", QString()).toString().toUtf8();
        if (!savedMicId.isEmpty()) {
            // Find matching device description from Qt Multimedia
            const auto devices = QMediaDevices::audioInputs();
            for (const QAudioDevice& dev : devices) {
                if (dev.id() == savedMicId) {
                    micDeviceName = dev.description();
                    break;
                }
            }
        }
        // If no saved device or no match found, use default device name
        if (micDeviceName.isEmpty()) {
            QAudioDevice defaultDev = QMediaDevices::defaultAudioInput();
            if (!defaultDev.isNull()) {
                micDeviceName = defaultDev.description();
            }
        }
    }

    m_ffmpegProcess = new QProcess(this);
    QStringList args;

    // Build audio input args (before the video pipe input)
    if (micEnabled && !micDeviceName.isEmpty()) {
        args.append(Platform::getAudioInputArgs(micDeviceName));
        qDebug() << "[VideoRecorder] Microphone audio input enabled. Device:" << micDeviceName;
    } else if (micEnabled) {
        qDebug() << "[VideoRecorder] Microphone enabled but no device found. Recording video only.";
    } else {
        qDebug() << "[VideoRecorder] Microphone disabled. Recording video only (no audio).";
    }

    args << "-y"
         << "-f" << "rawvideo"
         << "-vcodec" << "rawvideo"
         << "-s" << QString("%1x%2").arg(m_targetSize.width()).arg(m_targetSize.height())
         << "-pix_fmt" << "bgra"
         << "-r" << "30"
         << "-i" << "-"
         << "-c:v" << codec
         << "-pix_fmt" << "yuv420p";

    if (isHw && codec == "h264_nvenc") {
        args << "-preset" << "p1" << "-cq" << "23";
    } else if (!isHw || codec == "libx264") {
        args << "-preset" << "ultrafast" << "-crf" << "23";
    }

    // Map streams: video from pipe, audio from mic (if any)
    if (micEnabled && !micDeviceName.isEmpty()) {
        args << "-c:a" << "aac" << "-b:a" << "128k";
    }

    args << m_outputPath;

    m_ffmpegProcess->start(ffmpeg, args);
    if (!m_ffmpegProcess->waitForStarted(3000)) {
        qWarning() << "[VideoRecorder] Failed to start ffmpeg process:" << m_ffmpegProcess->errorString();
        // If specific hw codec failed, attempt software compression fallback
        if (isHw) {
            qDebug() << "[VideoRecorder] Hardware acceleration failed to start, falling back to software libx264...";
            m_ffmpegProcess->deleteLater();
            m_ffmpegProcess = new QProcess(this);
            args.clear();
            args << "-y" << "-f" << "rawvideo" << "-vcodec" << "rawvideo"
                 << "-s" << QString("%1x%2").arg(m_targetSize.width()).arg(m_targetSize.height())
                 << "-pix_fmt" << "bgra" << "-r" << "30" << "-i" << "-"
                 << "-c:v" << "libx264" << "-preset" << "ultrafast" << "-crf" << "23" << "-pix_fmt" << "yuv420p"
                 << m_outputPath;
            m_ffmpegProcess->start(ffmpeg, args);
            if (!m_ffmpegProcess->waitForStarted(3000)) {
                emit errorOccurred("Cannot start FFmpeg process for MP4 encoding. Please check ffmpeg installation.");
                return;
            }
        } else {
            emit errorOccurred("Cannot start FFmpeg process for MP4 encoding. Please check ffmpeg installation.");
            return;
        }
    }

    m_isRunning = true;
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &VideoCaptureWorker::captureFrame);
    m_timer->start(static_cast<int>(m_frameIntervalMs));
}

void VideoCaptureWorker::captureFrame() {
    if (!m_isRunning || !m_ffmpegProcess || m_ffmpegProcess->state() != QProcess::Running) {
        return;
    }

    QScreen* screen = QGuiApplication::primaryScreen();
    if (!screen) return;

    QPixmap snapshot = screen->grabWindow(0, m_rect.x(), m_rect.y(), m_rect.width(), m_rect.height());
    if (m_captureCursor) {
        CaptureEngine::instance()->drawCursorOnRegion(snapshot, m_rect.topLeft(), QCursor::pos());
    }

    QImage img = snapshot.toImage().convertToFormat(QImage::Format_ARGB32);
    if (img.size() != m_targetSize) {
        img = img.scaled(m_targetSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    m_ffmpegProcess->write(reinterpret_cast<const char*>(img.constBits()), img.sizeInBytes());
}

void VideoCaptureWorker::stopRecording() {
    if (!m_isRunning) return;
    m_isRunning = false;

    if (m_timer) {
        m_timer->stop();
        m_timer->deleteLater();
        m_timer = nullptr;
    }

    if (m_ffmpegProcess) {
        if (m_ffmpegProcess->state() == QProcess::Running) {
            m_ffmpegProcess->closeWriteChannel();
            if (!m_ffmpegProcess->waitForFinished(5000)) {
                m_ffmpegProcess->kill();
            }
        }
        m_ffmpegProcess->deleteLater();
        m_ffmpegProcess = nullptr;
    }

    qDebug() << "[VideoRecorder] Recording completed and saved to:" << m_outputPath;
    emit finished(m_outputPath);
}

// --- VideoRecorder Controller ---

VideoRecorder::VideoRecorder(QObject* parent) : QObject(parent) {
}

VideoRecorder::~VideoRecorder() {
    if (m_isRecording) {
        stop();
    }
}

void VideoRecorder::start(const QRect& rect) {
    if (m_isRecording) return;

    QString libraryDir = ScutProject::getLibraryDir();
    QString timeStr = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString outputPath = QDir(libraryDir).filePath("ScreenCut_Video_" + timeStr + ".mp4");

    bool captureCursor = CaptureEngine::instance()->isSessionCursorEnabled();

    m_workerThread = new QThread(this);
    m_worker = new VideoCaptureWorker(rect, outputPath, captureCursor);
    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::started, m_worker, &VideoCaptureWorker::startRecording);
    connect(m_worker, &VideoCaptureWorker::finished, this, [this](const QString& path) {
        m_isRecording = false;
        if (m_workerThread) {
            m_workerThread->quit();
            m_workerThread->wait();
            m_workerThread->deleteLater();
            m_workerThread = nullptr;
        }
        if (m_worker) {
            m_worker->deleteLater();
            m_worker = nullptr;
        }
        emit recordingFinished(path);
    });
    connect(m_worker, &VideoCaptureWorker::errorOccurred, this, [this](const QString& err) {
        m_isRecording = false;
        if (m_workerThread) {
            m_workerThread->quit();
            m_workerThread->wait();
            m_workerThread->deleteLater();
            m_workerThread = nullptr;
        }
        if (m_worker) {
            m_worker->deleteLater();
            m_worker = nullptr;
        }
        emit recordingError(err);
    });

    m_isRecording = true;
    m_workerThread->start();
}

void VideoRecorder::stop() {
    if (!m_isRecording || !m_worker) return;
    QMetaObject::invokeMethod(m_worker, "stopRecording", Qt::QueuedConnection);
}

} // namespace ScreenCut
