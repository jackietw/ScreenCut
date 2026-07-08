/**
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "capture_window.h"
#include "../core/capture_engine.h"
#include "../version.h"
#include <QApplication>
#include <QMessageBox>
#include <QTimer>
#include <QProcess>
#include <QDir>
#include <QStyle>
#include <QTabBar>
#include <QGraphicsDropShadowEffect>
#include "../resources/IconUtils.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif
#endif

namespace ScreenCut {

class AboutOverlayWidget : public QWidget {
public:
    explicit AboutOverlayWidget(QWidget* parent = nullptr) : QWidget(parent) {}
protected:
    void mousePressEvent(QMouseEvent* event) override {
        hide();
        deleteLater();
        event->accept();
    }
};

CaptureMainWindow* CaptureMainWindow::s_instance = nullptr;

CaptureMainWindow* CaptureMainWindow::instance() {
    if (!s_instance) {
        s_instance = new CaptureMainWindow();
    }
    return s_instance;
}

CaptureMainWindow::CaptureMainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    s_instance = this;
    setWindowTitle(QString("ScreenCut Capture v%1").arg(SCREENCUT_VERSION_STR));
    setWindowIcon(createSvgIcon(SVG_APP_ICON, 64, 64));
    resize(480, 300);
    setMinimumSize(480, 300);

    // Make window frameless & stay on top
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);

#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(winId());
    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
#endif

    setupStyleSheet();
    setupUi();
}

void CaptureMainWindow::setupStyleSheet() {
    setStyleSheet(R"(
        QMainWindow, #OuterContainer { background: transparent; border: none; }
        #CentralWidget { 
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                stop:0 rgba(42, 45, 52, 220), 
                stop:0.5 rgba(30, 32, 38, 225),
                stop:1 rgba(22, 24, 28, 235));
            border: 1px solid rgba(255, 255, 255, 14%);
            border-top: 1px solid rgba(255, 255, 255, 32%); /* macOS Liquid Glass Specular Highlight top border */
            border-radius: 14px;
        }
        QLabel { color: #f0f2f5; font-size: 13px; font-family: 'Segoe UI', '-apple-system', sans-serif; }
        QTabWidget, QTabWidget::pane { background: transparent; border: 0; }
        QTabWidget::tab-bar { alignment: left; left: 0px; }
        QTabBar { qproperty-expanding: 0; }
        QTabBar::tab { 
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 rgba(255, 255, 255, 0.06), stop:1 rgba(255, 255, 255, 0.02));
            color: #9095a0; 
            padding: 12px 26px; 
            font-size: 13px;
            font-weight: bold;
            border: none;
            border-bottom: 2px solid transparent;
        }
        QTabBar::tab:first {
            border-top-left-radius: 14px;
        }
        QTabBar::tab:hover {
            color: #d0d5e0;
            background: rgba(255, 255, 255, 0.08);
        }
        QTabBar::tab:selected { 
            color: #ffffff; 
            background: rgba(255, 255, 255, 0.12);
            border-bottom: 2px solid #007aff; /* macOS blue accent */
        }
        #CaptureButton { 
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #ff453a, stop:1 #d32f2f); /* Vibrant Apple Red gradient */
            color: white; 
            font-weight: bold; 
            font-size: 18px; 
            border-radius: 50px; /* Perfect circle for 100x100 */
            border: 1px solid rgba(255, 255, 255, 30%);
        }
        #CaptureButton:hover { 
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #ff5e54, stop:1 #e53935);
            border: 1px solid rgba(255, 255, 255, 50%);
        }
        #BottomBar {
            background: transparent;
            border: none;
        }
        #BottomBar QLabel {
            font-size: 12px;
            color: #a0a5b0;
        }
        #CloseButton {
            background: transparent;
            color: #aaaaaa;
            font-weight: bold;
            font-size: 16px;
            border: none;
            border-top-right-radius: 14px;
        }
        #CloseButton:hover {
            background: #ff453a;
            color: white;
        }
        #AboutButton {
            background: transparent;
            color: #aaaaaa;
            font-weight: bold;
            font-size: 16px;
            border: none;
        }
        #AboutButton:hover {
            background: rgba(255, 255, 255, 0.15);
            color: white;
        }
    )");
}

void CaptureMainWindow::setupUi() {
    QWidget* outerContainer = new QWidget(this);
    outerContainer->setObjectName("OuterContainer");
    setCentralWidget(outerContainer);

    QVBoxLayout* outerLayout = new QVBoxLayout(outerContainer);
    outerLayout->setContentsMargins(15, 15, 15, 15); // 15px margin for soft drop shadow
    outerLayout->setSpacing(0);

    QWidget* glassPanel = new QWidget(outerContainer);
    glassPanel->setObjectName("CentralWidget");

    auto* shadow = new QGraphicsDropShadowEffect(glassPanel);
    shadow->setBlurRadius(28);
    shadow->setColor(QColor(0, 0, 0, 160));
    shadow->setOffset(0, 8);
    glassPanel->setGraphicsEffect(shadow);

    outerLayout->addWidget(glassPanel);

    QVBoxLayout* mainLayout = new QVBoxLayout(glassPanel);
    mainLayout->setContentsMargins(1, 1, 1, 1);
    mainLayout->setSpacing(0);

    // Tabs at the top
    m_tabs = new QTabWidget(glassPanel);
    m_tabs->setObjectName("MainTabs");
    m_tabs->tabBar()->setExpanding(false);

    m_tabs->addTab(createImageTab(), createSvgIcon(SVG_TAB_IMAGE, 18, 18), "  Image");
    m_tabs->addTab(createVideoTab(), createSvgIcon(SVG_TAB_VIDEO, 18, 18), "  Video");
    m_tabs->setCurrentIndex(0);
    mainLayout->addWidget(m_tabs);

    // Bottom Bar
    QWidget* bottomBar = new QWidget(glassPanel);
    bottomBar->setObjectName("BottomBar");
    bottomBar->setFixedHeight(36);
    QHBoxLayout* bottomLayout = new QHBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(18, 0, 18, 0);

    m_btnPreferences = new QPushButton(" Preference", bottomBar);
    m_btnPreferences->setIcon(createSvgIcon(SVG_PREF, 16, 16));
    m_btnPreferences->setIconSize(QSize(16, 16));
    m_btnPreferences->setStyleSheet("color: #b0b5c0; font-size: 13px; background: transparent; border: none; text-align: left; font-weight: bold;");
    m_btnPreferences->setCursor(Qt::PointingHandCursor);
    connect(m_btnPreferences, &QPushButton::clicked, this, &CaptureMainWindow::onPreferencesClicked);

    m_btnEditor = new QPushButton(" Open Editor", bottomBar);
    m_btnEditor->setIcon(createSvgIcon(SVG_EDITOR, 16, 16));
    m_btnEditor->setIconSize(QSize(16, 16));
    m_btnEditor->setStyleSheet("color: #b0b5c0; font-size: 13px; background: transparent; border: none; text-align: left; font-weight: bold;");
    m_btnEditor->setCursor(Qt::PointingHandCursor);
    connect(m_btnEditor, &QPushButton::clicked, this, &CaptureMainWindow::onOpenEditorClicked);

    bottomLayout->addWidget(m_btnPreferences);
    bottomLayout->addStretch();
    bottomLayout->addWidget(m_btnEditor);
    mainLayout->addWidget(bottomBar);

    // Top right corner buttons inside tab bar (avoids any layout/overlap issues!)
    // Position close button exactly aligned with top-right border of glass panel (450 - 31 = 419)
    m_btnClose = new QPushButton("", glassPanel);
    m_btnClose->setIcon(createSvgIcon(SVG_CLOSE, 16, 16));
    m_btnClose->setIconSize(QSize(16, 16));
    m_btnClose->setObjectName("CloseButton");
    m_btnClose->setFixedSize(30, 30);
    m_btnClose->setCursor(Qt::PointingHandCursor);
    m_btnClose->move(450 - 31, 1);
    m_btnClose->raise();
    connect(m_btnClose, &QPushButton::clicked, this, &CaptureMainWindow::hide);

    // Position about button exactly to the left of close button (450 - 61 = 389)
    m_btnAbout = new QPushButton("", glassPanel);
    m_btnAbout->setIcon(createSvgIcon(SVG_ABOUT, 16, 16));
    m_btnAbout->setIconSize(QSize(16, 16));
    m_btnAbout->setObjectName("AboutButton");
    m_btnAbout->setFixedSize(30, 30);
    m_btnAbout->setCursor(Qt::PointingHandCursor);
    m_btnAbout->move(450 - 61, 1);
    m_btnAbout->raise();
    connect(m_btnAbout, &QPushButton::clicked, this, &CaptureMainWindow::onAboutClicked);

    // Connect tab change to toggle Editor button visibility
    connect(m_tabs, &QTabWidget::currentChanged, [this](int index) {
        m_btnEditor->setVisible(index == 0);
    });
}

void CaptureMainWindow::addSettingRow(QVBoxLayout* layout, const QString& labelText, bool isChecked) {
    QHBoxLayout* row = new QHBoxLayout();
    QLabel* lbl = new QLabel(labelText);
    SwitchWidget* toggle = new SwitchWidget();
    toggle->setChecked(isChecked);
    m_toggles[labelText] = toggle;

    row->addWidget(lbl);
    row->addStretch();

    QWidget* rightContainer = new QWidget();
    rightContainer->setFixedWidth(70);
    QHBoxLayout* rcLayout = new QHBoxLayout(rightContainer);
    rcLayout->setContentsMargins(0, 0, 0, 0);
    rcLayout->setSpacing(5);
    rcLayout->addWidget(toggle);

    QWidget* spacer = new QWidget();
    spacer->setFixedSize(20, 20);
    rcLayout->addWidget(spacer);

    row->addWidget(rightContainer);
    layout->addLayout(row);
}

QWidget* CaptureMainWindow::createImageTab() {
    QWidget* tab = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(tab);
    layout->setContentsMargins(20, 15, 20, 15);

    // Left side: Settings with Toggles
    QVBoxLayout* settingsLayout = new QVBoxLayout();
    settingsLayout->setSpacing(12);

    addSettingRow(settingsLayout, "Preview in Editor", false);
    addSettingRow(settingsLayout, "Copy to Clipboard", true);
    addSettingRow(settingsLayout, "Capture Cursor", false);
    addSettingRow(settingsLayout, "5 Second Delay", false);
    addSettingRow(settingsLayout, "Scroll Capture", false);

    settingsLayout->addStretch();
    layout->addLayout(settingsLayout, 2);

    // Right side: Capture Button
    QVBoxLayout* captureLayout = new QVBoxLayout();
    m_btnCapture = new QPushButton("Capture");
    m_btnCapture->setObjectName("CaptureButton");
    m_btnCapture->setFixedSize(100, 100);
    m_btnCapture->setCursor(Qt::PointingHandCursor);
    connect(m_btnCapture, &QPushButton::clicked, this, &CaptureMainWindow::onCaptureClicked);

    QLabel* lblHotkey = new QLabel("Ctrl+Shift+P");
    lblHotkey->setStyleSheet("color: #d8a040; font-size: 13px; font-weight: bold;");
    lblHotkey->setAlignment(Qt::AlignCenter);

    captureLayout->addStretch();
    captureLayout->addWidget(m_btnCapture, 0, Qt::AlignCenter);
    captureLayout->addSpacing(8);
    captureLayout->addWidget(lblHotkey, 0, Qt::AlignCenter);
    captureLayout->addStretch();

    layout->addLayout(captureLayout, 1);
    return tab;
}

QWidget* CaptureMainWindow::createVideoTab() {
    QWidget* tab = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(tab);
    layout->setContentsMargins(20, 15, 20, 15);

    // Left side: Settings with Toggles
    QVBoxLayout* settingsLayout = new QVBoxLayout();
    settingsLayout->setSpacing(15);

    addSettingRow(settingsLayout, "Capture Cursor", true);
    addSettingRow(settingsLayout, "Record Microphone", true);
    addSettingRow(settingsLayout, "Record System Audio", true);

    settingsLayout->addStretch();
    layout->addLayout(settingsLayout, 2);

    // Right side: Record Button
    QVBoxLayout* captureLayout = new QVBoxLayout();
    m_btnRecord = new QPushButton("Record");
    m_btnRecord->setObjectName("CaptureButton");
    m_btnRecord->setFixedSize(100, 100);
    m_btnRecord->setCursor(Qt::PointingHandCursor);
    connect(m_btnRecord, &QPushButton::clicked, this, &CaptureMainWindow::onRecordClicked);

    QLabel* lblHotkey = new QLabel("Ctrl+Shift+V");
    lblHotkey->setStyleSheet("color: #d8a040; font-size: 13px; font-weight: bold;");
    lblHotkey->setAlignment(Qt::AlignCenter);

    captureLayout->addStretch();
    captureLayout->addWidget(m_btnRecord, 0, Qt::AlignCenter);
    captureLayout->addSpacing(8);
    captureLayout->addWidget(lblHotkey, 0, Qt::AlignCenter);
    captureLayout->addStretch();

    layout->addLayout(captureLayout, 1);
    return tab;
}

void CaptureMainWindow::onCaptureClicked() {
    qDebug() << "[CaptureMainWindow] Capture button clicked. Hiding window...";
    hide();
    // Check toggle states
    bool scrollCapture = m_toggles.value("Scroll Capture") && m_toggles["Scroll Capture"]->isChecked();
    
    QTimer::singleShot(200, [scrollCapture]() {
        if (scrollCapture) {
            qDebug() << "[CaptureMainWindow] Triggering Scroll Capture mode.";
            CaptureEngine::instance()->startScrollingCapture();
        } else {
            qDebug() << "[CaptureMainWindow] Triggering Region Capture mode.";
            CaptureEngine::instance()->startRegionCapture();
        }
    });
}

void CaptureMainWindow::onRecordClicked() {
    qDebug() << "[CaptureMainWindow] Record button clicked.";
    QMessageBox::information(this, "ScreenCut Video", "🎥 Video Recording module will be connected in next step!");
}

void CaptureMainWindow::onOpenEditorClicked() {
    qDebug() << "[CaptureMainWindow] Open Editor clicked. Launching standalone studio...";
    hide();
    // Launch standalone ScreenCutEditor app without arguments
    QString editorPath = QDir(QCoreApplication::applicationDirPath()).filePath("ScreenCutEditor");
#ifdef Q_OS_WIN
    editorPath += ".exe";
#endif
    bool started = QProcess::startDetached(editorPath, QStringList());
    if (!started) {
        started = QProcess::startDetached("ScreenCutEditor", QStringList());
    }
    if (!started) {
        qCritical() << "[CaptureMainWindow] Failed to launch standalone ScreenCutEditor at path:" << editorPath;
        QMessageBox::critical(this, "ScreenCut Error", "Failed to launch standalone ScreenCutEditor application!");
    } else {
        qDebug() << "[CaptureMainWindow] Successfully launched ScreenCutEditor.";
    }
}

void CaptureMainWindow::onPreferencesClicked() {
    QMessageBox::information(this, "Preferences", "⚙️ Preferences dialog will be connected in next step!");
}

void CaptureMainWindow::onAboutClicked() {
    showAboutOverlay();
}

void CaptureMainWindow::showAboutOverlay() {
    if (m_aboutOverlay) {
        m_aboutOverlay->close();
        m_aboutOverlay->deleteLater();
        m_aboutOverlay = nullptr;
        return;
    }

    QWidget* target = m_tabs ? m_tabs->parentWidget() : centralWidget();
    auto* overlay = new AboutOverlayWidget(target);
    m_aboutOverlay = overlay;
    connect(overlay, &QObject::destroyed, this, [this]() {
        m_aboutOverlay = nullptr;
    });

    overlay->setObjectName("AboutOverlay");
    overlay->setGeometry(target->rect());
    overlay->setCursor(Qt::PointingHandCursor);
    overlay->setStyleSheet(R"(
        #AboutOverlay {
            background-color: rgba(18, 18, 20, 235);
            border-radius: 14px;
        }
        QLabel {
            background: transparent;
            border: none;
        }
    )");

    QVBoxLayout* layout = new QVBoxLayout(overlay);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(8);

    QLabel* title = new QLabel("ScreenCut");
    title->setStyleSheet("color: #f0f2f5; font-size: 24px; font-weight: bold; letter-spacing: 1px;");

    QLabel* subtitle = new QLabel("A simple and fast screen capture tool.");
    subtitle->setStyleSheet("color: #c0c5d0; font-size: 14px;");

    QLabel* info = new QLabel(QString("Version: v%1 • LGPL-2.0 License").arg(SCREENCUT_VERSION_STR));
    info->setStyleSheet("color: #8890a0; font-size: 12px; margin-top: 5px;");

    QLabel* tip = new QLabel("(Click anywhere to close)");
    tip->setStyleSheet("color: #606875; font-size: 11px; font-style: italic; margin-top: 15px;");

    layout->addWidget(title, 0, Qt::AlignCenter);
    layout->addWidget(subtitle, 0, Qt::AlignCenter);
    layout->addWidget(info, 0, Qt::AlignCenter);
    layout->addWidget(tip, 0, Qt::AlignCenter);

    overlay->show();
    overlay->raise();
}

void CaptureMainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    if (m_btnClose && m_btnAbout && m_btnClose->parentWidget()) {
        int pw = m_btnClose->parentWidget()->width();
        if (pw > 100) {
            m_btnClose->move(pw - 31, 1);
            m_btnAbout->move(pw - 61, 1);
        }
    }
    if (m_aboutOverlay && m_aboutOverlay->parentWidget()) {
        m_aboutOverlay->setGeometry(m_aboutOverlay->parentWidget()->rect());
    }
}

// --- Window Dragging Logic ---
void CaptureMainWindow::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
        m_isDragging = true;
        event->accept();
    }
}

void CaptureMainWindow::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton && m_isDragging) {
        move(event->globalPosition().toPoint() - m_dragPos);
        event->accept();
    }
}

void CaptureMainWindow::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
        event->accept();
    }
}

} // namespace ScreenCut
