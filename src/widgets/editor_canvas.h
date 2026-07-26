/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef EDITOR_CANVAS_H
#define EDITOR_CANVAS_H

#include <QWidget>
#include <QPixmap>
#include <QJsonArray>
#include <vector>
#include <memory>
#include <QString>
#include "../editor/editor_models.h"

class QTextEdit;

namespace ScreenCut {

class EditorCanvas : public QWidget {
    Q_OBJECT
public:
    explicit EditorCanvas(const QPixmap& background, QWidget* parent = nullptr);
    ~EditorCanvas() override;

    void setBackground(const QPixmap& background);
    const QPixmap& background() const { return m_background; }
    void setTool(ToolType tool);
    void setColor(const QColor& color);
    void setLineWidth(int width);
    
    void setArrowType(const QString& type);
    void setShapeStyle(const QString& style);
    void setLineStyle(const QString& style);
    void setFontFamily(const QString& family);
    void setFontSize(int size);
    
    // New text property setters
    void setTextIsBold(bool bold);
    void setTextIsItalic(bool italic);
    void setTextIsUnderline(bool underline);
    void setTextIsStrikeOut(bool strike);
    void setTextHAlign(TextAnnotation::TextAlign align);
    void setTextVAlign(TextAnnotation::VerticalAlign align);
    void setTextOpacity(int opacity);
    void setTextLineSpacing(int spacing);
    void setTextOutlineColor(const QColor& color);
    void setTextHasShadow(bool hasShadow);
    void setTextShadowDirection(ShadowDirection direction);
    void setTextOutlineWidth(int width);

    void setBlurType(ToolType type);
    void setBlurIntensity(int intensity);
    void setPenStyle(const QString& style);
    void resetStepCounter();

    bool canUndo() const { return !m_undoStack.empty(); }
    bool canRedo() const { return !m_redoStack.empty(); }
    void undo();
    void redo();
    void saveToHistory();
    void updateAutoCanvasSize();
    int hitTestCanvasHandle(const QPoint& pos) const;

    QPixmap renderFinalPixmap();
    QJsonArray saveAnnotationsJson() const;
    void loadAnnotationsJson(const QJsonArray& arr);

    void setZoom(qreal zoom);
    qreal zoom() const { return m_zoomFactor; }

private slots:
    void commitText();
    void resizeTextInput();

signals:
    void historyChanged();
    void mousePositionChanged(const QPoint& pos);
    void zoomChanged(qreal zoom);
    void itemSelected(std::shared_ptr<AnnotationItem> item);
    void fontSizeChanged(int size);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    QPoint mapToImage(const QPoint& viewPos) const;

    QPixmap m_background;
    QTextEdit* m_textInput = nullptr;
    std::shared_ptr<AnnotationItem> m_editingTextObj = nullptr;

    ToolType m_currentTool = ToolType::Arrow;
    QColor m_currentColor = QColor(255, 59, 48);
    int m_currentLineWidth = 3;
    int m_nextStepNumber = 1;
    qreal m_zoomFactor = 1.0;
    
    QString m_arrowType = "Single Arrow";
    QString m_shapeStyle = "Rectangle";
    QString m_lineStyle = "Solid";
    QString m_fontFamily = "Arial";
    int m_fontSize = 24;
    
    bool m_textIsBold = false;
    bool m_textIsItalic = false;
    bool m_textIsUnderline = false;
    bool m_textIsStrikeOut = false;
    TextAnnotation::TextAlign m_textHAlign = TextAnnotation::TextAlign::Left;
    TextAnnotation::VerticalAlign m_textVAlign = TextAnnotation::VerticalAlign::Top;
    int m_textOpacity = 100;
    int m_textLineSpacing = 0;
    QColor m_textOutlineColor = Qt::transparent;
    bool m_textHasShadow = false;
    ShadowDirection m_textShadowDirection = ShadowDirection::BottomRight;
    int m_textOutlineWidth = 0;

    ToolType m_blurType = ToolType::Mosaic;
    int m_blurIntensity = 15;
    QString m_penStyle = "Solid Pen";

    struct HistoryState {
        QPixmap background;
        QRect baseCanvasRect;
        std::vector<std::shared_ptr<AnnotationItem>> annotations;
    };
    
    std::vector<std::shared_ptr<AnnotationItem>> m_annotations;
    std::vector<HistoryState> m_undoStack;
    std::vector<HistoryState> m_redoStack;

    bool m_isDrawing = false;
    bool m_isDragging = false;
    bool m_isCanvasResizing = false;
    int m_activeHandle = -1;
    int m_canvasActiveHandle = -1;
    QPoint m_startPoint;
    QPoint m_startGlobalPos;
    
    QRect m_baseCanvasRect;
    QPixmap m_dragBackgroundOriginal;
    std::vector<std::shared_ptr<AnnotationItem>> m_dragAnnotationsOriginal;
    QRect m_dragOriginalRect;
    QPoint m_currentPoint;
    QPoint m_lastDragPoint;
    std::shared_ptr<AnnotationItem> m_tempItem;
    std::shared_ptr<AnnotationItem> m_selectedItem;
};

} // namespace ScreenCut

#endif // EDITOR_CANVAS_H
