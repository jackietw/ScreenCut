/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef CAPTURE_STATUS_H
#define CAPTURE_STATUS_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QTimer>

namespace ScreenCut {

class SwitchWidget;

class VuMeterBar : public QWidget {
    Q_OBJECT
public:
    explicit VuMeterBar(QWidget* parent = nullptr);
    void setLevel(float level); // 0.0 to 1.0

protected:
    void paintEvent(class QPaintEvent* event) override;

private:
    float m_level = 0.0f;
    static constexpr int TOTAL_BARS = 12;
};

class CaptureStatusWidget : public QWidget {
    Q_OBJECT
public:
    explicit CaptureStatusWidget(QWidget* parent = nullptr, bool interactiveSwitch = true);
    ~CaptureStatusWidget() override;

    void refreshStatus();
    void setAudioLevel(float level);
    void updateCursorState(bool on, bool hlOn = true, bool animOn = true);
    void updateMicState(bool on);
    void updateSysAudioState(bool on);

signals:
    void cursorToggled(bool checked);
    void micToggled(bool checked);
    void sysAudioToggled(bool checked);

private slots:
    void onUpdateTimer();

private:
    void setupUi(bool interactiveSwitch);
    QHBoxLayout* createStatusRow(const QString& svgIcon, const QString& titleText, QVBoxLayout* subLayout, const QList<QWidget*>& rightWidgets);

    QLabel* m_lblCursorStatus = nullptr;
    QLabel* m_lblCursorHl = nullptr;
    QLabel* m_lblCursorAnim = nullptr;

    QLabel* m_lblMicStatus = nullptr;
    QLabel* m_lblMicDevice = nullptr;
    VuMeterBar* m_vuMeter = nullptr;

    QLabel* m_lblSysAudioStatus = nullptr;
    QLabel* m_lblSysDevice = nullptr;

    SwitchWidget* m_toggleCursor = nullptr;
    SwitchWidget* m_toggleMic = nullptr;
    SwitchWidget* m_toggleSysAudio = nullptr;

    QTimer* m_timer = nullptr;
    float m_simulatedAudioLevel = 0.0f;
    bool m_isSimulating = false;
};

} // namespace ScreenCut

#endif // CAPTURE_STATUS_H
