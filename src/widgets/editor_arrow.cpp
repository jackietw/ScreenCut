/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "editor_arrow.h"
#include <QtMath>
#include <QPolygonF>
#include <QPainterPath>

namespace ScreenCut {

ArrowHead ArrowPainter::stringToArrowHead(const QString& styleStr) {
    if (styleStr == "Open") return ArrowHead::Open;
    if (styleStr == "Triangle") return ArrowHead::Triangle;
    if (styleStr == "FilledTriangle" || styleStr == "Arrow") return ArrowHead::FilledTriangle;
    if (styleStr == "Diamond") return ArrowHead::Diamond;
    if (styleStr == "FilledDiamond") return ArrowHead::FilledDiamond;
    if (styleStr == "Circle") return ArrowHead::Circle;
    if (styleStr == "FilledCircle") return ArrowHead::FilledCircle;
    if (styleStr == "Square") return ArrowHead::Square;
    if (styleStr == "FilledSquare") return ArrowHead::FilledSquare;
    if (styleStr == "Tee") return ArrowHead::Tee;
    return ArrowHead::None;
}

QString ArrowPainter::arrowHeadToString(ArrowHead head) {
    switch (head) {
        case ArrowHead::Open: return "Open";
        case ArrowHead::Triangle: return "Triangle";
        case ArrowHead::FilledTriangle: return "FilledTriangle";
        case ArrowHead::Diamond: return "Diamond";
        case ArrowHead::FilledDiamond: return "FilledDiamond";
        case ArrowHead::Circle: return "Circle";
        case ArrowHead::FilledCircle: return "FilledCircle";
        case ArrowHead::Square: return "Square";
        case ArrowHead::FilledSquare: return "FilledSquare";
        case ArrowHead::Tee: return "Tee";
        default: return "None";
    }
}

void ArrowPainter::draw(QPainter& painter, QPointF start, QPointF end, const ArrowStyle& style) {
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);

    QColor drawColor = style.color;
    drawColor.setAlphaF(style.opacity / 100.0);

    // Draw shadow if needed
    if (style.hasShadow && style.shadowDirection != 0) {
        int sx = 0, sy = 0;
        int dist = 3;
        switch (style.shadowDirection) {
            case 1: sx = -dist; sy = -dist; break; // TopLeft
            case 2: sx = 0; sy = -dist; break;    // Top
            case 3: sx = dist; sy = -dist; break;   // TopRight
            case 4: sx = -dist; sy = 0; break;      // Left
            case 5: sx = dist; sy = 0; break;       // Right
            case 6: sx = -dist; sy = dist; break;   // BottomLeft
            case 7: sx = 0; sy = dist; break;       // Bottom
            case 8: sx = dist; sy = dist; break;    // BottomRight
            default: break;
        }
        painter.save();
        QPen shadowPen(QColor(0, 0, 0, 80), style.lineWidth, style.penStyle, Qt::FlatCap, Qt::RoundJoin);
        painter.setPen(shadowPen);
        painter.translate(sx, sy);
        painter.drawLine(start, end);
        painter.restore();
    }

    QPen pen(drawColor, style.lineWidth, style.penStyle, Qt::FlatCap, Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(drawColor);

    painter.drawLine(start, end);

    // Draw heads
    drawHead(painter, start, end, style.startHead, style.lineWidth, style.startSize);
    drawHead(painter, end, start, style.endHead, style.lineWidth, style.endSize);

    painter.restore();
}

void ArrowPainter::drawHead(QPainter& painter, const QPointF& pt, const QPointF& otherPt, ArrowHead head, double lineWidth, double sizeMod) {
    if (head == ArrowHead::None) return;

    double angle = std::atan2(pt.y() - otherPt.y(), pt.x() - otherPt.x());
    double baseSize = qMax(10.0, lineWidth * 3.0);
    double actualSize = baseSize * (sizeMod / 3.0); // 3 is default

    switch (head) {
        case ArrowHead::Open:
            drawOpenArrow(painter, pt, angle, actualSize);
            break;
        case ArrowHead::Triangle:
            drawTriangleArrow(painter, pt, angle, actualSize, false);
            break;
        case ArrowHead::FilledTriangle:
            drawTriangleArrow(painter, pt, angle, actualSize, true);
            break;
        case ArrowHead::Diamond:
            drawDiamondArrow(painter, pt, angle, actualSize, false);
            break;
        case ArrowHead::FilledDiamond:
            drawDiamondArrow(painter, pt, angle, actualSize, true);
            break;
        case ArrowHead::Circle:
            drawCircleArrow(painter, pt, actualSize, false);
            break;
        case ArrowHead::FilledCircle:
            drawCircleArrow(painter, pt, actualSize, true);
            break;
        case ArrowHead::Square:
            drawSquareArrow(painter, pt, angle, actualSize, false);
            break;
        case ArrowHead::FilledSquare:
            drawSquareArrow(painter, pt, angle, actualSize, true);
            break;
        case ArrowHead::Tee:
            drawTeeArrow(painter, pt, angle, actualSize);
            break;
        default:
            break;
    }
}

void ArrowPainter::drawOpenArrow(QPainter& painter, const QPointF& pt, double angle, double size) {
    double arrowAngle = qDegreesToRadians(25.0);
    double length = size * std::cos(arrowAngle);
    double shift = length / 2.0;
    
    QPointF tip = QPointF(pt.x() + shift * std::cos(angle),
                          pt.y() + shift * std::sin(angle));

    QPointF p1 = QPointF(tip.x() - size * std::cos(angle - arrowAngle),
                         tip.y() - size * std::sin(angle - arrowAngle));
    QPointF p2 = QPointF(tip.x() - size * std::cos(angle + arrowAngle),
                         tip.y() - size * std::sin(angle + arrowAngle));

    QPainterPath path;
    path.moveTo(p1);
    path.lineTo(tip);
    path.lineTo(p2);
    
    // We want the lines to be thick like the main line, but drawOpenArrow is called with painter's pen
    QPen pen = painter.pen();
    pen.setStyle(Qt::SolidLine); // Force solid for head
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);
}

void ArrowPainter::drawTriangleArrow(QPainter& painter, const QPointF& pt, double angle, double size, bool filled) {
    double arrowAngle = qDegreesToRadians(25.0);
    double length = size * std::cos(arrowAngle);
    double shift = length / 2.0;
    
    QPointF tip = QPointF(pt.x() + shift * std::cos(angle),
                          pt.y() + shift * std::sin(angle));

    QPointF p1 = QPointF(tip.x() - size * std::cos(angle - arrowAngle),
                         tip.y() - size * std::sin(angle - arrowAngle));
    QPointF p2 = QPointF(tip.x() - size * std::cos(angle + arrowAngle),
                         tip.y() - size * std::sin(angle + arrowAngle));

    QPolygonF arrowhead;
    arrowhead << tip << p1 << p2;

    if (filled) {
        painter.setPen(Qt::NoPen);
        // Brush is already set to color
        painter.drawPolygon(arrowhead);
    } else {
        QPen pen = painter.pen();
        pen.setStyle(Qt::SolidLine);
        pen.setJoinStyle(Qt::MiterJoin);
        painter.setPen(pen);
        painter.setBrush(Qt::white); // or transparent, but Snagit usually fills with transparent
        painter.drawPolygon(arrowhead);
    }
}

void ArrowPainter::drawDiamondArrow(QPainter& painter, const QPointF& pt, double angle, double size, bool filled) {
    double length = size * 1.2;
    double halfWidth = size * 0.4;
    
    // pt is the center
    QPointF tip = QPointF(pt.x() + (length / 2.0) * std::cos(angle),
                          pt.y() + (length / 2.0) * std::sin(angle));
    QPointF base = QPointF(pt.x() - (length / 2.0) * std::cos(angle),
                           pt.y() - (length / 2.0) * std::sin(angle));
    
    double perpAngle = angle + M_PI / 2.0;
    QPointF p1 = QPointF(pt.x() + halfWidth * std::cos(perpAngle),
                         pt.y() + halfWidth * std::sin(perpAngle));
    QPointF p2 = QPointF(pt.x() - halfWidth * std::cos(perpAngle),
                         pt.y() - halfWidth * std::sin(perpAngle));
                         
    QPolygonF diamond;
    diamond << tip << p1 << base << p2;
    
    if (filled) {
        painter.setPen(Qt::NoPen);
        painter.drawPolygon(diamond);
    } else {
        QPen pen = painter.pen();
        pen.setStyle(Qt::SolidLine);
        painter.setPen(pen);
        painter.setBrush(Qt::white);
        painter.drawPolygon(diamond);
    }
}

void ArrowPainter::drawCircleArrow(QPainter& painter, const QPointF& pt, double size, bool filled) {
    if (filled) {
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(pt, (int)(size / 2.0), (int)(size / 2.0));
    } else {
        QPen pen = painter.pen();
        pen.setStyle(Qt::SolidLine);
        painter.setPen(pen);
        painter.setBrush(Qt::white);
        painter.drawEllipse(pt, (int)(size / 2.0), (int)(size / 2.0));
    }
}

void ArrowPainter::drawSquareArrow(QPainter& painter, const QPointF& pt, double angle, double size, bool filled) {
    QPainterPath path;
    double halfSize = size / 2.0;
    
    // Calculate the 4 corners relative to center, unrotated
    QPointF c1(halfSize, halfSize);
    QPointF c2(halfSize, -halfSize);
    QPointF c3(-halfSize, -halfSize);
    QPointF c4(-halfSize, halfSize);
    
    // Rotate by angle
    auto rotate = [angle](const QPointF& p) {
        return QPointF(p.x() * std::cos(angle) - p.y() * std::sin(angle),
                       p.x() * std::sin(angle) + p.y() * std::cos(angle));
    };
    
    QPolygonF square;
    square << pt + rotate(c1) << pt + rotate(c2) << pt + rotate(c3) << pt + rotate(c4);
    
    if (filled) {
        painter.setPen(Qt::NoPen);
        painter.drawPolygon(square);
    } else {
        QPen pen = painter.pen();
        pen.setStyle(Qt::SolidLine);
        painter.setPen(pen);
        painter.setBrush(Qt::white);
        painter.drawPolygon(square);
    }
}

void ArrowPainter::drawTeeArrow(QPainter& painter, const QPointF& pt, double angle, double size) {
    double length = size;
    double perpAngle = angle + M_PI / 2.0;
    
    QPointF p1 = QPointF(pt.x() + (length / 2.0) * std::cos(perpAngle),
                         pt.y() + (length / 2.0) * std::sin(perpAngle));
    QPointF p2 = QPointF(pt.x() - (length / 2.0) * std::cos(perpAngle),
                         pt.y() - (length / 2.0) * std::sin(perpAngle));
                         
    QPen pen = painter.pen();
    pen.setStyle(Qt::SolidLine);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    painter.drawLine(p1, p2);
}

} // namespace ScreenCut
