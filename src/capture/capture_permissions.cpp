/**
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "capture_permissions.h"
#include "../platform/platform.h"
#include <QEvent>
#include <QMouseEvent>
#include <QHBoxLayout>

namespace ScreenCut {

PermissionRow::PermissionRow(const QString &title, const QString &desc,
                             const QString &category,
                             std::function<bool()> checkFunc, QWidget *parent)
    : QWidget(parent), m_category(category), m_checkFunc(checkFunc) {

  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setContentsMargins(16, 12, 16, 12);
  layout->setSpacing(12);

  // Icon (Check or Cross)
  m_iconLabel = new QLabel(this);
  m_iconLabel->setFixedSize(14, 14);
  m_iconLabel->setAlignment(Qt::AlignCenter);
  layout->addWidget(m_iconLabel, 0, Qt::AlignTop | Qt::AlignHCenter);

  // Texts
  QVBoxLayout *textLayout = new QVBoxLayout();
  textLayout->setSpacing(4);

  QLabel *titleLabel = new QLabel(title, this);
  titleLabel->setStyleSheet(
      "font-size: 14px; font-weight: bold; color: #e0e5f0;");
  QLabel *descLabel = new QLabel(desc, this);
  descLabel->setStyleSheet("font-size: 12px; color: #8890a0;");
  descLabel->setWordWrap(true);

  textLayout->addWidget(titleLabel);
  textLayout->addWidget(descLabel);
  textLayout->addStretch(1);

  layout->addLayout(textLayout, 1); // 1 = stretch to take up available space

  // Button
  m_btnSettings = new QPushButton("Check / Request", this);
  m_btnSettings->setCursor(Qt::PointingHandCursor);
  m_btnSettings->setObjectName("BtnOpenSettings");
  connect(m_btnSettings, &QPushButton::clicked, this,
          [this]() { 
              // Check again. If it triggers prompt, it happens here!
              if (m_checkFunc()) {
                  refresh();
              } else {
                  Platform::openSystemSettings(m_category);
              }
          });
  
  layout->addWidget(m_btnSettings, 0, Qt::AlignVCenter);

  // Initially show red/unchecked for ALL permissions, to avoid auto-prompting on window load.
  // The user must manually click "Check / Request" to process permissions.
  m_iconLabel->setStyleSheet("background-color: #ff3b30; border-radius: 7px;");
  m_iconLabel->setText("");
  m_btnSettings->show();
}

void PermissionRow::refresh() {
  bool hasPermission = m_checkFunc();

  if (hasPermission) {
    // Green circle
    m_iconLabel->setFixedSize(14, 14);
    m_iconLabel->setStyleSheet("background-color: #4cd964; border-radius: 7px;");
    m_iconLabel->setText("");
    m_btnSettings->hide();
  } else {
    // Red circle
    m_iconLabel->setFixedSize(14, 14);
    m_iconLabel->setStyleSheet("background-color: #ff3b30; border-radius: 7px;");
    m_iconLabel->setText("");
    m_btnSettings->show();
  }
}

PermissionsWindow *PermissionsWindow::s_instance = nullptr;

PermissionsWindow *PermissionsWindow::instance(QWidget *parent) {
  if (!s_instance) {
    s_instance = new PermissionsWindow(parent);
  } else if (parent && s_instance->parent() != parent) {
    s_instance->setParent(
        parent, Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
  }
  return s_instance;
}

PermissionsWindow::PermissionsWindow(QWidget *parent)
    : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint) {
  setWindowTitle("System Permissions");
  setModal(true);
  setWindowModality(Qt::WindowModal);
  resize(500, 440);

  setObjectName("PermissionsWindow");
  setAttribute(Qt::WA_TranslucentBackground);

  setupUi();
  setupStyleSheet();
}

void PermissionsWindow::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    m_dragPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
    event->accept();
  }
}

void PermissionsWindow::mouseMoveEvent(QMouseEvent *event) {
  if (event->buttons() & Qt::LeftButton) {
    move(event->globalPosition().toPoint() - m_dragPos);
    event->accept();
  }
}

void PermissionsWindow::setupUi() {
  QVBoxLayout *outerLayout = new QVBoxLayout(this);
  outerLayout->setContentsMargins(0, 0, 0, 0);

  QWidget *centralWidget = new QWidget(this);
  centralWidget->setObjectName("CentralWidget");
  outerLayout->addWidget(centralWidget);

  m_mainLayout = new QVBoxLayout(centralWidget);
  m_mainLayout->setContentsMargins(0, 0, 0, 0);
  m_mainLayout->setSpacing(0);

  // Top Title Bar (for dragging)
  QWidget *titleBar = new QWidget(centralWidget);
  titleBar->setFixedHeight(32);
  QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
  titleLayout->setContentsMargins(12, 0, 12, 0);
  
  QLabel *titleLbl = new QLabel("System Permissions", titleBar);
  titleLbl->setStyleSheet("color: #a0a6b4; font-size: 13px; font-weight: bold;");
  titleLbl->setAlignment(Qt::AlignCenter);
  titleLayout->addWidget(titleLbl);
  
  m_mainLayout->addWidget(titleBar);

  // Header
  QWidget *header = new QWidget(centralWidget);
  header->setObjectName("HeaderBackground");
  QVBoxLayout *headerLayout = new QVBoxLayout(header);
  headerLayout->setContentsMargins(20, 16, 20, 16);
  QLabel *headerTitle = new QLabel("Permissions Required", header);
  headerTitle->setStyleSheet(
      "font-size: 18px; font-weight: bold; color: #e0e5f0;");
  QLabel *headerDesc = new QLabel(
      "ScreenCut requires certain system permissions to capture your screen, "
      "record audio, and save files. Please grant them below.",
      header);
  headerDesc->setStyleSheet("font-size: 13px; color: #a0a6b4;");
  headerDesc->setWordWrap(true);
  headerLayout->addWidget(headerTitle);
  headerLayout->addWidget(headerDesc);
  m_mainLayout->addWidget(header);

  // List
  QWidget *listContainer = new QWidget(centralWidget);
  QVBoxLayout *listLayout = new QVBoxLayout(listContainer);
  listLayout->setContentsMargins(0, 0, 0, 0);
  listLayout->setSpacing(1); // 1px spacing for border lines
  listContainer->setStyleSheet("background-color: transparent;");

  m_rows.append(new PermissionRow(
      "Screen Capture",
      "Required to capture screen and record videos. (macOS 10.15+)",
      "ScreenCapture", Platform::checkScreenCapturePermission, listContainer));
  m_rows.append(new PermissionRow(
      "Microphone", "Required to record audio from an external source.",
      "Microphone", Platform::checkMicrophonePermission, listContainer));
  m_rows.append(new PermissionRow(
      "Desktop Folder",
      "Required to save screenshots and recordings to your Desktop.",
      "FilesAndFolders", Platform::checkDesktopFolderPermission,
      listContainer));
  m_rows.append(new PermissionRow(
      "Download Folder",
      "Required to save screenshots and recordings to your Downloads.",
      "FilesAndFolders", Platform::checkDownloadFolderPermission,
      listContainer));

  for (PermissionRow *row : m_rows) {
    row->setObjectName("PermissionRowWidget");
    listLayout->addWidget(row);
  }

  m_mainLayout->addWidget(listContainer);
  m_mainLayout->addStretch();
  
  // Bottom Done Row
  QWidget *bottomContainer = new QWidget(centralWidget);
  QHBoxLayout *bottomLayout = new QHBoxLayout(bottomContainer);
  bottomLayout->setContentsMargins(0, 8, 20, 16);
  bottomLayout->addStretch();

  QPushButton *btnDone = new QPushButton("✔ Close", bottomContainer);
  btnDone->setObjectName("BtnDone");
  btnDone->setCursor(Qt::PointingHandCursor);
  connect(btnDone, &QPushButton::clicked, this, &PermissionsWindow::accept);
  bottomLayout->addWidget(btnDone);

  m_mainLayout->addWidget(bottomContainer);
}

void PermissionsWindow::setupStyleSheet() {
  setStyleSheet(R"(
        PermissionsWindow {
            background: transparent;
        }
        #CentralWidget {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                stop:0 rgb(42, 45, 52), 
                stop:0.5 rgb(30, 32, 38),
                stop:1 rgb(22, 24, 28));
            border: 1px solid rgba(255, 255, 255, 14%);
            border-top: 1px solid rgba(255, 255, 255, 32%); 
            border-radius: 14px;
            color: #e0e5f0;
        }
        QWidget#HeaderBackground {
            background: transparent;
            border-bottom: 1px solid rgba(255, 255, 255, 10%);
        }
        QWidget#PermissionRowWidget {
            background: transparent;
            border-bottom: 1px solid rgba(255, 255, 255, 5%);
        }
        QWidget#PermissionRowWidget:hover {
            background-color: #22252e;
        }
        QPushButton#BtnOpenSettings {
            background-color: #2e323e;
            color: #e0e5f0;
            border: 1px solid #4a5060;
            border-radius: 4px;
            padding: 6px 12px;
            font-size: 12px;
            font-weight: bold;
        }
        QPushButton#BtnOpenSettings:hover {
            background-color: #3e4454;
            border-color: #00a8ff;
        }
    )");
}

void PermissionsWindow::changeEvent(QEvent *event) {
  QDialog::changeEvent(event);
}

void PermissionsWindow::showEvent(QShowEvent *event) {
  QDialog::showEvent(event);
}

} // namespace ScreenCut
