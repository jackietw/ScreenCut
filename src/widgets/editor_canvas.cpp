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
    m_baseCanvasRect = m_background.rect();
    setZoom(m_zoomFactor); // re-apply size
    m_annotations.clear();
    m_undoStack.clear();
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

void EditorCanvas::saveToHistory() {
    HistoryState state;
    state.background = m_background;
    state.baseCanvasRect = m_baseCanvasRect;
    for (const auto& item : m_annotations) {
        state.annotations.push_back(item->clone());
    }
    m_undoStack.push_back(state);
    m_redoStack.clear();
}

void EditorCanvas::undo() {
    if (!m_undoStack.empty()) {
        HistoryState currentState;
        currentState.background = m_background;
        currentState.baseCanvasRect = m_baseCanvasRect;
        for (const auto& item : m_annotations) {
            currentState.annotations.push_back(item->clone());
        }
        m_redoStack.push_back(currentState);

        HistoryState state = m_undoStack.back();
        m_undoStack.pop_back();
        m_background = state.background;
        m_baseCanvasRect = state.baseCanvasRect;
        m_annotations = state.annotations;
        setZoom(m_zoomFactor);
        
        m_selectedItem = nullptr;
        m_activeHandle = -1;
        update();
        emit historyChanged();
    }
}

void EditorCanvas::redo() {
    if (!m_redoStack.empty()) {
        HistoryState currentState;
        currentState.background = m_background;
        currentState.baseCanvasRect = m_baseCanvasRect;
        for (const auto& item : m_annotations) {
            currentState.annotations.push_back(item->clone());
        }
        m_undoStack.push_back(currentState);

        HistoryState state = m_redoStack.back();
        m_redoStack.pop_back();
        m_background = state.background;
        m_baseCanvasRect = state.baseCanvasRect;
        m_annotations = state.annotations;
        setZoom(m_zoomFactor);

        m_selectedItem = nullptr;
        m_activeHandle = -1;
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

    // Draw checkerboard for transparent areas
    static QPixmap checkerboard;
    if (checkerboard.isNull()) {
        checkerboard = QPixmap(20, 20);
        checkerboard.fill(QColor(230, 230, 230)); // light grey
        QPainter cbPainter(&checkerboard);
        cbPainter.fillRect(0, 0, 10, 10, Qt::white);
        cbPainter.fillRect(10, 10, 10, 10, Qt::white);
        cbPainter.end();
    }
    painter.fillRect(m_background.rect(), QBrush(checkerboard));

    // Draw background snapshot
    painter.drawPixmap(0, 0, m_background);

    // Draw all completed annotations
    for (const auto& item : m_annotations) {
        item->draw(painter, &m_background);
    }

    // Draw temporary drawing item
    if (m_isDrawing) {
        if (m_tempItem) {
            m_tempItem->draw(painter, &m_background);
        } else if (m_currentTool == ToolType::Text) {
            QRect rect(m_startPoint, m_currentPoint);
            rect = rect.normalized();
            QPen pen(m_currentColor, 1, Qt::DashLine);
            painter.setPen(pen);
            painter.setBrush(QBrush(QColor(m_currentColor.red(), m_currentColor.green(), m_currentColor.blue(), 30)));
            painter.drawRect(rect);
        }
    }

    // Draw canvas handles in all modes (except while actively drawing)
    if (!m_isDrawing && !m_background.isNull()) {
        painter.setPen(QPen(QColor(0, 168, 255), 1));
        painter.setBrush(Qt::white);
        QRect r = m_background.rect();
        
        painter.drawRect(0, 0, 8, 8);
        painter.drawRect(r.right() - 8, 0, 8, 8);
        painter.drawRect(0, r.bottom() - 8, 8, 8);
        painter.drawRect(r.right() - 8, r.bottom() - 8, 8, 8);
        
        int midX = r.width() / 2;
        int midY = r.height() / 2;
        painter.drawRect(midX - 4, 0, 8, 8);
        painter.drawRect(midX - 4, r.bottom() - 8, 8, 8);
        painter.drawRect(0, midY - 4, 8, 8);
        painter.drawRect(r.right() - 8, midY - 4, 8, 8);
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
                updateAutoCanvasSize();
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
        saveToHistory();
        m_annotations.push_back(txtItem);
        m_selectedItem = txtItem;
    }
    
    m_editingTextObj = nullptr;
    updateAutoCanvasSize();
    update();
    emit historyChanged();
}

void EditorCanvas::updateAutoCanvasSize() {
    if (m_background.isNull()) return;

    QRect neededRect = m_baseCanvasRect;
    for (const auto& item : m_annotations) {
        QRect itemRect = item->boundingRect();
        itemRect.adjust(-4, -4, 4, 4); 
        neededRect = neededRect.united(itemRect);
    }

    if (neededRect == QRect(0, 0, m_background.width(), m_background.height())) {
        return;
    }

    QPixmap newBg(neededRect.size());
    newBg.fill(Qt::transparent);
    QPainter p(&newBg);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.drawPixmap(-neededRect.topLeft(), m_background);
    p.end();

    m_background = newBg;
    m_baseCanvasRect.translate(-neededRect.topLeft());

    for (auto& item : m_annotations) {
        item->moveBy(-neededRect.topLeft());
    }

    setZoom(m_zoomFactor);
    update();
}

int EditorCanvas::hitTestCanvasHandle(const QPoint& pos) const {
    if (m_background.isNull()) return -1;
    QRect r = m_background.rect();
    int midX = r.width() / 2;
    int midY = r.height() / 2;
    if (QRect(0, 0, 12, 12).contains(pos)) return 1;
    if (QRect(r.right() - 12, 0, 12, 12).contains(pos)) return 2;
    if (QRect(r.right() - 12, r.bottom() - 12, 12, 12).contains(pos)) return 3;
    if (QRect(0, r.bottom() - 12, 12, 12).contains(pos)) return 4;
    if (QRect(midX - 6, 0, 12, 12).contains(pos)) return 5;
    if (QRect(midX - 6, r.bottom() - 12, 12, 12).contains(pos)) return 6;
    if (QRect(0, midY - 6, 12, 12).contains(pos)) return 7;
    if (QRect(r.right() - 12, midY - 6, 12, 12).contains(pos)) return 8;
    return -1;
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

    // 1. Check annotation handles first if in None tool
    if (m_currentTool == ToolType::None && m_selectedItem) {
        int handle = m_selectedItem->hitTestHandle(m_startPoint);
        if (handle != -1) {
            m_activeHandle = handle;
            return;
        }
    }

    // 2. Check canvas resize handles regardless of tool
    if (!m_background.isNull()) {
        int hit = hitTestCanvasHandle(m_startPoint);

        if (hit != -1) {
            saveToHistory();
            m_isCanvasResizing = true;
            m_canvasActiveHandle = hit;
            m_dragBackgroundOriginal = m_background;
            m_dragAnnotationsOriginal.clear();
            for (const auto& item : m_annotations) {
                m_dragAnnotationsOriginal.push_back(item->clone());
            }
            m_dragOriginalRect = m_background.rect();
            m_startGlobalPos = event->globalPosition().toPoint();
            // Deselect item if we were in None tool
            if (m_currentTool == ToolType::None && m_selectedItem) {
                m_selectedItem->isSelected = false;
                m_selectedItem = nullptr;
                emit itemSelected(nullptr);
            }
            update();
            return;
        }
    }

    // 3. Normal selection logic for None tool
    if (m_currentTool == ToolType::None) {
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
        saveToHistory();
        auto step = std::make_shared<StepMarkerAnnotation>(m_startPoint, m_nextStepNumber);
        step->color = m_currentColor;
        step->radius = m_currentLineWidth * 4; // Use line width as size base
        m_annotations.push_back(step);
        m_nextStepNumber++;
        m_isDrawing = false;
        update();
        emit historyChanged();
    } else if (m_currentTool == ToolType::Text) {
        // Just let it drag, painted directly in paintEvent
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

    if (event->buttons() == Qt::NoButton) {
        int handle = hitTestCanvasHandle(m_currentPoint);
        if (handle == -1 && m_currentTool == ToolType::None && m_selectedItem) {
            handle = m_selectedItem->hitTestHandle(m_currentPoint);
        }
        
        if (handle != -1) {
            if (handle == 1 || handle == 3) setCursor(Qt::SizeFDiagCursor); // Top-Left, Bottom-Right
            else if (handle == 2 || handle == 4) setCursor(Qt::SizeBDiagCursor); // Top-Right, Bottom-Left
            else if (handle == 5 || handle == 6) setCursor(Qt::SizeVerCursor); // Top, Bottom
            else if (handle == 7 || handle == 8) setCursor(Qt::SizeHorCursor); // Left, Right
        } else {
            if (m_currentTool == ToolType::None) setCursor(Qt::ArrowCursor);
            else setCursor(Qt::CrossCursor);
        }
        return;
    }

    if (m_isCanvasResizing) {
        QPoint globalDelta = event->globalPosition().toPoint() - m_startGlobalPos;
        QPoint delta(globalDelta.x() / m_zoomFactor, globalDelta.y() / m_zoomFactor);
        
        QRect r = m_dragOriginalRect;
        
        if (m_canvasActiveHandle == 1) r.setTopLeft(r.topLeft() + delta);
        else if (m_canvasActiveHandle == 2) r.setTopRight(r.topRight() + delta);
        else if (m_canvasActiveHandle == 3) r.setBottomRight(r.bottomRight() + delta);
        else if (m_canvasActiveHandle == 4) r.setBottomLeft(r.bottomLeft() + delta);
        else if (m_canvasActiveHandle == 5) r.setTop(r.top() + delta.y());
        else if (m_canvasActiveHandle == 6) r.setBottom(r.bottom() + delta.y());
        else if (m_canvasActiveHandle == 7) r.setLeft(r.left() + delta.x());
        else if (m_canvasActiveHandle == 8) r.setRight(r.right() + delta.x());
        
        r = r.normalized();
        if (r.width() < 10) r.setWidth(10);
        if (r.height() < 10) r.setHeight(10);
        
        QPixmap newBg(r.size());
        newBg.fill(Qt::transparent);
        QPainter p(&newBg);
        p.setCompositionMode(QPainter::CompositionMode_Source); // Allow drawing transparent pixels if original had them
        p.drawPixmap(-r.topLeft(), m_dragBackgroundOriginal);
        p.end();
        
        m_background = newBg;
        m_baseCanvasRect = QRect(0, 0, newBg.width(), newBg.height());
        
        m_annotations.clear();
        for (const auto& item : m_dragAnnotationsOriginal) {
            auto clone = item->clone();
            clone->moveBy(-r.topLeft());
            m_annotations.push_back(clone);
        }
        
        setZoom(m_zoomFactor);
        update();
        return;
    }

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
        if (m_isCanvasResizing) {
            m_isCanvasResizing = false;
            m_canvasActiveHandle = -1;
            updateAutoCanvasSize();
            emit historyChanged();
            return;
        }
        
        if (m_currentTool == ToolType::None) {
            if (m_isDragging || m_activeHandle != -1) {
                m_isDragging = false;
                m_activeHandle = -1;
                updateAutoCanvasSize();
                emit historyChanged();
            }
        } else if (m_isDrawing) {
            m_isDrawing = false;
            if (m_currentTool == ToolType::Text) {
                QRect rect = QRect(m_startPoint, m_currentPoint).normalized();
                
                int box_w = rect.width();
                int box_h = rect.height();
                QPoint topLeft;
                int size = m_fontSize;
                
                if (box_w < 15 || box_h < 15) {
                    box_w = qMax(static_cast<int>(m_fontSize * 1.5), 250);
                    box_h = static_cast<int>(m_fontSize * 1.5);
                    topLeft = m_startPoint;
                } else {
                    topLeft = rect.topLeft();
                    size = qMax(10, qMin(200, static_cast<int>(box_h * 0.75)));
                    if (size != m_fontSize) {
                        m_fontSize = size;
                        emit fontSizeChanged(size);
                    }
                }
                
                m_startPoint = topLeft; // Ensure commitText uses the correct corner
                
                m_editingTextObj = nullptr;
                QFont font(m_fontFamily, qMax(8, static_cast<int>(size * m_zoomFactor)), QFont::Bold);
                m_textInput->setFont(font);
                m_textInput->setStyleSheet(QString("QTextEdit { color: %1; background: rgba(0,0,0,180); border: 1px dashed %1; padding: 2px; }").arg(m_currentColor.name()));
                m_textInput->setText("");
                
                int min_char_w = qMax(16, static_cast<int>(size * 0.5));
                box_w = qMax(min_char_w, box_w);
                
                QPoint screenPos = QPoint(topLeft.x() * m_zoomFactor, topLeft.y() * m_zoomFactor);
                m_textInput->setGeometry(screenPos.x(), screenPos.y(), qMax(100, static_cast<int>(box_w * m_zoomFactor)), qMax(40, static_cast<int>(box_h * m_zoomFactor)));
                m_textInput->show();
                m_textInput->setFocus();
                
                update();
            } else if (m_tempItem) {
                saveToHistory();
                m_annotations.push_back(m_tempItem);
                m_tempItem.reset();
                updateAutoCanvasSize();
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
    m_undoStack.clear();
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
