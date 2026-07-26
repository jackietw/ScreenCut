/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "editor_thumbs.h"
#include "../core/common_project.h"
#include <QDir>
#include <QFileInfoList>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QIcon>
#include <QDebug>

namespace ScreenCut {

EditorThumbsStrip::EditorThumbsStrip(QWidget* parent) : QWidget(parent) {
    setupUI();
}

void EditorThumbsStrip::setupUI() {
    setFixedHeight(72);
    setStyleSheet("QWidget { background-color: #0f172a; }");
    
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(12, 4, 12, 4);
    mainLayout->setSpacing(8);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setStyleSheet(R"(
        QScrollArea { border: none; background: transparent; }
        QScrollBar:horizontal { height: 6px; background: #1e293b; }
        QScrollBar::handle:horizontal { background: #475569; border-radius: 3px; }
    )");

    m_containerWidget = new QWidget(m_scrollArea);
    m_containerWidget->setStyleSheet("background: transparent;");
    m_containerLayout = new QHBoxLayout(m_containerWidget);
    m_containerLayout->setContentsMargins(0, 0, 0, 0);
    m_containerLayout->setSpacing(8);
    m_scrollArea->setWidget(m_containerWidget);
    
    mainLayout->addWidget(m_scrollArea, 1);

    QPushButton* refreshBtn = new QPushButton("🔄", this);
    refreshBtn->setFixedSize(28, 62);
    refreshBtn->setToolTip("Refresh Recent Thumbnails");
    refreshBtn->setCursor(Qt::PointingHandCursor);
    refreshBtn->setStyleSheet(R"(
        QPushButton { 
            background: #1e293b; border: 1px solid #334155; border-radius: 4px; font-size: 13px; 
        } 
        QPushButton:hover { border-color: #3b82f6; }
    )");
    connect(refreshBtn, &QPushButton::clicked, this, &EditorThumbsStrip::refreshLibrary);
    mainLayout->addWidget(refreshBtn);
}

void EditorThumbsStrip::refreshLibrary() {
    // Clear existing
    QLayoutItem* child;
    while ((child = m_containerLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }
    m_items.clear();

    QString libraryDir = ScutProject::getLibraryDir();
    QDir dir(libraryDir);
    if (!dir.exists()) return;

    QFileInfoList files = dir.entryInfoList(QStringList() << "*.scut", QDir::Files, QDir::Time);
    
    int count = 0;
    for (const QFileInfo& fi : files) {
        if (count >= 25) break; // limit to 25 recents
        
        QFile file(fi.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly)) continue;
        
        QByteArray data = file.readAll();
        file.close();
        
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isObject()) continue;
        
        QString thumbB64 = doc.object()["thumbnail_base64"].toString();
        if (thumbB64.isEmpty()) {
            thumbB64 = doc.object()["image_base64"].toString();
        }
        
        QPixmap pix;
        if (!thumbB64.isEmpty()) {
            QByteArray ba = QByteArray::fromBase64(thumbB64.toLatin1());
            pix.loadFromData(ba, "PNG");
        }
        
        if (!pix.isNull()) {
            QPushButton* btn = new QPushButton(m_containerWidget);
            btn->setFixedSize(96, 58);
            btn->setCursor(Qt::PointingHandCursor);
            btn->setIconSize(QSize(96, 58));
            
            QPixmap thumb = pix.scaled(96, 58, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            // Center crop
            if (thumb.width() > 96 || thumb.height() > 58) {
                thumb = thumb.copy((thumb.width() - 96) / 2, (thumb.height() - 58) / 2, 96, 58);
            }
            btn->setIcon(QIcon(thumb));
            
            QString path = fi.absoluteFilePath();
            connect(btn, &QPushButton::clicked, this, [this, path]() {
                emit fileSelected(path);
            });
            
            m_containerLayout->addWidget(btn);
            m_items.push_back({path, btn});
            count++;
        }
    }
    m_containerLayout->addStretch();
    setCurrentFile(m_currentFilePath);
}

void EditorThumbsStrip::setCurrentFile(const QString& filePath) {
    m_currentFilePath = filePath;
    for (const auto& item : m_items) {
        if (item.filePath == filePath) {
            item.button->setStyleSheet(R"(
                QPushButton { background: #1e293b; border: 3px solid #3b82f6; border-radius: 8px; padding: 0px; }
            )");
        } else {
            item.button->setStyleSheet(R"(
                QPushButton { background: #1e293b; border: 1px solid #334155; border-radius: 8px; padding: 0px; } 
                QPushButton:hover { border-color: #60a5fa; }
            )");
        }
    }
}

} // namespace ScreenCut
