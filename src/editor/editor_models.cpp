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

    if (isSelected) {
        painter.setPen(QPen(Qt::white, 1, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(QRect(startPoint, endPoint).normalized().adjusted(-4, -4, 4, 4));
    }
    painter.restore();
}

bool ArrowAnnotation::contains(const QPoint& pos) const {
    // Distance from point to line segment
    QRect bounds = QRect(startPoint, endPoint).normalized().adjusted(-8, -8, 8, 8);
    return bounds.contains(pos);
}

void ArrowAnnotation::moveBy(const QPoint& delta) {
    startPoint += delta;
    endPoint += delta;
}

std::shared_ptr<AnnotationItem> ArrowAnnotation::clone() const {
    auto copy = std::make_shared<ArrowAnnotation>(startPoint, endPoint);
    copy->color = color;
    copy->lineWidth = lineWidth;
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

    QPen pen(color, lineWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);

    if (isFilled) {
        QColor fillColor = color;
        fillColor.setAlpha(60);
        painter.setBrush(fillColor);
    } else {
        painter.setBrush(Qt::NoBrush);
    }

    if (shapeType == ToolType::Rectangle) {
        painter.drawRoundedRect(rect, 4, 4);
    } else if (shapeType == ToolType::Ellipse) {
        painter.drawEllipse(rect);
    }

    if (isSelected) {
        painter.setPen(QPen(Qt::white, 1, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(rect.adjusted(-4, -4, 4, 4));
    }
    painter.restore();
}

bool ShapeAnnotation::contains(const QPoint& pos) const {
    return rect.adjusted(-6, -6, 6, 6).contains(pos);
}

void ShapeAnnotation::moveBy(const QPoint& delta) {
    rect.translate(delta);
}

std::shared_ptr<AnnotationItem> ShapeAnnotation::clone() const {
    auto copy = std::make_shared<ShapeAnnotation>(shapeType, rect);
    copy->color = color;
    copy->lineWidth = lineWidth;
    copy->isFilled = isFilled;
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

    QPen pen(color, lineWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
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

    QFont font = painter.font();
    font.setPointSize(fontSize);
    font.setBold(true);
    painter.setFont(font);

    QFontMetrics fm(font);
    QRect textRect = fm.boundingRect(QRect(0, 0, 800, 800), Qt::AlignLeft | Qt::TextWordWrap, text);
    textRect.moveTo(position);
    textRect.adjust(-8, -6, 8, 6);

    if (hasBackgroundBox) {
        painter.setPen(Qt::NoPen);
        QColor bgColor = color;
        bgColor.setAlpha(220);
        painter.setBrush(bgColor);
        painter.drawRoundedRect(textRect, 6, 6);

        painter.setPen(Qt::white);
    } else {
        painter.setPen(color);
    }

    painter.drawText(textRect, Qt::AlignCenter, text);

    if (isSelected) {
        painter.setPen(QPen(Qt::white, 1, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(textRect.adjusted(-2, -2, 2, 2));
    }
    painter.restore();
}

bool TextAnnotation::contains(const QPoint& pos) const {
    return QRect(position, QSize(100, 30)).adjusted(-10, -10, 10, 10).contains(pos);
}

void TextAnnotation::moveBy(const QPoint& delta) {
    position += delta;
}

std::shared_ptr<AnnotationItem> TextAnnotation::clone() const {
    auto copy = std::make_shared<TextAnnotation>(position, text);
    copy->color = color;
    copy->fontSize = fontSize;
    copy->hasBackgroundBox = hasBackgroundBox;
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
        int step = qMax(4, blockSize);
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
        QImage blurred = sourceImg.scaled(qMax(1, sourceImg.width() / 10), qMax(1, sourceImg.height() / 10),
                                          Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        sourceImg = blurred.scaled(sourceImg.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    painter.drawImage(targetRect.topLeft(), sourceImg);

    if (isSelected) {
        painter.setPen(QPen(Qt::white, 1, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(targetRect.adjusted(-2, -2, 2, 2));
    }
    painter.restore();
}

bool ShaderAnnotation::contains(const QPoint& pos) const {
    return rect.contains(pos);
}

void ShaderAnnotation::moveBy(const QPoint& delta) {
    rect.translate(delta);
}

std::shared_ptr<AnnotationItem> ShaderAnnotation::clone() const {
    auto copy = std::make_shared<ShaderAnnotation>(shaderType, rect);
    copy->blockSize = blockSize;
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
        painter.drawRect(rect.adjusted(-2, -2, 2, 2));
    }
    painter.restore();
}

bool HighlightAnnotation::contains(const QPoint& pos) const {
    return rect.contains(pos);
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
        auto arrow = std::make_shared<ArrowAnnotation>(QPoint(sp[0].toInt(), sp[1].toInt()), QPoint(ep[0].toInt(), ep[1].toInt()));
        arrow->color = col;
        arrow->lineWidth = w;
        return arrow;
    } else if (objType == "shape") {
        QString st = json["shape_type"].toString("Rectangle");
        ToolType tt = (st == "Ellipse") ? ToolType::Ellipse : ToolType::Rectangle;
        QJsonArray r = json["rect"].toArray();
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
        auto txt = std::make_shared<TextAnnotation>(QPoint(pos[0].toInt(), pos[1].toInt()), json["text"].toString());
        txt->color = col;
        txt->lineWidth = w;
        txt->fontSize = json["font_size"].toInt(16);
        txt->hasBackgroundBox = json["has_bg"].toBool(true);
        return txt;
    } else if (objType == "step") {
        QJsonArray c = json["center"].toArray();
        auto step = std::make_shared<StepMarkerAnnotation>(QPoint(c[0].toInt(), c[1].toInt()), json["step_number"].toInt(1));
        step->color = col;
        step->lineWidth = w;
        step->radius = json["radius"].toInt(16);
        return step;
    } else if (objType == "shader") {
        QString st = json["shader_type"].toString("Mosaic");
        ToolType tt = (st == "Blur") ? ToolType::Blur : ToolType::Mosaic;
        QJsonArray r = json["rect"].toArray();
        auto shader = std::make_shared<ShaderAnnotation>(tt, QRect(r[0].toInt(), r[1].toInt(), r[2].toInt(), r[3].toInt()));
        shader->color = col;
        shader->lineWidth = w;
        shader->blockSize = json["block_size"].toInt(12);
        return shader;
    } else if (objType == "highlight") {
        QJsonArray r = json["rect"].toArray();
        auto hl = std::make_shared<HighlightAnnotation>(QRect(r[0].toInt(), r[1].toInt(), r[2].toInt(), r[3].toInt()));
        hl->color = col;
        hl->lineWidth = w;
        return hl;
    }
    return nullptr;
}

} // namespace ScreenCut
