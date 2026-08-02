/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "shape.h"
#include "../editor_canvas.h"

namespace ScreenCut {

void ShapeTool::mousePressEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& context) {
    auto poly = canvas->getCurrentPolygon();
    if (poly) {
        if (event->button() == Qt::RightButton) {
            poly->points.pop_back(); // remove preview
            if (poly->points.size() >= 3) {
                canvas->saveToHistory();
                canvas->addAnnotation(poly);
            }
            canvas->setCurrentPolygon(nullptr);
            canvas->setDrawing(false);
            canvas->updateCanvas();
            return;
        } else if (event->button() == Qt::LeftButton) {
            QPoint clickPoint = canvas->mapToImage(event->pos());
            poly->points.back() = clickPoint; // Commit
            poly->addPoint(clickPoint);       // New preview
            canvas->updateCanvas();
            return;
        }
    }

    if (event->button() != Qt::LeftButton) return;
    
    m_startPoint = canvas->mapToImage(event->pos());
    
    if (context.shapeStyle == "Polygon") {
        if (!poly) {
            poly = std::make_shared<PolygonAnnotation>();
            poly->fillColor = context.shapeFillColor;
            poly->outlineColor = context.shapeOutlineColor;
            poly->lineWidth = context.shapeThickness;
            poly->lineStyle = context.lineStyle;
            poly->opacity = context.shapeOpacity;
            poly->shadow = context.shapeShadow;
            poly->addPoint(m_startPoint);
            poly->addPoint(m_startPoint); // preview point
            canvas->setCurrentPolygon(poly);
            canvas->setDrawing(true);
        }
    } else {
        m_tempItem = std::make_shared<ShapeAnnotation>(m_type, QRect(m_startPoint, m_startPoint));
        m_tempItem->fillColor = context.shapeFillColor;
        m_tempItem->outlineColor = context.shapeOutlineColor;
        m_tempItem->lineWidth = context.shapeThickness;
        m_tempItem->shapeStyle = context.shapeStyle;
        m_tempItem->lineStyle = context.lineStyle;
        m_tempItem->opacity = context.shapeOpacity;
        m_tempItem->shadow = context.shapeShadow;
        
        canvas->setTempItem(m_tempItem);
        canvas->setDrawing(true);
    }
}

void ShapeTool::mouseMoveEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& /*context*/) {
    QPoint currentPoint = canvas->mapToImage(event->pos());
    
    auto poly = canvas->getCurrentPolygon();
    if (poly) {
        poly->points.back() = currentPoint;
        canvas->updateCanvas();
    }
    
    if (m_tempItem) {
        m_tempItem->rect = QRect(m_startPoint, currentPoint).normalized();
        canvas->updateCanvas();
    }
}

void ShapeTool::mouseReleaseEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& /*context*/) {
    if (event->button() != Qt::LeftButton) return;
    
    if (m_tempItem) {
        if (m_tempItem->rect.width() > 0 || m_tempItem->rect.height() > 0) {
            canvas->saveToHistory();
            canvas->addAnnotation(m_tempItem);
        }
        m_tempItem = nullptr;
        canvas->setTempItem(nullptr);
        canvas->setDrawing(false);
    }
}

void ShapeTool::mouseDoubleClickEvent(QMouseEvent* event, EditorCanvas* canvas, const ToolContext& /*context*/) {
    auto poly = canvas->getCurrentPolygon();
    if (poly) {
        if (event->button() == Qt::LeftButton) {
            // Finish polygon
            poly->points.pop_back(); // remove preview point
            if (poly->points.size() >= 3) {
                canvas->saveToHistory();
                canvas->addAnnotation(poly);
            }
            canvas->setCurrentPolygon(nullptr);
            canvas->setDrawing(false);
            canvas->updateCanvas();
        }
    }
}

} // namespace ScreenCut
