/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef EDITOR_TOOLBAR_H
#define EDITOR_TOOLBAR_H

#include <QToolBar>
#include <QActionGroup>
#include <QAction>
#include <QToolButton>
#include "../editor/editor_models.h"

namespace ScreenCut {

class EditorToolBar : public QToolBar {
    Q_OBJECT
public:
    explicit EditorToolBar(QWidget* parent = nullptr);
    ~EditorToolBar() override = default;

    void updateUndoRedoState(bool canUndo, bool canRedo);
    void setActiveTool(ToolType tool);

signals:
    void toolSelected(ToolType tool);
    void captureClicked();
    void undoClicked();
    void redoClicked();
    void copyClicked();
    void saveClicked();

private:
    void setupUI();
    void addToolAction(const QString& svg, ToolType type, const QString& text, const QString& tooltip);

    QActionGroup* m_toolGroup = nullptr;
    QAction* m_captureAction = nullptr;
    QAction* m_undoAction = nullptr;
    QAction* m_redoAction = nullptr;
    QAction* m_copyAction = nullptr;
    QAction* m_saveAction = nullptr;
};

} // namespace ScreenCut

#endif // EDITOR_TOOLBAR_H
