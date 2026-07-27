/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "editor_models.h"
#include <QtMath>
#include <QFont>
#include <QFontMetrics>
#include <QGraphicsBlurEffect>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QImage>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

namespace ScreenCut {

// ============================================================================
// ArrowAnnotation
// ============================================================================
ArrowAnnotation::ArrowAnnotation(const QPoint& start, const QPoint& end)
    : startPoint(start), endPoint(end) {}

void ArrowAnnotation::draw(QPainter& painter, const QPixmap* /*background*/) {
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);

    QPen pen(color, lineWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(color);

    // Draw main line
    painter.drawLine(startPoint, endPoint);

    // Calculate arrowhead geometry
    if (arrowType != "Plain Line") {
        double angle = std::atan2(endPoint.y() - startPoint.y(), endPoint.x() - startPoint.x());
        double arrowLength = qMax(12.0, lineWidth * 3.5);
        double arrowAngle = qDegreesToRadians(25.0);

        QPointF p1 = QPointF(endPoint.x() - arrowLength * std::cos(angle - arrowAngle),
                             endPoint.y() - arrowLength * std::sin(angle - arrowAngle));
        QPointF p2 = QPointF(endPoint.x() - arrowLength * std::cos(angle + arrowAngle),
                             endPoint.y() - arrowLength * std::sin(angle + arrowAngle));

        QPolygonF arrowhead;
        arrowhead << endPoint << p1 << p2;
        painter.drawPolygon(arrowhead);

        if (arrowType == "Double Arrow") {
            QPointF sp1 = QPointF(startPoint.x() + arrowLength * std::cos(angle - arrowAngle),
                                  startPoint.y() + arrowLength * std::sin(angle - arrowAngle));
            QPointF sp2 = QPointF(startPoint.x() + arrowLength * std::cos(angle + arrowAngle),
                                  startPoint.y() + arrowLength * std::sin(angle + arrowAngle));
            QPolygonF arrowhead2;
            arrowhead2 << startPoint << sp1 << sp2;
            painter.drawPolygon(arrowhead2);
        }
    }

    if (isSelected) {
        painter.setPen(QPen(Qt::white, 1, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(QRect(startPoint, endPoint).normalized().adjusted(-4, -4, 4, 4));
        
        painter.setPen(QPen(QColor(0, 168, 255), 1));
        painter.setBrush(Qt::white);
        painter.drawRect(startPoint.x() - 4, startPoint.y() - 4, 8, 8);
        painter.drawRect(endPoint.x() - 4, endPoint.y() - 4, 8, 8);
    }
    painter.restore();
}

bool ArrowAnnotation::contains(const QPoint& pos) const {
    QRect bounds = QRect(startPoint, endPoint).normalized().adjusted(-8, -8, 8, 8);
    return bounds.contains(pos);
}

void ArrowAnnotation::moveBy(const QPoint& delta) {
    startPoint += delta;
    endPoint += delta;
}

int ArrowAnnotation::hitTestHandle(const QPoint& pos) const {
    if (isSelected) {
        if (QRect(startPoint.x() - 6, startPoint.y() - 6, 12, 12).contains(pos)) return 1;
        if (QRect(endPoint.x() - 6, endPoint.y() - 6, 12, 12).contains(pos)) return 2;
    }
    return -1;
}

void ArrowAnnotation::moveHandle(int handleId, const QPoint& newPos) {
    if (handleId == 1) startPoint = newPos;
    else if (handleId == 2) endPoint = newPos;
}

std::shared_ptr<AnnotationItem> ArrowAnnotation::clone() const {
    auto copy = std::make_shared<ArrowAnnotation>(startPoint, endPoint);
    copy->color = color;
    copy->lineWidth = lineWidth;
    copy->arrowType = arrowType;
    return copy;
}

// ============================================================================
// ShapeAnnotation (Rectangle / Ellipse)
// ============================================================================
ShapeAnnotation::ShapeAnnotation(ToolType type, const QRect& r)
    : shapeType(type), rect(r.normalized()) {}

void ShapeAnnotation::draw(QPainter& painter, const QPixmap* /*background*/) {
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);

    QPen pen(color, lineWidth, (lineStyle == "Dashed") ? Qt::DashLine : Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);

    if (isFilled) {
        QColor fillColor = color;
        fillColor.setAlpha(60);
        painter.setBrush(fillColor);
    } else {
        painter.setBrush(Qt::NoBrush);
    }

    if (shapeStyle == "Rectangle") {
        painter.drawRect(rect);
    } else if (shapeStyle == "Rounded Rectangle") {
        painter.drawRoundedRect(rect, 8, 8);
    } else if (shapeStyle == "Ellipse") {
        painter.drawEllipse(rect);
    }

    if (isSelected) {
        painter.setPen(QPen(Qt::white, 1, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(rect.adjusted(-4, -4, 4, 4));
        
        painter.setPen(QPen(QColor(0, 168, 255), 1));
        painter.setBrush(Qt::white);
        QRect r = rect.normalized();
        painter.drawRect(r.left() - 4, r.top() - 4, 8, 8);
        painter.drawRect(r.right() - 4, r.top() - 4, 8, 8);
        painter.drawRect(r.left() - 4, r.bottom() - 4, 8, 8);
        painter.drawRect(r.right() - 4, r.bottom() - 4, 8, 8);
        
        int midX = r.left() + r.width() / 2;
        int midY = r.top() + r.height() / 2;
        painter.drawRect(midX - 4, r.top() - 4, 8, 8);
        painter.drawRect(midX - 4, r.bottom() - 4, 8, 8);
        painter.drawRect(r.left() - 4, midY - 4, 8, 8);
        painter.drawRect(r.right() - 4, midY - 4, 8, 8);
    }
    painter.restore();
}

bool ShapeAnnotation::contains(const QPoint& pos) const {
    if (isFilled) {
        return rect.adjusted(-6, -6, 6, 6).contains(pos);
    } else {
        QRect outer = rect.adjusted(-6, -6, 6, 6);
        QRect inner = rect.adjusted(6, 6, -6, -6);
        return outer.contains(pos) && (!inner.isValid() || !inner.contains(pos));
    }
}

void ShapeAnnotation::moveBy(const QPoint& delta) {
    rect.translate(delta);
}

int ShapeAnnotation::hitTestHandle(const QPoint& pos) const {
    if (isSelected) {
        QRect r = rect.normalized();
        if (QRect(r.left() - 6, r.top() - 6, 12, 12).contains(pos)) return 1;
        if (QRect(r.right() - 6, r.top() - 6, 12, 12).contains(pos)) return 2;
        if (QRect(r.right() - 6, r.bottom() - 6, 12, 12).contains(pos)) return 3;
        if (QRect(r.left() - 6, r.bottom() - 6, 12, 12).contains(pos)) return 4;
        
        int midX = r.left() + r.width() / 2;
        int midY = r.top() + r.height() / 2;
        if (QRect(midX - 6, r.top() - 6, 12, 12).contains(pos)) return 5;
        if (QRect(midX - 6, r.bottom() - 6, 12, 12).contains(pos)) return 6;
        if (QRect(r.left() - 6, midY - 6, 12, 12).contains(pos)) return 7;
        if (QRect(r.right() - 6, midY - 6, 12, 12).contains(pos)) return 8;
    }
    return -1;
}

void ShapeAnnotation::moveHandle(int handleId, const QPoint& newPos) {
    QRect r = rect.normalized();
    if (handleId == 1) r.setTopLeft(newPos);
    else if (handleId == 2) r.setTopRight(newPos);
    else if (handleId == 3) r.setBottomRight(newPos);
    else if (handleId == 4) r.setBottomLeft(newPos);
    else if (handleId == 5) r.setTop(newPos.y());
    else if (handleId == 6) r.setBottom(newPos.y());
    else if (handleId == 7) r.setLeft(newPos.x());
    else if (handleId == 8) r.setRight(newPos.x());
    rect = r.normalized();
}

std::shared_ptr<AnnotationItem> ShapeAnnotation::clone() const {
    auto copy = std::make_shared<ShapeAnnotation>(shapeType, rect);
    copy->color = color;
    copy->lineWidth = lineWidth;
    copy->isFilled = isFilled;
    copy->shapeStyle = shapeStyle;
    copy->lineStyle = lineStyle;
    return copy;
}

// ============================================================================
// FreehandAnnotation
// ============================================================================
FreehandAnnotation::FreehandAnnotation() = default;

void FreehandAnnotation::addPoint(const QPoint& point) {
    points.push_back(point);
    if (points.size() == 1) {
        path.moveTo(point);
    } else {
        path.lineTo(point);
    }
}

void FreehandAnnotation::draw(QPainter& painter, const QPixmap* /*background*/) {
    if (points.empty()) return;

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);

    QColor c = color;
    if (penStyle == "Highlighter") {
        c.setAlpha(120);
    }
    QPen pen(c, lineWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    painter.drawPath(path);

    if (isSelected) {
        painter.setPen(QPen(Qt::white, 1, Qt::DashLine));
        painter.drawRect(path.boundingRect().adjusted(-4, -4, 4, 4));
    }
    painter.restore();
}

bool FreehandAnnotation::contains(const QPoint& pos) const {
    return path.boundingRect().adjusted(-6, -6, 6, 6).contains(pos);
}

void FreehandAnnotation::moveBy(const QPoint& delta) {
    for (auto& pt : points) {
        pt += delta;
    }
    path.translate(delta);
}

std::shared_ptr<AnnotationItem> FreehandAnnotation::clone() const {
    auto copy = std::make_shared<FreehandAnnotation>();
    copy->color = color;
    copy->lineWidth = lineWidth;
    copy->points = points;
    copy->path = path;
    copy->penStyle = penStyle;
    return copy;
}

// ============================================================================
// TextAnnotation
// ============================================================================
TextAnnotation::TextAnnotation(const QPoint& pos, const QString& txt)
    : position(pos), text(txt) {}

void TextAnnotation::draw(QPainter& painter, const QPixmap* /*background*/) {
    if (text.isEmpty()) return;

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    // Apply Opacity
    painter.setOpacity(opacity / 100.0);

    QFont font = painter.font();
    font.setFamily(fontFamily);
    font.setPointSize(fontSize);
    font.setBold(isBold);
    font.setItalic(isItalic);
    font.setUnderline(isUnderline);
    font.setStrikeOut(isStrikeOut);
    painter.setFont(font);

    // Calculate bounding rect with line spacing
    QFontMetrics fm(font);
    int flags = Qt::TextWordWrap;
    if (hAlign == TextAlign::Left) flags |= Qt::AlignLeft;
    else if (hAlign == TextAlign::Center) flags |= Qt::AlignHCenter;
    else if (hAlign == TextAlign::Right) flags |= Qt::AlignRight;

    if (vAlign == VerticalAlign::Top) flags |= Qt::AlignTop;
    else if (vAlign == VerticalAlign::Middle) flags |= Qt::AlignVCenter;
    else if (vAlign == VerticalAlign::Bottom) flags |= Qt::AlignBottom;

    // A hack for line spacing is to adjust the rect height based on line count, but for QPainter::drawText it's tricky.
    // For now, we'll let drawText handle standard rendering and just use the font metrics.
    QRect textRect = fm.boundingRect(QRect(0, 0, 800, 800), flags, text);
    
    // Add extra padding for line spacing if we manually draw lines (skip for now to keep it simple, just add to height)
    int lineCount = text.split('\n').count();
    textRect.setHeight(textRect.height() + (lineCount - 1) * lineSpacing);
    
    textRect.moveTo(position);
    textRect.adjust(-8, -6, 8, 6); // Add some padding

    if (hasBackgroundBox) {
        painter.setPen(Qt::NoPen);
        QColor bgColor = color;
        bgColor.setAlpha(220);
        painter.setBrush(bgColor);
        painter.drawRoundedRect(textRect, 6, 6);
        painter.setPen(Qt::white); // default text color if box is enabled (legacy behavior)
    } else {
        painter.setPen(color); // Fill color
    }

    // Create a QPainterPath for the text to handle precise layout, outline, and shadow
    QPainterPath textPath;
    QStringList lines = text.split('\n');
    
    // We calculate the exact bounding box for the text to handle alignment
    int totalHeight = 0;
    for (int i = 0; i < lines.size(); ++i) {
        totalHeight += fm.height() + lineSpacing;
    }
    if (lines.size() > 0) totalHeight -= lineSpacing; // remove extra spacing after last line
    
    int startY = textRect.y() + 6; // top padding
    if (vAlign == VerticalAlign::Middle) {
        startY = textRect.y() + (textRect.height() - totalHeight) / 2;
    } else if (vAlign == VerticalAlign::Bottom) {
        startY = textRect.bottom() - 6 - totalHeight;
    }
    
    int currentY = startY + fm.ascent();
    
    for (const QString& line : lines) {
        int lineW = fm.horizontalAdvance(line);
        int currentX = textRect.x() + 8; // left padding
        if (hAlign == TextAlign::Center) {
            currentX = textRect.x() + (textRect.width() - lineW) / 2;
        } else if (hAlign == TextAlign::Right) {
            currentX = textRect.right() - 8 - lineW;
        }
        textPath.addText(currentX, currentY, font, line);
        currentY += fm.height() + lineSpacing;
    }

    // Shadow
    if (hasShadow && shadowDirection != ShadowDirection::None) {
        painter.save();
        QColor shadowColor = QColor(0, 0, 0, 150);
        
        int dx = 0, dy = 0;
        int offset = 4; // Slightly larger offset for better visibility
        if (shadowDirection == ShadowDirection::TopLeft) { dx = -offset; dy = -offset; }
        else if (shadowDirection == ShadowDirection::Top) { dx = 0; dy = -offset; }
        else if (shadowDirection == ShadowDirection::TopRight) { dx = offset; dy = -offset; }
        else if (shadowDirection == ShadowDirection::Left) { dx = -offset; dy = 0; }
        else if (shadowDirection == ShadowDirection::Right) { dx = offset; dy = 0; }
        else if (shadowDirection == ShadowDirection::BottomLeft) { dx = -offset; dy = offset; }
        else if (shadowDirection == ShadowDirection::Bottom) { dx = 0; dy = offset; }
        else if (shadowDirection == ShadowDirection::BottomRight) { dx = offset; dy = offset; }

        painter.translate(dx, dy);
        painter.fillPath(textPath, shadowColor);
        painter.translate(-dx, -dy);
        painter.restore();
    }

    // Outline
    if (outlineColor != Qt::transparent && outlineWidth > 0) {
        QPen outlinePen(outlineColor, outlineWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.strokePath(textPath, outlinePen);
    }
    
    // Fill
    painter.fillPath(textPath, color);

    if (isSelected) {
        painter.setPen(QPen(Qt::white, 1, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        QRect r = textRect.adjusted(-2, -2, 2, 2);
        painter.drawRect(r);
        
        painter.setPen(QPen(QColor(0, 168, 255), 1));
        painter.setBrush(Qt::white);
        painter.drawRect(r.left() - 4, r.top() - 4, 8, 8);
        painter.drawRect(r.right() - 4, r.top() - 4, 8, 8);
        painter.drawRect(r.left() - 4, r.bottom() - 4, 8, 8);
        painter.drawRect(r.right() - 4, r.bottom() - 4, 8, 8);
        
        int midX = r.left() + r.width() / 2;
        int midY = r.top() + r.height() / 2;
        painter.drawRect(midX - 4, r.top() - 4, 8, 8);
        painter.drawRect(midX - 4, r.bottom() - 4, 8, 8);
        painter.drawRect(r.left() - 4, midY - 4, 8, 8);
        painter.drawRect(r.right() - 4, midY - 4, 8, 8);
    }
    painter.restore();
}

bool TextAnnotation::contains(const QPoint& pos) const {
    QFont font(fontFamily);
    font.setPointSize(fontSize);
    font.setBold(isBold);
    font.setItalic(isItalic);
    QFontMetrics fm(font);
    
    int flags = Qt::TextWordWrap;
    QRect textRect = fm.boundingRect(QRect(0, 0, 800, 800), flags, text);
    int lineCount = qMax(1, (int)text.split('\n').count());
    textRect.setHeight(textRect.height() + (lineCount - 1) * lineSpacing);
    textRect.moveTo(position);
    textRect.adjust(-8, -6, 8, 6);
    
    return textRect.adjusted(-10, -10, 10, 10).contains(pos);
}

void TextAnnotation::moveBy(const QPoint& delta) {
    position += delta;
}

std::shared_ptr<AnnotationItem> TextAnnotation::clone() const {
    auto copy = std::make_shared<TextAnnotation>(position, text);
    copy->color = color;
    copy->fontSize = fontSize;
    copy->fontFamily = fontFamily;
    copy->hasBackgroundBox = hasBackgroundBox;
    copy->isBold = isBold;
    copy->isItalic = isItalic;
    copy->isUnderline = isUnderline;
    copy->isStrikeOut = isStrikeOut;
    copy->hAlign = hAlign;
    copy->vAlign = vAlign;
    copy->opacity = opacity;
    copy->lineSpacing = lineSpacing;
    copy->outlineColor = outlineColor;
    copy->hasShadow = hasShadow;
    copy->outlineWidth = outlineWidth;
    copy->shadowDirection = shadowDirection;
    return copy;
}

// ============================================================================
// StepMarkerAnnotation (1, 2, 3... Step sequence markers)
// ============================================================================
StepMarkerAnnotation::StepMarkerAnnotation(const QPoint& c, int num)
    : center(c), stepNumber(num) {}

void StepMarkerAnnotation::draw(QPainter& painter, const QPixmap* /*background*/) {
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    // Draw drop shadow
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 80));
    painter.drawEllipse(center + QPoint(2, 2), radius, radius);

    // Draw main colored circle
    painter.setBrush(color);
    painter.drawEllipse(center, radius, radius);

    // Draw white border around circle
    painter.setPen(QPen(Qt::white, 2, Qt::SolidLine));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(center, radius, radius);

    // Draw step number centered
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPointSize(qMax(9, radius - 4));
    font.setBold(true);
    painter.setFont(font);

    QRect textRect(center.x() - radius, center.y() - radius, radius * 2, radius * 2);
    painter.drawText(textRect, Qt::AlignCenter, QString::number(stepNumber));

    if (isSelected) {
        painter.setPen(QPen(Qt::white, 1, Qt::DashLine));
        painter.drawRect(textRect.adjusted(-4, -4, 4, 4));
    }
    painter.restore();
}

bool StepMarkerAnnotation::contains(const QPoint& pos) const {
    return (pos - center).manhattanLength() <= radius + 4;
}

void StepMarkerAnnotation::moveBy(const QPoint& delta) {
    center += delta;
}

std::shared_ptr<AnnotationItem> StepMarkerAnnotation::clone() const {
    auto copy = std::make_shared<StepMarkerAnnotation>(center, stepNumber);
    copy->color = color;
    copy->radius = radius;
    return copy;
}

// ============================================================================
// ShaderAnnotation (Mosaic / Gaussian Blur)
// ============================================================================
ShaderAnnotation::ShaderAnnotation(ToolType type, const QRect& r)
    : shaderType(type), rect(r.normalized()) {}

void ShaderAnnotation::draw(QPainter& painter, const QPixmap* background) {
    if (!background || background->isNull() || rect.isEmpty()) return;

    painter.save();
    QRect targetRect = rect.intersected(background->rect());
    if (targetRect.isEmpty()) {
        painter.restore();
        return;
    }

    QImage sourceImg = background->copy(targetRect).toImage();

    if (shaderType == ToolType::Mosaic) {
        // Pixelation shader effect
        int step = qMax(4, intensity);
        for (int y = 0; y < sourceImg.height(); y += step) {
            for (int x = 0; x < sourceImg.width(); x += step) {
                int rx = qMin(step, sourceImg.width() - x);
                int ry = qMin(step, sourceImg.height() - y);
                
                // Sample center or average color of block
                QColor blockColor = sourceImg.pixelColor(x + rx/2, y + ry/2);
                
                for (int by = y; by < y + ry; ++by) {
                    for (int bx = x; bx < x + rx; ++bx) {
                        sourceImg.setPixelColor(bx, by, blockColor);
                    }
                }
            }
        }
    } else if (shaderType == ToolType::Blur) {
        // Fast box blur / Gaussian blur simulation
        int blurFactor = qMax(1, intensity);
        QImage blurred = sourceImg.scaled(qMax(1, sourceImg.width() / blurFactor), qMax(1, sourceImg.height() / blurFactor),
                                          Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        sourceImg = blurred.scaled(sourceImg.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    painter.drawImage(targetRect.topLeft(), sourceImg);

    if (isSelected) {
        painter.setPen(QPen(Qt::white, 1, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        QRect r = targetRect.adjusted(-2, -2, 2, 2);
        painter.drawRect(r);
        
        painter.setPen(QPen(QColor(0, 168, 255), 1));
        painter.setBrush(Qt::white);
        painter.drawRect(r.left() - 4, r.top() - 4, 8, 8);
        painter.drawRect(r.right() - 4, r.top() - 4, 8, 8);
        painter.drawRect(r.left() - 4, r.bottom() - 4, 8, 8);
        painter.drawRect(r.right() - 4, r.bottom() - 4, 8, 8);
        
        int midX = r.left() + r.width() / 2;
        int midY = r.top() + r.height() / 2;
        painter.drawRect(midX - 4, r.top() - 4, 8, 8);
        painter.drawRect(midX - 4, r.bottom() - 4, 8, 8);
        painter.drawRect(r.left() - 4, midY - 4, 8, 8);
        painter.drawRect(r.right() - 4, midY - 4, 8, 8);
    }
    painter.restore();
}

bool ShaderAnnotation::contains(const QPoint& pos) const {
    return rect.contains(pos);
}

int ShaderAnnotation::hitTestHandle(const QPoint& pos) const {
    if (isSelected) {
        QRect r = rect.normalized();
        if (QRect(r.left() - 6, r.top() - 6, 12, 12).contains(pos)) return 1;
        if (QRect(r.right() - 6, r.top() - 6, 12, 12).contains(pos)) return 2;
        if (QRect(r.right() - 6, r.bottom() - 6, 12, 12).contains(pos)) return 3;
        if (QRect(r.left() - 6, r.bottom() - 6, 12, 12).contains(pos)) return 4;
        
        int midX = r.left() + r.width() / 2;
        int midY = r.top() + r.height() / 2;
        if (QRect(midX - 6, r.top() - 6, 12, 12).contains(pos)) return 5;
        if (QRect(midX - 6, r.bottom() - 6, 12, 12).contains(pos)) return 6;
        if (QRect(r.left() - 6, midY - 6, 12, 12).contains(pos)) return 7;
        if (QRect(r.right() - 6, midY - 6, 12, 12).contains(pos)) return 8;
    }
    return -1;
}

void ShaderAnnotation::moveHandle(int handleId, const QPoint& newPos) {
    QRect r = rect.normalized();
    if (handleId == 1) r.setTopLeft(newPos);
    else if (handleId == 2) r.setTopRight(newPos);
    else if (handleId == 3) r.setBottomRight(newPos);
    else if (handleId == 4) r.setBottomLeft(newPos);
    else if (handleId == 5) r.setTop(newPos.y());
    else if (handleId == 6) r.setBottom(newPos.y());
    else if (handleId == 7) r.setLeft(newPos.x());
    else if (handleId == 8) r.setRight(newPos.x());
    rect = r.normalized();
}

void ShaderAnnotation::moveBy(const QPoint& delta) {
    rect.translate(delta);
}

std::shared_ptr<AnnotationItem> ShaderAnnotation::clone() const {
    auto copy = std::make_shared<ShaderAnnotation>(shaderType, rect);
    copy->blockSize = blockSize;
    copy->intensity = intensity;
    return copy;
}

// ============================================================================
// HighlightAnnotation (Yellow / Green translucent highlighter)
// ============================================================================
HighlightAnnotation::HighlightAnnotation(const QRect& r)
    : rect(r.normalized()) {
    color = QColor(255, 235, 59); // Modern yellow highlighter
}

void HighlightAnnotation::draw(QPainter& painter, const QPixmap* /*background*/) {
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);

    QColor highlightColor = color;
    highlightColor.setAlpha(120);

    painter.setPen(Qt::NoPen);
    painter.setBrush(highlightColor);
    painter.drawRoundedRect(rect, 4, 4);

    if (isSelected) {
        painter.setPen(QPen(Qt::white, 1, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        QRect r = rect.adjusted(-2, -2, 2, 2);
        painter.drawRect(r);
        
        painter.setPen(QPen(QColor(0, 168, 255), 1));
        painter.setBrush(Qt::white);
        painter.drawRect(r.left() - 4, r.top() - 4, 8, 8);
        painter.drawRect(r.right() - 4, r.top() - 4, 8, 8);
        painter.drawRect(r.left() - 4, r.bottom() - 4, 8, 8);
        painter.drawRect(r.right() - 4, r.bottom() - 4, 8, 8);
        
        int midX = r.left() + r.width() / 2;
        int midY = r.top() + r.height() / 2;
        painter.drawRect(midX - 4, r.top() - 4, 8, 8);
        painter.drawRect(midX - 4, r.bottom() - 4, 8, 8);
        painter.drawRect(r.left() - 4, midY - 4, 8, 8);
        painter.drawRect(r.right() - 4, midY - 4, 8, 8);
    }
    painter.restore();
}

bool HighlightAnnotation::contains(const QPoint& pos) const {
    return rect.contains(pos);
}

int HighlightAnnotation::hitTestHandle(const QPoint& pos) const {
    if (isSelected) {
        QRect r = rect.normalized();
        if (QRect(r.left() - 6, r.top() - 6, 12, 12).contains(pos)) return 1;
        if (QRect(r.right() - 6, r.top() - 6, 12, 12).contains(pos)) return 2;
        if (QRect(r.right() - 6, r.bottom() - 6, 12, 12).contains(pos)) return 3;
        if (QRect(r.left() - 6, r.bottom() - 6, 12, 12).contains(pos)) return 4;
        
        int midX = r.left() + r.width() / 2;
        int midY = r.top() + r.height() / 2;
        if (QRect(midX - 6, r.top() - 6, 12, 12).contains(pos)) return 5;
        if (QRect(midX - 6, r.bottom() - 6, 12, 12).contains(pos)) return 6;
        if (QRect(r.left() - 6, midY - 6, 12, 12).contains(pos)) return 7;
        if (QRect(r.right() - 6, midY - 6, 12, 12).contains(pos)) return 8;
    }
    return -1;
}

void HighlightAnnotation::moveHandle(int handleId, const QPoint& newPos) {
    QRect r = rect.normalized();
    if (handleId == 1) r.setTopLeft(newPos);
    else if (handleId == 2) r.setTopRight(newPos);
    else if (handleId == 3) r.setBottomRight(newPos);
    else if (handleId == 4) r.setBottomLeft(newPos);
    else if (handleId == 5) r.setTop(newPos.y());
    else if (handleId == 6) r.setBottom(newPos.y());
    else if (handleId == 7) r.setLeft(newPos.x());
    else if (handleId == 8) r.setRight(newPos.x());
    rect = r.normalized();
}

void HighlightAnnotation::moveBy(const QPoint& delta) {
    rect.translate(delta);
}

std::shared_ptr<AnnotationItem> HighlightAnnotation::clone() const {
    auto copy = std::make_shared<HighlightAnnotation>(rect);
    copy->color = color;
    return copy;
}

// ============================================================================
// JSON Serialization / Deserialization
// ============================================================================

QJsonObject ArrowAnnotation::toJson() const {
    QJsonObject obj;
    obj["obj_type"] = "arrow";
    obj["color"] = color.name(QColor::HexArgb);
    obj["width"] = lineWidth;
    QJsonArray sp; sp.append(startPoint.x()); sp.append(startPoint.y());
    QJsonArray ep; ep.append(endPoint.x()); ep.append(endPoint.y());
    obj["start_pos"] = sp;
    obj["end_pos"] = ep;
    return obj;
}

QJsonObject ShapeAnnotation::toJson() const {
    QJsonObject obj;
    obj["obj_type"] = "shape";
    obj["color"] = color.name(QColor::HexArgb);
    obj["width"] = lineWidth;
    obj["shape_type"] = (shapeType == ToolType::Rectangle) ? "Rectangle" : "Ellipse";
    obj["is_filled"] = isFilled;
    QJsonArray r; r.append(rect.x()); r.append(rect.y()); r.append(rect.width()); r.append(rect.height());
    obj["rect"] = r;
    return obj;
}

QJsonObject FreehandAnnotation::toJson() const {
    QJsonObject obj;
    obj["obj_type"] = "freehand";
    obj["color"] = color.name(QColor::HexArgb);
    obj["width"] = lineWidth;
    QJsonArray pts;
    for (const QPoint& pt : points) {
        QJsonArray p; p.append(pt.x()); p.append(pt.y());
        pts.append(p);
    }
    obj["points"] = pts;
    return obj;
}

QJsonObject TextAnnotation::toJson() const {
    QJsonObject obj;
    obj["obj_type"] = "text";
    obj["color"] = color.name(QColor::HexArgb);
    obj["width"] = lineWidth;
    QJsonArray pos; pos.append(position.x()); pos.append(position.y());
    obj["pos"] = pos;
    obj["text"] = text;
    obj["font_size"] = fontSize;
    obj["has_bg"] = hasBackgroundBox;
    return obj;
}

QJsonObject StepMarkerAnnotation::toJson() const {
    QJsonObject obj;
    obj["obj_type"] = "step";
    obj["color"] = color.name(QColor::HexArgb);
    obj["width"] = lineWidth;
    QJsonArray c; c.append(center.x()); c.append(center.y());
    obj["center"] = c;
    obj["step_number"] = stepNumber;
    obj["radius"] = radius;
    return obj;
}

QJsonObject ShaderAnnotation::toJson() const {
    QJsonObject obj;
    obj["obj_type"] = "shader";
    obj["color"] = color.name(QColor::HexArgb);
    obj["width"] = lineWidth;
    obj["shader_type"] = (shaderType == ToolType::Mosaic) ? "Mosaic" : "Blur";
    obj["block_size"] = blockSize;
    QJsonArray r; r.append(rect.x()); r.append(rect.y()); r.append(rect.width()); r.append(rect.height());
    obj["rect"] = r;
    return obj;
}

QJsonObject HighlightAnnotation::toJson() const {
    QJsonObject obj;
    obj["obj_type"] = "highlight";
    obj["color"] = color.name(QColor::HexArgb);
    obj["width"] = lineWidth;
    QJsonArray r; r.append(rect.x()); r.append(rect.y()); r.append(rect.width()); r.append(rect.height());
    obj["rect"] = r;
    return obj;
}

std::shared_ptr<AnnotationItem> AnnotationItem::fromJson(const QJsonObject& json) {
    QString objType = json["obj_type"].toString();
    QColor col(json["color"].toString("#ff3b30"));
    int w = json["width"].toInt(3);

    if (objType == "arrow") {
        QJsonArray sp = json["start_pos"].toArray();
        QJsonArray ep = json["end_pos"].toArray();
        if (sp.size() < 2 || ep.size() < 2) return nullptr;
        auto arrow = std::make_shared<ArrowAnnotation>(QPoint(sp[0].toInt(), sp[1].toInt()), QPoint(ep[0].toInt(), ep[1].toInt()));
        arrow->color = col;
        arrow->lineWidth = w;
        return arrow;
    } else if (objType == "shape") {
        QString st = json["shape_type"].toString("Rectangle");
        ToolType tt = (st == "Ellipse") ? ToolType::Ellipse : ToolType::Rectangle;
        QJsonArray r = json["rect"].toArray();
        if (r.size() < 4) return nullptr;
        auto shape = std::make_shared<ShapeAnnotation>(tt, QRect(r[0].toInt(), r[1].toInt(), r[2].toInt(), r[3].toInt()));
        shape->color = col;
        shape->lineWidth = w;
        shape->isFilled = json["is_filled"].toBool();
        return shape;
    } else if (objType == "freehand") {
        auto fh = std::make_shared<FreehandAnnotation>();
        fh->color = col;
        fh->lineWidth = w;
        QJsonArray pts = json["points"].toArray();
        for (int i = 0; i < pts.size(); ++i) {
            QJsonArray p = pts[i].toArray();
            fh->addPoint(QPoint(p[0].toInt(), p[1].toInt()));
        }
        return fh;
    } else if (objType == "text") {
        QJsonArray pos = json["pos"].toArray();
        if (pos.size() < 2) return nullptr;
        auto txt = std::make_shared<TextAnnotation>(QPoint(pos[0].toInt(), pos[1].toInt()), json["text"].toString());
        txt->color = col;
        txt->lineWidth = w;
        txt->fontSize = json["font_size"].toInt(16);
        txt->hasBackgroundBox = json["has_bg"].toBool(false);
        return txt;
    } else if (objType == "step") {
        QJsonArray c = json["center"].toArray();
        if (c.size() < 2) return nullptr;
        auto step = std::make_shared<StepMarkerAnnotation>(QPoint(c[0].toInt(), c[1].toInt()), json["step_number"].toInt(1));
        step->color = col;
        step->lineWidth = w;
        step->radius = json["radius"].toInt(16);
        return step;
    } else if (objType == "shader") {
        QString st = json["shader_type"].toString("Mosaic");
        ToolType tt = (st == "Blur") ? ToolType::Blur : ToolType::Mosaic;
        QJsonArray r = json["rect"].toArray();
        if (r.size() < 4) return nullptr;
        auto shader = std::make_shared<ShaderAnnotation>(tt, QRect(r[0].toInt(), r[1].toInt(), r[2].toInt(), r[3].toInt()));
        shader->color = col;
        shader->lineWidth = w;
        shader->blockSize = json["block_size"].toInt(12);
        return shader;
    } else if (objType == "highlight") {
        QJsonArray r = json["rect"].toArray();
        if (r.size() < 4) return nullptr;
        auto hl = std::make_shared<HighlightAnnotation>(QRect(r[0].toInt(), r[1].toInt(), r[2].toInt(), r[3].toInt()));
        hl->color = col;
        hl->lineWidth = w;
        return hl;
    }
    return nullptr;
}

} // namespace ScreenCut


namespace ScreenCut {

QRect ArrowAnnotation::boundingRect() const {
    return QRect(startPoint, endPoint).normalized().adjusted(-10, -10, 10, 10);
}

QRect ShapeAnnotation::boundingRect() const {
    return rect.normalized().adjusted(-10, -10, 10, 10);
}

QRect FreehandAnnotation::boundingRect() const {
    if (points.empty()) return QRect();
    int minX = points.front().x(), minY = points.front().y();
    int maxX = minX, maxY = minY;
    for (const QPoint& p : points) {
        if (p.x() < minX) minX = p.x();
        if (p.y() < minY) minY = p.y();
        if (p.x() > maxX) maxX = p.x();
        if (p.y() > maxY) maxY = p.y();
    }
    return QRect(QPoint(minX, minY), QPoint(maxX, maxY)).adjusted(-10, -10, 10, 10);
}

QRect TextAnnotation::boundingRect() const {
    return QRect(position.x(), position.y(), 200, 100); 
}

QRect StepMarkerAnnotation::boundingRect() const {
    return QRect(center.x() - 20, center.y() - 20, 40, 40);
}

QRect ShaderAnnotation::boundingRect() const {
    return rect.normalized();
}

QRect HighlightAnnotation::boundingRect() const {
    return rect.normalized();
}

} // namespace ScreenCut
