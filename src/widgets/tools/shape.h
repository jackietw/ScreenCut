/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef SHAPE_TOOL_H
#define SHAPE_TOOL_H

#include "base.h"
#include <memory>
#include <QPoint>

namespace ScreenCut {

class ShapeAnnotation;
class PolygonAnnotation;

class ShapeTool : public BaseTool {
public:
    explicit ShapeTool(ToolType type) : m_type(type) {}
    ~ShapeTool() override = default;

    void mousePressEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) override;
    void mouseMoveEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) override;
    void mouseReleaseEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) override;
    void mouseDoubleClickEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) override;

private:
    ToolType m_type;
    QPoint m_startPoint;
    std::shared_ptr<ShapeAnnotation> m_tempItem;
};

} // namespace ScreenCut

#endif // SHAPE_TOOL_H
