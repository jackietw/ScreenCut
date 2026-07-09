/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef CAPTURE_ENGINE_H
#define CAPTURE_ENGINE_H

#include <QObject>
#include <QPixmap>
#include <QRect>
#include <QScreen>
#include <QWidget>
#include <QPoint>
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QTimer>
#include <QFrame>
#include <memory>
#include <vector>

namespace ScreenCut {

enum class CaptureMode {
    Region,
    Window,
    Scrolling,
    FullScreen
};

struct WindowInfo {
    QRect rect;
    QString title;
    QString processName;
    bool isValid = false;
};

class RegionSelectWidget;

class CaptureEngine : public QObject {
    Q_OBJECT
public:
    static CaptureEngine* instance();

    void startCapture(CaptureMode mode = CaptureMode::Region);
    void startRegionCapture();
    void startWindowCapture();
    void startScrollingCapture();
    void startFullScreenCapture();

    QPixmap captureRect(const QRect& rect);
    QPixmap captureScreen(QScreen* screen = nullptr);
    QPixmap captureAllScreens();
    void drawCursorOnSnapshot(QPixmap& snapshot);
    void drawCursorOnRegion(QPixmap& target, const QPoint& targetTopLeftGlobal, const QPoint& cursorGlobalPos);

    // Window detection capabilities
    WindowInfo findWindowAt(const QPoint& globalPos);
    std::vector<WindowInfo> getAllWindows();

signals:
    void captureStarted();
    void captureCompleted(const QPixmap& capturedPixmap);
    void captureEdited(const QPixmap& capturedPixmap);
    void captureCopied(const QPixmap& capturedPixmap);
    void captureSaved(const QString& filePath);
    void captureCancelled();
    void scrollingProgress(int currentHeight, int maxExpectedHeight);

private:
    explicit CaptureEngine(QObject* parent = nullptr);
    ~CaptureEngine() override;

    void doActualCapture(CaptureMode mode);

    static CaptureEngine* s_instance;
    QPixmap m_fullDesktopSnapshot;
    class RegionSelectWidget* m_overlayWidget = nullptr;
    CaptureMode m_currentMode = CaptureMode::Region;
    bool m_isPendingCapture = false;
};

class CaptureToolBarWidget;
class PinWidget;

// Overlay widget for selecting capture regions and windows
class RegionSelectWidget : public QWidget {
    Q_OBJECT
public:
    explicit RegionSelectWidget(const QPixmap& desktopSnapshot, CaptureMode mode, QWidget* parent = nullptr);
    ~RegionSelectWidget() override;

    QPoint lastSelectionMousePos() const { return m_lastSelectionMousePos; }

signals:
    void regionSelected(const QRect& rect);
    void copyRequested(const QRect& rect);
    void saveRequested(const QRect& rect);
    void confirmRequested(const QRect& rect);
    void cancelled();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void updateHoveredWindow(const QPoint& pos);
    void drawMagnifier(QPainter& painter, const QPoint& pos);
    void drawDimensionsTooltip(QPainter& painter, const QRect& rect);
    void updateToolbarPosition();
    void updateCursorForHandle(int handle);
    int hitTestHandle(const QPoint& pos) const;
    void drawResizeHandles(QPainter& painter, const QRect& rect);
    void executeCopy();
    void executeSave();
    void executePin();
    void executeEdit();
    void executeConfirm();

    QPixmap m_desktopSnapshot;
    CaptureMode m_mode;
    bool m_isSelecting = false;
    bool m_selectionConfirmed = false;
    int m_activeHandle = 0; // 0=None, 1-8=Handles, 9=Move
    QPoint m_startPoint;
    QPoint m_endPoint;
    QPoint m_currentMousePos;
    QPoint m_dragStartPos;
    QRect m_dragStartRect;
    WindowInfo m_hoveredWindow;
    QRect m_selectedRect;
    QPoint m_lastSelectionMousePos;
    CaptureToolBarWidget* m_toolbar = nullptr;
};

} // namespace ScreenCut

#endif // CAPTURE_ENGINE_H
