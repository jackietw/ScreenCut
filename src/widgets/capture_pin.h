/**
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef COMMON_PIN_H
#define COMMON_PIN_H

#include <QWidget>
#include <QPixmap>
#include <QPoint>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>

namespace ScreenCut {

class PinWidget : public QWidget {
    Q_OBJECT
public:
    explicit PinWidget(const QPixmap& pixmap, QWidget* parent = nullptr);
    ~PinWidget() override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    QPixmap m_pixmap;
    QPoint m_dragPosition;
    void saveImage();
    void copyImage();
};

} // namespace ScreenCut

#endif // COMMON_PIN_H
