/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "capture_status.h"
#include "../resources/IconUtils.h"
#include "../config.h"
#include "../capture/capture_window.h"
#include "../core/capture_engine.h"
#include <QPainter>
#include <QPainterPath>
#include <QRandomGenerator>
#include <QDebug>

namespace ScreenCut {

// ============================================================================
// VuMeterBar Implementation
// ============================================================================

VuMeterBar::VuMeterBar(QWidget* parent) : QWidget(parent) {
    setFixedHeight(14);
    setMinimumWidth(120);
}

void VuMeterBar::setLevel(float level) {
    float clamped = qBound(0.0f, level, 1.0f);
    if (!qFuzzyCompare(m_level, clamped)) {
        m_level = clamped;
        update();
    }
}

void VuMeterBar::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int n = TOTAL_BARS;
    int gap = 3;
    int bar_w = qMax(4, (w - gap * (n - 1)) / n);
    int total_w = bar_w * n + gap * (n - 1);
    int x_start = (w - total_w) / 2;

    int lit = qRound(m_level * n);

    for (int i = 0; i < n; ++i) {
        int x = x_start + i * (bar_w + gap);
        QRectF rect(x, 1, bar_w, h - 2);

        QColor color;
        if (i < lit) {
            if (i < n * 0.6f) {
                color = QColor("#4caf50");
            } else if (i < n * 0.85f) {
                color = QColor("#ffeb3b");
            } else {
                color = QColor("#f44336");
            }
        } else {
            color = QColor(70, 70, 70);
        }

        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(rect, 2, 2);
    }
}

// ============================================================================
// CaptureStatusWidget Implementation
// ============================================================================

CaptureStatusWidget::CaptureStatusWidget(QWidget* parent, bool interactiveSwitch)
    : QWidget(parent) {
    setAttribute(Qt::WA_StyledBackground, true);
    setupUi(interactiveSwitch);
    refreshStatus();

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &CaptureStatusWidget::onUpdateTimer);
    m_timer->start(100);
}

CaptureStatusWidget::~CaptureStatusWidget() = default;

void CaptureStatusWidget::setupUi(bool interactiveSwitch) {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QWidget* card = new QWidget(this);
    card->setObjectName("record_card");
    card->setStyleSheet(R"(
        #record_card {
            background-color: rgba(40, 40, 44, 235);
            border: 1px solid rgba(255, 255, 255, 25);
            border-radius: 14px;
        }
    )");

    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(18, 14, 18, 16);
    cardLayout->setSpacing(0);

    // Title
    QLabel* title = new QLabel("Record Status", card);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("color: white; font-size: 16px; font-weight: 700; letter-spacing: 0.5px; padding-bottom: 10px;");
    cardLayout->addWidget(title);

    // Separator
    QFrame* sep = new QFrame(card);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: rgba(255,255,255,30); background-color: rgba(255,255,255,30); max-height: 1px;");
    cardLayout->addWidget(sep);
    cardLayout->addSpacing(12);

    // --- Cursor Row ---
    QVBoxLayout* cursorCol = new QVBoxLayout();
    cursorCol->setSpacing(2);
    QLabel* lblHlTitle = new QLabel("Highlight", card);
    lblHlTitle->setStyleSheet("color: #888888; font-size: 11px;");
    QLabel* lblAnimTitle = new QLabel("Cursor Animation", card);
    lblAnimTitle->setStyleSheet("color: #888888; font-size: 11px;");
    cursorCol->addWidget(lblHlTitle);
    cursorCol->addWidget(lblAnimTitle);

    QVBoxLayout* cursorRightCol = new QVBoxLayout();
    cursorRightCol->setSpacing(2);
    cursorRightCol->setAlignment(Qt::AlignRight);

    m_lblCursorStatus = new QLabel("ON", card);
    m_lblCursorStatus->setAlignment(Qt::AlignRight);
    m_lblCursorStatus->setStyleSheet("color: #4caf50; font-size: 13px; font-weight: 700;");

    m_lblCursorHl = new QLabel("ON", card);
    m_lblCursorHl->setAlignment(Qt::AlignRight);
    m_lblCursorHl->setStyleSheet("color: #cccccc; font-size: 11px;");

    m_lblCursorAnim = new QLabel("OFF", card);
    m_lblCursorAnim->setAlignment(Qt::AlignRight);
    m_lblCursorAnim->setStyleSheet("color: #cccccc; font-size: 11px;");

    cursorRightCol->addWidget(m_lblCursorStatus);
    cursorRightCol->addWidget(m_lblCursorHl);
    cursorRightCol->addWidget(m_lblCursorAnim);

    QWidget* cursorRightWidget = new QWidget(card);
    cursorRightWidget->setMinimumWidth(44);
    cursorRightWidget->setLayout(cursorRightCol);
    cursorRightCol->setContentsMargins(0, 0, 0, 0);

    QList<QWidget*> cursorRightWidgets;
    cursorRightWidgets.append(cursorRightWidget);

    if (interactiveSwitch) {
        m_toggleCursor = new SwitchWidget(card);
        connect(m_toggleCursor, &SwitchWidget::toggled, this, [this](bool checked) {
            Config::setValue("Video_Capture Cursor", checked);
            if (CaptureMainWindow::instance()) {
                CaptureMainWindow::instance()->setSettingEnabled("Video_Capture Cursor", checked);
            }
            refreshStatus();
            emit cursorToggled(checked);
        });
        cursorRightWidgets.append(m_toggleCursor);
    }

    QHBoxLayout* cursorRow = createStatusRow(SVG_MOUSE, "Cursor", cursorCol, cursorRightWidgets);
    cardLayout->addLayout(cursorRow);
    cardLayout->addSpacing(14);

    // --- Microphone Row ---
    QVBoxLayout* micCol = new QVBoxLayout();
    micCol->setSpacing(4);
    m_lblMicDevice = new QLabel("Default Microphone", card);
    m_lblMicDevice->setStyleSheet("color: #888888; font-size: 11px;");
    m_vuMeter = new VuMeterBar(card);
    m_vuMeter->setFixedWidth(130);
    micCol->addWidget(m_lblMicDevice);
    micCol->addWidget(m_vuMeter);

    m_lblMicStatus = new QLabel("ON", card);
    m_lblMicStatus->setStyleSheet("color: #4caf50; font-size: 13px; font-weight: 700;");

    QList<QWidget*> micRightWidgets;
    micRightWidgets.append(m_lblMicStatus);

    if (interactiveSwitch) {
        m_toggleMic = new SwitchWidget(card);
        connect(m_toggleMic, &SwitchWidget::toggled, this, [this](bool checked) {
            Config::setValue("Record Microphone", checked);
            if (CaptureMainWindow::instance()) {
                CaptureMainWindow::instance()->setSettingEnabled("Record Microphone", checked);
            }
            refreshStatus();
            emit micToggled(checked);
        });
        micRightWidgets.append(m_toggleMic);
    }

    QHBoxLayout* micRow = createStatusRow(SVG_MIC, "Microphone", micCol, micRightWidgets);
    cardLayout->addLayout(micRow);
    cardLayout->addSpacing(14);

    // --- System Audio Row ---
    QVBoxLayout* sysCol = new QVBoxLayout();
    sysCol->setSpacing(4);
    m_lblSysDevice = new QLabel("Default Output", card);
    m_lblSysDevice->setStyleSheet("color: #888888; font-size: 11px;");
    sysCol->addWidget(m_lblSysDevice);

    m_lblSysAudioStatus = new QLabel("ON", card);
    m_lblSysAudioStatus->setStyleSheet("color: #4caf50; font-size: 13px; font-weight: 700;");

    QList<QWidget*> sysRightWidgets;
    sysRightWidgets.append(m_lblSysAudioStatus);

    if (interactiveSwitch) {
        m_toggleSysAudio = new SwitchWidget(card);
        connect(m_toggleSysAudio, &SwitchWidget::toggled, this, [this](bool checked) {
            Config::setValue("Record System Audio", checked);
            if (CaptureMainWindow::instance()) {
                CaptureMainWindow::instance()->setSettingEnabled("Record System Audio", checked);
            }
            refreshStatus();
            emit sysAudioToggled(checked);
        });
        sysRightWidgets.append(m_toggleSysAudio);
    }

    QHBoxLayout* sysRow = createStatusRow(SVG_SYS_AUDIO, "System Audio", sysCol, sysRightWidgets);
    cardLayout->addLayout(sysRow);

    mainLayout->addWidget(card);
}

QHBoxLayout* CaptureStatusWidget::createStatusRow(const QString& svgIcon, const QString& titleText, QVBoxLayout* subLayout, const QList<QWidget*>& rightWidgets) {
    QHBoxLayout* row = new QHBoxLayout();
    row->setSpacing(12);
    row->setAlignment(Qt::AlignVCenter);

    // Icon
    QLabel* iconLbl = new QLabel();
    iconLbl->setPixmap(createSvgIcon(svgIcon, 22, 22).pixmap(22, 22));
    iconLbl->setFixedSize(24, 24);
    row->addWidget(iconLbl);

    // Label Column
    QVBoxLayout* lblCol = new QVBoxLayout();
    lblCol->setSpacing(2);
    QLabel* mainLbl = new QLabel(titleText);
    mainLbl->setStyleSheet("color: #dddddd; font-size: 14px; font-weight: 600;");
    lblCol->addWidget(mainLbl);
    if (subLayout) {
        lblCol->addLayout(subLayout);
    }
    row->addLayout(lblCol);

    row->addStretch();

    // Right Widgets
    for (QWidget* w : rightWidgets) {
        if (w) row->addWidget(w);
    }

    return row;
}

void CaptureStatusWidget::refreshStatus() {
    bool cursorOn = CaptureEngine::instance()->isSessionCursorEnabled();
    bool hlOn = CaptureEngine::instance()->isSessionHighlightEnabled();
    bool animOn = CaptureEngine::instance()->isSessionAnimationEnabled();
    bool micOn = CaptureEngine::instance()->isSessionMicEnabled();
    bool sysAudioOn = CaptureEngine::instance()->isSessionSysAudioEnabled();

    updateCursorState(cursorOn, hlOn, animOn);
    updateMicState(micOn);
    updateSysAudioState(sysAudioOn);

    if (m_toggleCursor && m_toggleCursor->isChecked() != cursorOn) {
        m_toggleCursor->blockSignals(true);
        m_toggleCursor->setChecked(cursorOn);
        m_toggleCursor->blockSignals(false);
    }
    if (m_toggleMic && m_toggleMic->isChecked() != micOn) {
        m_toggleMic->blockSignals(true);
        m_toggleMic->setChecked(micOn);
        m_toggleMic->blockSignals(false);
    }
    if (m_toggleSysAudio && m_toggleSysAudio->isChecked() != sysAudioOn) {
        m_toggleSysAudio->blockSignals(true);
        m_toggleSysAudio->setChecked(sysAudioOn);
        m_toggleSysAudio->blockSignals(false);
    }
}

void CaptureStatusWidget::updateCursorState(bool on, bool hlOn, bool animOn) {
    if (m_lblCursorStatus) {
        if (on) {
            m_lblCursorStatus->setText("ON");
            m_lblCursorStatus->setStyleSheet("color: #4caf50; font-size: 13px; font-weight: 700;");
        } else {
            m_lblCursorStatus->setText("OFF");
            m_lblCursorStatus->setStyleSheet("color: #cccccc; font-size: 13px; font-weight: 500;");
        }
    }
    if (m_lblCursorHl && m_lblCursorAnim) {
        m_lblCursorHl->setText(hlOn ? "ON" : "OFF");
        m_lblCursorHl->setStyleSheet((on && hlOn) ? "color: #cccccc; font-size: 11px;" : "color: #777777; font-size: 11px;");
        m_lblCursorAnim->setText(animOn ? "ON" : "OFF");
        m_lblCursorAnim->setStyleSheet((on && animOn) ? "color: #cccccc; font-size: 11px;" : "color: #777777; font-size: 11px;");
    }
}

void CaptureStatusWidget::updateMicState(bool on) {
    QString savedDev = Config::getValue("audio_source", "Default Microphone").toString();
    if (savedDev.isEmpty()) savedDev = "Default Microphone";

    if (m_lblMicStatus) {
        if (on) {
            m_lblMicStatus->setText("ON");
            m_lblMicStatus->setStyleSheet("color: #4caf50; font-size: 13px; font-weight: 700;");
            if (m_lblMicDevice) m_lblMicDevice->setText(savedDev.left(26));
        } else {
            m_lblMicStatus->setText("OFF");
            m_lblMicStatus->setStyleSheet("color: #cccccc; font-size: 13px; font-weight: 500;");
            if (m_lblMicDevice) m_lblMicDevice->setText("None (Muted)");
            if (m_vuMeter) m_vuMeter->setLevel(0.0f);
        }
    }
}

void CaptureStatusWidget::updateSysAudioState(bool on) {
    if (m_lblSysAudioStatus) {
        if (on) {
            m_lblSysAudioStatus->setText("ON");
            m_lblSysAudioStatus->setStyleSheet("color: #4caf50; font-size: 13px; font-weight: 700;");
            if (m_lblSysDevice) m_lblSysDevice->setText("Default Output");
        } else {
            m_lblSysAudioStatus->setText("OFF");
            m_lblSysAudioStatus->setStyleSheet("color: #cccccc; font-size: 13px; font-weight: 500;");
            if (m_lblSysDevice) m_lblSysDevice->setText("None (Muted)");
        }
    }
}

void CaptureStatusWidget::setAudioLevel(float level) {
    if (m_vuMeter) {
        m_vuMeter->setLevel(level);
    }
}

void CaptureStatusWidget::onUpdateTimer() {
    bool micOn = CaptureEngine::instance()->isSessionMicEnabled();
    if (micOn && m_vuMeter) {
        // Subtle ambient / simulated level or active fluctuation for UI feel
        float randVal = static_cast<float>(QRandomGenerator::global()->bounded(100)) / 100.0f;
        float nextLevel = 0.05f + 0.35f * randVal;
        m_vuMeter->setLevel(nextLevel);
    } else if (!micOn && m_vuMeter) {
        m_vuMeter->setLevel(0.0f);
    }
}

} // namespace ScreenCut
