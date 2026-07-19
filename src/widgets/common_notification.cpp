/*
* SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
* SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "common_notification.h"
#include "../platform/platform.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QApplication>
#include <QScreen>
#include <QTimer>
#include <QGuiApplication>
#include <QList>

namespace ScreenCut {

static QList<Notification*> s_activeToasts;

Notification::Notification(const QString& message, QWidget* parent, int duration, bool isError)
    : QWidget(parent)
{
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_TransparentForMouseEvents);

    s_activeToasts.append(this);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    QWidget* bg = new QWidget(this);
    bg->setObjectName("NotificationBg");
    QString bgColor = isError ? "rgba(180, 40, 40, 210)" : "rgba(0, 0, 0, 180)";
    bg->setStyleSheet(QString("#NotificationBg { background-color: %1; border-radius: 8px; padding: 15px 30px; }").arg(bgColor));
    
    QVBoxLayout* bgLayout = new QVBoxLayout(bg);
    QLabel* lblMsg = new QLabel(message, bg);
    lblMsg->setStyleSheet("color: white; font-size: 16px; font-weight: bold; background: transparent;");
    lblMsg->setAlignment(Qt::AlignCenter);
    bgLayout->addWidget(lblMsg);

    layout->addWidget(bg);
    adjustSize();

    QTimer::singleShot(duration, this, &QWidget::close);
}

Notification::~Notification() {
    s_activeToasts.removeAll(this);
}

void Notification::showToast() {
    adjustSize();
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect geom = screen->geometry();
        int x = geom.center().x() - width() / 2;
        int y = geom.bottom() - 100 - height();
        move(x, y);
    }
    show();
    raise();
    Platform::excludeWindowFromCapture(winId());
}

bool Notification::hideAllToasts() {
    bool hidAny = false;
    QList<Notification*> copy = s_activeToasts;
    for (Notification* toast : copy) {
        if (toast && toast->isVisible()) {
            toast->hide();
            toast->close();
            hidAny = true;
        }
    }
    return hidAny;
}

void Notification::showMessage(const QString& message, int duration, bool isError) {
    Notification* toast = new Notification(message, nullptr, duration, isError);
    toast->setAttribute(Qt::WA_DeleteOnClose);
    toast->showToast();
}

} // namespace ScreenCut
