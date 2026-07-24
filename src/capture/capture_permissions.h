/**
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef SCREENCUT_PERMISSIONS_WINDOW_H
#define SCREENCUT_PERMISSIONS_WINDOW_H

#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <functional>

namespace ScreenCut {

class PermissionRow : public QWidget {
    Q_OBJECT
public:
    explicit PermissionRow(const QString& title, const QString& desc, const QString& category, std::function<bool()> checkFunc, QWidget* parent = nullptr);
    void refresh();

private:
    QLabel* m_iconLabel;
    QPushButton* m_btnSettings;
    QString m_category;
    std::function<bool()> m_checkFunc;
};

class PermissionsWindow : public QDialog {
    Q_OBJECT
public:
    static PermissionsWindow* instance(QWidget* parent = nullptr);

protected:
    void changeEvent(QEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    explicit PermissionsWindow(QWidget* parent = nullptr);
    void setupUi();
    void setupStyleSheet();

    static PermissionsWindow* s_instance;
    QVBoxLayout* m_mainLayout = nullptr;
    QList<PermissionRow*> m_rows;
    QPoint m_dragPos;
};

} // namespace ScreenCut

#endif // SCREENCUT_PERMISSIONS_WINDOW_H
