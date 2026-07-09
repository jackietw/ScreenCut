/*
* SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
* SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "capture_countdown.h"
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
    setAttribute(Qt::WA_DeleteOnClose);
    setCursor(Qt::PointingHandCursor);
    setToolTip("Click to cancel");

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    QWidget* bg = new QWidget(this);
    bg->setObjectName("CountdownBg");
    // Black background with alpha=40% (0.4 * 255 = 102) and white text
    bg->setStyleSheet("#CountdownBg { background-color: rgba(0, 0, 0, 102); border-radius: 12px; }");
    bg->setFixedSize(120, 120);

    QVBoxLayout* bgLayout = new QVBoxLayout(bg);
    m_lblNumber = new QLabel(QString::number(m_remainingSeconds), bg);
    m_lblNumber->setStyleSheet("color: white; font-size: 64px; font-weight: bold; background: transparent;");
    m_lblNumber->setAlignment(Qt::AlignCenter);
    bgLayout->addWidget(m_lblNumber);

    layout->addWidget(bg);
    adjustSize();

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
    m_timer->start();
}

void CountdownWidget::onTick() {
    m_remainingSeconds--;
    if (m_remainingSeconds <= 0) {
        m_timer->stop();
        emit completed();
        close();
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
