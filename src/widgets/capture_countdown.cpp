/*
* SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
* SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "capture_countdown.h"
#include "../platform/platform.h"
#include <QVBoxLayout>
#include <QGuiApplication>
#include <QScreen>
#include <QMouseEvent>
#include <QDebug>

namespace ScreenCut {

CountdownWidget::CountdownWidget(int seconds, QWidget* parent)
    : QWidget(parent)
    , m_remainingSeconds(seconds)
{
    // Use Qt::Window (not Qt::Tool) so macOS does NOT auto-hide it
    // when the app loses focus. Tool/NSPanel windows are hidden on deactivation.
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(15, 15, 15, 15);

    m_lblNumber = new QLabel(QString::number(m_remainingSeconds), this);
    m_lblNumber->setAlignment(Qt::AlignCenter);
    m_lblNumber->setStyleSheet("color: white; font-size: 48px; font-weight: bold; background-color: rgba(20, 22, 28, 220); border: 2px solid #00a8ff; border-radius: 12px; padding: 10px; min-width: 60px; min-height: 60px;");
    layout->addWidget(m_lblNumber);
}

CountdownWidget::~CountdownWidget() = default;

void CountdownWidget::startCountdown() {
    qDebug() << "[CountdownWidget] startCountdown(). Seconds:" << m_remainingSeconds;
    adjustSize();
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect geom = screen->geometry();
        int x = geom.right() - width() - 40;
        int y = geom.top() + 40;
        move(x, y);
    }
    show();
    raise();
    Platform::excludeWindowFromCapture(winId());
    // Use NSFloatingWindowLevel (3) instead of CGShieldingWindowLevel (~1000)
    // so the widget stays visible but doesn't interfere with event delivery.
    // We just need it above normal windows, not above the screen shield.
    Platform::elevateWindowAboveSystemBars(winId());
}

void CountdownWidget::updateDisplay(int secondsLeft) {
    m_lblNumber->setText(QString::number(secondsLeft));
}

void CountdownWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton) {
        qDebug() << "[CountdownWidget] Clicked by user. Cancelling countdown.";
        emit cancelled();
        close();
    } else {
        QWidget::mousePressEvent(event);
    }
}

} // namespace ScreenCut
