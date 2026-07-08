#ifndef EDITOR_WINDOW_H
#define EDITOR_WINDOW_H

#include <QMainWindow>
#include <QPixmap>
#include <QScrollArea>
#include <QWidget>
#include <QToolBar>
#include <QAction>
#include <QActionGroup>
#include <QSlider>
#include <QLabel>
#include <QColor>
#include <QPoint>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QFile>
#include <QFileInfo>
#include <vector>
#include <memory>
#include <QJsonArray>
#include "editor_models.h"

namespace ScreenCut {

class CanvasWidget;

class EditorMainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit EditorMainWindow(const QPixmap& initialPixmap, QWidget* parent = nullptr);
    ~EditorMainWindow() override;

    static EditorMainWindow* instance();
    static void openWithPixmap(const QPixmap& pixmap);
    void setPixmap(const QPixmap& pixmap);
    bool loadImageFile(const QString& filePath);
    CanvasWidget* canvas() const { return m_canvas; }

protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

public slots:
    void setTool(ToolType tool);
    void setColor(const QColor& color);
    void setLineWidth(int width);
    void undo();
    void redo();
    void copyToClipboard();
    void saveToFile();
    void openFile();

private:
    void initUI();
    void initToolBar();
    void initStatusBar();
    void updateUndoRedoActions();

    CanvasWidget* m_canvas = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QToolBar* m_mainToolBar = nullptr;

    QActionGroup* m_toolGroup = nullptr;
    QAction* m_undoAction = nullptr;
    QAction* m_redoAction = nullptr;
    QAction* m_copyAction = nullptr;
    QAction* m_saveAction = nullptr;
    QAction* m_openAction = nullptr;
    QString m_openedFilePath;
    bool m_isTempFile = false;

    QLabel* m_statusLabel = nullptr;
    QLabel* m_sizeLabel = nullptr;
    QSlider* m_widthSlider = nullptr;

    QColor m_currentColor = QColor(255, 59, 48); // Vibrant Red
    int m_currentLineWidth = 3;
};

// Canvas Widget responsible for displaying snapshot and interacting with annotations
class CanvasWidget : public QWidget {
    Q_OBJECT
public:
    explicit CanvasWidget(const QPixmap& background, QWidget* parent = nullptr);
    ~CanvasWidget() override;

    void setBackground(const QPixmap& background);
    const QPixmap& background() const { return m_background; }
    void setTool(ToolType tool);
    void setColor(const QColor& color);
    void setLineWidth(int width);

    bool canUndo() const { return !m_annotations.empty(); }
    bool canRedo() const { return !m_redoStack.empty(); }
    void undo();
    void redo();

    QPixmap renderFinalPixmap();
    QJsonArray saveAnnotationsJson() const;
    void loadAnnotationsJson(const QJsonArray& arr);

signals:
    void historyChanged();
    void mousePositionChanged(const QPoint& pos);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    QPixmap m_background;
    ToolType m_currentTool = ToolType::Arrow;
    QColor m_currentColor = QColor(255, 59, 48);
    int m_currentLineWidth = 3;
    int m_nextStepNumber = 1;

    std::vector<std::shared_ptr<AnnotationItem>> m_annotations;
    std::vector<std::shared_ptr<AnnotationItem>> m_redoStack;

    bool m_isDrawing = false;
    QPoint m_startPoint;
    QPoint m_currentPoint;
    std::shared_ptr<AnnotationItem> m_tempItem;
};

} // namespace ScreenCut

#endif // EDITOR_WINDOW_H
