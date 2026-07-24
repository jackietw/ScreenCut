/**
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef CAPTURE_TOOLBAR_H
#define CAPTURE_TOOLBAR_H

#include <QWidget>
#include <QToolButton>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QTimer>

class QMenu;

namespace ScreenCut {

enum class CaptureMode;

class CaptureToolBarWidget : public QWidget {
    Q_OBJECT
public:
    explicit CaptureToolBarWidget(QWidget* parent = nullptr, CaptureMode mode = static_cast<CaptureMode>(0));
    ~CaptureToolBarWidget() override;

    void setCaptureMode(CaptureMode mode);
    void updateTargetDimensions(int width, int height);
    void refreshMenuStates();
    bool isVideoRecording() const { return m_isRecording; }

signals:
    void actionConfirm();
    void actionEdit();
    void actionCopy();
    void actionSave();
    void actionPin();
    void actionCancel();
    
    // Video signals
    void actionVideoRecordStateChanged(bool isRecording);
    void actionVideoStop();
    void cursorToggled(bool checked);
    void highlightToggled(bool checked);
    void animationToggled(bool checked);
    void micToggled(bool checked);
    void micDeviceChanged(const QString& devId);
    void sysAudioToggled(bool checked);

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onRecordPauseClicked();
    void onCursorClicked();
    void onMicClicked();
    void onSysAudioClicked();
    void onVideoStopClicked();
    void updateVideoStatus();
    void onImgEditorClicked();
    void onImgClipboardClicked();
    void onImgCursorClicked();
    void onMicMenuAboutToShow();

private:
    void setupUi();
    struct ButtonConfig {
        QString tooltip;
        QString svgIcon;
        bool checkable = false;
        bool checked = false;
        int iconSize = 20;
        int width = 34;
        int height = 34;

        QString bgNormal = "rgba(255, 255, 255, 0.08)";
        QString borderNormal = "rgba(255, 255, 255, 0.12)";
        QString bgHover = "#00a8ff";
        QString borderHover = "#00a8ff";
        QString bgPressed = "#0088cc";
        QString bgChecked = "rgba(0, 168, 255, 0.4)";
        QString borderChecked = "#00a8ff";

        QString extraStyle = "";
    };

    QToolButton* createButton(const ButtonConfig& config);
    void setButtonColors(QToolButton* btn, const ButtonConfig& config);

    struct SplitButtonResult {
        QWidget* container;
        QToolButton* mainButton;
        QMenu* menu;
    };
    SplitButtonResult createSplitButton(const QString& tooltip, const QString& svgIconString, bool checked);
    QFrame* createSeparator();
    void updateRecordButtonStyle();

    CaptureMode m_mode;
    QHBoxLayout* m_layout = nullptr;

    // Image capture buttons
    QToolButton* m_btnDone = nullptr;
    QToolButton* m_btnImgCancel = nullptr;
    QFrame* m_imageSeparator = nullptr;
    QToolButton* m_btnImgEditor = nullptr;
    QToolButton* m_btnImgClipboard = nullptr;
    QToolButton* m_btnImgCursor = nullptr;

    // Video capture buttons & indicators
    QToolButton* m_btnRecordPause = nullptr;
    QWidget* m_cursorContainer = nullptr;
    QToolButton* m_btnCursor = nullptr;
    QWidget* m_micContainer = nullptr;
    QToolButton* m_btnMic = nullptr;
    QToolButton* m_btnSysAudio = nullptr;
    QMenu* m_cursorMenu = nullptr;
    QMenu* m_micMenu = nullptr;
    QLabel* m_lblVideoStatus = nullptr;
    QToolButton* m_btnVideoStop = nullptr;
    QFrame* m_videoSeparator = nullptr;
    QToolButton* m_btnVideoCancel = nullptr;

    bool m_isRecording = false;
    bool m_redDotVisible = true;
    int m_elapsedSeconds = 0;
    QTimer* m_recordTimer = nullptr;

    int m_targetWidth = 1920;
    int m_targetHeight = 1080;
};

} // namespace ScreenCut

#endif // CAPTURE_TOOLBAR_H
