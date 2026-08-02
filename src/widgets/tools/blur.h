/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef BLUR_TOOL_H
#define BLUR_TOOL_H

#include "base.h"
#include <memory>
#include <QPoint>

namespace ScreenCut {

class ShaderAnnotation;

class BlurTool : public BaseTool {
public:
    BlurTool() = default;
    ~BlurTool() override = default;

    void mousePressEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) override;
    void mouseMoveEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) override;
    void mouseReleaseEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) override;

private:
    QPoint m_startPoint;
    std::shared_ptr<ShaderAnnotation> m_tempItem;
};

} // namespace ScreenCut

#endif // BLUR_TOOL_H
