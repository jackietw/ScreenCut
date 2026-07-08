/**
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "capture_toolbar.h"
#include "../resources/IconUtils.h"
#include <QCursor>

namespace ScreenCut {

CaptureToolBarWidget::CaptureToolBarWidget(QWidget* parent)
    : QWidget(parent) {
    setAttribute(Qt::WA_StyledBackground, true);
    setCursor(Qt::ArrowCursor); // Use arrow cursor instead of crosshair when hovering toolbar
    setupUi();
}

CaptureToolBarWidget::~CaptureToolBarWidget() = default;

void CaptureToolBarWidget::setupUi() {
    setStyleSheet(
        "CaptureToolBarWidget { "
        "   background-color: rgba(26, 29, 36, 245); "
        "   border: 1px solid #384252; "
        "   border-radius: 8px; "
        "}"
        "QToolButton { "
        "   background: transparent; "
        "   border: none; "
        "   border-radius: 6px; "
        "   padding: 6px; "
        "}"
        "QToolButton:hover { "
        "   background-color: #00a8ff; "
        "}"
        "QToolButton:pressed { "
        "   background-color: #0088cc; "
        "}"
    );

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(6);

    // Edit in Studio
    QToolButton* btnEdit = createToolButton("Edit in Studio / Annotate (E)", SVG_EDITOR, SIGNAL(actionEdit()));
    layout->addWidget(btnEdit);

    // Copy to Clipboard
    QToolButton* btnCopy = createToolButton("Copy to Clipboard (Ctrl+C / Enter)", SVG_COPY, SIGNAL(actionCopy()));
    layout->addWidget(btnCopy);

    // Save to File
    QToolButton* btnSave = createToolButton("Save to File (Ctrl+S)", SVG_SAVE, SIGNAL(actionSave()));
    layout->addWidget(btnSave);

    // Pin to Desktop
    QToolButton* btnPin = createToolButton("Pin to Desktop / Stamp (P)", SVG_STAMP, SIGNAL(actionPin()));
    layout->addWidget(btnPin);

    layout->addWidget(createSeparator());

    // Cancel
    QToolButton* btnCancel = createToolButton("Cancel Capture (Esc)", SVG_CANCEL, SIGNAL(actionCancel()));
    layout->addWidget(btnCancel);

    // Done / Confirm
    QToolButton* btnDone = createToolButton("Confirm & Copy (Enter)", SVG_DONE, SIGNAL(actionCopy()));
    layout->addWidget(btnDone);

    adjustSize();
}

QToolButton* CaptureToolBarWidget::createToolButton(const QString& tooltip, const QString& svgIconString, const char* slotSignal) {
    QToolButton* btn = new QToolButton(this);
    btn->setIcon(createSvgIcon(svgIconString, 20, 20));
    btn->setIconSize(QSize(20, 20));
    btn->setToolTip(tooltip);
    btn->setFixedSize(34, 34);
    btn->setCursor(Qt::PointingHandCursor);
    connect(btn, SIGNAL(clicked()), this, slotSignal);
    return btn;
}

QFrame* CaptureToolBarWidget::createSeparator() {
    QFrame* line = new QFrame(this);
    line->setFrameShape(QFrame::VLine);
    line->setFrameShadow(QFrame::Plain);
    line->setStyleSheet("color: #384252; background-color: #384252; max-width: 1px; margin: 4px 2px;");
    return line;
}

} // namespace ScreenCut
