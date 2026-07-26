/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "editor_canvas.h"
#include <QPainter>
#include <QMouseEvent>
#include <QInputDialog>
#include <QTextEdit>
#include <QKeyEvent>

namespace ScreenCut {

EditorCanvas::EditorCanvas(const QPixmap& background, QWidget* parent)
    : QWidget(parent), m_background(background) {
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
    setFocusPolicy(Qt::StrongFocus);

    m_textInput = new QTextEdit(this);
    m_textInput->hide();
    m_textInput->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_textInput->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_textInput->installEventFilter(this);
    connect(m_textInput, &QTextEdit::textChanged, this, &EditorCanvas::resizeTextInput);

    if (!m_background.isNull()) {
        setFixedSize(m_background.size());
    }
}

EditorCanvas::~EditorCanvas() = default;

void EditorCanvas::setBackground(const QPixmap& background) {
    m_background = background;
    setZoom(m_zoomFactor); // re-apply size
    m_annotations.clear();
    m_redoStack.clear();
    update();
    emit historyChanged();
}

void EditorCanvas::setZoom(qreal zoom) {
    m_zoomFactor = zoom;
    if (!m_background.isNull()) {
        setFixedSize(m_background.size() * m_zoomFactor);
    }
    update();
    emit zoomChanged(m_zoomFactor);
}

void EditorCanvas::setTool(ToolType tool) {
    m_currentTool = tool;
}

void EditorCanvas::setColor(const QColor& color) {
    m_currentColor = color;
    if (m_selectedItem) {
        m_selectedItem->color = color;
        update();
        emit historyChanged();
    }
}

void EditorCanvas::setLineWidth(int width) {
    m_currentLineWidth = width;
    if (m_selectedItem) {
        m_selectedItem->lineWidth = width;
        update();
        emit historyChanged();
    }
}

void EditorCanvas::setArrowType(const QString& type) {
    m_arrowType = type;
    if (m_selectedItem && m_selectedItem->getType() == ToolType::Arrow) {
        std::static_pointer_cast<ArrowAnnotation>(m_selectedItem)->arrowType = type;
        update();
        emit historyChanged();
    }
}

void EditorCanvas::setShapeStyle(const QString& style) {
    m_shapeStyle = style;
    if (m_selectedItem && (m_selectedItem->getType() == ToolType::Rectangle || m_selectedItem->getType() == ToolType::Ellipse)) {
        std::static_pointer_cast<ShapeAnnotation>(m_selectedItem)->shapeStyle = style;
        update();
        emit historyChanged();
    }
}

void EditorCanvas::setLineStyle(const QString& style) {
    m_lineStyle = style;
    if (m_selectedItem && (m_selectedItem->getType() == ToolType::Rectangle || m_selectedItem->getType() == ToolType::Ellipse)) {
        std::static_pointer_cast<ShapeAnnotation>(m_selectedItem)->lineStyle = style;
        update();
        emit historyChanged();
    }
}

void EditorCanvas::setFontFamily(const QString& family) {
    m_fontFamily = family;
    if (m_selectedItem && m_selectedItem->getType() == ToolType::Text) {
        std::static_pointer_cast<TextAnnotation>(m_selectedItem)->fontFamily = family;
        update();
        emit historyChanged();
    }
}

void EditorCanvas::setFontSize(int size) {
    m_fontSize = size;
    if (m_selectedItem && m_selectedItem->getType() == ToolType::Text) {
        std::static_pointer_cast<TextAnnotation>(m_selectedItem)->fontSize = size;
        update();
        emit historyChanged();
    }
}

void EditorCanvas::setTextIsBold(bool bold) {
    m_textIsBold = bold;
    if (m_selectedItem && m_selectedItem->getType() == ToolType::Text) {
        std::static_pointer_cast<TextAnnotation>(m_selectedItem)->isBold = bold;
        update(); emit historyChanged();
    }
}

void EditorCanvas::setTextIsItalic(bool italic) {
    m_textIsItalic = italic;
    if (m_selectedItem && m_selectedItem->getType() == ToolType::Text) {
        std::static_pointer_cast<TextAnnotation>(m_selectedItem)->isItalic = italic;
        update(); emit historyChanged();
    }
}

void EditorCanvas::setTextIsUnderline(bool underline) {
    m_textIsUnderline = underline;
    if (m_selectedItem && m_selectedItem->getType() == ToolType::Text) {
        std::static_pointer_cast<TextAnnotation>(m_selectedItem)->isUnderline = underline;
        update(); emit historyChanged();
    }
}

void EditorCanvas::setTextIsStrikeOut(bool strike) {
    m_textIsStrikeOut = strike;
    if (m_selectedItem && m_selectedItem->getType() == ToolType::Text) {
        std::static_pointer_cast<TextAnnotation>(m_selectedItem)->isStrikeOut = strike;
        update(); emit historyChanged();
    }
}

void EditorCanvas::setTextHAlign(TextAnnotation::TextAlign align) {
    m_textHAlign = align;
    if (m_selectedItem && m_selectedItem->getType() == ToolType::Text) {
        std::static_pointer_cast<TextAnnotation>(m_selectedItem)->hAlign = align;
        update(); emit historyChanged();
    }
}

void EditorCanvas::setTextVAlign(TextAnnotation::VerticalAlign align) {
    m_textVAlign = align;
    if (m_selectedItem && m_selectedItem->getType() == ToolType::Text) {
        std::static_pointer_cast<TextAnnotation>(m_selectedItem)->vAlign = align;
        update(); emit historyChanged();
    }
}

void EditorCanvas::setTextOpacity(int opacity) {
    m_textOpacity = opacity;
    if (m_selectedItem && m_selectedItem->getType() == ToolType::Text) {
        std::static_pointer_cast<TextAnnotation>(m_selectedItem)->opacity = opacity;
        update(); emit historyChanged();
    }
}

void EditorCanvas::setTextLineSpacing(int spacing) {
    m_textLineSpacing = spacing;
    if (m_selectedItem && m_selectedItem->getType() == ToolType::Text) {
        std::static_pointer_cast<TextAnnotation>(m_selectedItem)->lineSpacing = spacing;
        update(); emit historyChanged();
    }
}

void EditorCanvas::setTextOutlineColor(const QColor& color) {
    m_textOutlineColor = color;
    if (m_selectedItem && m_selectedItem->getType() == ToolType::Text) {
        std::static_pointer_cast<TextAnnotation>(m_selectedItem)->outlineColor = color;
        update(); emit historyChanged();
    }
}

void EditorCanvas::setTextHasShadow(bool hasShadow) {
    m_textHasShadow = hasShadow;
    if (m_selectedItem && m_selectedItem->getType() == ToolType::Text) {
        std::static_pointer_cast<TextAnnotation>(m_selectedItem)->hasShadow = hasShadow;
        update(); emit historyChanged();
    }
}

void EditorCanvas::setTextShadowDirection(ShadowDirection direction) {
    m_textShadowDirection = direction;
    if (m_selectedItem && m_selectedItem->getType() == ToolType::Text) {
        std::static_pointer_cast<TextAnnotation>(m_selectedItem)->shadowDirection = direction;
        update(); emit historyChanged();
    }
}

void EditorCanvas::setTextOutlineWidth(int width) {
    m_textOutlineWidth = width;
    if (m_selectedItem && m_selectedItem->getType() == ToolType::Text) {
        std::static_pointer_cast<TextAnnotation>(m_selectedItem)->outlineWidth = width;
        update(); emit historyChanged();
    }
}

void EditorCanvas::setBlurType(ToolType type) {
    m_blurType = type;
    if (m_selectedItem && (m_selectedItem->getType() == ToolType::Mosaic || m_selectedItem->getType() == ToolType::Blur)) {
        std::static_pointer_cast<ShaderAnnotation>(m_selectedItem)->shaderType = type;
        update();
        emit historyChanged();
    }
}

void EditorCanvas::setBlurIntensity(int intensity) {
    m_blurIntensity = intensity;
    if (m_selectedItem && (m_selectedItem->getType() == ToolType::Mosaic || m_selectedItem->getType() == ToolType::Blur)) {
        std::static_pointer_cast<ShaderAnnotation>(m_selectedItem)->intensity = intensity;
        update();
        emit historyChanged();
    }
}

void EditorCanvas::setPenStyle(const QString& style) {
    m_penStyle = style;
    if (m_selectedItem && m_selectedItem->getType() == ToolType::Freehand) {
        std::static_pointer_cast<FreehandAnnotation>(m_selectedItem)->penStyle = style;
        update();
        emit historyChanged();
    }
}

void EditorCanvas::resetStepCounter() {
    m_nextStepNumber = 1;
}

void EditorCanvas::undo() {
    if (!m_annotations.empty()) {
        m_redoStack.push_back(m_annotations.back());
        m_annotations.pop_back();
        update();
        emit historyChanged();
    }
}

void EditorCanvas::redo() {
    if (!m_redoStack.empty()) {
        m_annotations.push_back(m_redoStack.back());
        m_redoStack.pop_back();
        update();
        emit historyChanged();
    }
}

QPixmap EditorCanvas::renderFinalPixmap() {
    QPixmap result = m_background;
    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    for (const auto& item : m_annotations) {
        item->draw(painter, &m_background);
    }
    painter.end();
    return result;
}

void EditorCanvas::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    painter.scale(m_zoomFactor, m_zoomFactor);

    // Draw background snapshot
    painter.drawPixmap(0, 0, m_background);

    // Draw all completed annotations
    for (const auto& item : m_annotations) {
        item->draw(painter, &m_background);
    }

    // Draw temporary drawing item
    if (m_isDrawing && m_tempItem) {
        m_tempItem->draw(painter, &m_background);
    }
}

QPoint EditorCanvas::mapToImage(const QPoint& viewPos) const {
    if (m_zoomFactor <= 0) return viewPos;
    return QPoint(viewPos.x() / m_zoomFactor, viewPos.y() / m_zoomFactor);
}

void EditorCanvas::resizeTextInput() {
    if (!m_textInput->isVisible()) return;
    int docHeight = static_cast<int>(m_textInput->document()->size().height());
    QMargins margins = m_textInput->contentsMargins();
    int neededHeight = qMax(m_textInput->minimumHeight(), docHeight + margins.top() + margins.bottom() + 14);
    
    QFontMetrics fm(m_textInput->font());
    int neededWidth = qMax(100, fm.boundingRect(m_textInput->toPlainText()).width() + 40);
    
    if (neededHeight != m_textInput->height() || neededWidth != m_textInput->width()) {
        m_textInput->resize(neededWidth, neededHeight);
    }
}

void EditorCanvas::commitText() {
    if (!m_textInput->isVisible()) return;
    
    QString text = m_textInput->toPlainText().trimmed();
    m_textInput->hide();
    
    if (text.isEmpty()) {
        if (m_editingTextObj) {
            auto it = std::find(m_annotations.begin(), m_annotations.end(), m_editingTextObj);
            if (it != m_annotations.end()) {
                m_annotations.erase(it);
                m_selectedItem = nullptr;
                emit historyChanged();
            }
        }
        m_editingTextObj = nullptr;
        update();
        return;
    }
    
    if (m_editingTextObj) {
        auto txtObj = std::static_pointer_cast<TextAnnotation>(m_editingTextObj);
        txtObj->text = text;
        txtObj->color = m_currentColor;
        txtObj->fontFamily = m_fontFamily;
        txtObj->fontSize = m_fontSize;
        txtObj->isBold = m_textIsBold;
        txtObj->isItalic = m_textIsItalic;
        txtObj->isUnderline = m_textIsUnderline;
        txtObj->isStrikeOut = m_textIsStrikeOut;
        txtObj->hAlign = m_textHAlign;
        txtObj->vAlign = m_textVAlign;
        txtObj->opacity = m_textOpacity;
        txtObj->lineSpacing = m_textLineSpacing;
        txtObj->outlineColor = m_textOutlineColor;
        txtObj->hasShadow = m_textHasShadow;
        txtObj->shadowDirection = m_textShadowDirection;
        txtObj->outlineWidth = m_textOutlineWidth;
    } else {
        auto txtItem = std::make_shared<TextAnnotation>(m_startPoint, text);
        txtItem->color = m_currentColor;
        txtItem->fontFamily = m_fontFamily;
        txtItem->fontSize = m_fontSize;
        txtItem->isBold = m_textIsBold;
        txtItem->isItalic = m_textIsItalic;
        txtItem->isUnderline = m_textIsUnderline;
        txtItem->isStrikeOut = m_textIsStrikeOut;
        txtItem->hAlign = m_textHAlign;
        txtItem->vAlign = m_textVAlign;
        txtItem->opacity = m_textOpacity;
        txtItem->lineSpacing = m_textLineSpacing;
        txtItem->outlineColor = m_textOutlineColor;
        txtItem->hasShadow = m_textHasShadow;
        txtItem->shadowDirection = m_textShadowDirection;
        txtItem->outlineWidth = m_textOutlineWidth;
        m_annotations.push_back(txtItem);
        m_selectedItem = txtItem;
        m_redoStack.clear();
    }
    
    m_editingTextObj = nullptr;
    update();
    emit historyChanged();
}

bool EditorCanvas::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_textInput && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (((keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) && (keyEvent->modifiers() & Qt::ControlModifier)) || keyEvent->key() == Qt::Key_Escape) {
            commitText();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void EditorCanvas::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) return;

    if (m_textInput->isVisible()) {
        if (!m_textInput->geometry().contains(event->pos())) {
            commitText();
        }
        return;
    }

    m_startPoint = mapToImage(event->pos());
    m_currentPoint = m_startPoint;

    if (m_currentTool == ToolType::None) {
        if (m_selectedItem) {
            int handle = m_selectedItem->hitTestHandle(m_startPoint);
            if (handle != -1) {
                m_activeHandle = handle;
                return;
            }
        }

        m_selectedItem = nullptr;
        m_activeHandle = -1;
        for (auto& item : m_annotations) {
            item->isSelected = false;
        }
        for (auto it = m_annotations.rbegin(); it != m_annotations.rend(); ++it) {
            if ((*it)->contains(m_startPoint)) {
                m_selectedItem = *it;
                m_selectedItem->isSelected = true;
                m_isDragging = true;
                m_lastDragPoint = m_startPoint;
                emit itemSelected(m_selectedItem);
                break;
            }
        }
        update();
        return;
    }

    m_isDrawing = true;

    if (m_currentTool == ToolType::Arrow) {
        auto arrow = std::make_shared<ArrowAnnotation>(m_startPoint, m_currentPoint);
        arrow->color = m_currentColor;
        arrow->lineWidth = m_currentLineWidth;
        arrow->arrowType = m_arrowType;
        m_tempItem = arrow;
    } else if (m_currentTool == ToolType::Rectangle || m_currentTool == ToolType::Ellipse) {
        auto shape = std::make_shared<ShapeAnnotation>(m_currentTool, QRect(m_startPoint, m_currentPoint));
        shape->color = m_currentColor;
        shape->lineWidth = m_currentLineWidth;
        shape->shapeStyle = m_shapeStyle;
        shape->lineStyle = m_lineStyle;
        m_tempItem = shape;
    } else if (m_currentTool == ToolType::Freehand) {
        auto freehand = std::make_shared<FreehandAnnotation>();
        freehand->color = m_currentColor;
        freehand->lineWidth = m_currentLineWidth;
        freehand->penStyle = m_penStyle;
        freehand->addPoint(m_startPoint);
        m_tempItem = freehand;
    } else if (m_currentTool == ToolType::StepMarker) {
        auto step = std::make_shared<StepMarkerAnnotation>(m_startPoint, m_nextStepNumber);
        step->color = m_currentColor;
        step->radius = m_currentLineWidth * 4; // Use line width as size base
        m_annotations.push_back(step);
        m_nextStepNumber++;
        m_redoStack.clear();
        m_isDrawing = false;
        update();
        emit historyChanged();
    } else if (m_currentTool == ToolType::Text) {
        m_editingTextObj = nullptr;
        QFont font(m_fontFamily, qMax(8, static_cast<int>(m_fontSize * m_zoomFactor)), QFont::Bold);
        m_textInput->setFont(font);
        m_textInput->setStyleSheet(QString("QTextEdit { color: %1; background: rgba(0,0,0,180); border: 1px dashed %1; padding: 2px; }").arg(m_currentColor.name()));
        m_textInput->setText("");
        
        QPoint screenPos = event->pos();
        m_textInput->setGeometry(screenPos.x(), screenPos.y(), 100, 40);
        m_textInput->show();
        m_textInput->setFocus();
        resizeTextInput();
        
        m_isDrawing = false;
    } else if (m_currentTool == ToolType::Mosaic || m_currentTool == ToolType::Blur) {
        auto shader = std::make_shared<ShaderAnnotation>(m_blurType, QRect(m_startPoint, m_currentPoint));
        shader->intensity = m_blurIntensity;
        m_tempItem = shader;
    } else if (m_currentTool == ToolType::Highlight) {
        auto highlight = std::make_shared<HighlightAnnotation>(QRect(m_startPoint, m_currentPoint));
        highlight->color = QColor(255, 235, 59); // Yellow
        m_tempItem = highlight;
    }
    update();
}

void EditorCanvas::mouseMoveEvent(QMouseEvent* event) {
    m_currentPoint = mapToImage(event->pos());
    emit mousePositionChanged(m_currentPoint);

    if (m_currentTool == ToolType::None && m_selectedItem) {
        if (m_activeHandle != -1) {
            m_selectedItem->moveHandle(m_activeHandle, m_currentPoint);
            update();
            return;
        } else if (m_isDragging) {
            QPoint delta = m_currentPoint - m_lastDragPoint;
            m_selectedItem->moveBy(delta);
            m_lastDragPoint = m_currentPoint;
            update();
            return;
        }
    }

    if (m_isDrawing && m_tempItem) {
        if (m_currentTool == ToolType::Arrow) {
            auto arrow = std::dynamic_pointer_cast<ArrowAnnotation>(m_tempItem);
            if (arrow) arrow->endPoint = m_currentPoint;
        } else if (m_currentTool == ToolType::Rectangle || m_currentTool == ToolType::Ellipse) {
            auto shape = std::dynamic_pointer_cast<ShapeAnnotation>(m_tempItem);
            if (shape) shape->rect = QRect(m_startPoint, m_currentPoint).normalized();
        } else if (m_currentTool == ToolType::Freehand) {
            auto freehand = std::dynamic_pointer_cast<FreehandAnnotation>(m_tempItem);
            if (freehand) freehand->addPoint(m_currentPoint);
        } else if (m_currentTool == ToolType::Mosaic || m_currentTool == ToolType::Blur) {
            auto shader = std::dynamic_pointer_cast<ShaderAnnotation>(m_tempItem);
            if (shader) shader->rect = QRect(m_startPoint, m_currentPoint).normalized();
        } else if (m_currentTool == ToolType::Highlight) {
            auto highlight = std::dynamic_pointer_cast<HighlightAnnotation>(m_tempItem);
            if (highlight) highlight->rect = QRect(m_startPoint, m_currentPoint).normalized();
        }
        update();
    }
}

void EditorCanvas::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (m_currentTool == ToolType::None) {
            if (m_isDragging || m_activeHandle != -1) {
                m_isDragging = false;
                m_activeHandle = -1;
                emit historyChanged();
            }
        } else if (m_isDrawing) {
            m_isDrawing = false;
            if (m_tempItem) {
                m_annotations.push_back(m_tempItem);
                m_tempItem.reset();
                m_redoStack.clear();
                update();
                emit historyChanged();
            }
        }
    }
}

void EditorCanvas::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_currentTool == ToolType::None) {
        QPoint pt = mapToImage(event->pos());
        for (auto it = m_annotations.rbegin(); it != m_annotations.rend(); ++it) {
            if ((*it)->getType() == ToolType::Text && (*it)->contains(pt)) {
                auto txtItem = std::dynamic_pointer_cast<TextAnnotation>(*it);
                if (txtItem) {
                    m_editingTextObj = *it;
                    auto txtObj = std::static_pointer_cast<TextAnnotation>(m_editingTextObj);
                    
                    QFont font(txtObj->fontFamily, qMax(8, static_cast<int>(txtObj->fontSize * m_zoomFactor)), QFont::Bold);
                    m_textInput->setFont(font);
                    m_textInput->setStyleSheet(QString("QTextEdit { color: %1; background: rgba(0,0,0,180); border: 1px dashed %1; padding: 2px; }").arg(txtObj->color.name()));
                    m_textInput->setText(txtObj->text);
                    
                    QPoint screenPos = QPoint(txtObj->position.x() * m_zoomFactor, txtObj->position.y() * m_zoomFactor);
                    m_textInput->setGeometry(screenPos.x(), screenPos.y(), 100, 40);
                    m_textInput->show();
                    m_textInput->setFocus();
                    resizeTextInput();
                }
                break;
            }
        }
    }
}

QJsonArray EditorCanvas::saveAnnotationsJson() const {
    QJsonArray arr;
    for (const auto& item : m_annotations) {
        if (item) {
            arr.append(item->toJson());
        }
    }
    return arr;
}

void EditorCanvas::loadAnnotationsJson(const QJsonArray& arr) {
    m_annotations.clear();
    m_redoStack.clear();
    for (int i = 0; i < arr.size(); ++i) {
        QJsonObject obj = arr[i].toObject();
        auto item = AnnotationItem::fromJson(obj);
        if (item) {
            m_annotations.push_back(item);
        }
    }
    update();
    emit historyChanged();
}

} // namespace ScreenCut
