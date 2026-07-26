/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "editor_toolbar.h"
#include "../resources/IconUtils.h"

namespace ScreenCut {

EditorToolBar::EditorToolBar(QWidget* parent) : QToolBar(parent) {
    setupUI();
}

void EditorToolBar::setupUI() {
    setMovable(false);
    setFloatable(false);
    setIconSize(QSize(20, 20));
    setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    // Styling matching the prototype
    setStyleSheet(R"(
        QToolBar {
            background-color: #2b2b2b;
            border-bottom: 1px solid #3c3c3c;
            spacing: 6px;
            padding: 4px;
        }
        QToolButton {
            background: transparent;
            color: #cccccc;
            border-radius: 6px;
            padding: 4px;
        }
        QToolButton:hover {
            background: #3d3d3d;
            color: white;
        }
        QToolButton:checked {
            background: #246bb2;
            color: white;
        }
        QToolButton:disabled {
            color: #666666;
        }
    )");

    m_toolGroup = new QActionGroup(this);
    m_toolGroup->setExclusive(true);
    
    connect(m_toolGroup, &QActionGroup::triggered, this, [this](QAction* action) {
        if (action) {
            auto type = static_cast<ToolType>(action->data().toInt());
            emit toolSelected(type);
        }
    });

    addToolAction(SVG_SELECT, ToolType::None, "Select", "Select (V)");
    addSeparator();
    addToolAction(SVG_ARROW, ToolType::Arrow, "Arrow", "Arrow (A)");
    addToolAction(SVG_TEXT, ToolType::Text, "Text", "Text (T)");
    addToolAction(SVG_SHAPE, ToolType::Rectangle, "Shape", "Shape (R)");
    addToolAction(SVG_BLUR, ToolType::Mosaic, "Blur", "Blur (B)");
    addToolAction(SVG_PEN, ToolType::Freehand, "Pen", "Pen (P)");
    addToolAction(SVG_STEP, ToolType::StepMarker, "Step", "Step Marker (S)");

    // Default select arrow
    if (auto actionsList = m_toolGroup->actions(); actionsList.size() > 1) {
        actionsList[1]->setChecked(true); // Arrow by default
    }

    QWidget* spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    spacer->setStyleSheet("background: transparent;");
    addWidget(spacer);

    m_undoAction = addAction(createSvgIcon(SVG_UNDO), "Undo");
    m_undoAction->setShortcut(QKeySequence::Undo);
    connect(m_undoAction, &QAction::triggered, this, &EditorToolBar::undoClicked);
    
    m_redoAction = addAction(createSvgIcon(SVG_REDO), "Redo");
    m_redoAction->setShortcut(QKeySequence::Redo);
    connect(m_redoAction, &QAction::triggered, this, &EditorToolBar::redoClicked);

    addSeparator();

    m_copyAction = addAction(createSvgIcon(SVG_COPY), "Copy");
    m_copyAction->setShortcut(QKeySequence::Copy);
    connect(m_copyAction, &QAction::triggered, this, &EditorToolBar::copyClicked);

    m_saveAction = addAction(createSvgIcon(SVG_SAVE), "Save");
    m_saveAction->setShortcut(QKeySequence::Save);
    connect(m_saveAction, &QAction::triggered, this, &EditorToolBar::saveClicked);

    updateUndoRedoState(false, false);
}

void EditorToolBar::addToolAction(const QString& svg, ToolType type, const QString& text, const QString& tooltip) {
    QAction* action = new QAction(createSvgIcon(svg), text, this);
    action->setToolTip(tooltip);
    action->setCheckable(true);
    action->setData(static_cast<int>(type));
    m_toolGroup->addAction(action);
    addAction(action);
}

void EditorToolBar::updateUndoRedoState(bool canUndo, bool canRedo) {
    m_undoAction->setEnabled(canUndo);
    m_redoAction->setEnabled(canRedo);
}

} // namespace ScreenCut
