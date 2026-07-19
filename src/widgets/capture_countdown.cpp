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
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(15, 15, 15, 15);

    m_lblNumber = new QLabel(QString::number(m_remainingSeconds), this);
    m_lblNumber->setAlignment(Qt::AlignCenter);
    m_lblNumber->setStyleSheet("color: white; font-size: 48px; font-weight: bold; background-color: rgba(20, 22, 28, 220); border: 2px solid #00a8ff; border-radius: 12px; padding: 10px; min-width: 60px; min-height: 60px;");
    layout->addWidget(m_lblNumber);

    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &CountdownWidget::onTick);
}

CountdownWidget::~CountdownWidget() = default;

void CountdownWidget::startCountdown() {
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
    Platform::elevateWindowAboveSystemBars(winId());
    m_timer->start();
}

void CountdownWidget::onTick() {
    m_remainingSeconds--;
    if (m_remainingSeconds <= 0) {
        m_timer->stop();
        hide();
        close();
        QTimer::singleShot(250, this, [this]() {
            emit completed();
        });
    } else {
        m_lblNumber->setText(QString::number(m_remainingSeconds));
    }
}

void CountdownWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton) {
        qDebug() << "[CountdownWidget] Clicked by user. Cancelling countdown.";
        m_timer->stop();
        emit cancelled();
        close();
    } else {
        QWidget::mousePressEvent(event);
    }
}

} // namespace ScreenCut
