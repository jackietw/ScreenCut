/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "editor_colorpicker.h"
#include <QPainter>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QLinearGradient>
#include "../resources/IconUtils.h"

namespace ScreenCut {

// --- SVPickerArea ---

SVPickerArea::SVPickerArea(QWidget* parent) : QWidget(parent) {
    setFixedSize(180, 180);
    setCursor(Qt::CrossCursor);
}

void SVPickerArea::setHue(int hue) {
    if (m_hue != hue) {
        m_hue = hue;
        update();
    }
}

void SVPickerArea::setSV(int s, int v) {
    if (m_sat != s || m_val != v) {
        m_sat = s;
        m_val = v;
        update();
    }
}

void SVPickerArea::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    
    // Background color based on Hue
    QColor hueColor = QColor::fromHsv(m_hue, 255, 255);
    painter.fillRect(rect(), hueColor);
    
    // Saturation gradient (white to transparent, left to right)
    QLinearGradient satGrad(0, 0, width(), 0);
    satGrad.setColorAt(0, Qt::white);
    satGrad.setColorAt(1, Qt::transparent);
    painter.fillRect(rect(), satGrad);
    
    // Value gradient (transparent to black, top to bottom)
    QLinearGradient valGrad(0, 0, 0, height());
    valGrad.setColorAt(0, Qt::transparent);
    valGrad.setColorAt(1, Qt::black);
    painter.fillRect(rect(), valGrad);
    
    // Draw border
    painter.setPen(QPen(QColor(85, 85, 85), 1));
    painter.drawRect(0, 0, width() - 1, height() - 1);
    
    // Draw handle
    int px = m_sat * width() / 255.0;
    int py = (255 - m_val) * height() / 255.0;
    
    painter.setPen(QPen(Qt::white, 2));
    painter.drawEllipse(QPoint(px, py), 4, 4);
    painter.setPen(QPen(Qt::black, 1));
    painter.drawEllipse(QPoint(px, py), 5, 5);
}

void SVPickerArea::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        updateFromMouse(event->pos());
    }
}

void SVPickerArea::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        updateFromMouse(event->pos());
    }
}

void SVPickerArea::updateFromMouse(const QPoint& pos) {
    int x = qBound(0, pos.x(), width());
    int y = qBound(0, pos.y(), height());
    
    m_sat = (x * 255.0) / width();
    m_val = 255 - ((y * 255.0) / height());
    
    update();
    emit svChanged(m_sat, m_val);
}

// --- HueSliderArea ---

HueSliderArea::HueSliderArea(QWidget* parent) : QWidget(parent) {
    setFixedSize(16, 180);
    setCursor(Qt::PointingHandCursor);
}

void HueSliderArea::setHue(int hue) {
    if (m_hue != hue) {
        m_hue = hue;
        update();
    }
}

void HueSliderArea::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    
    QLinearGradient grad(0, 0, 0, height());
    grad.setColorAt(0.0, QColor(255, 0, 0));
    grad.setColorAt(1.0/6.0, QColor(255, 0, 255));
    grad.setColorAt(2.0/6.0, QColor(0, 0, 255));
    grad.setColorAt(3.0/6.0, QColor(0, 255, 255));
    grad.setColorAt(4.0/6.0, QColor(0, 255, 0));
    grad.setColorAt(5.0/6.0, QColor(255, 255, 0));
    grad.setColorAt(1.0, QColor(255, 0, 0));
    
    painter.fillRect(rect(), grad);
    
    // Draw border
    painter.setPen(QPen(QColor(85, 85, 85), 1));
    painter.drawRect(0, 0, width() - 1, height() - 1);
    
    // Draw handle
    int py = (359 - m_hue) * height() / 359.0;
    py = qBound(2, py, height() - 2);
    
    painter.setPen(QPen(Qt::white, 2));
    painter.drawRect(0, py - 2, width() - 1, 4);
    painter.setPen(QPen(Qt::black, 1));
    painter.drawRect(1, py - 1, width() - 3, 2);
}

void HueSliderArea::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        updateFromMouse(event->pos());
    }
}

void HueSliderArea::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        updateFromMouse(event->pos());
    }
}

void HueSliderArea::updateFromMouse(const QPoint& pos) {
    int y = qBound(0, pos.y(), height());
    m_hue = 359 - ((y * 359.0) / height());
    update();
    emit hueChanged(m_hue);
}

// --- EditorColorPicker ---

EditorColorPicker::EditorColorPicker(const QColor& initialColor, QWidget* parent, bool allowTransparent) 
    : QWidget(parent), m_color(initialColor), m_allowTransparent(allowTransparent)
{
    setupUI();
    setColor(m_color);
}

QColor EditorColorPicker::color() const {
    return m_color;
}

void EditorColorPicker::setColor(const QColor& color) {
    if (m_updating) return;
    
    m_color = color;
    
    m_updating = true;
    if (m_color == Qt::transparent) {
        m_svPicker->setHue(0);
        m_svPicker->setSV(0, 255);
        m_hueSlider->setHue(0);
    } else {
        int h = m_color.hue();
        if (h < 0) h = 0;
        m_svPicker->setHue(h);
        m_svPicker->setSV(m_color.saturation(), m_color.value());
        m_hueSlider->setHue(h);
    }
    updateInputs();
    m_updating = false;
    
    emit colorChanged(m_color);
}

void EditorColorPicker::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(12);

    // 1. Preset colors grid
    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->setSpacing(6);
    
    QList<QColor> presets = {
        Qt::white, QColor("#a9c1c9"), QColor("#ff5c5c"), QColor("#00cc99"), QColor("#7db2ff"), QColor("#b2e040"),
        Qt::black, QColor("#445566"), QColor("#8e44ad"), QColor("#f39c12"), QColor("#ff3399")
    };
    if (m_allowTransparent) {
        presets.append(Qt::transparent);
    } else {
        presets.append(QColor("#888888")); // Fallback color
    }
    
    int row = 0, col = 0;
    for (const QColor& c : presets) {
        QPushButton* btn = new QPushButton();
        btn->setFixedSize(24, 24);
        
        if (c == Qt::transparent) {
            btn->setIcon(ScreenCut::createSvgIcon(ScreenCut::SVG_TRANSPARENT, 24, 24));
            btn->setIconSize(QSize(20, 20));
            btn->setStyleSheet(
                "QPushButton { "
                "background-color: white; "
                "border: 1px solid #555; border-radius: 4px; }"
                "QPushButton:hover { border: 1px solid #fff; }"
            );
        } else {
            btn->setStyleSheet(QString(
                "QPushButton { background-color: %1; border: 1px solid #555; border-radius: 4px; }"
                "QPushButton:hover { border: 1px solid #fff; }"
            ).arg(c.name()));
        }
        
        connect(btn, &QPushButton::clicked, this, [this, c]() {
            onPresetClicked(c);
        });
        
        gridLayout->addWidget(btn, row, col);
        col++;
        if (col >= 6) {
            col = 0;
            row++;
        }
    }
    mainLayout->addLayout(gridLayout);

    // 2. Gradients area
    QHBoxLayout* gradLayout = new QHBoxLayout();
    gradLayout->setSpacing(12);
    
    m_svPicker = new SVPickerArea();
    m_hueSlider = new HueSliderArea();
    
    gradLayout->addWidget(m_svPicker);
    gradLayout->addWidget(m_hueSlider);
    
    connect(m_svPicker, &SVPickerArea::svChanged, this, [this](int s, int v){
        if (m_updating) return;
        m_updating = true;
        
        int h = m_color.hue();
        if (h < 0) h = 0;
        
        m_color = QColor::fromHsv(h, s, v);
        // if user picks a color on SV picker, it cannot be transparent.
        if (m_color.alpha() == 0) {
            m_color.setAlpha(255);
        }
        
        updateInputs();
        m_updating = false;
        emit colorChanged(m_color);
    });
    
    connect(m_hueSlider, &HueSliderArea::hueChanged, this, [this](int h){
        if (m_updating) return;
        m_updating = true;
        
        m_svPicker->setHue(h);
        
        int s = m_color.saturation();
        int v = m_color.value();
        m_color = QColor::fromHsv(h, s, v);
        if (m_color.alpha() == 0) {
            m_color.setAlpha(255);
        }
        
        updateInputs();
        m_updating = false;
        emit colorChanged(m_color);
    });
    
    mainLayout->addLayout(gradLayout);
    
    // 3. RGB inputs
    QHBoxLayout* rgbLayout = new QHBoxLayout();
    rgbLayout->setSpacing(4);
    
    auto createInput = [this](const QString& labelText, QLineEdit*& input) {
        QLabel* lbl = new QLabel(labelText);
        lbl->setStyleSheet("color: #aaa; font-size: 12px;");
        input = new QLineEdit();
        input->setFixedWidth(36);
        input->setStyleSheet("QLineEdit { background: #333; color: white; border: 1px solid #555; border-radius: 2px; padding: 2px; font-size: 12px; }");
        
        connect(input, &QLineEdit::textEdited, this, [this](const QString&) {
            if (m_updating) return;
            m_updating = true;
            
            int r = m_rInput->text().toInt();
            int g = m_gInput->text().toInt();
            int b = m_bInput->text().toInt();
            r = qBound(0, r, 255);
            g = qBound(0, g, 255);
            b = qBound(0, b, 255);
            
            m_color = QColor(r, g, b);
            
            int h = m_color.hue();
            if (h < 0) h = 0;
            
            m_svPicker->setHue(h);
            m_svPicker->setSV(m_color.saturation(), m_color.value());
            m_hueSlider->setHue(h);
            
            m_updating = false;
            emit colorChanged(m_color);
        });
        
        QHBoxLayout* hl = new QHBoxLayout();
        hl->setSpacing(2);
        hl->addWidget(lbl);
        hl->addWidget(input);
        return hl;
    };
    
    rgbLayout->addLayout(createInput("R:", m_rInput));
    rgbLayout->addSpacing(4);
    rgbLayout->addLayout(createInput("G:", m_gInput));
    rgbLayout->addSpacing(4);
    rgbLayout->addLayout(createInput("B:", m_bInput));
    
    rgbLayout->addStretch();
    mainLayout->addLayout(rgbLayout);
}

void EditorColorPicker::updateInputs() {
    if (m_color == Qt::transparent) {
        m_rInput->setText("0");
        m_gInput->setText("0");
        m_bInput->setText("0");
    } else {
        m_rInput->setText(QString::number(m_color.red()));
        m_gInput->setText(QString::number(m_color.green()));
        m_bInput->setText(QString::number(m_color.blue()));
    }
}

void EditorColorPicker::onPresetClicked(const QColor& color) {
    setColor(color);
}

} // namespace ScreenCut
