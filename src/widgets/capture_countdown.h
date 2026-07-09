/*
* SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
* SPDX-License-Identifier: LGPL-2.1-or-later
*/

#ifndef COMMON_COUNTDOWN_H
#define COMMON_COUNTDOWN_H

#include <QWidget>
#include <QLabel>
#include <QTimer>

namespace ScreenCut {

class CountdownWidget : public QWidget {
    Q_OBJECT
public:
    explicit CountdownWidget(int seconds = 5, QWidget* parent = nullptr);
    ~CountdownWidget() override;

    void startCountdown();

signals:
    void completed();
    void cancelled();

protected:
    void mousePressEvent(QMouseEvent* event) override;

private slots:
    void onTick();

private:
    int m_remainingSeconds;
    QLabel* m_lblNumber;
    QTimer* m_timer;
};

} // namespace ScreenCut

#endif // COMMON_COUNTDOWN_H
