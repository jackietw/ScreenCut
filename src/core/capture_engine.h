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
#include <QThread>
#include <QSize>
#include <QString>
#include <QProcess>
#include <QImage>
#include <functional>
#include <memory>
#include <vector>

namespace ScreenCut {

enum class CaptureMode {
    Region,
    Window,
    Scrolling,
    FullScreen,
    VideoRegion
};

struct WindowInfo {
    QRect rect;
    QString title;
    QString processName;
    bool isValid = false;
};

class RegionSelectWidget;

struct HwEncoderInfo {
    QString codec;
    QString displayName;
};

class VideoCaptureWorker : public QObject {
    Q_OBJECT
public:
    explicit VideoCaptureWorker(const QRect& rect, const QString& outputPath, bool captureCursor, QObject* parent = nullptr);
    ~VideoCaptureWorker() override;

public slots:
    void startRecording();
    void stopRecording();
    void setCaptureCursor(bool capture) { m_captureCursor = capture; }

signals:
    void finished(const QString& outputPath);
    void errorOccurred(const QString& err);

private slots:
    void captureFrame();

private:
    QString getFFmpegPath() const;
    QString selectVideoCodec(bool& outIsHw) const;

    QRect m_rect;
    QSize m_targetSize;
    QString m_outputPath;
    bool m_captureCursor;
    bool m_isRunning = false;
    QTimer* m_timer = nullptr;
    QProcess* m_ffmpegProcess = nullptr;
    qint64 m_frameIntervalMs = 33; // ~30 FPS
};

class VideoRecorder : public QObject {
    Q_OBJECT
public:
    explicit VideoRecorder(QObject* parent = nullptr);
    ~VideoRecorder() override;

    static QString getFFmpegPath();
    static QList<HwEncoderInfo> detectAvailableHwEncoders(bool forceRefresh = false);
    static bool isHwDetected();
    static void detectAvailableHwEncodersAsync(std::function<void(const QList<HwEncoderInfo>&)> callback = nullptr, bool forceRefresh = false);

    bool isRecording() const { return m_isRecording; }
    VideoCaptureWorker* worker() const { return m_worker; }
    void start(const QRect& rect);
    void stop();

signals:
    void recordingFinished(const QString& outputPath);
    void recordingError(const QString& err);

private:
    bool m_isRecording = false;
    QThread* m_workerThread = nullptr;
    VideoCaptureWorker* m_worker = nullptr;
};

class CaptureEngine : public QObject {
    Q_OBJECT
public:
    static CaptureEngine* instance();

    void startCapture(CaptureMode mode = CaptureMode::Region);
    void startRegionCapture();
    void startVideoRegionCapture();
    void startWindowCapture();
    void startScrollingCapture();
    void startFullScreenCapture();

    void startVideoRecord(const QRect& rect);
    void stopVideoRecord();
    VideoRecorder* videoRecorder() const { return m_videoRecorder; }

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

public:
    void initSessionStateFromConfig();
    bool isSessionCursorEnabled() const { return m_sessionCursorEnabled; }
    void setSessionCursorEnabled(bool enabled);
    bool isSessionHighlightEnabled() const { return m_sessionCursorEnabled; }
    bool isSessionAnimationEnabled() const { return m_sessionCursorEnabled; }
    bool isSessionMicEnabled() const { return m_sessionMicEnabled; }
    void setSessionMicEnabled(bool enabled);
    bool isSessionSysAudioEnabled() const { return m_sessionSysAudioEnabled; }
    void setSessionSysAudioEnabled(bool enabled);

    bool isSessionEditorEnabled() const { return m_sessionEditorEnabled; }
    void setSessionEditorEnabled(bool enabled) { m_sessionEditorEnabled = enabled; }
    bool isSessionClipboardEnabled() const { return m_sessionClipboardEnabled; }
    void setSessionClipboardEnabled(bool enabled) { m_sessionClipboardEnabled = enabled; }
    bool isSessionImgCursorEnabled() const { return m_sessionImgCursorEnabled; }
    void setSessionImgCursorEnabled(bool enabled) { m_sessionImgCursorEnabled = enabled; }

private:
    explicit CaptureEngine(QObject* parent = nullptr);
    ~CaptureEngine() override;

    void doActualCapture(CaptureMode mode);

    static CaptureEngine* s_instance;
    QPixmap m_fullDesktopSnapshot;
    class RegionSelectWidget* m_overlayWidget = nullptr;
    VideoRecorder* m_videoRecorder = nullptr;
    CaptureMode m_currentMode = CaptureMode::Region;
    bool m_isPendingCapture = false;
    bool m_sessionCursorEnabled = true;
    bool m_sessionMicEnabled = true;
    bool m_sessionSysAudioEnabled = true;
    bool m_sessionEditorEnabled = true;
    bool m_sessionClipboardEnabled = true;
    bool m_sessionImgCursorEnabled = false;
};

class CaptureToolBarWidget;
class CaptureStatusWidget;
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
    void videoRegionSelected(const QRect& rect);
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
    void ensureToolbarAndShow();

    void setVideoRecordingActive(bool active);
    void startVideoCountdown();

    QPixmap m_desktopSnapshot;
    CaptureMode m_mode;
    bool m_isSelecting = false;
    bool m_selectionConfirmed = false;
    bool m_isVideoRecordingActive = false;
    bool m_isCountingDown = false;
    int m_countdownValue = 3;
    QTimer* m_countdownTimer = nullptr;
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
    CaptureStatusWidget* m_statusCard = nullptr;
    class VideoBorderOverlay* m_borderOverlay = nullptr;
};

} // namespace ScreenCut

#endif // CAPTURE_ENGINE_H
