#include "editor_window.h"
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QMessageBox>
#include <QColorDialog>
#include <QStatusBar>
#include <QInputDialog>
#include <QHBoxLayout>
#include <QPainter>
#include <QMouseEvent>
#include <QKeySequence>
#include <QShortcut>
#include <QDebug>
#include <QDateTime>
#include <QCloseEvent>
#include "../resources/IconUtils.h"
#include "../core/common_project.h"

namespace ScreenCut {

static EditorMainWindow* s_editorInstance = nullptr;

EditorMainWindow* EditorMainWindow::instance() {
    if (!s_editorInstance) {
        QPixmap defaultPix(800, 600);
        defaultPix.fill(QColor("#2d323f"));
        s_editorInstance = new EditorMainWindow(defaultPix);
    }
    return s_editorInstance;
}

void EditorMainWindow::openWithPixmap(const QPixmap& pixmap) {
    EditorMainWindow* editor = instance();
    if (!pixmap.isNull()) {
        editor->setPixmap(pixmap);
        editor->m_openedFilePath.clear();
        editor->m_isTempFile = false;
        editor->setWindowTitle("ScreenCut Editor — 0ms Native Studio");
    }
    editor->show();
    editor->activateWindow();
    editor->raise();
}

EditorMainWindow::EditorMainWindow(const QPixmap& initialPixmap, QWidget* parent)
    : QMainWindow(parent) {
    s_editorInstance = this;
    setWindowTitle("ScreenCut Editor — 0ms Native Studio");
    setWindowIcon(createSvgIcon(SVG_EDITOR_APP_ICON, 64, 64));
    resize(qMax(1000, initialPixmap.width() + 100), qMax(700, initialPixmap.height() + 150));

    m_canvas = new CanvasWidget(initialPixmap, this);
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidget(m_canvas);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setAlignment(Qt::AlignCenter);
    m_scrollArea->setStyleSheet("QScrollArea { background-color: #1a1d24; border: none; }");
    setCentralWidget(m_scrollArea);
    setAcceptDrops(true);

    initUI();
    initToolBar();
    initStatusBar();

    connect(m_canvas, &CanvasWidget::historyChanged, this, &EditorMainWindow::updateUndoRedoActions);
    connect(m_canvas, &CanvasWidget::mousePositionChanged, this, [this](const QPoint& pos) {
        m_statusLabel->setText(QString("Cursor: X: %1, Y: %2").arg(pos.x()).arg(pos.y()));
    });

    m_sizeLabel->setText(QString("%1 × %2 px").arg(initialPixmap.width()).arg(initialPixmap.height()));
    updateUndoRedoActions();
}

EditorMainWindow::~EditorMainWindow() = default;

void EditorMainWindow::closeEvent(QCloseEvent* event) {
    if (m_isTempFile && !m_openedFilePath.isEmpty() && !m_openedFilePath.endsWith(".scut", Qt::CaseInsensitive)) {
        qDebug() << "Cleaning up temporary capture file:" << m_openedFilePath;
        QFile::remove(m_openedFilePath);
    }
    event->accept();
}

void EditorMainWindow::setPixmap(const QPixmap& pixmap) {
    if (m_canvas && !pixmap.isNull()) {
        m_canvas->setBackground(pixmap);
        m_sizeLabel->setText(QString("%1 × %2 px").arg(pixmap.width()).arg(pixmap.height()));
        resize(qMax(1000, pixmap.width() + 100), qMax(700, pixmap.height() + 150));
    }
}

bool EditorMainWindow::loadImageFile(const QString& filePath) {
    qDebug() << "[EditorMainWindow] Loading file:" << filePath;
    QPixmap pixmap;
    bool loaded = false;
    QJsonArray annotations;
    if (filePath.endsWith(".scut", Qt::CaseInsensitive)) {
        qDebug() << "[EditorMainWindow] File format is .scut project. Deserializing...";
        loaded = ScutProject::loadScutFile(filePath, pixmap, annotations);
    } else {
        qDebug() << "[EditorMainWindow] File format is image. Loading pixmap...";
        loaded = pixmap.load(filePath);
    }

    if (loaded) {
        qDebug() << "[EditorMainWindow] Successfully loaded file. Pixmap size:" << pixmap.size() << "Annotations count:" << annotations.size();
        setPixmap(pixmap);
        if (!annotations.isEmpty() && m_canvas) {
            m_canvas->loadAnnotationsJson(annotations);
        }
        m_openedFilePath = filePath;
        m_isTempFile = QFileInfo(filePath).fileName().startsWith("screencut_temp_");
        setWindowTitle(QString("ScreenCut Editor — %1").arg(QFileInfo(filePath).fileName()));
        statusBar()->showMessage(QString("✅ Loaded project/image: %1").arg(filePath), 4000);
        return true;
    } else {
        qWarning() << "[EditorMainWindow] Failed to load image or project:" << filePath;
        return false;
    }
}

void EditorMainWindow::initUI() {
    // Apply modern sleek dark theme for editor window
    setStyleSheet(
        "QMainWindow { background-color: #1a1d24; color: #ffffff; }"
        "QToolBar { background-color: #232731; border-bottom: 1px solid #2d323f; spacing: 6px; padding: 6px; }"
        "QToolButton { color: #e0e0e0; background: transparent; border-radius: 6px; padding: 6px 10px; font-weight: bold; }"
        "QToolButton:hover { background-color: #323846; color: #00a8ff; }"
        "QToolButton:checked { background-color: #00a8ff; color: #ffffff; }"
        "QStatusBar { background-color: #1a1d24; color: #9aa0ac; border-top: 1px solid #232731; }"
        "QSlider::groove:horizontal { height: 4px; background: #323846; border-radius: 2px; }"
        "QSlider::handle:horizontal { background: #00a8ff; width: 14px; margin: -5px 0; border-radius: 7px; }"
    );
}

void EditorMainWindow::initToolBar() {
    m_mainToolBar = addToolBar("Main Tools");
    m_mainToolBar->setMovable(false);
    m_mainToolBar->setFloatable(false);
    m_mainToolBar->setIconSize(QSize(20, 20));

    m_toolGroup = new QActionGroup(this);
    m_toolGroup->setExclusive(true);

    auto addToolAction = [this](const QString& text, const QString& tip, const QString& svg, ToolType type, bool checked = false) {
        QAction* act = new QAction(createSvgIcon(svg, 20, 20), text, this);
        act->setToolTip(tip);
        act->setCheckable(true);
        act->setChecked(checked);
        m_toolGroup->addAction(act);
        m_mainToolBar->addAction(act);
        connect(act, &QAction::triggered, this, [this, type]() { setTool(type); });
        return act;
    };

    addToolAction("Arrow (箭頭)", "Draw Arrow (A)", SVG_ARROW, ToolType::Arrow, true);
    addToolAction("Rect (矩形)", "Draw Rectangle (R)", SVG_RECT, ToolType::Rectangle);
    addToolAction("Ellipse (圓形)", "Draw Ellipse (E)", SVG_ELLIPSE, ToolType::Ellipse);
    addToolAction("Freehand (畫筆)", "Draw Freehand Polyline (F)", SVG_PEN, ToolType::Freehand);
    addToolAction("Text (文字)", "Insert Text Box (T)", SVG_TEXT, ToolType::Text);
    addToolAction("Step (步驟標籤)", "Insert Step Markers 1, 2, 3... (S)", SVG_STEP, ToolType::StepMarker);
    addToolAction("Mosaic (馬賽克)", "Pixelate Sensitive Region (M)", SVG_MOSAIC, ToolType::Mosaic);
    addToolAction("Blur (模糊)", "Gaussian Blur Region (B)", SVG_BLUR, ToolType::Blur);
    addToolAction("Highlight (螢光筆)", "Yellow Highlighter (H)", SVG_HIGHLIGHT, ToolType::Highlight);

    m_mainToolBar->addSeparator();

    // Color Picker Button
    QAction* colorAction = new QAction(createSvgIcon(SVG_COLOR, 20, 20), "Color (顏色)", this);
    colorAction->setToolTip("Pick Annotation Color");
    m_mainToolBar->addAction(colorAction);
    connect(colorAction, &QAction::triggered, this, [this]() {
        QColor chosen = QColorDialog::getColor(m_currentColor, this, "Select Annotation Color");
        if (chosen.isValid()) {
            setColor(chosen);
        }
    });

    // Line Width Slider
    m_mainToolBar->addWidget(new QLabel("   Width: ", this));
    m_widthSlider = new QSlider(Qt::Horizontal, this);
    m_widthSlider->setRange(1, 20);
    m_widthSlider->setValue(3);
    m_widthSlider->setFixedWidth(100);
    m_mainToolBar->addWidget(m_widthSlider);
    connect(m_widthSlider, &QSlider::valueChanged, this, &EditorMainWindow::setLineWidth);

    m_mainToolBar->addSeparator();

    // Undo / Redo
    m_undoAction = new QAction(createSvgIcon(SVG_UNDO, 20, 20), "Undo (復原)", this);
    m_undoAction->setShortcut(QKeySequence::Undo);
    connect(m_undoAction, &QAction::triggered, this, &EditorMainWindow::undo);
    m_mainToolBar->addAction(m_undoAction);

    m_redoAction = new QAction(createSvgIcon(SVG_REDO, 20, 20), "Redo (重做)", this);
    m_redoAction->setShortcut(QKeySequence::Redo);
    connect(m_redoAction, &QAction::triggered, this, &EditorMainWindow::redo);
    m_mainToolBar->addAction(m_redoAction);

    m_mainToolBar->addSeparator();

    // Open, Copy & Save
    m_openAction = new QAction(createSvgIcon(SVG_OPEN, 20, 20), "Open (開啟)", this);
    m_openAction->setShortcut(QKeySequence::Open);
    connect(m_openAction, &QAction::triggered, this, &EditorMainWindow::openFile);
    m_mainToolBar->addAction(m_openAction);

    m_copyAction = new QAction(createSvgIcon(SVG_COPY, 20, 20), "Copy (複製)", this);
    m_copyAction->setShortcut(QKeySequence::Copy);
    connect(m_copyAction, &QAction::triggered, this, &EditorMainWindow::copyToClipboard);
    m_mainToolBar->addAction(m_copyAction);

    m_saveAction = new QAction(createSvgIcon(SVG_SAVE, 20, 20), "Save (儲存)", this);
    m_saveAction->setShortcut(QKeySequence::Save);
    connect(m_saveAction, &QAction::triggered, this, &EditorMainWindow::saveToFile);
    m_mainToolBar->addAction(m_saveAction);
}

void EditorMainWindow::initStatusBar() {
    QStatusBar* bar = statusBar();
    m_statusLabel = new QLabel("Ready", this);
    m_sizeLabel = new QLabel("0 × 0 px", this);
    
    bar->addWidget(m_statusLabel, 1);
    bar->addPermanentWidget(m_sizeLabel);
}

void EditorMainWindow::setTool(ToolType tool) {
    if (m_canvas) m_canvas->setTool(tool);
}

void EditorMainWindow::setColor(const QColor& color) {
    m_currentColor = color;
    if (m_canvas) m_canvas->setColor(color);
}

void EditorMainWindow::setLineWidth(int width) {
    m_currentLineWidth = width;
    if (m_canvas) m_canvas->setLineWidth(width);
}

void EditorMainWindow::undo() {
    if (m_canvas) m_canvas->undo();
}

void EditorMainWindow::redo() {
    if (m_canvas) m_canvas->redo();
}

void EditorMainWindow::copyToClipboard() {
    if (!m_canvas) return;
    qDebug() << "[EditorMainWindow] Rendering final pixmap for clipboard copy...";
    QPixmap finalPix = m_canvas->renderFinalPixmap();
    QApplication::clipboard()->setPixmap(finalPix);
    qDebug() << "[EditorMainWindow] Copied image to clipboard. Size:" << finalPix.size();
    statusBar()->showMessage("✅ Image successfully copied to clipboard!", 3000);
}

void EditorMainWindow::saveToFile() {
    if (!m_canvas) return;
    QString defaultPath = ScutProject::getLibraryDir() + "/" + 
        QString("Project_%1.scut").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    if (!m_openedFilePath.isEmpty() && m_openedFilePath.endsWith(".scut", Qt::CaseInsensitive)) {
        defaultPath = m_openedFilePath;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "Save ScreenCut Project / Image",
        defaultPath,
        "ScreenCut Project (*.scut);;PNG Image (*.png);;JPEG Image (*.jpg);;WebP Image (*.webp)");

    if (!fileName.isEmpty()) {
        qDebug() << "[EditorMainWindow] Saving to target file:" << fileName;
        bool saved = false;
        if (fileName.endsWith(".scut", Qt::CaseInsensitive)) {
            QJsonArray annotations = m_canvas->saveAnnotationsJson();
            qDebug() << "[EditorMainWindow] Serializing as .scut with" << annotations.size() << "annotations.";
            saved = ScutProject::saveImageAsScut(m_canvas->background(), fileName, annotations);
        } else {
            qDebug() << "[EditorMainWindow] Exporting flat image pixmap...";
            QPixmap finalPix = m_canvas->renderFinalPixmap();
            saved = finalPix.save(fileName);
        }
        if (saved) {
            qDebug() << "[EditorMainWindow] Successfully saved file:" << fileName;
            m_openedFilePath = fileName;
            m_isTempFile = false;
            setWindowTitle(QString("ScreenCut Editor — %1").arg(QFileInfo(fileName).fileName()));
            statusBar()->showMessage(QString("✅ Saved to %1").arg(fileName), 4000);
        } else {
            qCritical() << "[EditorMainWindow] Failed to save file to location:" << fileName;
            QMessageBox::critical(this, "Save Error", "Failed to save project or image to specified location.");
        }
    } else {
        qDebug() << "[EditorMainWindow] Save dialog cancelled by user.";
    }
}

void EditorMainWindow::updateUndoRedoActions() {
    if (m_canvas) {
        m_undoAction->setEnabled(m_canvas->canUndo());
        m_redoAction->setEnabled(m_canvas->canRedo());
    }
}

void EditorMainWindow::openFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open Project or Image File", ScutProject::getLibraryDir(),
        "ScreenCut Files (*.scut *.png *.jpg *.jpeg *.bmp *.webp *.svg *.gif);;ScreenCut Projects (*.scut);;Images (*.png *.jpg *.jpeg *.bmp *.webp *.svg *.gif);;All Files (*.*)");
    if (!fileName.isEmpty()) {
        loadImageFile(fileName);
    }
}

void EditorMainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> urls = event->mimeData()->urls();
        if (!urls.isEmpty()) {
            QString path = urls.first().toLocalFile();
            QFileInfo fi(path);
            QString ext = fi.suffix().toLower();
            if (ext == "scut" || ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" || ext == "webp" || ext == "gif" || ext == "svg") {
                event->acceptProposedAction();
                return;
            }
        }
    }
    event->ignore();
}

void EditorMainWindow::dropEvent(QDropEvent* event) {
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> urls = event->mimeData()->urls();
        if (!urls.isEmpty()) {
            QString path = urls.first().toLocalFile();
            if (loadImageFile(path)) {
                event->acceptProposedAction();
                return;
            }
        }
    }
    event->ignore();
}

// ============================================================================
// CanvasWidget Implementation
// ============================================================================

CanvasWidget::CanvasWidget(const QPixmap& background, QWidget* parent)
    : QWidget(parent), m_background(background) {
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
    if (!m_background.isNull()) {
        setFixedSize(m_background.size());
    }
}

CanvasWidget::~CanvasWidget() = default;

void CanvasWidget::setBackground(const QPixmap& background) {
    m_background = background;
    if (!m_background.isNull()) {
        setFixedSize(m_background.size());
    }
    m_annotations.clear();
    m_redoStack.clear();
    update();
    emit historyChanged();
}

void CanvasWidget::setTool(ToolType tool) {
    m_currentTool = tool;
}

void CanvasWidget::setColor(const QColor& color) {
    m_currentColor = color;
}

void CanvasWidget::setLineWidth(int width) {
    m_currentLineWidth = width;
}

void CanvasWidget::undo() {
    if (!m_annotations.empty()) {
        m_redoStack.push_back(m_annotations.back());
        m_annotations.pop_back();
        update();
        emit historyChanged();
    }
}

void CanvasWidget::redo() {
    if (!m_redoStack.empty()) {
        m_annotations.push_back(m_redoStack.back());
        m_redoStack.pop_back();
        update();
        emit historyChanged();
    }
}

QPixmap CanvasWidget::renderFinalPixmap() {
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

void CanvasWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

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

void CanvasWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) return;

    m_isDrawing = true;
    m_startPoint = event->pos();
    m_currentPoint = event->pos();

    if (m_currentTool == ToolType::Arrow) {
        auto arrow = std::make_shared<ArrowAnnotation>(m_startPoint, m_currentPoint);
        arrow->color = m_currentColor;
        arrow->lineWidth = m_currentLineWidth;
        m_tempItem = arrow;
    } else if (m_currentTool == ToolType::Rectangle || m_currentTool == ToolType::Ellipse) {
        auto shape = std::make_shared<ShapeAnnotation>(m_currentTool, QRect(m_startPoint, m_currentPoint));
        shape->color = m_currentColor;
        shape->lineWidth = m_currentLineWidth;
        m_tempItem = shape;
    } else if (m_currentTool == ToolType::Freehand) {
        auto freehand = std::make_shared<FreehandAnnotation>();
        freehand->color = m_currentColor;
        freehand->lineWidth = m_currentLineWidth;
        freehand->addPoint(m_startPoint);
        m_tempItem = freehand;
    } else if (m_currentTool == ToolType::StepMarker) {
        auto step = std::make_shared<StepMarkerAnnotation>(m_startPoint, m_nextStepNumber);
        step->color = m_currentColor;
        m_annotations.push_back(step);
        m_nextStepNumber++;
        m_redoStack.clear();
        m_isDrawing = false;
        update();
        emit historyChanged();
    } else if (m_currentTool == ToolType::Text) {
        bool ok = false;
        QString text = QInputDialog::getText(this, "Insert Text Annotation", "Enter text:", QLineEdit::Normal, "", &ok);
        if (ok && !text.isEmpty()) {
            auto txtItem = std::make_shared<TextAnnotation>(m_startPoint, text);
            txtItem->color = m_currentColor;
            m_annotations.push_back(txtItem);
            m_redoStack.clear();
            update();
            emit historyChanged();
        }
        m_isDrawing = false;
    } else if (m_currentTool == ToolType::Mosaic || m_currentTool == ToolType::Blur) {
        auto shader = std::make_shared<ShaderAnnotation>(m_currentTool, QRect(m_startPoint, m_currentPoint));
        m_tempItem = shader;
    } else if (m_currentTool == ToolType::Highlight) {
        auto highlight = std::make_shared<HighlightAnnotation>(QRect(m_startPoint, m_currentPoint));
        highlight->color = QColor(255, 235, 59); // Yellow
        m_tempItem = highlight;
    }
    update();
}

void CanvasWidget::mouseMoveEvent(QMouseEvent* event) {
    m_currentPoint = event->pos();
    emit mousePositionChanged(m_currentPoint);

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

void CanvasWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_isDrawing) {
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

QJsonArray CanvasWidget::saveAnnotationsJson() const {
    QJsonArray arr;
    for (const auto& item : m_annotations) {
        if (item) {
            arr.append(item->toJson());
        }
    }
    return arr;
}

void CanvasWidget::loadAnnotationsJson(const QJsonArray& arr) {
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
