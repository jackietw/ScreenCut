/**
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef CAPTURE_PREFERENCES_H
#define CAPTURE_PREFERENCES_H

#include <QDialog>
#include <QListWidget>
#include <QStackedWidget>
#include <QMap>
#include <QComboBox>
#include <QLabel>
#include "../widgets/common_switch.h"

namespace ScreenCut {

class CapturePreferencesDialog : public QDialog {
    Q_OBJECT

public:
    explicit CapturePreferencesDialog(QWidget* parent = nullptr);
    ~CapturePreferencesDialog() override = default;

    static CapturePreferencesDialog* instance(QWidget* parent = nullptr);
    void updateHwEncodersList();

protected:
    void closeEvent(QCloseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    void setupUi();
    void setupStyleSheet();
    QWidget* createGeneralPage();
    QWidget* createVideoAudioPage();

    static CapturePreferencesDialog* s_instance;
    QPoint m_dragPos;

    QListWidget* m_sidebar = nullptr;
    QStackedWidget* m_pages = nullptr;
    QMap<QString, SwitchWidget*> m_toggles;
    QComboBox* m_comboEnc = nullptr;
    QLabel* m_lblEnc = nullptr;
    SwitchWidget* m_toggleHw = nullptr;
    QLabel* m_lblHw = nullptr;
    QLabel* m_lblHwDesc = nullptr;
};

} // namespace ScreenCut

#endif // CAPTURE_PREFERENCES_H
