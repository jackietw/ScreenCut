/**
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "capture_pin.h"
#include <QPainter>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDateTime>
#include <QAction>
#include "../core/common_project.h"
#include "../platform/platform.h"

namespace ScreenCut {

#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
PinWidget::PinWidget(const QPixmap& pixmap, QWidget* parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint),
      m_pixmap(pixmap) {
#else
PinWidget::PinWidget(const QPixmap& pixmap, QWidget* parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool),
      m_pixmap(pixmap) {
#endif
    setAttribute(Qt::WA_DeleteOnClose, true);
    setCursor(Qt::SizeAllCursor);
    resize(m_pixmap.size());
}

PinWidget::~PinWidget() = default;

void PinWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.drawPixmap(0, 0, m_pixmap);
    // Draw subtle border around pinned image
    painter.setPen(QPen(QColor("#00a8ff"), 1));
    painter.drawRect(0, 0, width() - 1, height() - 1);
}

void PinWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void PinWidget::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}

void PinWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        close();
    }
}

void PinWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    Platform::elevateWindowAboveSystemBars(winId());
}

void PinWidget::contextMenuEvent(QContextMenuEvent* event) {
    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background-color: #1a1d24; color: #ffffff; border: 1px solid #384252; padding: 4px; }"
        "QMenu::item { padding: 6px 20px; border-radius: 4px; }"
        "QMenu::item:selected { background-color: #00a8ff; }"
    );

    QAction* copyAct = menu.addAction("📋 Copy to Clipboard");
    QAction* saveAct = menu.addAction("💾 Save to File...");
    menu.addSeparator();
    QAction* closeAct = menu.addAction("❌ Close Pin (Double-Click)");

    connect(copyAct, &QAction::triggered, this, &PinWidget::copyImage);
    connect(saveAct, &QAction::triggered, this, &PinWidget::saveImage);
    connect(closeAct, &QAction::triggered, this, &PinWidget::close);

    menu.exec(event->globalPos());
}

void PinWidget::copyImage() {
    QApplication::clipboard()->setPixmap(m_pixmap);
}

void PinWidget::saveImage() {
    QString defaultPath = ScutProject::getLibraryDir();
    QString fileName = QString("screencut_pin_%1.png").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    QString filePath = QFileDialog::getSaveFileName(this, "Save Pinned Image", defaultPath + "/" + fileName, "Images (*.png *.jpg *.bmp)");
    if (!filePath.isEmpty()) {
        m_pixmap.save(filePath);
    }
}

} // namespace ScreenCut
