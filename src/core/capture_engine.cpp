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

#include "../widgets/capture_toolbar.h"
#include "../widgets/capture_pin.h"
#include <QClipboard>
#include <QFileDialog>
#include <QStandardPaths>
#include "common_project.h"
#include "../widgets/capture_countdown.h"
#include "../widgets/capture_scroll.h"

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
}

CaptureEngine::~CaptureEngine() {
    if (m_overlayWidget) {
        m_overlayWidget->deleteLater();
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
    m_fullDesktopSnapshot = captureAllScreens();
    if (mode == CaptureMode::FullScreen && CaptureMainWindow::instance() && CaptureMainWindow::instance()->isSettingEnabled("Image_Capture Cursor")) {
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
    m_overlayWidget->showFullScreen();
    m_overlayWidget->activateWindow();
    m_overlayWidget->setFocus();
}

void CaptureEngine::startRegionCapture() {
    startCapture(CaptureMode::Region);
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

    if (CaptureMainWindow::instance() && CaptureMainWindow::instance()->isSettingEnabled("Image_Capture Cursor")) {
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

    QPixmap combined(virtualGeometry.size());
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

WindowInfo CaptureEngine::findWindowAt(const QPoint& globalPos) {
    WindowInfo info;
#ifdef Q_OS_WIN
    POINT pt = { globalPos.x(), globalPos.y() };
    HWND hwnd = WindowFromPoint(pt);
    if (!hwnd) return info;

    // Get top-level window or ancestor
    HWND rootHwnd = GetAncestor(hwnd, GA_ROOT);
    if (rootHwnd) {
        hwnd = rootHwnd;
    }

    // Skip invisible or minimized windows
    if (!IsWindowVisible(hwnd) || IsIconic(hwnd)) {
        return info;
    }

    // Try to get DWM frame bounds (excluding invisible drop shadow borders in Win10/11)
    RECT rect;
    HRESULT hr = DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rect, sizeof(rect));
    if (FAILED(hr)) {
        GetWindowRect(hwnd, &rect);
    }

    info.rect = QRect(QPoint(rect.left, rect.top), QPoint(rect.right - 1, rect.bottom - 1));
    
    wchar_t titleBuf[512] = {0};
    GetWindowTextW(hwnd, titleBuf, 511);
    info.title = QString::fromWCharArray(titleBuf);
    info.isValid = !info.rect.isEmpty() && info.rect.width() > 10 && info.rect.height() > 10;
#else
    // Fallback for non-Windows or when Win32 detection fails
    info.rect = QRect(0, 0, m_fullDesktopSnapshot.width(), m_fullDesktopSnapshot.height());
    info.isValid = true;
#endif
    return info;
}

std::vector<WindowInfo> CaptureEngine::getAllWindows() {
    std::vector<WindowInfo> windows;
    // Can be expanded with EnumWindows on Win32 or CGWindowList on macOS
    return windows;
}

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
}

RegionSelectWidget::~RegionSelectWidget() = default;

void RegionSelectWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // 1. Draw base desktop snapshot
    painter.drawPixmap(0, 0, m_desktopSnapshot);

    // 2. Draw semi-transparent dark mask over the entire screen
    QColor maskColor(0, 0, 0, 120);
    
    QRect highlightRect;
    if (m_selectionConfirmed && m_selectedRect.isValid()) {
        highlightRect = m_selectedRect;
    } else if (m_isSelecting) {
        highlightRect = QRect(m_startPoint, m_currentMousePos).normalized();
    } else if (m_mode == CaptureMode::Window && m_hoveredWindow.isValid) {
        highlightRect = m_hoveredWindow.rect;
    }

    if (highlightRect.isValid() && !highlightRect.isEmpty()) {
        // Draw mask excluding the highlighted region using QPainterPath
        QPainterPath path;
        path.addRect(rect());
        path.addRect(highlightRect);
        painter.fillPath(path, maskColor);

        // 3. Draw vibrant border around highlighted area
        QPen borderPen(QColor(0, 168, 255), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(borderPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(highlightRect);

        if (m_selectionConfirmed) {
            drawResizeHandles(painter, highlightRect);
        } else {
            // Draw corner dots
            int handleSize = 6;
            painter.setBrush(QColor(0, 168, 255));
            painter.drawRect(highlightRect.left() - handleSize/2, highlightRect.top() - handleSize/2, handleSize, handleSize);
            painter.drawRect(highlightRect.right() - handleSize/2, highlightRect.top() - handleSize/2, handleSize, handleSize);
            painter.drawRect(highlightRect.left() - handleSize/2, highlightRect.bottom() - handleSize/2, handleSize, handleSize);
            painter.drawRect(highlightRect.right() - handleSize/2, highlightRect.bottom() - handleSize/2, handleSize, handleSize);
        }

        // 4. Draw dimensions tooltip
        drawDimensionsTooltip(painter, highlightRect);
    } else {
        // No selection yet, fill entire screen with mask
        painter.fillRect(rect(), maskColor);
    }

    // 5. Draw magnifier loupe near mouse cursor only during aiming/selecting
    if (!m_selectionConfirmed && (!m_isSelecting || (m_startPoint - m_currentMousePos).manhattanLength() < 500)) {
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
    int loupeSize = 120;
    int zoomFactor = 3;
    
    QPoint loupePos = pos + QPoint(20, 20);
    if (loupePos.x() + loupeSize > width()) {
        loupePos.setX(pos.x() - loupeSize - 20);
    }
    if (loupePos.y() + loupeSize > height()) {
        loupePos.setY(pos.y() - loupeSize - 20);
    }

    QRect sourceRect(pos.x() - (loupeSize / zoomFactor / 2),
                     pos.y() - (loupeSize / zoomFactor / 2),
                     loupeSize / zoomFactor,
                     loupeSize / zoomFactor);

    QPixmap zoomed = m_desktopSnapshot.copy(sourceRect).scaled(loupeSize, loupeSize, Qt::KeepAspectRatio, Qt::FastTransformation);

    painter.save();
    QPainterPath clipPath;
    clipPath.addRoundedRect(loupePos.x(), loupePos.y(), loupeSize, loupeSize, loupeSize/2, loupeSize/2);
    painter.setClipPath(clipPath);

    painter.drawPixmap(loupePos, zoomed);

    // Draw center crosshair
    painter.setPen(QPen(QColor(0, 168, 255, 180), 1));
    painter.drawLine(loupePos.x() + loupeSize/2, loupePos.y(), loupePos.x() + loupeSize/2, loupePos.y() + loupeSize);
    painter.drawLine(loupePos.x(), loupePos.y() + loupeSize/2, loupePos.x() + loupeSize, loupePos.y() + loupeSize/2);
    painter.restore();

    // Draw border around loupe
    painter.setPen(QPen(QColor(255, 255, 255, 220), 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(loupePos.x(), loupePos.y(), loupeSize, loupeSize, loupeSize/2, loupeSize/2);

    // Draw RGB hex tooltip below loupe
    QRgb rgb = m_desktopSnapshot.toImage().pixel(pos);
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
                update();
                return;
            }
        }
        if (m_mode == CaptureMode::Window && m_hoveredWindow.isValid) {
            m_selectedRect = m_hoveredWindow.rect;
            m_selectionConfirmed = true;
            if (!m_toolbar) {
                m_toolbar = new CaptureToolBarWidget(this);
                connect(m_toolbar, &CaptureToolBarWidget::actionConfirm, this, &RegionSelectWidget::executeConfirm);
                connect(m_toolbar, &CaptureToolBarWidget::actionEdit, this, &RegionSelectWidget::executeEdit);
                connect(m_toolbar, &CaptureToolBarWidget::actionCopy, this, &RegionSelectWidget::executeCopy);
                connect(m_toolbar, &CaptureToolBarWidget::actionSave, this, &RegionSelectWidget::executeSave);
                connect(m_toolbar, &CaptureToolBarWidget::actionPin, this, &RegionSelectWidget::executePin);
                connect(m_toolbar, &CaptureToolBarWidget::actionCancel, this, &RegionSelectWidget::cancelled);
            }
            updateToolbarPosition();
            m_toolbar->show();
            m_toolbar->raise();
            update();
            return;
        }
        m_isSelecting = true;
        m_startPoint = event->pos();
        m_currentMousePos = event->pos();
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
    m_currentMousePos = event->pos();
    if (m_isSelecting || (m_mode == CaptureMode::Window || m_mode == CaptureMode::Region)) {
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
    } else if (m_mode == CaptureMode::Window || m_mode == CaptureMode::Region) {
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
            m_selectedRect = QRect(m_startPoint, m_endPoint).normalized();
            if (m_selectedRect.width() > 4 && m_selectedRect.height() > 4) {
                m_selectionConfirmed = true;
                if (!m_toolbar) {
                    m_toolbar = new CaptureToolBarWidget(this);
                    connect(m_toolbar, &CaptureToolBarWidget::actionConfirm, this, &RegionSelectWidget::executeConfirm);
                    connect(m_toolbar, &CaptureToolBarWidget::actionEdit, this, &RegionSelectWidget::executeEdit);
                    connect(m_toolbar, &CaptureToolBarWidget::actionCopy, this, &RegionSelectWidget::executeCopy);
                    connect(m_toolbar, &CaptureToolBarWidget::actionSave, this, &RegionSelectWidget::executeSave);
                    connect(m_toolbar, &CaptureToolBarWidget::actionPin, this, &RegionSelectWidget::executePin);
                    connect(m_toolbar, &CaptureToolBarWidget::actionCancel, this, &RegionSelectWidget::cancelled);
                }
                updateToolbarPosition();
                m_toolbar->show();
                m_toolbar->raise();
                update();
            } else if (m_hoveredWindow.isValid) {
                m_selectedRect = m_hoveredWindow.rect;
                m_selectionConfirmed = true;
                if (!m_toolbar) {
                    m_toolbar = new CaptureToolBarWidget(this);
                    connect(m_toolbar, &CaptureToolBarWidget::actionConfirm, this, &RegionSelectWidget::executeConfirm);
                    connect(m_toolbar, &CaptureToolBarWidget::actionEdit, this, &RegionSelectWidget::executeEdit);
                    connect(m_toolbar, &CaptureToolBarWidget::actionCopy, this, &RegionSelectWidget::executeCopy);
                    connect(m_toolbar, &CaptureToolBarWidget::actionSave, this, &RegionSelectWidget::executeSave);
                    connect(m_toolbar, &CaptureToolBarWidget::actionPin, this, &RegionSelectWidget::executePin);
                    connect(m_toolbar, &CaptureToolBarWidget::actionCancel, this, &RegionSelectWidget::cancelled);
                }
                updateToolbarPosition();
                m_toolbar->show();
                m_toolbar->raise();
                update();
            }
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
    m_hoveredWindow = CaptureEngine::instance()->findWindowAt(pos);
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
    painter.setBrush(QColor("#00a8ff"));
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
    m_toolbar->adjustSize();
    int tbWidth = m_toolbar->width();
    int tbHeight = m_toolbar->height();
    int x = m_selectedRect.right() - tbWidth;
    int y = m_selectedRect.bottom() + 8;
    if (x + tbWidth > width() - 8) x = width() - tbWidth - 8;
    if (x < 8) x = 8;
    if (y + tbHeight > height() - 8) {
        y = m_selectedRect.top() - tbHeight - 8;
        if (y < 8) y = m_selectedRect.bottom() - tbHeight - 8;
    }
    m_toolbar->move(x, y);
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

void RegionSelectWidget::executeSave() {
    if (m_selectedRect.isValid() && !m_selectedRect.isEmpty()) {
        emit saveRequested(m_selectedRect);
    }
}

void RegionSelectWidget::executePin() {
    if (m_selectedRect.isValid() && !m_selectedRect.isEmpty()) {
        QPixmap cropped = m_desktopSnapshot.copy(m_selectedRect);
        if (CaptureMainWindow::instance() && CaptureMainWindow::instance()->isSettingEnabled("Image_Capture Cursor")) {
            QPoint cursorPos = m_lastSelectionMousePos.isNull() ? QCursor::pos() : m_lastSelectionMousePos;
            CaptureEngine::instance()->drawCursorOnRegion(cropped, m_selectedRect.topLeft(), cursorPos);
        }
        PinWidget* pin = new PinWidget(cropped);
        pin->move(m_selectedRect.topLeft());
        pin->show();
        emit cancelled();
    }
}

} // namespace ScreenCut
