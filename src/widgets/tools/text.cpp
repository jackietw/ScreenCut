/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "text.h"
#include "../editor_canvas.h"
#include <QTextEdit>

namespace ScreenCut {

void TextTool::mousePressEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) {
    if (event->button() != Qt::LeftButton) return;
    
    QTextEdit* textInput = canvas->getTextInput();
    if (textInput && textInput->isVisible()) {
        if (!textInput->geometry().contains(event->pos())) {
            canvas->commitText();
        }
        return;
    }
    
    QPoint clickPoint = canvas->mapToImage(event->pos());
    canvas->showTextInput(clickPoint, "");
    
    // Apply styling to text input so user sees what they type
    if (textInput) {
        textInput->setTextColor(context.color);
        QFont font(context.fontFamily, context.fontSize);
        font.setBold(context.textIsBold);
        font.setItalic(context.textIsItalic);
        font.setUnderline(context.textIsUnderline);
        font.setStrikeOut(context.textIsStrikeOut);
        textInput->setFont(font);
    }
}

void TextTool::mouseMoveEvent(QMouseEvent* /*event*/, EditorCanvas* /*canvas*/, const ToolContext& /*context*/) {
    // Text doesn't drag to create
}

void TextTool::mouseReleaseEvent(QMouseEvent* /*event*/, EditorCanvas* /*canvas*/, const ToolContext& /*context*/) {
    // Release is ignored for text (handled on commit)
}

} // namespace ScreenCut
