/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <QPainter>
#include <QPointF>
#include <QColor>
#include "../editor/editor_models.h"

namespace ScreenCut {

enum class ArrowHead {
    None,

    // Arrow
    Open,
    FilledTriangle,

    // Diamond
    FilledDiamond,

    // Circle
    FilledCircle,

    // Square
    FilledSquare,

    // Tee
    Tee
};

struct ArrowStyle {
    QColor color = Qt::black;
    double lineWidth = 2.0;
    Qt::PenStyle penStyle = Qt::SolidLine;
    
    ArrowHead startHead = ArrowHead::None;
    ArrowHead endHead = ArrowHead::FilledTriangle;
    
    double startSize = 3.0; // Size modifier
    double endSize = 3.0;   // Size modifier
    
    ShadowStyle shadow;
    int opacity = 100;
};

class ArrowPainter {
public:
    static void draw(QPainter& painter, QPointF start, QPointF end, const ArrowStyle& style);
    static ArrowHead stringToArrowHead(const QString& styleStr);
    static QString arrowHeadToString(ArrowHead head);

private:
    static void drawHead(QPainter& painter, const QPointF& pt, const QPointF& otherPt, ArrowHead head, double lineWidth, double sizeMod);
    static void drawOpenArrow(QPainter& painter, const QPointF& pt, double angle, double size);
    static void drawTriangleArrow(QPainter& painter, const QPointF& pt, double angle, double size, bool filled);
    static void drawDiamondArrow(QPainter& painter, const QPointF& pt, double angle, double size, bool filled);
    static void drawCircleArrow(QPainter& painter, const QPointF& pt, double size, bool filled);
    static void drawSquareArrow(QPainter& painter, const QPointF& pt, double angle, double size, bool filled);
    static void drawTeeArrow(QPainter& painter, const QPointF& pt, double angle, double size);
};

} // namespace ScreenCut
