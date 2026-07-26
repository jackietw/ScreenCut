/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef EDITOR_WINDOW_H
#define EDITOR_WINDOW_H

#include <QMainWindow>
#include <QPixmap>
#include <QScrollArea>
#include <QDockWidget>
#include <QLabel>
#include <QMenu>
#include <QPushButton>

#include "../widgets/editor_toolbar.h"
#include "../widgets/editor_props.h"
#include "../widgets/editor_canvas.h"
#include "../widgets/editor_thumbs.h"

namespace ScreenCut {

class EditorMainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit EditorMainWindow(const QPixmap& initialPixmap, QWidget* parent = nullptr);
    ~EditorMainWindow() override;

    static EditorMainWindow* instance();
    static void openWithPixmap(const QPixmap& pixmap);
    void setPixmap(const QPixmap& pixmap);
    bool loadImageFile(const QString& filePath);

protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

public slots:
    void copyToClipboard();
    void saveToFile();
    void openFile();

private:
    void initUI();
    void initBottomBar();
    void updateZoomLabel();
    void updateResolutionLabel();

    EditorToolBar* m_toolBar = nullptr;
    EditorCanvas* m_canvas = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    
    QDockWidget* m_propsDock = nullptr;
    EditorPropsPanel* m_propsPanel = nullptr;
    
    QDockWidget* m_bottomDock = nullptr;
    EditorThumbsStrip* m_thumbsStrip = nullptr;
    
    QWidget* m_recentStripContainer = nullptr;
    QPushButton* m_btnToggleRecent = nullptr;
    QPushButton* m_btnZoom = nullptr;
    QPushButton* m_btnResize = nullptr;

    QString m_openedFilePath;
    bool m_isTempFile = false;
};

} // namespace ScreenCut

#endif // EDITOR_WINDOW_H
