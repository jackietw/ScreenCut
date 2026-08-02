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
#include <QPushButton>
#include "../editor/editor_models.h"
#include "tools/context.h"
#include "tools/base.h"

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
    ToolContext& getToolContext() { return m_toolContext; }
    
    // API for Tool Handlers
    QPoint mapToImage(const QPoint& viewPos) const;
    void setTempItem(std::shared_ptr<AnnotationItem> item);
    std::shared_ptr<AnnotationItem> getTempItem() const { return m_tempItem; }
    void addAnnotation(std::shared_ptr<AnnotationItem> item);
    std::shared_ptr<PolygonAnnotation> getCurrentPolygon() const { return m_currentPolygon; }
    void setCurrentPolygon(std::shared_ptr<PolygonAnnotation> poly) { m_currentPolygon = poly; }
    std::shared_ptr<AnnotationItem> getEditingTextObj() const { return m_editingTextObj; }
    void setEditingTextObj(std::shared_ptr<AnnotationItem> obj) { m_editingTextObj = obj; }
    QTextEdit* getTextInput() const { return m_textInput; }
    void showTextInput(const QPoint& pos, const QString& text);
    void commitText();
    void setDrawing(bool drawing);
    void setDragging(bool dragging);
    std::shared_ptr<AnnotationItem> getSelectedItem() const { return m_selectedItem; }
    void setSelectedItem(std::shared_ptr<AnnotationItem> item);
    void updateCanvas();

    void resetStepCounter();
    int getNextStepNumber() { return m_nextStepNumber++; }

    


    void applyCrop();
    void cancelCrop();
    void updateOverlayPosition();

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

public slots:
    void commitTextSlot();
    void resizeTextInput();

signals:
    void historyChanged();
    void mousePositionChanged(const QPoint& pos);
    void zoomChanged(qreal zoom);
    void itemSelected(std::shared_ptr<AnnotationItem> item);
    void fontSizeChanged(int size);
    void toolChanged(ToolType tool);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:

    QPixmap m_background;
    QTextEdit* m_textInput = nullptr;
    std::shared_ptr<AnnotationItem> m_editingTextObj = nullptr;
    std::shared_ptr<PolygonAnnotation> m_currentPolygon = nullptr;

    ToolType m_currentTool = ToolType::Arrow;
    int m_nextStepNumber = 1;
    qreal m_zoomFactor = 1.0;
    std::unique_ptr<BaseTool> m_currentToolHandler;
    ToolContext m_toolContext;

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
    
    // Crop state
    bool m_isCroppingMode = false;
    bool m_isCroppingDrag = false;
    QRect m_cropRect;
    int m_cropActiveHandle = -1;
    QPoint m_lastCropDragPoint;
    QWidget* m_cropOverlayWidget = nullptr;
    QPushButton* m_btnCrop = nullptr;
    QPushButton* m_btnCancelCrop = nullptr;

    int m_activeHandle = -1;
    int m_canvasActiveHandle = -1;
    int hitTestCropHandle(const QPoint& pos) const;
    
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
