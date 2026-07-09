/**
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef CAPTURE_TOOLBAR_H
#define CAPTURE_TOOLBAR_H

#include <QWidget>
#include <QToolButton>
#include <QHBoxLayout>
#include <QFrame>

namespace ScreenCut {

class CaptureToolBarWidget : public QWidget {
    Q_OBJECT
public:
    explicit CaptureToolBarWidget(QWidget* parent = nullptr);
    ~CaptureToolBarWidget() override;

signals:
    void actionConfirm();
    void actionEdit();
    void actionCopy();
    void actionSave();
    void actionPin();
    void actionCancel();

private:
    void setupUi();
    QToolButton* createToolButton(const QString& tooltip, const QString& svgIconString, const char* slotSignal);
    QFrame* createSeparator();
};

} // namespace ScreenCut

#endif // CAPTURE_TOOLBAR_WIDGET_H
