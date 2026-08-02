/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef TOOL_CONTEXT_H
#define TOOL_CONTEXT_H

#include <QColor>
#include <QString>
#include "../../editor/editor_models.h"

namespace ScreenCut {

struct ToolContext {
    // Shared Props
    QColor color = QColor(255, 59, 48);
    int lineWidth = 3;
    
    // Arrow Props
    QString arrowType = "Custom";
    QString arrowStartStyle = "None";
    QString arrowEndStyle = "Arrow";
    QString arrowLineStyle = "Solid";
    int arrowOpacity = 100;
    int arrowStartSize = 3;
    int arrowEndSize = 3;
    ShadowStyle arrowShadow;

    // Shape Props
    QString shapeStyle = "Rectangle";
    QColor shapeFillColor = Qt::transparent;
    QColor shapeOutlineColor = QColor(255, 59, 48);
    int shapeThickness = 3;
    int shapeOpacity = 100;
    ShadowStyle shapeShadow;
    QString lineStyle = "Solid";

    // Text Props
    QString fontFamily = "Arial";
    int fontSize = 24;
    bool textIsBold = false;
    bool textIsItalic = false;
    bool textIsUnderline = false;
    bool textIsStrikeOut = false;
    TextAnnotation::TextAlign textHAlign = TextAnnotation::TextAlign::Left;
    TextAnnotation::VerticalAlign textVAlign = TextAnnotation::VerticalAlign::Top;
    int textOpacity = 100;
    int textLineSpacing = 0;
    QColor textOutlineColor = Qt::transparent;
    ShadowStyle textShadow;
    int textOutlineWidth = 0;

    // Other Props
    ToolType blurType = ToolType::Mosaic;
    int blurIntensity = 15;
    QString penStyle = "Solid Pen";
};

} // namespace ScreenCut

#endif // TOOL_CONTEXT_H
