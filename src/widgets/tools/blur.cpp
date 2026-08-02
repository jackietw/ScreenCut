/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "blur.h"
#include "../editor_canvas.h"

namespace ScreenCut {

void BlurTool::mousePressEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) {
    if (event->button() != Qt::LeftButton) return;
    
    m_startPoint = canvas->mapToImage(event->pos());
    
    m_tempItem = std::make_shared<ShaderAnnotation>(context.blurType, QRect(m_startPoint, m_startPoint));
    m_tempItem->intensity = context.blurIntensity;
    
    canvas->setTempItem(m_tempItem);
    canvas->setDrawing(true);
}

void BlurTool::mouseMoveEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& /*context*/) {
    if (m_tempItem) {
        QPoint currentPoint = canvas->mapToImage(event->pos());
        m_tempItem->rect = QRect(m_startPoint, currentPoint).normalized();
        canvas->updateCanvas();
    }
}

void BlurTool::mouseReleaseEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& /*context*/) {
    if (event->button() != Qt::LeftButton) return;
    
    if (m_tempItem) {
        if (m_tempItem->rect.width() > 0 || m_tempItem->rect.height() > 0) {
            canvas->saveToHistory();
            canvas->addAnnotation(m_tempItem);
        }
        m_tempItem = nullptr;
        canvas->setTempItem(nullptr);
        canvas->setDrawing(false);
    }
}

} // namespace ScreenCut
