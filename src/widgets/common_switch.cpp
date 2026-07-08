/**
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "common_switch.h"
#include <QPainter>
#include <QColor>
#include <QBrush>

namespace ScreenCut {

SwitchWidget::SwitchWidget(QWidget* parent)
    : QCheckBox(parent), m_position(3)
{
    setFixedSize(36, 20);
    setCursor(Qt::PointingHandCursor);

    m_animation = new QPropertyAnimation(this, "position", this);
    m_animation->setEasingCurve(QEasingCurve::InOutSine);
    m_animation->setDuration(150);

    connect(this, &QCheckBox::checkStateChanged, this, &SwitchWidget::setupAnimation);
}

void SwitchWidget::setPosition(int pos) {
    m_position = pos;
    update();
}

void SwitchWidget::setupAnimation(Qt::CheckState state) {
    m_animation->stop();
    if (state == Qt::Checked) {
        m_animation->setEndValue(19);
    } else {
        m_animation->setEndValue(3);
    }
    m_animation->start();
}

bool SwitchWidget::hitButton(const QPoint& pos) const {
    return contentsRect().contains(pos);
}

void SwitchWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw background
    QColor bgColor = isChecked() ? QColor("#246bb2") : QColor("#5a5a5a");
    painter.setBrush(QBrush(bgColor));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(0, 0, width(), height(), height() / 2.0, height() / 2.0);

    // Draw knob
    QColor knobColor = isChecked() ? QColor("#ffffff") : QColor("#c0c0c0");
    painter.setBrush(QBrush(knobColor));
    int knobRadius = height() - 6;
    painter.drawEllipse(m_position, 3, knobRadius, knobRadius);
}

} // namespace ScreenCut
