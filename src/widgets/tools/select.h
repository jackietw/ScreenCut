/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef SELECT_TOOL_H
#define SELECT_TOOL_H

#include "base.h"

namespace ScreenCut {

class SelectTool : public BaseTool {
public:
    SelectTool() = default;
    ~SelectTool() override = default;

    void mousePressEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) override;
    void mouseMoveEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) override;
    void mouseReleaseEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) override;
};

} // namespace ScreenCut

#endif // SELECT_TOOL_H
