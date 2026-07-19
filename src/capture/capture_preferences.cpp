/**
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "capture_preferences.h"
#include "capture_window.h"
#include "../config.h"
#include "../core/capture_engine.h"
#include "../core/common_project.h"
#include "../widgets/capture_status.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QPointer>
#include <QDebug>

namespace ScreenCut {

CapturePreferencesDialog* CapturePreferencesDialog::s_instance = nullptr;

CapturePreferencesDialog* CapturePreferencesDialog::instance(QWidget* parent) {
    if (!s_instance) {
        s_instance = new CapturePreferencesDialog(parent);
    } else if (parent && s_instance->parent() != parent) {
        s_instance->setParent(parent, Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint | Qt::WindowStaysOnTopHint);
    }
    return s_instance;
}

CapturePreferencesDialog::CapturePreferencesDialog(QWidget* parent)
    : QDialog(parent, Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint | Qt::WindowStaysOnTopHint) {
    setWindowTitle("Preferences & Settings (偏好設定)");
    setModal(true);
    setWindowModality(Qt::ApplicationModal);
    resize(620, 460);
    setMinimumSize(540, 400);

    setupUi();
    setupStyleSheet();
}

void CapturePreferencesDialog::closeEvent(QCloseEvent* event) {
    QDialog::closeEvent(event);
}

void CapturePreferencesDialog::setupStyleSheet() {
    setStyleSheet(R"(
        CapturePreferencesDialog {
            background-color: #14161c;
            color: #e0e5f0;
        }
        QListWidget {
            background-color: #1e2128;
            border: none;
            border-right: 1px solid #2e323e;
            outline: none;
            padding: 10px 4px;
            font-size: 13px;
        }
        QListWidget::item {
            padding: 10px 12px;
            border-radius: 6px;
            color: #a0a6b4;
            margin-bottom: 4px;
        }
        QListWidget::item:hover {
            background-color: #282c36;
            color: #e0e5f0;
        }
        QListWidget::item:selected {
            background-color: #00a8ff;
            color: #ffffff;
            font-weight: bold;
        }
        QScrollArea {
            background: transparent;
            border: none;
        }
        QScrollArea > QWidget > QWidget {
            background: transparent;
        }
        QLabel {
            background: transparent;
            border: none;
            color: #e0e5f0;
        }
        QLabel:disabled {
            color: #555b68;
        }
        QComboBox {
            background-color: #252830;
            color: #e0e5f0;
            border: 1px solid #3d4454;
            border-radius: 5px;
            padding: 4px 8px;
            font-size: 12px;
            min-width: 200px;
            max-height: 28px;
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
            width: 24px;
        }
        QComboBox QAbstractItemView {
            background-color: #1e2128;
            color: #e0e5f0;
            border: 1px solid #3d4454;
            selection-background-color: #00a8ff;
            selection-color: #ffffff;
        }
        QPushButton#BtnDone {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #007aff, stop:1 #0062cc);
            color: #ffffff;
            font-size: 13px;
            font-weight: bold;
            border-radius: 6px;
            border: none;
            padding: 6px 20px;
        }
        QPushButton#BtnDone:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #1a8cff, stop:1 #007aff);
        }
        QPushButton#BtnDone:pressed {
            background: #0052ad;
        }
    )");
}

void CapturePreferencesDialog::setupUi() {
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Left Sidebar
    m_sidebar = new QListWidget(this);
    m_sidebar->setFixedWidth(160);
    m_sidebar->addItem("⚙️ General");
    m_sidebar->addItem("🎬 Video & Audio");
    m_sidebar->addItem("🖱️ Cursor Effects");
    mainLayout->addWidget(m_sidebar);

    // Right Content Box
    QVBoxLayout* rightLayout = new QVBoxLayout();
    rightLayout->setContentsMargins(20, 16, 20, 16);
    rightLayout->setSpacing(12);

    m_pages = new QStackedWidget(this);
    m_pages->addWidget(createGeneralPage());
    m_pages->addWidget(createVideoAudioPage());
    m_pages->addWidget(createCursorPage());
    rightLayout->addWidget(m_pages, 1);

    // Bottom Done Row
    QWidget* bottomContainer = new QWidget(this);
    QHBoxLayout* bottomLayout = new QHBoxLayout(bottomContainer);
    bottomLayout->setContentsMargins(0, 8, 0, 0);
    bottomLayout->addStretch();

    QPushButton* btnDone = new QPushButton("✔ Close / Done", bottomContainer);
    btnDone->setObjectName("BtnDone");
    btnDone->setCursor(Qt::PointingHandCursor);
    connect(btnDone, &QPushButton::clicked, this, &CapturePreferencesDialog::accept);
    bottomLayout->addWidget(btnDone);

    rightLayout->addWidget(bottomContainer);
    mainLayout->addLayout(rightLayout, 1);

    connect(m_sidebar, &QListWidget::currentRowChanged, m_pages, &QStackedWidget::setCurrentIndex);
    m_sidebar->setCurrentRow(0);
}

static QHBoxLayout* createSettingRow(QWidget* parent, SwitchWidget*& toggleOut, const QString& title, const QString& desc, bool checkedVal, std::function<void(bool)> onToggle) {
    QHBoxLayout* row = new QHBoxLayout();
    QVBoxLayout* txtLayout = new QVBoxLayout();
    txtLayout->setSpacing(2);

    QLabel* lblTitle = new QLabel(title, parent);
    lblTitle->setStyleSheet("font-size: 13px; font-weight: bold; color: #e0e5f0;");
    txtLayout->addWidget(lblTitle);

    if (!desc.isEmpty()) {
        QLabel* lblDesc = new QLabel(desc, parent);
        lblDesc->setStyleSheet("font-size: 11px; color: #8890a0;");
        txtLayout->addWidget(lblDesc);
    }

    SwitchWidget* toggle = new SwitchWidget(parent);
    toggle->setChecked(checkedVal);
    toggleOut = toggle;

    QObject::connect(toggle, &SwitchWidget::toggled, toggle, [onToggle](bool checked) {
        onToggle(checked);
    });

    row->addLayout(txtLayout);
    row->addStretch();
    row->addWidget(toggle);
    return row;
}

QWidget* CapturePreferencesDialog::createGeneralPage() {
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget* page = new QWidget(scrollArea);
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 10, 0);
    layout->setSpacing(16);

    QLabel* header = new QLabel("General Preferences (一般設定)", page);
    header->setStyleSheet("font-size: 15px; font-weight: bold; color: #00a8ff; padding-bottom: 4px; border-bottom: 1px solid #2e323e;");
    layout->addWidget(header);

    SwitchWidget* toggleDebug = nullptr;
    layout->addLayout(createSettingRow(page, toggleDebug, "Debug Mode (JSON setting)", "Write detailed debug logs (QtDebugMsg) to screencut.log.",
        Config::getValue("debug_mode", false).toBool(), [](bool checked) {
            Config::setDebugMode(checked);
            qDebug() << "[CapturePreferencesDialog] Debug mode updated:" << checked;
        }));
    m_toggles["debug_mode"] = toggleDebug;

    SwitchWidget* toggleEditor = nullptr;
    layout->addLayout(createSettingRow(page, toggleEditor, "Preview in Editor", "Automatically open image captures in the built-in SCEditor.",
        Config::getValue("Preview in Editor", false).toBool(), [](bool checked) {
            Config::setValue("Preview in Editor", checked);
            if (CaptureMainWindow::instance()) CaptureMainWindow::instance()->setSettingEnabled("Preview in Editor", checked);
            if (CaptureEngine::instance()) CaptureEngine::instance()->setSessionEditorEnabled(checked);
        }));
    m_toggles["Preview in Editor"] = toggleEditor;

    SwitchWidget* toggleClip = nullptr;
    layout->addLayout(createSettingRow(page, toggleClip, "Copy to Clipboard", "Automatically copy captured images or recorded file paths to clipboard.",
        Config::getValue("Copy to Clipboard", true).toBool(), [](bool checked) {
            Config::setValue("Copy to Clipboard", checked);
            if (CaptureMainWindow::instance()) CaptureMainWindow::instance()->setSettingEnabled("Copy to Clipboard", checked);
            if (CaptureEngine::instance()) CaptureEngine::instance()->setSessionClipboardEnabled(checked);
        }));
    m_toggles["Copy to Clipboard"] = toggleClip;

    SwitchWidget* toggleDelay = nullptr;
    layout->addLayout(createSettingRow(page, toggleDelay, "5 Second Delay", "Wait 5 seconds countdown before starting screen or region capture.",
        Config::getValue("5 Second Delay", false).toBool(), [](bool checked) {
            Config::setValue("5 Second Delay", checked);
            if (CaptureMainWindow::instance()) CaptureMainWindow::instance()->setSettingEnabled("5 Second Delay", checked);
        }));
    m_toggles["5 Second Delay"] = toggleDelay;

    SwitchWidget* toggleScroll = nullptr;
    layout->addLayout(createSettingRow(page, toggleScroll, "Scroll Capture", "Enable scrolling capture feature by default on region selection.",
        Config::getValue("Scroll Capture", false).toBool(), [](bool checked) {
            Config::setValue("Scroll Capture", checked);
            if (CaptureMainWindow::instance()) CaptureMainWindow::instance()->setSettingEnabled("Scroll Capture", checked);
        }));
    m_toggles["Scroll Capture"] = toggleScroll;

    layout->addStretch();
    scrollArea->setWidget(page);
    return scrollArea;
}

QWidget* CapturePreferencesDialog::createVideoAudioPage() {
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget* page = new QWidget(scrollArea);
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 10, 0);
    layout->setSpacing(16);

    QLabel* header = new QLabel("Video & Audio Settings (錄製與音訊)", page);
    header->setStyleSheet("font-size: 15px; font-weight: bold; color: #00a8ff; padding-bottom: 4px; border-bottom: 1px solid #2e323e;");
    layout->addWidget(header);

    SwitchWidget* toggleLimit = nullptr;
    layout->addLayout(createSettingRow(page, toggleLimit, "Record Limit 1080p", "Downscale resolutions over 1920x1080 to 1080p to save CPU/GPU.",
        Config::getValue("limit_1080p", false).toBool(), [](bool checked) {
            Config::setValue("limit_1080p", checked);
            qDebug() << "[CapturePreferencesDialog] Record Limit 1080p updated:" << checked;
        }));
    m_toggles["limit_1080p"] = toggleLimit;

    // Hardware Acceleration Switch
    QHBoxLayout* hwRow = new QHBoxLayout();
    QVBoxLayout* hwTxtLayout = new QVBoxLayout();
    hwTxtLayout->setSpacing(2);
    m_lblHw = new QLabel("Hardware Video Acceleration (MP4)", page);
    m_lblHw->setStyleSheet("font-size: 13px; font-weight: bold; color: #e0e5f0;");
    m_lblHwDesc = new QLabel("Use GPU acceleration for MP4. Uncheck for software compression.", page);
    m_lblHwDesc->setStyleSheet("font-size: 11px; color: #8890a0;");
    hwTxtLayout->addWidget(m_lblHw);
    hwTxtLayout->addWidget(m_lblHwDesc);

    m_toggleHw = new SwitchWidget(page);
    m_toggleHw->setChecked(Config::getValue("hw_accel", true).toBool());
    m_toggles["hw_accel"] = m_toggleHw;

    connect(m_toggleHw, &SwitchWidget::toggled, this, [this](bool checked) {
        Config::setValue("hw_accel", checked);
        if (m_comboEnc) m_comboEnc->setEnabled(checked);
        if (m_lblEnc) m_lblEnc->setEnabled(checked);
        qDebug() << "[CapturePreferencesDialog] Hardware acceleration updated:" << checked;
    });

    hwRow->addLayout(hwTxtLayout);
    hwRow->addStretch();
    hwRow->addWidget(m_toggleHw);
    layout->addLayout(hwRow);

    // Hardware Encoder dropdown
    QHBoxLayout* encRow = new QHBoxLayout();
    m_lblEnc = new QLabel("Encoder Choice:", page);
    m_lblEnc->setStyleSheet("font-size: 12px; font-weight: bold;");

    m_comboEnc = new QComboBox(page);
    m_comboEnc->setCursor(Qt::PointingHandCursor);

    connect(m_comboEnc, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        if (!m_comboEnc) return;
        Config::setValue("hw_encoder", m_comboEnc->currentData().toString());
        qDebug() << "[CapturePreferencesDialog] Hardware encoder choice updated:" << m_comboEnc->currentData().toString();
    });

    encRow->addWidget(m_lblEnc);
    encRow->addSpacing(8);
    encRow->addWidget(m_comboEnc, 1);
    layout->addLayout(encRow);

    updateHwEncodersList();

    // Microphone Dropdown
    QHBoxLayout* micRow = new QHBoxLayout();
    QLabel* lblMic = new QLabel("Microphone Source:", page);
    lblMic->setStyleSheet("font-size: 12px; font-weight: bold;");

    QComboBox* comboMic = new QComboBox(page);
    comboMic->setCursor(Qt::PointingHandCursor);
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
        Config::setValue("audio_source", comboMic->currentText());
        if (CaptureEngine::instance() && CaptureEngine::instance()->statusCard()) {
            CaptureEngine::instance()->statusCard()->refreshStatus();
        }
        qDebug() << "[CapturePreferencesDialog] Microphone source updated:" << comboMic->currentText();
    });

    micRow->addWidget(lblMic);
    micRow->addSpacing(8);
    micRow->addWidget(comboMic, 1);
    layout->addLayout(micRow);

    SwitchWidget* toggleSysAudio = nullptr;
    layout->addLayout(createSettingRow(page, toggleSysAudio, "Record System Audio", "Capture internal desktop sound during video recording.",
        Config::getValue("Record System Audio", true).toBool(), [](bool checked) {
            Config::setValue("Record System Audio", checked);
            if (CaptureMainWindow::instance()) CaptureMainWindow::instance()->setSettingEnabled("Record System Audio", checked);
            if (CaptureEngine::instance()) CaptureEngine::instance()->setSessionSysAudioEnabled(checked);
        }));
    m_toggles["Record System Audio"] = toggleSysAudio;

    layout->addStretch();
    scrollArea->setWidget(page);
    return scrollArea;
}

void CapturePreferencesDialog::updateHwEncodersList() {
    if (!m_toggleHw || !m_comboEnc) return;

    auto applyList = [this](const QList<HwEncoderInfo>& list) {
        if (!m_toggleHw || !m_comboEnc) return;
        if (list.isEmpty()) {
            m_toggleHw->setChecked(false);
            m_toggleHw->setEnabled(false);
            if (m_lblHw) m_lblHw->setEnabled(false);
            if (m_lblHwDesc) m_lblHwDesc->setEnabled(false);
            Config::setValue("hw_accel", false);

            m_comboEnc->blockSignals(true);
            m_comboEnc->clear();
            m_comboEnc->addItem("找不到支援的 GPU 硬體壓縮 (停用)", "");
            m_comboEnc->setEnabled(false);
            if (m_lblEnc) m_lblEnc->setEnabled(false);
            m_comboEnc->blockSignals(false);
        } else {
            m_toggleHw->setEnabled(true);
            if (m_lblHw) m_lblHw->setEnabled(true);
            if (m_lblHwDesc) m_lblHwDesc->setEnabled(true);

            QString savedEnc = Config::getValue("hw_encoder", "").toString();
            m_comboEnc->blockSignals(true);
            m_comboEnc->clear();
            m_comboEnc->addItem("Auto / Best Available (自動偵測推薦)", "");
            for (const auto& hw : list) {
                if (hw.codec == "libx264") continue;
                m_comboEnc->addItem(hw.displayName, hw.codec);
            }
            int idx = m_comboEnc->findData(savedEnc);
            if (idx >= 0) m_comboEnc->setCurrentIndex(idx);
            m_comboEnc->blockSignals(false);

            bool isChecked = m_toggleHw->isChecked();
            m_comboEnc->setEnabled(isChecked);
            if (m_lblEnc) m_lblEnc->setEnabled(isChecked);
        }
    };

    if (VideoRecorder::isHwDetected()) {
        applyList(VideoRecorder::detectAvailableHwEncoders());
    } else {
        m_comboEnc->addItem("Auto / Best Available (自動偵測推薦)", "");
        m_comboEnc->addItem("🔍 正在後台偵測 GPU 加速硬體...", "detecting");
        QPointer<CapturePreferencesDialog> safeThis = this;
        VideoRecorder::detectAvailableHwEncodersAsync([safeThis, applyList](const QList<HwEncoderInfo>& list) {
            if (safeThis) applyList(list);
        });
    }
}

QWidget* CapturePreferencesDialog::createCursorPage() {
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget* page = new QWidget(scrollArea);
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 10, 0);
    layout->setSpacing(16);

    QLabel* header = new QLabel("Cursor Capture & Effects (游標與動畫)", page);
    header->setStyleSheet("font-size: 15px; font-weight: bold; color: #00a8ff; padding-bottom: 4px; border-bottom: 1px solid #2e323e;");
    layout->addWidget(header);

    SwitchWidget* toggleCursor = nullptr;
    layout->addLayout(createSettingRow(page, toggleCursor, "Capture Cursor", "Show mouse cursor during video recording.",
        Config::getValue("Video_Capture Cursor", true).toBool(), [](bool checked) {
            Config::setValue("Video_Capture Cursor", checked);
            if (CaptureMainWindow::instance()) CaptureMainWindow::instance()->setSettingEnabled("Video_Capture Cursor", checked);
            if (CaptureEngine::instance()) CaptureEngine::instance()->setSessionCursorEnabled(checked);
        }));
    m_toggles["Video_Capture Cursor"] = toggleCursor;

    SwitchWidget* toggleHl = nullptr;
    layout->addLayout(createSettingRow(page, toggleHl, "Cursor Highlight (光圈高亮)", "Draw a colored circle ring around the mouse cursor while recording.",
        Config::getValue("Video_Capture Cursor Highlight", true).toBool(), [](bool checked) {
            Config::setValue("Video_Capture Cursor Highlight", checked);
            if (CaptureEngine::instance()) {
                CaptureEngine::instance()->setSessionHighlightEnabled(checked);
                if (CaptureEngine::instance()->statusCard()) CaptureEngine::instance()->statusCard()->refreshStatus();
            }
        }));
    m_toggles["Video_Capture Cursor Highlight"] = toggleHl;

    SwitchWidget* toggleAnim = nullptr;
    layout->addLayout(createSettingRow(page, toggleAnim, "Click Animation (點擊動畫)", "Show ripple animation effect when mouse buttons are clicked during recording.",
        Config::getValue("Video_Capture Cursor Animation", true).toBool(), [](bool checked) {
            Config::setValue("Video_Capture Cursor Animation", checked);
            if (CaptureEngine::instance()) {
                CaptureEngine::instance()->setSessionAnimationEnabled(checked);
                if (CaptureEngine::instance()->statusCard()) CaptureEngine::instance()->statusCard()->refreshStatus();
            }
        }));
    m_toggles["Video_Capture Cursor Animation"] = toggleAnim;

    layout->addStretch();
    scrollArea->setWidget(page);
    return scrollArea;
}

} // namespace ScreenCut
