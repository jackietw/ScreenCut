/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "arrow.h"
#include "../editor_canvas.h"

namespace ScreenCut {

void ArrowTool::mousePressEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) {
    if (event->button() != Qt::LeftButton) return;
    
    m_startPoint = canvas->mapToImage(event->pos());
    
    auto arrow = std::make_shared<ArrowAnnotation>(m_startPoint, m_startPoint);
    arrow->color = context.color;
    arrow->lineWidth = context.lineWidth;
    arrow->arrowType = context.arrowType;
    arrow->startStyle = context.arrowStartStyle;
    arrow->endStyle = context.arrowEndStyle;
    arrow->lineStyle = context.arrowLineStyle;
    arrow->opacity = context.arrowOpacity;
    arrow->startSize = context.arrowStartSize;
    arrow->endSize = context.arrowEndSize;
    arrow->shadow = context.arrowShadow;
    
    m_tempItem = arrow;
    canvas->setTempItem(m_tempItem);
    canvas->setDrawing(true);
}

void ArrowTool::mouseMoveEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& /*context*/) {
    if (m_tempItem) {
        m_tempItem->endPoint = canvas->mapToImage(event->pos());
        canvas->updateCanvas();
    }
}

void ArrowTool::mouseReleaseEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& /*context*/) {
    if (event->button() != Qt::LeftButton) return;
    
    if (m_tempItem) {
        if (m_tempItem->startPoint != m_tempItem->endPoint) {
            canvas->saveToHistory();
            canvas->addAnnotation(m_tempItem);
        }
        m_tempItem = nullptr;
        canvas->setTempItem(nullptr);
        canvas->setDrawing(false);
    }
}

} // namespace ScreenCut
