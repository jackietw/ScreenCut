/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef ARROW_TOOL_H
#define ARROW_TOOL_H

#include "base.h"
#include <memory>
#include <QPoint>

namespace ScreenCut {

class ArrowAnnotation;

class ArrowTool : public BaseTool {
public:
    ArrowTool() = default;
    ~ArrowTool() override = default;

    void mousePressEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) override;
    void mouseMoveEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) override;
    void mouseReleaseEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) override;

private:
    QPoint m_startPoint;
    std::shared_ptr<ArrowAnnotation> m_tempItem;
};

} // namespace ScreenCut

#endif // ARROW_TOOL_H
