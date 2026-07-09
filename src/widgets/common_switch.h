/**
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef COMMON_SWITCH_H
#define COMMON_SWITCH_H

#include <QCheckBox>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QPainter>
#include <QColor>
#include <QBrush>
#include <QMouseEvent>

namespace ScreenCut {

class SwitchWidget : public QCheckBox {
    Q_OBJECT
    Q_PROPERTY(int position READ position WRITE setPosition)

public:
    explicit SwitchWidget(QWidget* parent = nullptr);

    int position() const { return m_position; }
    void setPosition(int pos);

protected:
    bool hitButton(const QPoint& pos) const override;
    void paintEvent(QPaintEvent* event) override;

private slots:
    void setupAnimation(Qt::CheckState state);

private:
    int m_position;
    QPropertyAnimation* m_animation;
};

} // namespace ScreenCut

#endif // COMMON_SWITCH_H
