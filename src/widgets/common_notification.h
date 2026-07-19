/*
* SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
* SPDX-License-Identifier: LGPL-2.1-or-later
*/

#ifndef COMMON_NOTIFICATION_H
#define COMMON_NOTIFICATION_H

#include <QWidget>
#include <QString>

namespace ScreenCut {

class Notification : public QWidget {
    Q_OBJECT
public:
    explicit Notification(const QString& message, QWidget* parent = nullptr, int duration = 3000, bool isError = false);
    ~Notification() override;

    void showToast();

    static void showMessage(const QString& message, int duration = 3000, bool isError = false);
    static bool hideAllToasts();
};

} // namespace ScreenCut

#endif // COMMON_NOTIFICATION_H
