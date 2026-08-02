/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef FREEHAND_TOOL_H
#define FREEHAND_TOOL_H

#include "base.h"
#include <memory>
#include <QPoint>

namespace ScreenCut {

class FreehandAnnotation;

class FreehandTool : public BaseTool {
public:
    FreehandTool() = default;
    ~FreehandTool() override = default;

    void mousePressEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) override;
    void mouseMoveEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) override;
    void mouseReleaseEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) override;

private:
    std::shared_ptr<FreehandAnnotation> m_tempItem;
};

} // namespace ScreenCut

#endif // FREEHAND_TOOL_H
