/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef BASE_TOOL_H
#define BASE_TOOL_H

#include <QMouseEvent>
#include <QKeyEvent>
#include "context.h"

namespace ScreenCut {

class EditorCanvas;

class BaseTool {
public:
    virtual ~BaseTool() = default;

    virtual void mousePressEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) = 0;
    virtual void mouseMoveEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) = 0;
    virtual void mouseReleaseEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) = 0;
    
    virtual void mouseDoubleClickEvent(QMouseEvent* /*event*/, EditorCanvas* /*canvas*/, const ToolContext& /*context*/) {}
    virtual void keyPressEvent(QKeyEvent* /*event*/, EditorCanvas* /*canvas*/, const ToolContext& /*context*/) {}
};

} // namespace ScreenCut

#endif // BASE_TOOL_H
