/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "editor_shadowpicker.h"
#include "editor_colorpicker.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QDial>
#include <QSlider>
#include <QLineEdit>
#include <QMenu>
#include <QWidgetAction>
#include <QIntValidator>
#include <QPushButton>

namespace ScreenCut {

// ============================================================================
// ShadowGridWidget
// ============================================================================
ShadowGridWidget::ShadowGridWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
}

void ShadowGridWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QFrame* container = new QFrame(this);
    container->setStyleSheet(
        "QFrame {"
        "  background-color: #555555;"
        "  border: 1px solid #555555;"
        "  border-radius: 6px;"
        "}"
    );

    QGridLayout* layout = new QGridLayout(container);
    layout->setContentsMargins(1, 1, 1, 1);
    layout->setSpacing(1);
    
    QString baseBtnStyle = 
        "QPushButton {"
        "  background-color: #3b3b3b;"
        "  border: none;"
        "  color: #fff;"
        "  font-size: 10px;"
        "  font-weight: bold;"
        "  padding: 0px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #4b4b4b;"
        "}"
        "QPushButton:checked {"
        "  background-color: #1a73e8;"
        "}";

    // Create 3x3 grid
    for (int i = 0; i < 9; ++i) {
        QPushButton* btn = new QPushButton(container);
        btn->setFixedSize(12, 12);
        btn->setCheckable(true);
        
        QString cornerStyle = "border-radius: 0px;";
        if (i == 0) cornerStyle = "border-top-left-radius: 5px;";
        else if (i == 2) cornerStyle = "border-top-right-radius: 5px;";
        else if (i == 6) cornerStyle = "border-bottom-left-radius: 5px;";
        else if (i == 8) cornerStyle = "border-bottom-right-radius: 5px;";
        
        btn->setStyleSheet(baseBtnStyle + "QPushButton { " + cornerStyle + " }");
        
        layout->addWidget(btn, i / 3, i % 3);
        m_cells[i] = btn;
        
        connect(btn, &QPushButton::clicked, this, [this, i]() {
            if (m_updating) return;
            
            // Map cell to angle
            // 0 1 2   TopLeft(225) Top(270) TopRight(315)
            // 3 4 5   Left(180)    None     Right(0)
            // 6 7 8   BotLeft(135) Bot(90)  BotRight(45)
            
            if (i == 4) { // Center
                m_style.enabled = false;
            } else {
                m_style.enabled = true;
                if (i == 0) m_style.angle = 225;
                else if (i == 1) m_style.angle = 270;
                else if (i == 2) m_style.angle = 315;
                else if (i == 3) m_style.angle = 180;
                else if (i == 5) m_style.angle = 0;
                else if (i == 6) m_style.angle = 135;
                else if (i == 7) m_style.angle = 90;
                else if (i == 8) m_style.angle = 45;
            }
            
            updateGridUI();
            emit shadowStyleChanged(m_style);
        });
    }
    mainLayout->addWidget(container);
}

void ShadowGridWidget::setShadowStyle(const ShadowStyle& style) {
    m_style = style;
    updateGridUI();
}

ShadowStyle ShadowGridWidget::shadowStyle() const {
    return m_style;
}

void ShadowGridWidget::updateGridUI() {
    m_updating = true;
    int activeIndex = 4; // Center by default
    
    if (m_style.enabled) {
        // Find closest preset angle
        int a = m_style.angle % 360;
        if (a < 0) a += 360;
        
        // Map back to cell
        if (a >= 202 && a < 247) activeIndex = 0;
        else if (a >= 247 && a < 292) activeIndex = 1;
        else if (a >= 292 && a < 337) activeIndex = 2;
        else if (a >= 157 && a < 202) activeIndex = 3;
        else if (a >= 337 || a < 22) activeIndex = 5;
        else if (a >= 112 && a < 157) activeIndex = 6;
        else if (a >= 67 && a < 112) activeIndex = 7;
        else if (a >= 22 && a < 67) activeIndex = 8;
    }
    
    for (int i = 0; i < 9; ++i) {
        m_cells[i]->setChecked(i == activeIndex);
        m_cells[i]->setText((i == activeIndex && i != 4) ? "✓" : "");
    }
    m_updating = false;
}


// ============================================================================
// ShadowAdvancedPopup
// ============================================================================
ShadowAdvancedPopup::ShadowAdvancedPopup(const ShadowStyle& initialStyle, QWidget* parent) 
    : QWidget(parent), m_style(initialStyle) 
{
    setupUI();
    updateUI();
}

void ShadowAdvancedPopup::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);
    
    auto createRow = [this](const QString& labelText, QWidget* control1, QWidget* control2) {
        QHBoxLayout* row = new QHBoxLayout();
        QLabel* label = new QLabel(labelText, this);
        label->setFixedWidth(60);
        label->setStyleSheet("color: #ccc; font-size: 13px;");
        row->addWidget(label);
        row->addWidget(control1);
        if (control2) row->addWidget(control2);
        return row;
    };
    
    // Color & Angle Row
    QHBoxLayout* colorAngleRow = new QHBoxLayout();
    
    QLabel* colorLabel = new QLabel("Color", this);
    colorLabel->setStyleSheet("color: #ccc; font-size: 13px;");
    m_btnColor = new QPushButton(this);
    m_btnColor->setFixedSize(40, 24);
    
    connect(m_btnColor, &QPushButton::clicked, this, [this]() {
        QMenu* menu = new QMenu(this);
        menu->setStyleSheet("QMenu { background: #252525; border: 1px solid #444; border-radius: 8px; }");
        EditorColorPicker* picker = new EditorColorPicker(m_style.color, menu);
        connect(picker, &EditorColorPicker::colorChanged, [this](const QColor& c){
            m_style.color = c;
            m_btnColor->setStyleSheet(QString("border: 1px solid #777; border-radius: 4px; background-color: %1;").arg(c.name()));
            emit shadowStyleChanged(m_style);
        });
        QWidgetAction* action = new QWidgetAction(menu);
        action->setDefaultWidget(picker);
        menu->addAction(action);
        menu->exec(m_btnColor->mapToGlobal(QPoint(0, m_btnColor->height() + 2)));
        menu->deleteLater();
    });
    
    colorAngleRow->addWidget(colorLabel);
    colorAngleRow->addWidget(m_btnColor);
    colorAngleRow->addSpacing(10);
    
    QLabel* angleLabel = new QLabel("Angle", this);
    angleLabel->setStyleSheet("color: #ccc; font-size: 13px;");
    
    m_dialAngle = new QDial(this);
    m_dialAngle->setRange(0, 359);
    m_dialAngle->setFixedSize(30, 30);
    m_dialAngle->setWrapping(true);
    
    m_editAngle = new QLineEdit(this);
    m_editAngle->setFixedWidth(40);
    m_editAngle->setAlignment(Qt::AlignRight);
    m_editAngle->setStyleSheet("background: #333; color: #fff; border: 1px solid #555; border-radius: 2px;");
    m_editAngle->setValidator(new QIntValidator(0, 360, this));
    
    colorAngleRow->addWidget(angleLabel);
    colorAngleRow->addWidget(m_dialAngle);
    colorAngleRow->addWidget(m_editAngle);
    
    layout->addLayout(colorAngleRow);
    
    // Sliders
    auto setupSlider = [this](QSlider*& slider, QLineEdit*& edit, int min, int max) {
        slider = new QSlider(Qt::Horizontal, this);
        slider->setRange(min, max);
        edit = new QLineEdit(this);
        edit->setFixedWidth(40);
        edit->setAlignment(Qt::AlignRight);
        edit->setStyleSheet("background: #333; color: #fff; border: 1px solid #555; border-radius: 2px;");
        edit->setValidator(new QIntValidator(min, max, this));
        
        connect(slider, &QSlider::valueChanged, this, [edit](int val){
            edit->setText(QString::number(val));
        });
        connect(edit, &QLineEdit::editingFinished, this, [slider, edit, min, max](){
            int val = edit->text().toInt();
            if (val < min) val = min;
            if (val > max) val = max;
            slider->setValue(val);
            edit->setText(QString::number(val));
        });
    };
    
    setupSlider(m_sliderDistance, m_editDistance, 0, 100);
    layout->addLayout(createRow("Distance", m_sliderDistance, m_editDistance));
    
    setupSlider(m_sliderOpacity, m_editOpacity, 0, 100);
    layout->addLayout(createRow("Opacity", m_sliderOpacity, m_editOpacity));
    
    setupSlider(m_sliderBlur, m_editBlur, 0, 100);
    layout->addLayout(createRow("Blur", m_sliderBlur, m_editBlur));
    
    // Connections for dial and sliders to update m_style
    auto updateProp = [this]() {
        if (m_updating) return;
        m_style.angle = m_dialAngle->value();
        m_style.distance = m_sliderDistance->value();
        m_style.opacity = m_sliderOpacity->value();
        m_style.blur = m_sliderBlur->value();
        
        if (m_style.distance > 0 || m_style.blur > 0) m_style.enabled = true;
        
        emit shadowStyleChanged(m_style);
    };
    
    connect(m_dialAngle, &QDial::valueChanged, this, [this, updateProp](int val){
        if (!m_updating) m_editAngle->setText(QString::number(val));
        updateProp();
    });
    connect(m_editAngle, &QLineEdit::editingFinished, this, [this](){
        if (!m_updating) {
            int val = m_editAngle->text().toInt() % 360;
            if (val < 0) val += 360;
            m_dialAngle->setValue(val);
            m_editAngle->setText(QString::number(val));
        }
    });
    
    connect(m_sliderDistance, &QSlider::valueChanged, this, updateProp);
    connect(m_sliderOpacity, &QSlider::valueChanged, this, updateProp);
    connect(m_sliderBlur, &QSlider::valueChanged, this, updateProp);
}

void ShadowAdvancedPopup::setShadowStyle(const ShadowStyle& style) {
    m_style = style;
    updateUI();
}

ShadowStyle ShadowAdvancedPopup::shadowStyle() const {
    return m_style;
}

void ShadowAdvancedPopup::updateUI() {
    m_updating = true;
    m_btnColor->setStyleSheet(QString("border: 1px solid #777; border-radius: 4px; background-color: %1;").arg(m_style.color.name()));
    
    m_dialAngle->setValue(m_style.angle);
    m_editAngle->setText(QString::number(m_style.angle));
    
    m_sliderDistance->setValue(m_style.distance);
    m_editDistance->setText(QString::number(m_style.distance));
    
    m_sliderOpacity->setValue(m_style.opacity);
    m_editOpacity->setText(QString::number(m_style.opacity));
    
    m_sliderBlur->setValue(m_style.blur);
    m_editBlur->setText(QString::number(m_style.blur));
    m_updating = false;
}

// ============================================================================
// ShadowPropertyWidget
// ============================================================================
ShadowPropertyWidget::ShadowPropertyWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
}

void ShadowPropertyWidget::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    
    m_titleButton = new QPushButton("Shadow ▼", this);
    m_titleButton->setFlat(true);
    m_titleButton->setStyleSheet("QPushButton { color: #ccc; font-size: 13px; text-align: center; border: none; } QPushButton:hover { color: #fff; }");
    layout->addWidget(m_titleButton, 0, Qt::AlignCenter);
    
    m_grid = new ShadowGridWidget(this);
    layout->addWidget(m_grid, 0, Qt::AlignCenter);
    
    connect(m_grid, &ShadowGridWidget::shadowStyleChanged, this, [this](const ShadowStyle& style) {
        m_style = style;
        emit shadowStyleChanged(m_style);
    });
    
    connect(m_titleButton, &QPushButton::clicked, this, [this]() {
        QMenu* menu = new QMenu(this);
        menu->setStyleSheet("QMenu { background: #2b2b2b; border: 1px solid #444; border-radius: 8px; }");
        
        ShadowAdvancedPopup* popup = new ShadowAdvancedPopup(m_style, menu);
        connect(popup, &ShadowAdvancedPopup::shadowStyleChanged, [this](const ShadowStyle& style) {
            m_style = style;
            m_grid->setShadowStyle(style); // Sync back to grid
            emit shadowStyleChanged(m_style);
        });
        
        QWidgetAction* action = new QWidgetAction(menu);
        action->setDefaultWidget(popup);
        menu->addAction(action);
        
        menu->exec(m_titleButton->mapToGlobal(QPoint(0, m_titleButton->height() + 2)));
        menu->deleteLater();
    });
}

void ShadowPropertyWidget::setShadowStyle(const ShadowStyle& style) {
    m_style = style;
    m_grid->setShadowStyle(style);
}

ShadowStyle ShadowPropertyWidget::shadowStyle() const {
    return m_style;
}

} // namespace ScreenCut
