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
  ButtonConfig doneCfg;
  doneCfg.tooltip = "Confirm Capture (Enter)";
  doneCfg.svgIcon = SVG_DONE;
  m_btnDone = createButton(doneCfg);
  connect(m_btnDone, SIGNAL(clicked()), this, SIGNAL(actionConfirm()));
  m_layout->addWidget(m_btnDone);

  ButtonConfig cancelCfg;
  cancelCfg.tooltip = "Cancel (Esc)";
  cancelCfg.svgIcon = SVG_CANCEL;
  m_btnImgCancel = createButton(cancelCfg);
  connect(m_btnImgCancel, SIGNAL(clicked()), this, SIGNAL(actionCancel()));
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

  ButtonConfig editorCfg;
  editorCfg.tooltip = "Open in Editor after capture";
  editorCfg.svgIcon = SVG_EDITOR;
  editorCfg.checkable = true;
  editorCfg.checked = editorEnabled;
  m_btnImgEditor = createButton(editorCfg);
  connect(m_btnImgEditor, &QToolButton::clicked, this,
          &CaptureToolBarWidget::onImgEditorClicked);
  m_layout->addWidget(m_btnImgEditor);

  ButtonConfig clipboardCfg;
  clipboardCfg.tooltip = "Copy to Clipboard after capture";
  clipboardCfg.svgIcon = SVG_COPY;
  clipboardCfg.checkable = true;
  clipboardCfg.checked = clipboardEnabled;
  m_btnImgClipboard = createButton(clipboardCfg);
  connect(m_btnImgClipboard, &QToolButton::clicked, this,
          &CaptureToolBarWidget::onImgClipboardClicked);
  m_layout->addWidget(m_btnImgClipboard);

  ButtonConfig imgCursorCfg;
  imgCursorCfg.tooltip = "Capture Mouse Cursor";
  imgCursorCfg.svgIcon = SVG_MOUSE;
  imgCursorCfg.checkable = true;
  imgCursorCfg.checked = imgCursorEnabled;
  m_btnImgCursor = createButton(imgCursorCfg);
  connect(m_btnImgCursor, &QToolButton::clicked, this,
          &CaptureToolBarWidget::onImgCursorClicked);
  m_layout->addWidget(m_btnImgCursor);

  // --- Video Capture Buttons & Status ---
  ButtonConfig recordCfg;
  recordCfg.tooltip = "Start Recording";
  recordCfg.svgIcon = SVG_RECORD;
  m_btnRecordPause = createButton(recordCfg);
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

  auto cursorGroup = createSplitButton("Record Mouse Cursor", SVG_MOUSE, cursorEnabled);
  m_cursorContainer = cursorGroup.container;
  m_btnCursor = cursorGroup.mainButton;
  m_cursorMenu = cursorGroup.menu;
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
  connect(m_btnCursor, &QToolButton::clicked, this,
          &CaptureToolBarWidget::onCursorClicked);
  m_layout->addWidget(m_cursorContainer);

  auto micGroup = createSplitButton("Record Microphone", micEnabled ? SVG_MIC : SVG_MIC_OFF, micEnabled);
  m_micContainer = micGroup.container;
  m_btnMic = micGroup.mainButton;
  m_micMenu = micGroup.menu;
  connect(m_micMenu, &QMenu::aboutToShow, this,
          &CaptureToolBarWidget::onMicMenuAboutToShow);
  connect(m_btnMic, &QToolButton::clicked, this,
          &CaptureToolBarWidget::onMicClicked);
  m_layout->addWidget(m_micContainer);

  ButtonConfig sysAudioCfg;
  sysAudioCfg.tooltip = "Record System Audio";
  sysAudioCfg.svgIcon = sysAudioEnabled ? SVG_SYS_AUDIO : SVG_SYS_AUDIO_OFF;
  sysAudioCfg.checkable = true;
  sysAudioCfg.checked = sysAudioEnabled;
  m_btnSysAudio = createButton(sysAudioCfg);
  connect(m_btnSysAudio, &QToolButton::clicked, this,
          &CaptureToolBarWidget::onSysAudioClicked);
  m_layout->addWidget(m_btnSysAudio);

  m_lblVideoStatus = new QLabel(
      QString("%1 × %2").arg(m_targetWidth).arg(m_targetHeight), this);
  m_layout->addWidget(m_lblVideoStatus);

  m_videoSeparator = createSeparator();
  m_layout->addWidget(m_videoSeparator);

  ButtonConfig videoStopCfg;
  videoStopCfg.tooltip = "Finish Recording (Done)";
  videoStopCfg.svgIcon = SVG_DONE;
  m_btnVideoStop = createButton(videoStopCfg);
  connect(m_btnVideoStop, &QToolButton::clicked, this,
          &CaptureToolBarWidget::onVideoStopClicked);
  m_layout->addWidget(m_btnVideoStop);

  ButtonConfig videoCancelCfg;
  videoCancelCfg.tooltip = "Cancel (Esc)";
  videoCancelCfg.svgIcon = SVG_CANCEL;
  m_btnVideoCancel = createButton(videoCancelCfg);
  connect(m_btnVideoCancel, SIGNAL(clicked()), this, SIGNAL(actionCancel()));
  m_layout->addWidget(m_btnVideoCancel);

  updateRecordButtonStyle();
  adjustSize();
}

void CaptureToolBarWidget::updateRecordButtonStyle() {
  if (!m_btnRecordPause)
    return;
  ButtonConfig config;
  config.tooltip = m_isRecording ? "Stop Recording & Save" : "Start Recording";
  config.svgIcon = m_isRecording ? SVG_STOP : SVG_RECORD;
  if (m_isRecording) {
    config.bgNormal = "#e53935";
    config.borderNormal = "#ff5252";
    config.bgHover = "#ff5252";
    config.borderHover = "#ff7b7b";
    config.bgPressed = "#b71c1c";
  } else {
    config.bgNormal = "rgba(255, 255, 255, 0.14)";
    config.borderNormal = "rgba(255, 255, 255, 0.28)";
  }
  setButtonColors(m_btnRecordPause, config);
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
          m_isRecording ? "Stop Recording & Save"
                        : "Start Recording");
      updateRecordButtonStyle();
    }
  }
  if (m_cursorContainer)
    m_cursorContainer->setVisible(isVideo);
  if (m_micContainer)
    m_micContainer->setVisible(isVideo);
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

QToolButton *CaptureToolBarWidget::createButton(const ButtonConfig& config) {
  QToolButton *btn = new QToolButton(this);
  btn->setIcon(createSvgIcon(config.svgIcon, config.iconSize, config.iconSize));
  btn->setIconSize(QSize(config.iconSize, config.iconSize));
  if (!config.tooltip.isEmpty()) {
    btn->setToolTip(config.tooltip);
  }
  btn->setFixedSize(config.width, config.height);
  btn->setCheckable(config.checkable);
  btn->setChecked(config.checked);
  btn->setCursor(Qt::PointingHandCursor);
  setButtonColors(btn, config);
  return btn;
}

void CaptureToolBarWidget::setButtonColors(QToolButton* btn, const ButtonConfig& config) {
  QString style = QString(
      "QToolButton { "
      "   background-color: %1; "
      "   border: 1px solid %2; "
      "   border-radius: 6px; "
      "   padding: 0px; "
      "} "
      "QToolButton:hover { "
      "   background-color: %3; "
      "   border: 1px solid %4; "
      "} "
      "QToolButton:pressed { "
      "   background-color: %5; "
      "} "
      "QToolButton:checked { "
      "   background-color: %6; "
      "   border: 1px solid %7; "
      "} "
      "%8"
  ).arg(config.bgNormal, config.borderNormal, 
        config.bgHover, config.borderHover, 
        config.bgPressed, 
        config.bgChecked, config.borderChecked,
        config.extraStyle);
  btn->setStyleSheet(style);
}

CaptureToolBarWidget::SplitButtonResult CaptureToolBarWidget::createSplitButton(
    const QString &tooltip, const QString &svgIconString, bool checked) {
  QWidget* container = new QWidget(this);
  QHBoxLayout* layout = new QHBoxLayout(container);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  ButtonConfig mainCfg;
  mainCfg.tooltip = tooltip;
  mainCfg.svgIcon = svgIconString;
  mainCfg.checkable = true;
  mainCfg.checked = checked;
  mainCfg.width = 32;
  mainCfg.extraStyle = 
        "QToolButton { "
        "   border-top-right-radius: 0px; "
        "   border-bottom-right-radius: 0px; "
        "   border-right: none; "
        "} "
        "QToolButton:checked { "
        "   border-right: none; "
        "} ";
  QToolButton* mainBtn = createButton(mainCfg);
  mainBtn->setProperty("split_left", true);

  ButtonConfig arrowCfg;
  arrowCfg.svgIcon = SVG_DROP_DOWN;
  arrowCfg.iconSize = 16;
  arrowCfg.width = 16;
  arrowCfg.bgHover = "rgba(255, 255, 255, 0.16)";
  arrowCfg.borderHover = "rgba(255, 255, 255, 0.15)";
  arrowCfg.extraStyle = 
        "QToolButton { "
        "   border-top-left-radius: 0px; "
        "   border-bottom-left-radius: 0px; "
        "   border-left: 1px solid rgba(255, 255, 255, 0.15); "
        "} "
        "QToolButton[checked_state=\"true\"] { "
        "   background-color: rgba(0, 168, 255, 0.4); "
        "   border: 1px solid #00a8ff; "
        "   border-left: 1px solid #00a8ff; "
        "} "
        "QToolButton[checked_state=\"true\"]:hover { "
        "   background-color: rgba(0, 168, 255, 0.5); "
        "   border: 1px solid #00a8ff; "
        "   border-left: 1px solid #00a8ff; "
        "} ";
  QToolButton* arrowBtn = createButton(arrowCfg);
  arrowBtn->setProperty("split_right", true);
  arrowBtn->setProperty("checked_state", checked);

  layout->addWidget(mainBtn);
  layout->addWidget(arrowBtn);

  QMenu* menu = new QMenu(container);
  menu->setStyleSheet(
      "QMenu { background-color: #282e38; color: #f0f0f0; border: 1px solid "
      "#384252; border-radius: 6px; padding: 4px 0px; font-size: 13px; }"
      "QMenu::item { padding: 6px 28px 6px 28px; }"
      "QMenu::item:selected { background-color: #00a8ff; color: #ffffff; }"
      "QMenu::indicator { width: 14px; height: 14px; left: 8px; }");

  connect(arrowBtn, &QToolButton::clicked, this, [menu, arrowBtn]() {
    QPoint pos = arrowBtn->mapToGlobal(QPoint(0, arrowBtn->height() + 4));
    menu->exec(pos);
  });
  connect(mainBtn, &QToolButton::toggled, this, [arrowBtn](bool c) {
    arrowBtn->setProperty("checked_state", c);
    arrowBtn->style()->unpolish(arrowBtn);
    arrowBtn->style()->polish(arrowBtn);
  });

  return {container, mainBtn, menu};
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
    // Start Recording
    m_isRecording = true;
    m_elapsedSeconds = 0;
    m_redDotVisible = true;
    if (m_lblVideoStatus) {
      m_lblVideoStatus->setText(QString("🔴 00:00"));
    }
    m_btnRecordPause->setIcon(createSvgIcon(SVG_STOP, 20, 20));
    m_btnRecordPause->setToolTip("Stop Recording & Save");
    updateRecordButtonStyle();
    if (!m_recordTimer) {
      m_recordTimer = new QTimer(this);
      connect(m_recordTimer, &QTimer::timeout, this,
              &CaptureToolBarWidget::updateVideoStatus);
    }
    m_recordTimer->start(500);
    emit actionVideoRecordStateChanged(true);
  } else {
    // Stop Recording & Save
    if (m_recordTimer)
      m_recordTimer->stop();
    m_isRecording = false;
    m_btnRecordPause->setIcon(createSvgIcon(SVG_RECORD, 20, 20));
    m_btnRecordPause->setToolTip("Start Recording");
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
