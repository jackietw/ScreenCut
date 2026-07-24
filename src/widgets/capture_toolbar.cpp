/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "capture_toolbar.h"
#include "../capture/capture_window.h"
#include "../core/capture_engine.h"
#include "../resources/IconUtils.h"
#include "../config.h"
#include <QCursor>
#include <QPainter>
#include <QPainterPath>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QStyle>

namespace ScreenCut {

CaptureToolBarWidget::CaptureToolBarWidget(QWidget *parent, CaptureMode mode)
    : QWidget(parent), m_mode(mode) {
  setAttribute(Qt::WA_StyledBackground, false);
  setAttribute(Qt::WA_TranslucentBackground, true);
  setCursor(Qt::ArrowCursor);
  setupUi();
  setCaptureMode(mode);
}

CaptureToolBarWidget::~CaptureToolBarWidget() = default;

void CaptureToolBarWidget::paintEvent(QPaintEvent * /*event*/) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);
  QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
  QPainterPath path;
  path.addRoundedRect(r, 8, 8);
  painter.setPen(QPen(QColor(255, 255, 255, 45), 1));
  painter.setBrush(QColor(40, 43, 50, 180));
  painter.drawPath(path);
}

void CaptureToolBarWidget::setupUi() {
  QString styleStr = 
                "QToolButton { "
                "   background-color: rgba(255, 255, 255, 0.08); "
                "   border: 1px solid rgba(255, 255, 255, 0.12); "
                "   border-radius: 6px; "
                "   padding: 0px; "
                "}"
                "QToolButton:hover { "
                "   background-color: #00a8ff; "
                "   border: 1px solid #00a8ff; "
                "}"
                "QToolButton:pressed { "
                "   background-color: #0088cc; "
                "}"
                "QToolButton:checked { "
                "   background-color: rgba(0, 168, 255, 0.4); "
                "   border: 1px solid #00a8ff; "
                "}"
                "QToolButton[split_left=\"true\"] { "
                "   border-top-right-radius: 0px; "
                "   border-bottom-right-radius: 0px; "
                "   border-right: none; "
                "}"
                "QToolButton[split_left=\"true\"]:checked { "
                "   border-right: none; "
                "}"
                "QToolButton[split_right=\"true\"] { "
                "   border-top-left-radius: 0px; "
                "   border-bottom-left-radius: 0px; "
                "   border-left: 1px solid rgba(255, 255, 255, 0.15); "
                "}"
                "QToolButton[split_right=\"true\"]:hover { "
                "   background-color: rgba(255, 255, 255, 0.16); "
                "}"
                "QToolButton[split_right=\"true\"][checked_state=\"true\"] { "
                "   background-color: rgba(0, 168, 255, 0.4); "
                "   border: 1px solid #00a8ff; "
                "   border-left: 1px solid #00a8ff; "
                "}"
                "QToolButton[split_right=\"true\"][checked_state=\"true\"]:hover { "
                "   background-color: rgba(0, 168, 255, 0.5); "
                "   border: 1px solid #00a8ff; "
                "   border-left: 1px solid #00a8ff; "
                "}"
                "QLabel { "
                "   color: #e0e6ed; "
                "   font-family: 'Noto Sans', sans-serif; "
                "   font-size: 13px; "
                "   font-weight: 600; "
                "   padding: 0 6px; "
                "}";
  setStyleSheet(styleStr);

  m_layout = new QHBoxLayout(this);
  m_layout->setContentsMargins(8, 6, 8, 6);
  m_layout->setSpacing(6);

  // --- Image Capture Buttons ---
  m_btnDone = createToolButton("Confirm Capture (Enter)", SVG_DONE,
                               SIGNAL(actionConfirm()));
  m_layout->addWidget(m_btnDone);

  m_btnImgCancel =
      createToolButton("Cancel (Esc)", SVG_CANCEL, SIGNAL(actionCancel()));
  m_layout->addWidget(m_btnImgCancel);

  m_imageSeparator = createSeparator();
  m_layout->addWidget(m_imageSeparator);

  bool editorEnabled = CaptureEngine::instance()
                           ? CaptureEngine::instance()->isSessionEditorEnabled()
                           : true;
  bool clipboardEnabled =
      CaptureEngine::instance()
          ? CaptureEngine::instance()->isSessionClipboardEnabled()
          : true;
  bool imgCursorEnabled =
      CaptureEngine::instance()
          ? CaptureEngine::instance()->isSessionImgCursorEnabled()
          : false;

  m_btnImgEditor = createCheckableButton("Open in Editor after capture",
                                         SVG_EDITOR, editorEnabled);
  connect(m_btnImgEditor, &QToolButton::clicked, this,
          &CaptureToolBarWidget::onImgEditorClicked);
  m_layout->addWidget(m_btnImgEditor);

  m_btnImgClipboard = createCheckableButton("Copy to Clipboard after capture",
                                            SVG_COPY, clipboardEnabled);
  connect(m_btnImgClipboard, &QToolButton::clicked, this,
          &CaptureToolBarWidget::onImgClipboardClicked);
  m_layout->addWidget(m_btnImgClipboard);

  m_btnImgCursor = createCheckableButton("Capture Mouse Cursor", SVG_MOUSE,
                                         imgCursorEnabled);
  connect(m_btnImgCursor, &QToolButton::clicked, this,
          &CaptureToolBarWidget::onImgCursorClicked);
  m_layout->addWidget(m_btnImgCursor);

  // --- Video Capture Buttons & Status ---
  m_btnRecordPause = new QToolButton(this);
  m_btnRecordPause->setIcon(createSvgIcon(SVG_RECORD, 20, 20));
  m_btnRecordPause->setIconSize(QSize(20, 20));
  m_btnRecordPause->setToolTip("Start Recording");
  m_btnRecordPause->setFixedSize(34, 34);
  m_btnRecordPause->setCursor(Qt::PointingHandCursor);
  connect(m_btnRecordPause, &QToolButton::clicked, this,
          &CaptureToolBarWidget::onRecordPauseClicked);
  m_layout->addWidget(m_btnRecordPause);

  bool cursorEnabled = true;
  bool micEnabled = true;
  bool sysAudioEnabled = true;
  if (CaptureMainWindow::instance()) {
    cursorEnabled =
        CaptureMainWindow::instance()->isSettingEnabled("Video_Capture Cursor");
    micEnabled =
        CaptureMainWindow::instance()->isSettingEnabled("Record Microphone");
    sysAudioEnabled =
        CaptureMainWindow::instance()->isSettingEnabled("Record System Audio");
  }

  QWidget* cursorContainer = new QWidget(this);
  QHBoxLayout* cursorLayout = new QHBoxLayout(cursorContainer);
  cursorLayout->setContentsMargins(0, 0, 0, 0);
  cursorLayout->setSpacing(0);
  m_btnCursor =
      createCheckableButton("Record Mouse Cursor", SVG_MOUSE, cursorEnabled);
  m_btnCursor->setProperty("split_left", true);
  m_btnCursor->setFixedSize(32, 34);
  QToolButton* cursorArrow = new QToolButton(this);
  cursorArrow->setProperty("split_right", true);
  cursorArrow->setProperty("checked_state", cursorEnabled);
  cursorArrow->setFixedSize(16, 34);
  cursorArrow->setCursor(Qt::PointingHandCursor);
  cursorArrow->setIcon(createSvgIcon("<svg xmlns=\"http://www.w3.org/2000/svg\" height=\"24px\" viewBox=\"0 -960 960 960\" width=\"24px\" fill=\"#FFFFFF\"><path d=\"M480-345 240-585l56-56 184 183 184-183 56 56-240 240Z\"/></svg>", 16, 16));
  cursorArrow->setIconSize(QSize(16, 16));
  cursorLayout->addWidget(m_btnCursor);
  cursorLayout->addWidget(cursorArrow);

  m_cursorMenu = new QMenu(cursorContainer);
  m_cursorMenu->setStyleSheet(
      "QMenu { background-color: #282e38; color: #f0f0f0; border: 1px solid "
      "#384252; border-radius: 6px; padding: 4px 0px; font-size: 13px; }"
      "QMenu::item { padding: 6px 28px 6px 28px; }"
      "QMenu::item:selected { background-color: #00a8ff; color: #ffffff; }"
      "QMenu::indicator { width: 14px; height: 14px; left: 8px; }");
  QAction *actHl = m_cursorMenu->addAction("Highlight");
  actHl->setCheckable(true);
  actHl->setChecked(CaptureEngine::instance()->isSessionHighlightEnabled());
  connect(actHl, &QAction::toggled, this, [this](bool checked) {
    Config::setValue("Video_Capture Cursor Highlight", checked);
    CaptureEngine::instance()->setSessionHighlightEnabled(checked);
    emit highlightToggled(checked);
  });
  QAction *actAnim = m_cursorMenu->addAction("Animation");
  actAnim->setCheckable(true);
  actAnim->setChecked(CaptureEngine::instance()->isSessionAnimationEnabled());
  connect(actAnim, &QAction::toggled, this, [this](bool checked) {
    Config::setValue("Video_Capture Cursor Animation", checked);
    CaptureEngine::instance()->setSessionAnimationEnabled(checked);
    emit animationToggled(checked);
  });
  connect(m_cursorMenu, &QMenu::aboutToShow, this,
          &CaptureToolBarWidget::refreshMenuStates);
  connect(cursorArrow, &QToolButton::clicked, this, [this, cursorArrow]() {
      QPoint pos = cursorArrow->mapToGlobal(QPoint(0, cursorArrow->height() + 4));
      m_cursorMenu->exec(pos);
  });
  connect(m_btnCursor, &QToolButton::toggled, this, [cursorArrow](bool checked){
      cursorArrow->setProperty("checked_state", checked);
      cursorArrow->style()->unpolish(cursorArrow);
      cursorArrow->style()->polish(cursorArrow);
  });
  connect(m_btnCursor, &QToolButton::clicked, this,
          &CaptureToolBarWidget::onCursorClicked);
  m_layout->addWidget(cursorContainer);

  QWidget* micContainer = new QWidget(this);
  QHBoxLayout* micLayout = new QHBoxLayout(micContainer);
  micLayout->setContentsMargins(0, 0, 0, 0);
  micLayout->setSpacing(0);
  m_btnMic = createCheckableButton(
      "Record Microphone", micEnabled ? SVG_MIC : SVG_MIC_OFF, micEnabled);
  m_btnMic->setProperty("split_left", true);
  m_btnMic->setFixedSize(32, 34);
  QToolButton* micArrow = new QToolButton(this);
  micArrow->setProperty("split_right", true);
  micArrow->setProperty("checked_state", micEnabled);
  micArrow->setFixedSize(16, 34);
  micArrow->setCursor(Qt::PointingHandCursor);
  micArrow->setIcon(createSvgIcon("<svg xmlns=\"http://www.w3.org/2000/svg\" height=\"24px\" viewBox=\"0 -960 960 960\" width=\"24px\" fill=\"#FFFFFF\"><path d=\"M480-345 240-585l56-56 184 183 184-183 56 56-240 240Z\"/></svg>", 16, 16));
  micArrow->setIconSize(QSize(16, 16));
  micLayout->addWidget(m_btnMic);
  micLayout->addWidget(micArrow);

  m_micMenu = new QMenu(micContainer);
  m_micMenu->setStyleSheet(
      "QMenu { background-color: #282e38; color: #f0f0f0; border: 1px solid "
      "#384252; border-radius: 6px; padding: 4px 0px; font-size: 13px; }"
      "QMenu::item { padding: 6px 28px 6px 28px; }"
      "QMenu::item:selected { background-color: #00a8ff; color: #ffffff; }"
      "QMenu::indicator { width: 14px; height: 14px; left: 8px; }");
  connect(m_micMenu, &QMenu::aboutToShow, this,
          &CaptureToolBarWidget::onMicMenuAboutToShow);
  connect(micArrow, &QToolButton::clicked, this, [this, micArrow]() {
      QPoint pos = micArrow->mapToGlobal(QPoint(0, micArrow->height() + 4));
      m_micMenu->exec(pos);
  });
  connect(m_btnMic, &QToolButton::toggled, this, [micArrow](bool checked){
      micArrow->setProperty("checked_state", checked);
      micArrow->style()->unpolish(micArrow);
      micArrow->style()->polish(micArrow);
  });
  connect(m_btnMic, &QToolButton::clicked, this,
          &CaptureToolBarWidget::onMicClicked);
  m_layout->addWidget(micContainer);

  m_btnSysAudio = createCheckableButton(
      "Record System Audio",
      sysAudioEnabled ? SVG_SYS_AUDIO : SVG_SYS_AUDIO_OFF, sysAudioEnabled);
  connect(m_btnSysAudio, &QToolButton::clicked, this,
          &CaptureToolBarWidget::onSysAudioClicked);
  m_layout->addWidget(m_btnSysAudio);

  m_lblVideoStatus = new QLabel(
      QString("%1 × %2").arg(m_targetWidth).arg(m_targetHeight), this);
  m_layout->addWidget(m_lblVideoStatus);

  m_videoSeparator = createSeparator();
  m_layout->addWidget(m_videoSeparator);

  m_btnVideoStop =
      createToolButton("Finish Recording (Done)", SVG_DONE, nullptr);
  connect(m_btnVideoStop, &QToolButton::clicked, this,
          &CaptureToolBarWidget::onVideoStopClicked);
  m_layout->addWidget(m_btnVideoStop);

  m_btnVideoCancel =
      createToolButton("Cancel (Esc)", SVG_CANCEL, SIGNAL(actionCancel()));
  m_layout->addWidget(m_btnVideoCancel);

  updateRecordButtonStyle();
  adjustSize();
}

void CaptureToolBarWidget::updateRecordButtonStyle() {
  if (!m_btnRecordPause)
    return;
  if (m_isRecording) {
    m_btnRecordPause->setStyleSheet(
        "QToolButton { background-color: #e53935; border: 1px solid #ff5252; "
        "border-radius: 6px; }"
        "QToolButton:hover { background-color: #ff5252; border: 1px solid "
        "#ff7b7b; }"
        "QToolButton:pressed { background-color: #b71c1c; }");
  } else {
    m_btnRecordPause->setStyleSheet(
        "QToolButton { background: rgba(255, 255, 255, 0.14); border: 1px "
        "solid rgba(255, 255, 255, 0.28); border-radius: 6px; }"
        "QToolButton:hover { background-color: #00a8ff; border: 1px solid "
        "#00a8ff; }"
        "QToolButton:pressed { background-color: #0088cc; }");
  }
}

void CaptureToolBarWidget::setCaptureMode(CaptureMode mode) {
  m_mode = mode;
  bool isVideo = (mode == CaptureMode::VideoRegion);

  // Image buttons
  if (m_btnDone)
    m_btnDone->setVisible(!isVideo);
  if (m_btnImgCancel)
    m_btnImgCancel->setVisible(!isVideo);
  if (m_imageSeparator)
    m_imageSeparator->setVisible(!isVideo);
  if (m_btnImgEditor)
    m_btnImgEditor->setVisible(!isVideo);
  if (m_btnImgClipboard)
    m_btnImgClipboard->setVisible(!isVideo);
  if (m_btnImgCursor)
    m_btnImgCursor->setVisible(!isVideo);

  // Video buttons
  if (m_btnRecordPause) {
    m_btnRecordPause->setVisible(isVideo);
    if (isVideo) {
      m_btnRecordPause->setIcon(
          createSvgIcon(m_isRecording ? SVG_STOP : SVG_RECORD, 20, 20));
      m_btnRecordPause->setToolTip(
          m_isRecording ? "Stop Recording & Save (停止錄影並回寫到檔案)"
                        : "Start Recording (啟動錄影)");
      updateRecordButtonStyle();
    }
  }
  if (m_btnCursor)
    m_btnCursor->setVisible(isVideo);
  if (m_btnMic)
    m_btnMic->setVisible(isVideo);
  if (m_btnSysAudio)
    m_btnSysAudio->setVisible(isVideo);
  if (m_lblVideoStatus) {
    m_lblVideoStatus->setVisible(isVideo);
    if (isVideo && !m_isRecording) {
      m_lblVideoStatus->setText(
          QString("%1 × %2").arg(m_targetWidth).arg(m_targetHeight));
    }
  }
  if (m_videoSeparator)
    m_videoSeparator->setVisible(false);
  if (m_btnVideoStop)
    m_btnVideoStop->setVisible(false);
  if (m_btnVideoCancel)
    m_btnVideoCancel->setVisible(isVideo);

  adjustSize();
}

void CaptureToolBarWidget::updateTargetDimensions(int width, int height) {
  m_targetWidth = width;
  m_targetHeight = height;
  if (m_mode == CaptureMode::VideoRegion && m_lblVideoStatus) {
    if (!m_isRecording) {
      m_lblVideoStatus->setText(
          QString("%1 × %2").arg(m_targetWidth).arg(m_targetHeight));
    } else {
      int mins = m_elapsedSeconds / 60;
      int secs = m_elapsedSeconds % 60;
      QString dot = m_redDotVisible ? "🔴" : "⚪";
      m_lblVideoStatus->setText(QString("%1 %2:%3")
                                    .arg(dot)
                                    .arg(mins, 2, 10, QChar('0'))
                                    .arg(secs, 2, 10, QChar('0')));
    }
    adjustSize();
  }
}

QToolButton *
CaptureToolBarWidget::createToolButton(const QString &tooltip,
                                       const QString &svgIconString,
                                       const char *slotSignal) {
  QToolButton *btn = new QToolButton(this);
  btn->setIcon(createSvgIcon(svgIconString, 20, 20));
  btn->setIconSize(QSize(20, 20));
  btn->setToolTip(tooltip);
  btn->setFixedSize(34, 34);
  btn->setCursor(Qt::PointingHandCursor);
  if (slotSignal) {
    connect(btn, SIGNAL(clicked()), this, slotSignal);
  }
  return btn;
}

QToolButton *CaptureToolBarWidget::createCheckableButton(
    const QString &tooltip, const QString &svgIconString, bool checked) {
  QToolButton *btn = new QToolButton(this);
  btn->setIcon(createSvgIcon(svgIconString, 20, 20));
  btn->setIconSize(QSize(20, 20));
  btn->setToolTip(tooltip);
  btn->setFixedSize(34, 34);
  btn->setCheckable(true);
  btn->setChecked(checked);
  btn->setCursor(Qt::PointingHandCursor);
  return btn;
}

QFrame *CaptureToolBarWidget::createSeparator() {
  QFrame *line = new QFrame(this);
  line->setFrameShape(QFrame::VLine);
  line->setFrameShadow(QFrame::Plain);
  line->setStyleSheet("color: #384252; background-color: #384252; max-width: "
                      "1px; margin: 4px 2px;");
  return line;
}

void CaptureToolBarWidget::onRecordPauseClicked() {
  if (!m_isRecording) {
    // 啟動: 開始錄影
    m_isRecording = true;
    m_elapsedSeconds = 0;
    m_redDotVisible = true;
    if (m_lblVideoStatus) {
      m_lblVideoStatus->setText(QString("🔴 00:00"));
    }
    m_btnRecordPause->setIcon(createSvgIcon(SVG_STOP, 20, 20));
    m_btnRecordPause->setToolTip(
        "Stop Recording & Save (停止錄影並回寫到檔案)");
    updateRecordButtonStyle();
    if (!m_recordTimer) {
      m_recordTimer = new QTimer(this);
      connect(m_recordTimer, &QTimer::timeout, this,
              &CaptureToolBarWidget::updateVideoStatus);
    }
    m_recordTimer->start(500);
    emit actionVideoRecordStateChanged(true);
  } else {
    // 停止: 關閉錄影(檔案) 回寫到檔案
    if (m_recordTimer)
      m_recordTimer->stop();
    m_isRecording = false;
    m_btnRecordPause->setIcon(createSvgIcon(SVG_RECORD, 20, 20));
    m_btnRecordPause->setToolTip("Start Recording (啟動錄影)");
    updateRecordButtonStyle();
    if (m_lblVideoStatus) {
      m_lblVideoStatus->setText("Saving...");
    }
    emit actionVideoRecordStateChanged(false);
    emit actionVideoStop();
  }
  adjustSize();
}

void CaptureToolBarWidget::onCursorClicked() {
  bool checked = m_btnCursor->isChecked();
  emit cursorToggled(checked);
}

void CaptureToolBarWidget::onMicClicked() {
  bool checked = m_btnMic->isChecked();
  m_btnMic->setIcon(createSvgIcon(checked ? SVG_MIC : SVG_MIC_OFF, 20, 20));
  emit micToggled(checked);
}

void CaptureToolBarWidget::onSysAudioClicked() {
  bool checked = m_btnSysAudio->isChecked();
  m_btnSysAudio->setIcon(
      createSvgIcon(checked ? SVG_SYS_AUDIO : SVG_SYS_AUDIO_OFF, 20, 20));
  emit sysAudioToggled(checked);
}

void CaptureToolBarWidget::onVideoStopClicked() {
  if (m_recordTimer)
    m_recordTimer->stop();
  m_isRecording = false;
  if (m_btnVideoStop)
    m_btnVideoStop->setVisible(false);
  if (m_videoSeparator)
    m_videoSeparator->setVisible(false);
  emit actionVideoStop();
}

void CaptureToolBarWidget::updateVideoStatus() {
  m_redDotVisible = !m_redDotVisible;
  if (m_redDotVisible) {
    m_elapsedSeconds++;
  }
  int mins = m_elapsedSeconds / 60;
  int secs = m_elapsedSeconds % 60;
  QString dot = m_redDotVisible ? "🔴" : "⚪";
  if (m_lblVideoStatus) {
    m_lblVideoStatus->setText(QString("%1 %2:%3")
                                  .arg(dot)
                                  .arg(mins, 2, 10, QChar('0'))
                                  .arg(secs, 2, 10, QChar('0')));
  }
  adjustSize();
}

void CaptureToolBarWidget::onImgEditorClicked() {
  bool checked = m_btnImgEditor->isChecked();
  if (CaptureEngine::instance()) {
    CaptureEngine::instance()->setSessionEditorEnabled(checked);
  }
}

void CaptureToolBarWidget::onImgClipboardClicked() {
  bool checked = m_btnImgClipboard->isChecked();
  if (CaptureEngine::instance()) {
    CaptureEngine::instance()->setSessionClipboardEnabled(checked);
  }
}

void CaptureToolBarWidget::onImgCursorClicked() {
  bool checked = m_btnImgCursor->isChecked();
  if (CaptureEngine::instance()) {
    CaptureEngine::instance()->setSessionImgCursorEnabled(checked);
  }
}

void CaptureToolBarWidget::refreshMenuStates() {
  if (m_cursorMenu && CaptureEngine::instance()) {
    const auto actions = m_cursorMenu->actions();
    if (actions.size() >= 2) {
      actions[0]->blockSignals(true);
      actions[0]->setChecked(CaptureEngine::instance()->isSessionHighlightEnabled());
      actions[0]->blockSignals(false);

      actions[1]->blockSignals(true);
      actions[1]->setChecked(CaptureEngine::instance()->isSessionAnimationEnabled());
      actions[1]->blockSignals(false);
    }
  }
}

void CaptureToolBarWidget::onMicMenuAboutToShow() {
  if (!m_micMenu)
    return;
  m_micMenu->clear();
  QActionGroup *group = new QActionGroup(m_micMenu);
  group->setExclusive(true);

  QString currentMicId = Config::getValue("mic_device", QString()).toString();

  QAction *actDefault = m_micMenu->addAction("Default Microphone (預設麥克風)");
  actDefault->setCheckable(true);
  group->addAction(actDefault);
  if (currentMicId.isEmpty()) {
    actDefault->setChecked(true);
  }
  connect(actDefault, &QAction::triggered, this, [this]() {
    Config::setValue("mic_device", QString());
    Config::setValue("audio_source", "Default Microphone");
    emit micDeviceChanged("Default Microphone");
  });

  const auto devices = QMediaDevices::audioInputs();
  for (const QAudioDevice &dev : devices) {
    QAction *actDev = m_micMenu->addAction(dev.description());
    actDev->setCheckable(true);
    group->addAction(actDev);
    QString devIdStr = QString::fromUtf8(dev.id());
    if (devIdStr == currentMicId) {
      actDev->setChecked(true);
    }
    connect(actDev, &QAction::triggered, this,
            [this, devIdStr, desc = dev.description()]() {
              Config::setValue("mic_device", devIdStr);
              Config::setValue("audio_source", desc);
              emit micDeviceChanged(desc);
            });
  }
}

} // namespace ScreenCut
