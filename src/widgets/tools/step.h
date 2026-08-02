/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef STEP_TOOL_H
#define STEP_TOOL_H

#include "base.h"

namespace ScreenCut {

class StepTool : public BaseTool {
public:
    StepTool() = default;
    ~StepTool() override = default;

    void mousePressEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) override;
    void mouseMoveEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) override;
    void mouseReleaseEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) override;
};

} // namespace ScreenCut

#endif // STEP_TOOL_H
