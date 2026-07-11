/**
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "capture_window.h"
#include "../core/capture_engine.h"
#include "../core/common_project.h"
#include "../config.h"
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
#include <QDesktopServices>
#include <QUrl>
#include <QComboBox>
#include <QPointer>
#include <QScrollArea>
#include <QMediaDevices>
#include <QAudioDevice>

#include "../platform/platform.h"

namespace ScreenCut {

class PreferencesOverlayWidget : public QWidget {
public:
    QWidget* dialogBox = nullptr;
    explicit PreferencesOverlayWidget(QWidget* parent = nullptr) : QWidget(parent) {}
protected:
    void resizeEvent(QResizeEvent* event) override {
        QWidget::resizeEvent(event);
        if (dialogBox) {
            dialogBox->setFixedSize(size());
        }
    }
    void mousePressEvent(QMouseEvent* event) override {
        if (dialogBox && !dialogBox->geometry().contains(event->pos())) {
            hide();
            deleteLater();
            event->accept();
        } else {
            QWidget::mousePressEvent(event);
        }
    }
};

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

    Platform::setDarkTitlebar(winId());

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

void CaptureMainWindow::addSettingRow(QVBoxLayout* layout, const QString& labelText, bool isChecked, const QString& configKey) {
    QString key = configKey.isEmpty() ? labelText : configKey;
    QHBoxLayout* row = new QHBoxLayout();
    QLabel* lbl = new QLabel(labelText);
    SwitchWidget* toggle = new SwitchWidget();
    
    bool savedVal = Config::getValue(key, isChecked).toBool();
    toggle->setChecked(savedVal);
    m_toggles[key] = toggle;

    connect(toggle, &SwitchWidget::toggled, this, [key](bool checked) {
        Config::setValue(key, checked);
        qDebug() << "[CaptureMainWindow] Saved setting:" << key << "=" << checked;
    });

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

bool CaptureMainWindow::isSettingEnabled(const QString& key) const {
    if (m_toggles.contains(key)) {
        return m_toggles.value(key)->isChecked();
    }
    bool defaultVal = (key == "Video_Capture Cursor" || key == "Record Microphone" || key == "Record System Audio");
    return Config::getValue(key, defaultVal).toBool();
}

void CaptureMainWindow::setSettingEnabled(const QString& key, bool enabled) {
    if (m_toggles.contains(key)) {
        m_toggles.value(key)->setChecked(enabled);
    }
    Config::setValue(key, enabled);
}

QWidget* CaptureMainWindow::createImageTab() {
    QWidget* tab = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(tab);
    layout->setContentsMargins(20, 15, 20, 15);

    // Left side: Settings with Toggles
    QVBoxLayout* settingsLayout = new QVBoxLayout();
    settingsLayout->setSpacing(12);

    addSettingRow(settingsLayout, "Preview in Editor", false, "Preview in Editor");
    addSettingRow(settingsLayout, "Copy to Clipboard", true, "Copy to Clipboard");
    addSettingRow(settingsLayout, "Capture Cursor", false, "Image_Capture Cursor");
    addSettingRow(settingsLayout, "5 Second Delay", false, "5 Second Delay");
    addSettingRow(settingsLayout, "Scroll Capture", false, "Scroll Capture");

    settingsLayout->addStretch();
    layout->addLayout(settingsLayout, 2);

    // Right side: Capture Button
    QVBoxLayout* captureLayout = new QVBoxLayout();
    m_btnCapture = new QPushButton("Capture");
    m_btnCapture->setObjectName("CaptureButton");
    m_btnCapture->setFixedSize(100, 100);
    m_btnCapture->setCursor(Qt::PointingHandCursor);
    connect(m_btnCapture, &QPushButton::clicked, this, &CaptureMainWindow::onCaptureClicked);

    QLabel* lblHotkey = new QLabel("Ctrl+Shift+A");
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

    addSettingRow(settingsLayout, "Capture Cursor", true, "Video_Capture Cursor");
    addSettingRow(settingsLayout, "Record Microphone", true, "Record Microphone");
    addSettingRow(settingsLayout, "Record System Audio", true, "Record System Audio");

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
    qDebug() << "[CaptureMainWindow] Capture button clicked.";
    if (!Platform::checkAndRequestScreenCapturePermission(true, true)) {
        qWarning() << "[CaptureMainWindow] Screen capture permission not granted on macOS. Aborting capture start.";
        return;
    }
    qDebug() << "[CaptureMainWindow] Hiding window before capture...";
    hide();
    bool scrollCapture = isSettingEnabled("Scroll Capture");
    bool delay5s = isSettingEnabled("5 Second Delay");
    int hideDelay = delay5s ? 0 : 250;
    
    QTimer::singleShot(hideDelay, [scrollCapture]() {
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
    if (!Platform::checkAndRequestScreenCapturePermission(true, true)) {
        qWarning() << "[CaptureMainWindow] Screen capture permission not granted on macOS. Aborting video record start.";
        return;
    }
    qDebug() << "[CaptureMainWindow] Hiding window before video capture...";
    hide();
    int hideDelay = 250;
    QTimer::singleShot(hideDelay, []() {
        CaptureEngine::instance()->startVideoRegionCapture();
    });
}

void CaptureMainWindow::onOpenEditorClicked() {
    qDebug() << "[CaptureMainWindow] Open Editor clicked. Launching standalone studio...";
    hide();
    bool started = ScreenCut::ScutProject::launchEditor();
    if (!started) {
        qCritical() << "[CaptureMainWindow] Failed to launch standalone SCEditor.";
        QMessageBox::critical(this, "ScreenCut Error", "Failed to launch standalone SCEditor application!");
    } else {
        qDebug() << "[CaptureMainWindow] Successfully launched SCEditor.";
    }
}

void CaptureMainWindow::onPreferencesClicked() {
    showPreferencesOverlay();
}

void CaptureMainWindow::onAboutClicked() {
    showAboutOverlay();
}

void CaptureMainWindow::showPreferencesOverlay() {
    if (m_prefOverlay) {
        m_prefOverlay->close();
        m_prefOverlay->deleteLater();
        m_prefOverlay = nullptr;
        return;
    }

    QWidget* target = m_tabs ? m_tabs->parentWidget() : centralWidget();
    auto* overlay = new PreferencesOverlayWidget(target);
    m_prefOverlay = overlay;
    connect(overlay, &QObject::destroyed, this, [this]() {
        m_prefOverlay = nullptr;
    });

    overlay->setObjectName("PreferencesOverlay");
    overlay->setGeometry(target->rect());
    overlay->setStyleSheet(R"(
        #PreferencesOverlay {
            background-color: rgb(20, 22, 28);
            border: 1px solid rgba(255, 255, 255, 14%);
            border-top: 1px solid rgba(255, 255, 255, 32%);
            border-radius: 14px;
        }
        #PrefDialogBox {
            background: transparent;
            border: none;
        }
        QLabel {
            background: transparent;
            border: none;
            color: #e0e5f0;
        }
        QLabel:disabled {
            color: #555b68;
        }
    )");

    QVBoxLayout* mainLayout = new QVBoxLayout(overlay);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QWidget* dialogBox = new QWidget(overlay);
    dialogBox->setObjectName("PrefDialogBox");
    dialogBox->setFixedSize(target->size());
    overlay->dialogBox = dialogBox;

    QVBoxLayout* boxLayout = new QVBoxLayout(dialogBox);
    boxLayout->setContentsMargins(24, 18, 24, 18);
    boxLayout->setSpacing(10);

    // Header row (`⚙️ Preferences & Settings` on left, `✔ DONE` on top right)
    QHBoxLayout* headerRow = new QHBoxLayout();
    QLabel* title = new QLabel("⚙️ Preferences & Settings");
    title->setStyleSheet("font-size: 14px; font-weight: bold; color: #f0f2f5; letter-spacing: 0.5px;");

    QPushButton* btnDone = new QPushButton("✔ DONE", dialogBox);
    btnDone->setCursor(Qt::PointingHandCursor);
    btnDone->setFixedSize(76, 24);
    btnDone->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #007aff, stop:1 #0062cc);
            color: #ffffff;
            font-size: 11px;
            font-weight: bold;
            border-radius: 5px;
            border: none;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #1a8cff, stop:1 #007aff);
        }
        QPushButton:pressed {
            background: #0052ad;
        }
    )");
    connect(btnDone, &QPushButton::clicked, overlay, [overlay]() {
        overlay->hide();
        overlay->deleteLater();
    });

    headerRow->addWidget(title);
    headerRow->addStretch();
    headerRow->addWidget(btnDone);
    boxLayout->addLayout(headerRow);

    // Divider
    QWidget* line = new QWidget(dialogBox);
    line->setFixedHeight(1);
    line->setStyleSheet("background-color: #363a45;");
    boxLayout->addWidget(line);

    // ScrollArea to gracefully handle any future options that exceed UI height
    QScrollArea* scrollArea = new QScrollArea(dialogBox);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(R"(
        QScrollArea { background: transparent; border: none; }
        QScrollArea > QWidget > QWidget { background: transparent; }
        QScrollBar:vertical {
            background: rgba(0, 0, 0, 0.15);
            width: 6px;
            margin: 0px;
            border-radius: 3px;
        }
        QScrollBar::handle:vertical {
            background: rgba(255, 255, 255, 0.25);
            min-height: 20px;
            border-radius: 3px;
        }
        QScrollBar::handle:vertical:hover {
            background: rgba(255, 255, 255, 0.45);
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
    )");

    QWidget* scrollContent = new QWidget(scrollArea);
    scrollContent->setObjectName("ScrollContent");
    scrollContent->setStyleSheet("background: transparent;");
    QVBoxLayout* settingsLayout = new QVBoxLayout(scrollContent);
    settingsLayout->setContentsMargins(0, 4, 6, 4);
    settingsLayout->setSpacing(12);

    // 1. Debug checkbox/switch row
    QHBoxLayout* debugRow = new QHBoxLayout();
    QVBoxLayout* debugTxtLayout = new QVBoxLayout();
    debugTxtLayout->setSpacing(2);
    QLabel* lblDebug = new QLabel("Debug Mode (JSON setting)");
    lblDebug->setStyleSheet("font-size: 12px; font-weight: bold; color: #e0e5f0;");
    QLabel* lblDebugDesc = new QLabel("Write detailed debug logs (QtDebugMsg) to screencut.log.");
    lblDebugDesc->setStyleSheet("font-size: 10px; color: #8890a0;");
    debugTxtLayout->addWidget(lblDebug);
    debugTxtLayout->addWidget(lblDebugDesc);

    SwitchWidget* toggleDebug = new SwitchWidget(scrollContent);
    bool isDebugChecked = Config::getValue("debug_mode", false).toBool();
    toggleDebug->setChecked(isDebugChecked);
    m_toggles["debug_mode"] = toggleDebug;

    connect(toggleDebug, &SwitchWidget::toggled, this, [](bool checked) {
        Config::setDebugMode(checked);
        qDebug() << "[Preferences] Debug mode (config.json) updated:" << checked;
    });

    debugRow->addLayout(debugTxtLayout);
    debugRow->addStretch();
    debugRow->addWidget(toggleDebug);
    settingsLayout->addLayout(debugRow);

    // 2. Record Limit 1080p switch row
    QHBoxLayout* limitRow = new QHBoxLayout();
    QVBoxLayout* limitTxtLayout = new QVBoxLayout();
    limitTxtLayout->setSpacing(2);
    QLabel* lblLimit = new QLabel("Record Limit 1080p");
    lblLimit->setStyleSheet("font-size: 12px; font-weight: bold; color: #e0e5f0;");
    QLabel* lblLimitDesc = new QLabel("Downscale resolutions over 1920x1080 to 1080p to save CPU/GPU.");
    lblLimitDesc->setStyleSheet("font-size: 10px; color: #8890a0;");
    limitTxtLayout->addWidget(lblLimit);
    limitTxtLayout->addWidget(lblLimitDesc);

    SwitchWidget* toggleLimit = new SwitchWidget(scrollContent);
    bool isLimitChecked = Config::getValue("limit_1080p", false).toBool();
    toggleLimit->setChecked(isLimitChecked);
    m_toggles["limit_1080p"] = toggleLimit;

    connect(toggleLimit, &SwitchWidget::toggled, this, [](bool checked) {
        Config::setValue("limit_1080p", checked);
        qDebug() << "[Preferences] Record Limit 1080p updated:" << checked;
    });

    limitRow->addLayout(limitTxtLayout);
    limitRow->addStretch();
    limitRow->addWidget(toggleLimit);
    settingsLayout->addLayout(limitRow);

    // 3. Hardware Video Acceleration (MP4) switch
    QHBoxLayout* hwRow = new QHBoxLayout();
    QVBoxLayout* hwTxtLayout = new QVBoxLayout();
    hwTxtLayout->setSpacing(2);
    QLabel* lblHw = new QLabel("Hardware Video Acceleration (MP4)");
    lblHw->setStyleSheet("font-size: 12px; font-weight: bold; color: #e0e5f0;");
    QLabel* lblHwDesc = new QLabel("Use GPU acceleration for MP4. Uncheck for software compression.");
    lblHwDesc->setStyleSheet("font-size: 10px; color: #8890a0;");
    hwTxtLayout->addWidget(lblHw);
    hwTxtLayout->addWidget(lblHwDesc);

    SwitchWidget* toggleHw = new SwitchWidget(scrollContent);
    bool isHwChecked = Config::getValue("hw_accel", true).toBool();
    toggleHw->setChecked(isHwChecked);
    m_toggles["hw_accel"] = toggleHw;

    hwRow->addLayout(hwTxtLayout);
    hwRow->addStretch();
    hwRow->addWidget(toggleHw);
    settingsLayout->addLayout(hwRow);

    // 4. Hardware Encoder choice dropdown
    QHBoxLayout* encRow = new QHBoxLayout();
    QLabel* lblEnc = new QLabel("Encoder Choice:");
    lblEnc->setStyleSheet("font-size: 11px; font-weight: bold;");

    QComboBox* comboEnc = new QComboBox(scrollContent);
    comboEnc->setCursor(Qt::PointingHandCursor);
    comboEnc->setStyleSheet(R"(
        QComboBox {
            background-color: #252830;
            color: #e0e5f0;
            border: 1px solid #3d4454;
            border-radius: 5px;
            padding: 2px 8px;
            font-size: 11px;
            min-width: 200px;
            max-height: 24px;
        }
        QComboBox:hover {
            border-color: #00a8ff;
        }
        QComboBox:disabled {
            background-color: #181a20;
            color: #555b68;
            border: 1px solid #282c36;
        }
        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
        QComboBox QAbstractItemView {
            background-color: #1e2128;
            color: #e0e5f0;
            border: 1px solid #3d4454;
            selection-background-color: #00a8ff;
            selection-color: #ffffff;
        }
    )");

    auto updateHwAvailability = [toggleHw, lblHw, lblHwDesc, comboEnc, lblEnc](const QList<HwEncoderInfo>& list) {
        if (!toggleHw || !comboEnc) return;
        if (list.isEmpty()) {
            toggleHw->setChecked(false);
            toggleHw->setEnabled(false);
            if (lblHw) lblHw->setEnabled(false);
            if (lblHwDesc) lblHwDesc->setEnabled(false);
            Config::setValue("hw_accel", false);

            comboEnc->blockSignals(true);
            comboEnc->clear();
            comboEnc->addItem("找不到支援的 GPU 硬體壓縮 (停用)", "");
            comboEnc->setEnabled(false);
            if (lblEnc) lblEnc->setEnabled(false);
            comboEnc->blockSignals(false);
        } else {
            toggleHw->setEnabled(true);
            if (lblHw) lblHw->setEnabled(true);
            if (lblHwDesc) lblHwDesc->setEnabled(true);

            QString savedEnc = Config::getValue("hw_encoder", "").toString();
            comboEnc->blockSignals(true);
            comboEnc->clear();
            comboEnc->addItem("Auto / Best Available (自動偵測推薦)", "");
            for (const auto& hw : list) {
                if (hw.codec == "libx264") continue;
                comboEnc->addItem(hw.displayName, hw.codec);
            }
            int idx = comboEnc->findData(savedEnc);
            if (idx >= 0) comboEnc->setCurrentIndex(idx);
            comboEnc->blockSignals(false);

            bool isChecked = toggleHw->isChecked();
            comboEnc->setEnabled(isChecked);
            if (lblEnc) lblEnc->setEnabled(isChecked);
        }
    };

    if (VideoRecorder::isHwDetected()) {
        updateHwAvailability(VideoRecorder::detectAvailableHwEncoders());
    } else {
        comboEnc->addItem("Auto / Best Available (自動偵測推薦)", "");
        comboEnc->addItem("🔍 正在後台偵測 GPU 加速硬體...", "detecting");
        QPointer<SwitchWidget> safeToggle = toggleHw;
        QPointer<QLabel> safeLblHw = lblHw;
        QPointer<QLabel> safeLblDesc = lblHwDesc;
        QPointer<QComboBox> safeCombo = comboEnc;
        QPointer<QLabel> safeLblEnc = lblEnc;
        VideoRecorder::detectAvailableHwEncodersAsync([safeToggle, safeLblHw, safeLblDesc, safeCombo, safeLblEnc](const QList<HwEncoderInfo>& list) {
            if (!safeToggle || !safeCombo) return;
            if (list.isEmpty()) {
                safeToggle->setChecked(false);
                safeToggle->setEnabled(false);
                if (safeLblHw) safeLblHw->setEnabled(false);
                if (safeLblDesc) safeLblDesc->setEnabled(false);
                Config::setValue("hw_accel", false);

                safeCombo->blockSignals(true);
                safeCombo->clear();
                safeCombo->addItem("找不到支援的 GPU 硬體壓縮 (停用)", "");
                safeCombo->setEnabled(false);
                if (safeLblEnc) safeLblEnc->setEnabled(false);
                safeCombo->blockSignals(false);
            } else {
                safeToggle->setEnabled(true);
                if (safeLblHw) safeLblHw->setEnabled(true);
                if (safeLblDesc) safeLblDesc->setEnabled(true);

                QString savedEnc = Config::getValue("hw_encoder", "").toString();
                safeCombo->blockSignals(true);
                safeCombo->clear();
                safeCombo->addItem("Auto / Best Available (自動偵測推薦)", "");
                for (const auto& hw : list) {
                    if (hw.codec == "libx264") continue;
                    safeCombo->addItem(hw.displayName, hw.codec);
                }
                int idx = safeCombo->findData(savedEnc);
                if (idx >= 0) safeCombo->setCurrentIndex(idx);
                safeCombo->blockSignals(false);

                bool isChecked = safeToggle->isChecked();
                safeCombo->setEnabled(isChecked);
                if (safeLblEnc) safeLblEnc->setEnabled(isChecked);
            }
        });
    }

    connect(toggleHw, &SwitchWidget::toggled, this, [comboEnc, lblEnc](bool checked) {
        Config::setValue("hw_accel", checked);
        comboEnc->setEnabled(checked);
        lblEnc->setEnabled(checked);
        qDebug() << "[Preferences] Hardware acceleration updated:" << checked;
    });

    connect(comboEnc, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [comboEnc](int) {
        Config::setValue("hw_encoder", comboEnc->currentData().toString());
        qDebug() << "[Preferences] Hardware encoder choice updated:" << comboEnc->currentData().toString();
    });

    encRow->addWidget(lblEnc);
    encRow->addSpacing(8);
    encRow->addWidget(comboEnc, 1);
    settingsLayout->addLayout(encRow);

    // 5. Microphone Source dropdown
    QHBoxLayout* micRow = new QHBoxLayout();
    QLabel* lblMic = new QLabel("Microphone Source:");
    lblMic->setStyleSheet("font-size: 11px; font-weight: bold;");

    QComboBox* comboMic = new QComboBox(scrollContent);
    comboMic->setCursor(Qt::PointingHandCursor);
    comboMic->setStyleSheet(comboEnc->styleSheet()); // reuse same dark style

    // Populate from Qt Multimedia audio input device list
    comboMic->addItem("Default System Microphone", QString());
    {
        const auto devices = QMediaDevices::audioInputs();
        for (const QAudioDevice& dev : devices) {
            comboMic->addItem(dev.description(), QString::fromUtf8(dev.id()));
        }
    }
    {
        QString savedId = Config::getValue("mic_device", QString()).toString();
        int idx = comboMic->findData(savedId);
        if (idx >= 0) comboMic->setCurrentIndex(idx);
    }
    connect(comboMic, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [comboMic](int) {
        QString devId = comboMic->currentData().toString();
        Config::setValue("mic_device", devId);
        qDebug() << "[Preferences] Microphone source updated:" << comboMic->currentText() << "id:" << devId;
    });

    micRow->addWidget(lblMic);
    micRow->addSpacing(8);
    micRow->addWidget(comboMic, 1);
    settingsLayout->addLayout(micRow);
    settingsLayout->addStretch();

    scrollArea->setWidget(scrollContent);
    boxLayout->addWidget(scrollArea, 1);

    mainLayout->addWidget(dialogBox);

    overlay->show();
    overlay->raise();
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

    QLabel* info = new QLabel(QString("Version: v%1 • LGPL-2.1 License").arg(SCREENCUT_VERSION_STR));
    info->setStyleSheet("color: #8890a0; font-size: 12px; margin-top: 5px;");

    QLabel* qtInfo = new QLabel("Powered by Qt 6 (LGPL v3.0)");
    qtInfo->setStyleSheet("color: #707888; font-size: 11px; margin-top: 2px;");

    QLabel* tip = new QLabel("(Click anywhere to close)");
    tip->setStyleSheet("color: #606875; font-size: 11px; font-style: italic; margin-top: 15px;");

    layout->addWidget(title, 0, Qt::AlignCenter);
    layout->addWidget(subtitle, 0, Qt::AlignCenter);
    layout->addWidget(info, 0, Qt::AlignCenter);
    layout->addWidget(qtInfo, 0, Qt::AlignCenter);
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
    if (m_prefOverlay && m_prefOverlay->parentWidget()) {
        m_prefOverlay->setGeometry(m_prefOverlay->parentWidget()->rect());
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
