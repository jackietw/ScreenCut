/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef TEXT_TOOL_H
#define TEXT_TOOL_H

#include "base.h"
#include <memory>
#include <QPoint>

namespace ScreenCut {

class TextAnnotation;

class TextTool : public BaseTool {
public:
    TextTool() = default;
    ~TextTool() override = default;

    void mousePressEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) override;
    void mouseMoveEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) override;
    void mouseReleaseEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) override;

private:
    std::shared_ptr<TextAnnotation> m_tempItem;
};

} // namespace ScreenCut

#endif // TEXT_TOOL_H
