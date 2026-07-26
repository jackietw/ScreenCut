/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef EDITOR_THUMBS_H
#define EDITOR_THUMBS_H

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QPixmap>
#include <vector>

namespace ScreenCut {

class EditorThumbsStrip : public QWidget {
    Q_OBJECT
public:
    explicit EditorThumbsStrip(QWidget* parent = nullptr);
    ~EditorThumbsStrip() override = default;

    void refreshLibrary();
    void setCurrentFile(const QString& filePath);

signals:
    void fileSelected(const QString& filePath);

private:
    void setupUI();
    
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_containerWidget = nullptr;
    QHBoxLayout* m_containerLayout = nullptr;
    
    QString m_currentFilePath;
    struct ThumbItem {
        QString filePath;
        QPushButton* button;
    };
    std::vector<ThumbItem> m_items;
};

} // namespace ScreenCut

#endif // EDITOR_THUMBS_H
