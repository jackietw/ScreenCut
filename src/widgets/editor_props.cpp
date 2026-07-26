/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "editor_props.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QGridLayout>
#include <QComboBox>
#include <QFontComboBox>

namespace ScreenCut {

EditorPropsPanel::EditorPropsPanel(QWidget* parent) : QWidget(parent) {
    setupUI();
}

void EditorPropsPanel::setupUI() {
    setFixedWidth(240);
    setStyleSheet(R"(
        QWidget { background-color: #252525; color: #ffffff; }
        QLabel { color: #dddddd; font-size: 13px; font-weight: bold; }
        QSlider::groove:horizontal { height: 6px; background: #3c3c3c; border-radius: 3px; }
        QSlider::handle:horizontal { width: 14px; margin: -4px 0; background: #246bb2; border-radius: 7px; }
        QComboBox, QFontComboBox, QSpinBox { background: #333333; border: 1px solid #555555; border-radius: 4px; padding: 4px; color: white; }
        QPushButton { background: #333333; border: 1px solid #555555; border-radius: 4px; padding: 6px; color: white; font-weight: bold; }
        QPushButton:hover { background: #444444; border-color: #246bb2; }
    )");

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(15, 20, 15, 20);
    m_mainLayout->setSpacing(15);

    m_toolNameLabel = new QLabel("Properties", this);
    m_toolNameLabel->setStyleSheet("font-size: 16px; color: #246bb2; border-bottom: 1px solid #3c3c3c; padding-bottom: 8px;");
    m_mainLayout->addWidget(m_toolNameLabel);

    createQuickStyles();
    
    createArrowProps();
    createShapeProps();
    createTextProps();
    createBlurProps();
    createPenProps();
    
    createColorPalette();
    createSizeSlider();
    
    createStepProps();

    m_mainLayout->addStretch();
    
    updateVisibility(ToolType::Arrow);
}

void EditorPropsPanel::createQuickStyles() {
    m_quickStylesWidget = new QWidget(this);
    QVBoxLayout* l = new QVBoxLayout(m_quickStylesWidget);
    l->setContentsMargins(0,0,0,0);
    l->addWidget(new QLabel("Quick Styles", m_quickStylesWidget));
    
    QHBoxLayout* hl = new QHBoxLayout();
    QList<QPair<QColor, int>> presets = {
        {QColor("#ff3333"), 4}, {QColor("#ffcc00"), 6}, {QColor("#33cc33"), 4}, {QColor("#3399ff"), 4}, {QColor("#ffffff"), 3}
    };
    for (auto& preset : presets) {
        QPushButton* btn = new QPushButton(m_quickStylesWidget);
        btn->setFixedSize(32, 32);
        btn->setStyleSheet(QString("background-color: %1; border-radius: 16px; border: 2px solid #555;").arg(preset.first.name()));
        connect(btn, &QPushButton::clicked, this, [this, preset]() {
            emit colorChanged(preset.first);
            emit lineWidthChanged(preset.second);
            syncFromSelection(preset.first, preset.second);
        });
        hl->addWidget(btn);
    }
    l->addLayout(hl);
    m_mainLayout->addWidget(m_quickStylesWidget);
}

void EditorPropsPanel::createColorPalette() {
    m_colorWidget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(m_colorWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addWidget(new QLabel("Color", m_colorWidget));

    QGridLayout* grid = new QGridLayout();
    grid->setSpacing(6);
    QList<QColor> colors = {
        QColor(255, 59, 48), QColor(255, 149, 0), QColor(255, 204, 0), QColor(76, 217, 100), QColor(90, 200, 250),
        QColor(0, 122, 255), QColor(88, 86, 214), QColor(255, 45, 85), QColor(255, 255, 255), QColor(0, 0, 0)
    };

    int row = 0, col = 0;
    for (const QColor& color : colors) {
        QPushButton* btn = new QPushButton(m_colorWidget);
        btn->setFixedSize(24, 24);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setProperty("color", color);
        btn->setStyleSheet(QString("background-color: %1; border: 1px solid #333; border-radius: 12px;").arg(color.name()));
        
        connect(btn, &QPushButton::clicked, this, [this, color]() {
            emit colorChanged(color);
            syncFromSelection(color, m_currentWidth);
        });
        m_colorButtons.push_back(btn);
        grid->addWidget(btn, row, col);
        col++; if (col >= 5) { col = 0; row++; }
    }
    layout->addLayout(grid);
    m_mainLayout->addWidget(m_colorWidget);
}

void EditorPropsPanel::createSizeSlider() {
    m_sizeWidget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(m_sizeWidget);
    layout->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout* titleLayout = new QHBoxLayout();
    m_widthTitle = new QLabel("Line Width", m_sizeWidget);
    m_widthLabel = new QLabel(QString::number(m_currentWidth) + "px", m_sizeWidget);
    m_widthLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    titleLayout->addWidget(m_widthTitle);
    titleLayout->addWidget(m_widthLabel);
    layout->addLayout(titleLayout);

    m_widthSlider = new QSlider(Qt::Horizontal, m_sizeWidget);
    m_widthSlider->setRange(1, 40);
    m_widthSlider->setValue(m_currentWidth);
    connect(m_widthSlider, &QSlider::valueChanged, this, [this](int value) {
        m_currentWidth = value;
        m_widthLabel->setText(QString::number(value));
        emit lineWidthChanged(value);
        emit blurIntensityChanged(value);
        emit fontSizeChanged(value);
    });
    layout->addWidget(m_widthSlider);
    m_mainLayout->addWidget(m_sizeWidget);
}

void EditorPropsPanel::createArrowProps() {
    m_arrowWidget = new QWidget(this);
    QVBoxLayout* l = new QVBoxLayout(m_arrowWidget);
    l->setContentsMargins(0,0,0,0);
    l->addWidget(new QLabel("Arrow Type", m_arrowWidget));
    m_arrowTypeCombo = new QComboBox(m_arrowWidget);
    m_arrowTypeCombo->addItems({"Single Arrow", "Double Arrow", "Plain Line"});
    connect(m_arrowTypeCombo, &QComboBox::currentTextChanged, this, &EditorPropsPanel::arrowTypeChanged);
    l->addWidget(m_arrowTypeCombo);
    m_mainLayout->addWidget(m_arrowWidget);
}

void EditorPropsPanel::createShapeProps() {
    m_shapeWidget = new QWidget(this);
    QVBoxLayout* l = new QVBoxLayout(m_shapeWidget);
    l->setContentsMargins(0,0,0,0);
    l->addWidget(new QLabel("Shape Type", m_shapeWidget));
    m_shapeStyleCombo = new QComboBox(m_shapeWidget);
    m_shapeStyleCombo->addItems({"Rectangle", "Rounded Rectangle", "Ellipse"});
    connect(m_shapeStyleCombo, &QComboBox::currentTextChanged, this, &EditorPropsPanel::shapeStyleChanged);
    l->addWidget(m_shapeStyleCombo);
    
    l->addWidget(new QLabel("Line Style", m_shapeWidget));
    m_lineStyleCombo = new QComboBox(m_shapeWidget);
    m_lineStyleCombo->addItems({"Solid", "Dashed"});
    connect(m_lineStyleCombo, &QComboBox::currentTextChanged, this, &EditorPropsPanel::lineStyleChanged);
    l->addWidget(m_lineStyleCombo);
    m_mainLayout->addWidget(m_shapeWidget);
}

void EditorPropsPanel::createTextProps() {
    m_textWidget = new QWidget(this);
    QVBoxLayout* l = new QVBoxLayout(m_textWidget);
    l->setContentsMargins(0,0,0,0);
    l->addWidget(new QLabel("Font Family", m_textWidget));
    m_fontCombo = new QFontComboBox(m_textWidget);
    connect(m_fontCombo, &QFontComboBox::currentFontChanged, this, [this](const QFont& f){ emit fontFamilyChanged(f.family()); });
    l->addWidget(m_fontCombo);
    m_mainLayout->addWidget(m_textWidget);
}

void EditorPropsPanel::createBlurProps() {
    m_blurWidget = new QWidget(this);
    QVBoxLayout* l = new QVBoxLayout(m_blurWidget);
    l->setContentsMargins(0,0,0,0);
    l->addWidget(new QLabel("Blur Effect", m_blurWidget));
    m_blurTypeCombo = new QComboBox(m_blurWidget);
    m_blurTypeCombo->addItems({"Mosaic", "Gaussian Blur"});
    connect(m_blurTypeCombo, &QComboBox::currentTextChanged, this, [this](const QString& t){
        emit blurTypeChanged((t == "Mosaic") ? ToolType::Mosaic : ToolType::Blur);
    });
    l->addWidget(m_blurTypeCombo);
    m_mainLayout->addWidget(m_blurWidget);
}

void EditorPropsPanel::createPenProps() {
    m_penWidget = new QWidget(this);
    QVBoxLayout* l = new QVBoxLayout(m_penWidget);
    l->setContentsMargins(0,0,0,0);
    l->addWidget(new QLabel("Pen Mode", m_penWidget));
    m_penStyleCombo = new QComboBox(m_penWidget);
    m_penStyleCombo->addItems({"Solid Pen", "Highlighter"});
    connect(m_penStyleCombo, &QComboBox::currentTextChanged, this, &EditorPropsPanel::penStyleChanged);
    l->addWidget(m_penStyleCombo);
    m_mainLayout->addWidget(m_penWidget);
}

void EditorPropsPanel::createStepProps() {
    m_stepWidget = new QWidget(this);
    QVBoxLayout* l = new QVBoxLayout(m_stepWidget);
    l->setContentsMargins(0,0,0,0);
    QPushButton* resetBtn = new QPushButton("Reset Step Counter (1)", m_stepWidget);
    connect(resetBtn, &QPushButton::clicked, this, &EditorPropsPanel::resetStepCounter);
    l->addWidget(resetBtn);
    m_mainLayout->addWidget(m_stepWidget);
}

void EditorPropsPanel::setCurrentTool(ToolType type) {
    updateVisibility(type);
    
    QString title = "Properties";
    switch (type) {
        case ToolType::None: title = "Selection"; break;
        case ToolType::Arrow: title = "Arrow"; break;
        case ToolType::Rectangle: 
        case ToolType::Ellipse: title = "Shape"; break;
        case ToolType::Freehand: title = "Freehand"; break;
        case ToolType::Text: title = "Text"; break;
        case ToolType::StepMarker: title = "Step Marker"; break;
        case ToolType::Mosaic: 
        case ToolType::Blur: title = "Blur / Pixelate"; break;
        case ToolType::Highlight: title = "Highlight"; break;
    }
    m_toolNameLabel->setText(title);
}

void EditorPropsPanel::updateVisibility(ToolType type) {
    bool showColor = true;
    bool showSize = true;
    QString sizeTitle = "Line Width";

    m_arrowWidget->setVisible(false);
    m_shapeWidget->setVisible(false);
    m_textWidget->setVisible(false);
    m_blurWidget->setVisible(false);
    m_penWidget->setVisible(false);
    m_stepWidget->setVisible(false);

    switch (type) {
        case ToolType::None: showColor = false; showSize = false; break;
        case ToolType::Arrow: m_arrowWidget->setVisible(true); break;
        case ToolType::Rectangle:
        case ToolType::Ellipse: m_shapeWidget->setVisible(true); break;
        case ToolType::Text: 
            m_textWidget->setVisible(true); 
            sizeTitle = "Font Size"; 
            break;
        case ToolType::Freehand: 
            m_penWidget->setVisible(true); 
            break;
        case ToolType::StepMarker: 
            m_stepWidget->setVisible(true); 
            sizeTitle = "Sticker Size"; 
            break;
        case ToolType::Mosaic:
        case ToolType::Blur: 
            m_blurWidget->setVisible(true); 
            showColor = false; 
            sizeTitle = "Intensity"; 
            break;
        case ToolType::Highlight: break;
    }

    m_colorWidget->setVisible(showColor);
    m_sizeWidget->setVisible(showSize);
    m_widthTitle->setText(sizeTitle);
    
    // Adjust slider range based on property type
    m_widthSlider->blockSignals(true);
    if (sizeTitle == "Font Size") {
        m_widthSlider->setRange(12, 120);
    } else if (sizeTitle == "Intensity" || sizeTitle == "Sticker Size") {
        m_widthSlider->setRange(1, 100);
    } else {
        m_widthSlider->setRange(1, 40);
    }
    m_widthSlider->blockSignals(false);
    
    // Quick styles only if color is supported
    m_quickStylesWidget->setVisible(showColor);
}

void EditorPropsPanel::syncFromSelection(const std::shared_ptr<AnnotationItem>& item) {
    if (!item) return;
    
    updateVisibility(item->getType());
    m_toolNameLabel->setText("Selected Object");
    
    syncFromSelection(item->color, item->lineWidth);
    
    // Set combos without triggering signals
    m_arrowTypeCombo->blockSignals(true);
    m_shapeStyleCombo->blockSignals(true);
    m_lineStyleCombo->blockSignals(true);
    m_fontCombo->blockSignals(true);
    m_blurTypeCombo->blockSignals(true);
    m_penStyleCombo->blockSignals(true);
    
    if (item->getType() == ToolType::Arrow) {
        m_arrowTypeCombo->setCurrentText(std::static_pointer_cast<ArrowAnnotation>(item)->arrowType);
    } else if (item->getType() == ToolType::Rectangle || item->getType() == ToolType::Ellipse) {
        auto shape = std::static_pointer_cast<ShapeAnnotation>(item);
        m_shapeStyleCombo->setCurrentText(shape->shapeStyle);
        m_lineStyleCombo->setCurrentText(shape->lineStyle);
    } else if (item->getType() == ToolType::Text) {
        auto txt = std::static_pointer_cast<TextAnnotation>(item);
        m_fontCombo->setCurrentFont(QFont(txt->fontFamily));
        m_widthSlider->blockSignals(true);
        m_widthSlider->setValue(txt->fontSize);
        m_widthLabel->setText(QString::number(txt->fontSize));
        m_widthSlider->blockSignals(false);
    } else if (item->getType() == ToolType::Freehand) {
        m_penStyleCombo->setCurrentText(std::static_pointer_cast<FreehandAnnotation>(item)->penStyle);
    } else if (item->getType() == ToolType::Mosaic || item->getType() == ToolType::Blur) {
        auto shader = std::static_pointer_cast<ShaderAnnotation>(item);
        m_blurTypeCombo->setCurrentText((shader->shaderType == ToolType::Mosaic) ? "Mosaic" : "Gaussian Blur");
        m_widthSlider->blockSignals(true);
        m_widthSlider->setValue(shader->intensity);
        m_widthLabel->setText(QString::number(shader->intensity));
        m_widthSlider->blockSignals(false);
    }
    
    m_arrowTypeCombo->blockSignals(false);
    m_shapeStyleCombo->blockSignals(false);
    m_lineStyleCombo->blockSignals(false);
    m_fontCombo->blockSignals(false);
    m_blurTypeCombo->blockSignals(false);
    m_penStyleCombo->blockSignals(false);
}

void EditorPropsPanel::syncFromSelection(const QColor& color, int width) {
    m_selectedColor = color;
    m_currentWidth = width;

    m_widthSlider->blockSignals(true);
    m_widthSlider->setValue(width);
    m_widthLabel->setText(QString::number(width));
    m_widthSlider->blockSignals(false);

    for (QPushButton* btn : m_colorButtons) {
        QColor c = btn->property("color").value<QColor>();
        if (c == color) {
            btn->setStyleSheet(QString("background-color: %1; border: 2px solid white; border-radius: 12px;").arg(c.name()));
        } else {
            btn->setStyleSheet(QString("background-color: %1; border: 1px solid #333; border-radius: 12px;").arg(c.name()));
        }
    }
}

} // namespace ScreenCut
