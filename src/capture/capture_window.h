/**
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef CAPTURE_WINDOW_H
#define CAPTURE_WINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMap>
#include <QPoint>
#include <QMouseEvent>
#include <QResizeEvent>
#include "../widgets/common_switch.h"

namespace ScreenCut {

class CaptureMainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit CaptureMainWindow(QWidget* parent = nullptr);
    ~CaptureMainWindow() override = default;

    static CaptureMainWindow* instance();
    void showAboutOverlay();
    void showPreferencesOverlay();
    bool isSettingEnabled(const QString& key) const;
    void setSettingEnabled(const QString& key, bool enabled);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onCaptureClicked();
    void onRecordClicked();
    void onOpenEditorClicked();
    void onPreferencesClicked();
    void onAboutClicked();

private:
    void setupUi();
    void setupStyleSheet();
    QWidget* createImageTab();
    QWidget* createVideoTab();
    void addSettingRow(QVBoxLayout* layout, const QString& labelText, bool isChecked, const QString& configKey = QString());

    static CaptureMainWindow* s_instance;

    QTabWidget* m_tabs;
    QPushButton* m_btnPreferences;
    QPushButton* m_btnEditor;
    QPushButton* m_btnClose;
    QPushButton* m_btnAbout;
    QPushButton* m_btnCapture;
    QPushButton* m_btnRecord;

    QMap<QString, SwitchWidget*> m_toggles;
    QPoint m_dragPos;
    bool m_isDragging = false;
    QWidget* m_aboutOverlay = nullptr;
    QWidget* m_prefOverlay = nullptr;
};

} // namespace ScreenCut

#endif // CAPTUREMAINWINDOW_H
