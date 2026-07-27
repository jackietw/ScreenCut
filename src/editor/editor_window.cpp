/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "editor_window.h"
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QDebug>
#include <QDateTime>
#include <QCloseEvent>
#include <QMimeData>
#include <QUrl>
#include "../resources/IconUtils.h"
#include "../core/common_project.h"
#include "../widgets/common_notification.h"
#include <QTimer>
#include <QProcess>
#include <QCoreApplication>

namespace ScreenCut {

static EditorMainWindow* s_editorInstance = nullptr;

EditorMainWindow* EditorMainWindow::instance() {
    if (!s_editorInstance) {
        QPixmap defaultPix(800, 600);
        defaultPix.fill(QColor("#1a1a1a"));
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
        editor->setWindowTitle("ScreenCut Editor");
    }
    editor->show();
    editor->activateWindow();
    editor->raise();
}

EditorMainWindow::EditorMainWindow(const QPixmap& initialPixmap, QWidget* parent)
    : QMainWindow(parent) {
    s_editorInstance = this;
    setWindowTitle("ScreenCut Editor");
    setWindowIcon(createSvgIcon(SVG_EDITOR_APP_ICON, 64, 64));
    resize(1080, 720);

    initUI();
    setPixmap(initialPixmap);
    
    setAcceptDrops(true);
}

EditorMainWindow::~EditorMainWindow() = default;

void EditorMainWindow::closeEvent(QCloseEvent* event) {
    if (m_isTempFile && !m_openedFilePath.isEmpty() && !m_openedFilePath.endsWith(".scut", Qt::CaseInsensitive)) {
        QFile::remove(m_openedFilePath);
    }
    event->accept();
}

void EditorMainWindow::setPixmap(const QPixmap& pixmap) {
    if (m_canvas && !pixmap.isNull()) {
        m_canvas->setBackground(pixmap);
        updateResolutionLabel();
        updateZoomLabel();
        QTimer::singleShot(10, this, &EditorMainWindow::autoFit);
    }
}

bool EditorMainWindow::loadImageFile(const QString& filePath) {
    QPixmap pixmap;
    bool loaded = false;
    QJsonArray annotations;
    if (filePath.endsWith(".scut", Qt::CaseInsensitive)) {
        loaded = ScutProject::loadScutFile(filePath, pixmap, annotations);
    } else {
        loaded = pixmap.load(filePath);
    }

    if (loaded) {
        setPixmap(pixmap);
        if (!annotations.isEmpty() && m_canvas) {
            m_canvas->loadAnnotationsJson(annotations);
        }
        m_openedFilePath = filePath;
        m_isTempFile = QFileInfo(filePath).fileName().startsWith("screencut_temp_");
        setWindowTitle(QString("ScreenCut Editor — %1").arg(QFileInfo(filePath).fileName()));
        if (m_thumbsStrip) m_thumbsStrip->setCurrentFile(filePath);
        return true;
    }
    return false;
}

void EditorMainWindow::initUI() {
    setStyleSheet(R"(
        QMainWindow { background-color: #1e1e1e; }
        QScrollArea { background-color: #141414; border: none; }
        QScrollBar:horizontal { height: 10px; background: #141414; margin: 0px; }
        QScrollBar::handle:horizontal { background: #475569; min-width: 24px; border-radius: 5px; margin: 1px; }
        QScrollBar:vertical { width: 10px; background: #141414; margin: 0px; }
        QScrollBar::handle:vertical { background: #475569; min-height: 24px; border-radius: 5px; margin: 1px; }
    )");

    m_canvas = new EditorCanvas(QPixmap(), this);
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidget(m_canvas);
    m_scrollArea->setAlignment(Qt::AlignCenter);
    setCentralWidget(m_scrollArea);

    m_toolBar = new EditorToolBar(this);
    addToolBar(m_toolBar);

    m_propsPanel = new EditorPropsPanel(this);
    m_propsDock = new QDockWidget("Properties", this);
    m_propsDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    m_propsDock->setTitleBarWidget(new QWidget()); // Hide titlebar
    m_propsDock->setWidget(m_propsPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_propsDock);

    initBottomBar();

    // Wiring Toolbar <-> Canvas <-> Props
    connect(m_toolBar, &EditorToolBar::toolSelected, m_canvas, &EditorCanvas::setTool);
    connect(m_toolBar, &EditorToolBar::toolSelected, m_propsPanel, &EditorPropsPanel::setCurrentTool);
    connect(m_toolBar, &EditorToolBar::toolSelected, this, [this](ToolType tool) {
        if (tool == ToolType::Crop) {
            m_propsDock->hide();
        } else {
            m_propsDock->show();
        }
    });
    
    connect(m_propsPanel, &EditorPropsPanel::colorChanged, m_canvas, &EditorCanvas::setColor);
    connect(m_propsPanel, &EditorPropsPanel::lineWidthChanged, m_canvas, &EditorCanvas::setLineWidth);

    connect(m_toolBar, &EditorToolBar::undoClicked, m_canvas, &EditorCanvas::undo);
    connect(m_toolBar, &EditorToolBar::redoClicked, m_canvas, &EditorCanvas::redo);
    connect(m_canvas, &EditorCanvas::toolChanged, m_toolBar, &EditorToolBar::setActiveTool);
    connect(m_toolBar, &EditorToolBar::captureClicked, this, []() {
        QString captureAppPath = QCoreApplication::applicationDirPath() + "/ScreenCut";
#ifdef Q_OS_WIN
        captureAppPath += ".exe";
#endif
        QProcess::startDetached(captureAppPath, QStringList());
    });
    
    connect(m_canvas, &EditorCanvas::historyChanged, this, [this]() {
        m_toolBar->updateUndoRedoState(m_canvas->canUndo(), m_canvas->canRedo());
    });

    connect(m_canvas, &EditorCanvas::itemSelected, m_propsPanel, static_cast<void(EditorPropsPanel::*)(const std::shared_ptr<AnnotationItem>&)>(&EditorPropsPanel::syncFromSelection));
    connect(m_canvas, &EditorCanvas::fontSizeChanged, m_propsPanel, &EditorPropsPanel::updateFontSizeUI);

    // Dynamic properties connections
    connect(m_propsPanel, &EditorPropsPanel::arrowTypeChanged, m_canvas, &EditorCanvas::setArrowType);
    connect(m_propsPanel, &EditorPropsPanel::arrowStartStyleChanged, m_canvas, &EditorCanvas::setArrowStartStyle);
    connect(m_propsPanel, &EditorPropsPanel::arrowEndStyleChanged, m_canvas, &EditorCanvas::setArrowEndStyle);
    connect(m_propsPanel, &EditorPropsPanel::arrowLineStyleChanged, m_canvas, &EditorCanvas::setArrowLineStyle);
    connect(m_propsPanel, &EditorPropsPanel::arrowOpacityChanged, m_canvas, &EditorCanvas::setArrowOpacity);
    connect(m_propsPanel, &EditorPropsPanel::arrowStartSizeChanged, m_canvas, &EditorCanvas::setArrowStartSize);
    connect(m_propsPanel, &EditorPropsPanel::arrowEndSizeChanged, m_canvas, &EditorCanvas::setArrowEndSize);
    connect(m_propsPanel, &EditorPropsPanel::arrowShadowChanged, m_canvas, &EditorCanvas::setArrowShadow);
    connect(m_propsPanel, &EditorPropsPanel::shapeStyleChanged, m_canvas, &EditorCanvas::setShapeStyle);
    connect(m_propsPanel, &EditorPropsPanel::lineStyleChanged, m_canvas, &EditorCanvas::setLineStyle);
    connect(m_propsPanel, &EditorPropsPanel::fontFamilyChanged, m_canvas, &EditorCanvas::setFontFamily);
    connect(m_propsPanel, &EditorPropsPanel::fontSizeChanged, m_canvas, &EditorCanvas::setFontSize);
    
    // Advanced text properties
    connect(m_propsPanel, &EditorPropsPanel::textIsBoldChanged, m_canvas, &EditorCanvas::setTextIsBold);
    connect(m_propsPanel, &EditorPropsPanel::textIsItalicChanged, m_canvas, &EditorCanvas::setTextIsItalic);
    connect(m_propsPanel, &EditorPropsPanel::textIsUnderlineChanged, m_canvas, &EditorCanvas::setTextIsUnderline);
    connect(m_propsPanel, &EditorPropsPanel::textIsStrikeOutChanged, m_canvas, &EditorCanvas::setTextIsStrikeOut);
    connect(m_propsPanel, &EditorPropsPanel::textHAlignChanged, m_canvas, &EditorCanvas::setTextHAlign);
    connect(m_propsPanel, &EditorPropsPanel::textVAlignChanged, m_canvas, &EditorCanvas::setTextVAlign);
    connect(m_propsPanel, &EditorPropsPanel::textOpacityChanged, m_canvas, &EditorCanvas::setTextOpacity);
    connect(m_propsPanel, &EditorPropsPanel::textLineSpacingChanged, m_canvas, &EditorCanvas::setTextLineSpacing);
    connect(m_propsPanel, &EditorPropsPanel::textOutlineColorChanged, m_canvas, &EditorCanvas::setTextOutlineColor);
    connect(m_propsPanel, &EditorPropsPanel::textShadowChanged, m_canvas, &EditorCanvas::setTextShadow);
    connect(m_propsPanel, &EditorPropsPanel::textOutlineWidthChanged, m_canvas, &EditorCanvas::setTextOutlineWidth);
    connect(m_propsPanel, &EditorPropsPanel::blurTypeChanged, m_canvas, &EditorCanvas::setBlurType);
    connect(m_propsPanel, &EditorPropsPanel::blurIntensityChanged, m_canvas, &EditorCanvas::setBlurIntensity);
    connect(m_propsPanel, &EditorPropsPanel::penStyleChanged, m_canvas, &EditorCanvas::setPenStyle);
    connect(m_propsPanel, &EditorPropsPanel::resetStepCounter, m_canvas, &EditorCanvas::resetStepCounter);

    connect(m_toolBar, &EditorToolBar::copyClicked, this, &EditorMainWindow::copyToClipboard);
    connect(m_toolBar, &EditorToolBar::saveClicked, this, &EditorMainWindow::saveToFile);
}

void EditorMainWindow::initBottomBar() {
    QFrame* bottomContainer = new QFrame(this);
    bottomContainer->setStyleSheet("QFrame { background-color: #0f172a; border-top: 1px solid #1e293b; color: white; }");
    QVBoxLayout* mainBottomLayout = new QVBoxLayout(bottomContainer);
    mainBottomLayout->setContentsMargins(0, 0, 0, 0);
    mainBottomLayout->setSpacing(0);

    QFrame* upperBar = new QFrame();
    upperBar->setFixedHeight(32);
    upperBar->setStyleSheet("QFrame { background-color: #1a202c; border-bottom: 1px solid #111827; }");
    QHBoxLayout* row1 = new QHBoxLayout(upperBar);
    row1->setContentsMargins(16, 0, 16, 0);
    row1->setSpacing(10);

    m_btnToggleRecent = new QPushButton(" Hide Recent", this);
    m_btnToggleRecent->setIcon(createSvgIcon(SVG_RECENT, 16, 16));
    m_btnToggleRecent->setStyleSheet("QPushButton { background: transparent; border: none; color: #cbd5e1; font-weight: 600; } QPushButton:hover { color: #60a5fa; }");
    m_btnToggleRecent->setCursor(Qt::PointingHandCursor);
    
    row1->addWidget(m_btnToggleRecent);
    row1->addStretch();

    m_btnZoom = new QPushButton("100% ▼", this);
    m_btnZoom->setStyleSheet("QPushButton { background: #2d3748; border: 1px solid #4a5568; border-radius: 4px; padding: 2px 10px; color: #f7fafc; font-weight: bold; } QPushButton:hover { border-color: #60a5fa; }");
    
    QMenu* zoomMenu = new QMenu(this);
    zoomMenu->setStyleSheet("QMenu { background-color: #2d3748; color: white; } QMenu::item:selected { background-color: #3b82f6; }");
    
    QAction* actAutoFit = zoomMenu->addAction("Auto fit");
    connect(actAutoFit, &QAction::triggered, this, &EditorMainWindow::autoFit);
    zoomMenu->addSeparator();

    for (qreal z : {0.5, 0.75, 1.0, 1.25, 1.5, 2.0, 3.0}) {
        QAction* act = zoomMenu->addAction(QString("%1%").arg(z * 100));
        connect(act, &QAction::triggered, this, [this, z]() { m_canvas->setZoom(z); });
    }
    m_btnZoom->setMenu(zoomMenu);
    row1->addWidget(m_btnZoom);

    m_btnResize = new QPushButton("0 x 0 px", this);
    m_btnResize->setStyleSheet("QPushButton { background: #2d3748; border: 1px solid #4a5568; border-radius: 4px; padding: 2px 10px; color: #f7fafc; font-weight: bold; }");
    row1->addWidget(m_btnResize);

    mainBottomLayout->addWidget(upperBar);

    m_recentStripContainer = new QWidget();
    QVBoxLayout* row2 = new QVBoxLayout(m_recentStripContainer);
    row2->setContentsMargins(0, 0, 0, 0);
    m_thumbsStrip = new EditorThumbsStrip(this);
    row2->addWidget(m_thumbsStrip);
    mainBottomLayout->addWidget(m_recentStripContainer);

    m_bottomDock = new QDockWidget("BottomBar", this);
    m_bottomDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    m_bottomDock->setTitleBarWidget(new QWidget());
    m_bottomDock->setWidget(bottomContainer);

    setCorner(Qt::BottomLeftCorner, Qt::BottomDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, m_bottomDock);

    connect(m_btnToggleRecent, &QPushButton::clicked, this, [this]() {
        bool isVis = m_recentStripContainer->isVisible();
        m_recentStripContainer->setVisible(!isVis);
        m_btnToggleRecent->setText(isVis ? " Show Recent" : " Hide Recent");
    });
    connect(m_canvas, &EditorCanvas::zoomChanged, this, &EditorMainWindow::updateZoomLabel);
    connect(m_thumbsStrip, &EditorThumbsStrip::fileSelected, this, [this](const QString& path) {
        loadImageFile(path);
    });

    m_thumbsStrip->refreshLibrary();
}

void EditorMainWindow::updateZoomLabel() {
    if (m_canvas) {
        m_btnZoom->setText(QString("%1% ▼").arg(static_cast<int>(m_canvas->zoom() * 100)));
    }
}

void EditorMainWindow::updateResolutionLabel() {
    if (m_canvas && !m_canvas->background().isNull()) {
        m_btnResize->setText(QString("%1 x %2 px").arg(m_canvas->background().width()).arg(m_canvas->background().height()));
    }
}

void EditorMainWindow::copyToClipboard() {
    if (!m_canvas) return;
    QPixmap finalPix = m_canvas->renderFinalPixmap();
    QApplication::clipboard()->setPixmap(finalPix);
    Notification::showMessage("Image copied to clipboard!", 2500);
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
        "ScreenCut Project (*.scut);;PNG Image (*.png);;JPEG Image (*.jpg)");

    if (!fileName.isEmpty()) {
        bool saved = false;
        if (fileName.endsWith(".scut", Qt::CaseInsensitive)) {
            QJsonArray annotations = m_canvas->saveAnnotationsJson();
            saved = ScutProject::saveImageAsScut(m_canvas->background(), fileName, annotations);
        } else {
            QPixmap finalPix = m_canvas->renderFinalPixmap();
            saved = finalPix.save(fileName);
        }
        if (saved) {
            m_openedFilePath = fileName;
            m_isTempFile = false;
            setWindowTitle(QString("ScreenCut Editor — %1").arg(QFileInfo(fileName).fileName()));
            Notification::showMessage(QString("Saved:\n%1").arg(QFileInfo(fileName).fileName()), 3000);
            if (m_thumbsStrip) m_thumbsStrip->refreshLibrary();
        }
    }
}

void EditorMainWindow::openFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open Project or Image", ScutProject::getLibraryDir(),
        "ScreenCut Files (*.scut *.png *.jpg *.jpeg *.bmp);;All Files (*.*)");
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
            if (ext == "scut" || ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp") {
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

void EditorMainWindow::autoFit() {
    if (!m_canvas || !m_scrollArea || m_canvas->background().isNull()) return;
    QSize viewSize = m_scrollArea->viewport()->size();
    QSize imageSize = m_canvas->background().size();
    if (imageSize.width() <= 0 || imageSize.height() <= 0) return;
    
    qreal zoomX = (qreal)(viewSize.width() - 40) / imageSize.width();
    qreal zoomY = (qreal)(viewSize.height() - 40) / imageSize.height();
    qreal z = qMin(zoomX, zoomY);
    if (z > 1.0) z = 1.0;
    m_canvas->setZoom(z);
}

} // namespace ScreenCut
