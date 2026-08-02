/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "select.h"
#include "../editor_canvas.h"
#include <QCursor>

namespace ScreenCut {

void SelectTool::mousePressEvent(QMouseEvent* /*event*/, EditorCanvas* /*canvas*/, const ToolContext& /*context*/) {
    // Left empty for now. Moving and resizing is still somewhat deeply coupled in Canvas
    // due to active handles and list of annotations. We will migrate this gradually.
}

void SelectTool::mouseMoveEvent(QMouseEvent* /*event*/, EditorCanvas* canvas, const ToolContext& /*context*/) {
    canvas->setCursor(Qt::ArrowCursor);
}

void SelectTool::mouseReleaseEvent(QMouseEvent* /*event*/, EditorCanvas* /*canvas*/, const ToolContext& /*context*/) {
}

} // namespace ScreenCut
