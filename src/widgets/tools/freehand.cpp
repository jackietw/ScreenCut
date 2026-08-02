/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "freehand.h"
#include "../editor_canvas.h"

namespace ScreenCut {

void FreehandTool::mousePressEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) {
    if (event->button() != Qt::LeftButton) return;
    
    QPoint startPoint = canvas->mapToImage(event->pos());
    
    auto freehand = std::make_shared<FreehandAnnotation>();
    freehand->color = context.color;
    freehand->lineWidth = context.lineWidth;
    freehand->penStyle = context.penStyle;
    freehand->addPoint(startPoint);
    
    m_tempItem = freehand;
    canvas->setTempItem(m_tempItem);
    canvas->setDrawing(true);
}

void FreehandTool::mouseMoveEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& /*context*/) {
    if (m_tempItem) {
        QPoint currentPoint = canvas->mapToImage(event->pos());
        m_tempItem->addPoint(currentPoint);
        canvas->updateCanvas();
    }
}

void FreehandTool::mouseReleaseEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& /*context*/) {
    if (event->button() != Qt::LeftButton) return;
    
    if (m_tempItem) {
        if (m_tempItem->points.size() > 1) {
            canvas->saveToHistory();
            canvas->addAnnotation(m_tempItem);
        }
        m_tempItem = nullptr;
        canvas->setTempItem(nullptr);
        canvas->setDrawing(false);
    }
}

} // namespace ScreenCut
