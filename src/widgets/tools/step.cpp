/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "step.h"
#include "../editor_canvas.h"

namespace ScreenCut {

void StepTool::mousePressEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) {
    if (event->button() != Qt::LeftButton) return;
    
    QPoint startPoint = canvas->mapToImage(event->pos());
    
    canvas->saveToHistory();
    auto step = std::make_shared<StepMarkerAnnotation>(startPoint, canvas->getNextStepNumber());
    step->color = context.color;
    step->radius = context.lineWidth * 4; // Use line width as size base
    
    canvas->addAnnotation(step);
    canvas->updateCanvas();
}

void StepTool::mouseMoveEvent(QMouseEvent* /*event*/, EditorCanvas* /*canvas*/, const ToolContext& /*context*/) {
    // Doesn't drag
}

void StepTool::mouseReleaseEvent(QMouseEvent* /*event*/, EditorCanvas* /*canvas*/, const ToolContext& /*context*/) {
    // Handled in press
}

} // namespace ScreenCut
