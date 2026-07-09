/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef EDITOR_MODELS_H
#define EDITOR_MODELS_H

#include <QObject>
#include <QPainter>
#include <QPoint>
#include <QRect>
#include <QColor>
#include <QString>
#include <QPainterPath>
#include <QJsonObject>
#include <memory>
#include <vector>

namespace ScreenCut {

enum class ToolType {
    None,
    Arrow,
    Rectangle,
    Ellipse,
    Freehand,
    Text,
    StepMarker,
    Mosaic,
    Blur,
    Highlight
};

class AnnotationItem {
public:
    virtual ~AnnotationItem() = default;

    virtual ToolType getType() const = 0;
    virtual void draw(QPainter& painter, const QPixmap* background = nullptr) = 0;
    virtual bool contains(const QPoint& pos) const = 0;
    virtual void moveBy(const QPoint& delta) = 0;
    virtual std::shared_ptr<AnnotationItem> clone() const = 0;
    virtual QJsonObject toJson() const = 0;
    static std::shared_ptr<AnnotationItem> fromJson(const QJsonObject& json);

    QColor color = QColor(255, 59, 48); // Modern vibrant red
    int lineWidth = 3;
    bool isSelected = false;
};

class ArrowAnnotation : public AnnotationItem {
public:
    ArrowAnnotation(const QPoint& start, const QPoint& end);

    ToolType getType() const override { return ToolType::Arrow; }
    void draw(QPainter& painter, const QPixmap* background = nullptr) override;
    bool contains(const QPoint& pos) const override;
    void moveBy(const QPoint& delta) override;
    std::shared_ptr<AnnotationItem> clone() const override;
    QJsonObject toJson() const override;

    QPoint startPoint;
    QPoint endPoint;
};

class ShapeAnnotation : public AnnotationItem {
public:
    ShapeAnnotation(ToolType type, const QRect& rect);

    ToolType getType() const override { return shapeType; }
    void draw(QPainter& painter, const QPixmap* background = nullptr) override;
    bool contains(const QPoint& pos) const override;
    void moveBy(const QPoint& delta) override;
    std::shared_ptr<AnnotationItem> clone() const override;
    QJsonObject toJson() const override;

    ToolType shapeType; // Rectangle or Ellipse
    QRect rect;
    bool isFilled = false;
};

class FreehandAnnotation : public AnnotationItem {
public:
    FreehandAnnotation();

    void addPoint(const QPoint& point);

    ToolType getType() const override { return ToolType::Freehand; }
    void draw(QPainter& painter, const QPixmap* background = nullptr) override;
    bool contains(const QPoint& pos) const override;
    void moveBy(const QPoint& delta) override;
    std::shared_ptr<AnnotationItem> clone() const override;
    QJsonObject toJson() const override;

    std::vector<QPoint> points;
    QPainterPath path;
};

class TextAnnotation : public AnnotationItem {
public:
    TextAnnotation(const QPoint& pos, const QString& text);

    ToolType getType() const override { return ToolType::Text; }
    void draw(QPainter& painter, const QPixmap* background = nullptr) override;
    bool contains(const QPoint& pos) const override;
    void moveBy(const QPoint& delta) override;
    std::shared_ptr<AnnotationItem> clone() const override;
    QJsonObject toJson() const override;

    QPoint position;
    QString text;
    int fontSize = 16;
    bool hasBackgroundBox = true;
};

class StepMarkerAnnotation : public AnnotationItem {
public:
    StepMarkerAnnotation(const QPoint& center, int stepNumber);

    ToolType getType() const override { return ToolType::StepMarker; }
    void draw(QPainter& painter, const QPixmap* background = nullptr) override;
    bool contains(const QPoint& pos) const override;
    void moveBy(const QPoint& delta) override;
    std::shared_ptr<AnnotationItem> clone() const override;
    QJsonObject toJson() const override;

    QPoint center;
    int stepNumber;
    int radius = 16;
};

class ShaderAnnotation : public AnnotationItem {
public:
    ShaderAnnotation(ToolType type, const QRect& rect);

    ToolType getType() const override { return shaderType; }
    void draw(QPainter& painter, const QPixmap* background = nullptr) override;
    bool contains(const QPoint& pos) const override;
    void moveBy(const QPoint& delta) override;
    std::shared_ptr<AnnotationItem> clone() const override;
    QJsonObject toJson() const override;

    ToolType shaderType; // Mosaic or Blur
    QRect rect;
    int blockSize = 12; // For Mosaic
};

class HighlightAnnotation : public AnnotationItem {
public:
    HighlightAnnotation(const QRect& rect);

    ToolType getType() const override { return ToolType::Highlight; }
    void draw(QPainter& painter, const QPixmap* background = nullptr) override;
    bool contains(const QPoint& pos) const override;
    void moveBy(const QPoint& delta) override;
    std::shared_ptr<AnnotationItem> clone() const override;
    QJsonObject toJson() const override;

    QRect rect;
};

} // namespace ScreenCut

#endif // EDITOR_MODELS_H
