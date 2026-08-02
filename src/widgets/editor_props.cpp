/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "editor_props.h"
#include "editor_arrow.h"
#include "editor_colorpicker.h"
#include "editor_shadowpicker.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QGridLayout>
#include <QComboBox>
#include <QFontComboBox>
#include <QColorDialog>
#include <QMenu>
#include <QWidgetAction>
#include <QScrollArea>
#include <QDir>
#include <QFile>
#include "../resources/IconUtils.h"

namespace {
    void updateOutlineIcon(QPushButton* btn, const QColor& c) {
        if (!btn) return;
        
        btn->setStyleSheet("border: 1px solid #94a3b8; border-radius: 4px; background-color: #334155; padding: 2px;"); // Base panel background
        
        if (c == Qt::transparent) {
            QString svg = ScreenCut::SVG_TRANSPARENT;
            svg.replace("#ff0000", "#FFFFFF", Qt::CaseInsensitive);
            btn->setIcon(ScreenCut::createSvgIcon(svg, 24, 24));
            btn->setIconSize(QSize(24, 24));
            return;
        }
        
        QPixmap pixmap(32, 32);
        pixmap.fill(Qt::transparent);
        QPainter p(&pixmap);
        
        // Outer white frame (1px)
        p.fillRect(2, 2, 28, 28, Qt::white);
        
        // Color middle
        p.fillRect(3, 3, 26, 26, c);
        
        // Inner white frame (1px)
        p.fillRect(7, 7, 18, 18, Qt::white);
        
        // Inner transparent
        p.setCompositionMode(QPainter::CompositionMode_Clear);
        p.fillRect(8, 8, 16, 16, Qt::transparent);
        p.setCompositionMode(QPainter::CompositionMode_SourceOver);
        
        btn->setIcon(QIcon(pixmap));
        btn->setIconSize(QSize(32, 32));
    }

    void updateFillIcon(QPushButton* btn, const QColor& c) {
        if (!btn) return;
        
        btn->setStyleSheet("border: 1px solid #94a3b8; border-radius: 4px; background-color: #334155; padding: 2px;"); // Base panel background
        
        if (c == Qt::transparent) {
            btn->setIcon(QIcon()); // Just show the base background if transparent
            return;
        }
        
        QPixmap pixmap(32, 32);
        pixmap.fill(Qt::transparent);
        QPainter p(&pixmap);
        
        // Outer white frame (1px)
        p.fillRect(2, 2, 28, 28, Qt::white);
        
        // Color middle (solid fill)
        p.fillRect(3, 3, 26, 26, c);
        
        btn->setIcon(QIcon(pixmap));
        btn->setIconSize(QSize(32, 32));
    }
}
namespace ScreenCut {

EditorPropsPanel::EditorPropsPanel(QWidget* parent) : QWidget(parent) {
    setupUI();
}

QWidget* EditorPropsPanel::createSectionTitle(const QString& title) {
    QWidget* w = new QWidget();
    w->setStyleSheet("background-color: #cbd5e1; border-top: 1px solid #94a3b8; border-bottom: 1px solid #94a3b8;");
    QHBoxLayout* l = new QHBoxLayout(w);
    l->setContentsMargins(8, 4, 8, 4);
    QLabel* label = new QLabel(title);
    label->setStyleSheet("font-weight: bold; color: #334155; border: none; background: transparent;");
    l->addWidget(label, 0, Qt::AlignCenter);
    return w;
}

QWidget* EditorPropsPanel::createPickerButton(const QString& title, QPushButton*& btn) {
    QWidget* w = new QWidget();
    QVBoxLayout* l = new QVBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(2);
    QLabel* label = new QLabel(title);
    label->setAlignment(Qt::AlignCenter);
    label->setFixedHeight(20);
    label->setStyleSheet("font-weight: normal; color: #ccc; padding: 0px; margin: 0px;");
    btn = new QPushButton();
    btn->setFixedSize(40, 40);
    // Base style before icon updates
    btn->setStyleSheet("border: 1px solid #94a3b8; border-radius: 4px; background-color: #334155; padding: 2px;");
    l->addWidget(label);
    l->addWidget(btn, 0, Qt::AlignCenter);
    return w;
}

void EditorPropsPanel::createSliderRow(QVBoxLayout* parentLayout, const QString& title, QSlider*& slider, QLabel*& label, int min, int max, int defaultVal, const QString& suffix) {
    QHBoxLayout* l = new QHBoxLayout();
    l->addWidget(new QLabel(title));
    slider = new QSlider(Qt::Horizontal);
    slider->setRange(min, max);
    slider->setValue(defaultVal);
    label = new QLabel(QString::number(defaultVal) + suffix);
    label->setFixedWidth(40);
    l->addWidget(slider);
    l->addWidget(label);
    parentLayout->addLayout(l);
}

void EditorPropsPanel::pickColor(QPushButton* sourceBtn, const QColor& initial, std::function<void(const QColor&)> onSelected, bool allowTransparent) {
    QMenu* menu = new QMenu(this);
    menu->setStyleSheet("QMenu { background: #252525; border: 1px solid #444; border-radius: 8px; }");
    
    EditorColorPicker* picker = new EditorColorPicker(initial, menu, allowTransparent);
    connect(picker, &EditorColorPicker::colorChanged, onSelected);
    
    QWidgetAction* action = new QWidgetAction(menu);
    action->setDefaultWidget(picker);
    menu->addAction(action);
    
    menu->exec(sourceBtn->mapToGlobal(QPoint(0, sourceBtn->height() + 4)));
    menu->deleteLater();
}

void EditorPropsPanel::setupUI() {
    setFixedWidth(260);
    // Write the down-arrow SVG to a temp file so Qt stylesheet can reference it
    QString arrowPath = QDir::temp().filePath("screencut_down_arrow.svg");
    QFile arrowFile(arrowPath);
    if (arrowFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        arrowFile.write(SVG_DOWN_ARROW.toUtf8());
        arrowFile.close();
    }

    setStyleSheet(QString(R"(
        QWidget { background-color: #252525; color: #ffffff; }
        QLabel { color: #dddddd; font-size: 13px; font-weight: bold; }
        QSlider::groove:horizontal { height: 6px; background: #3c3c3c; border-radius: 3px; }
        QSlider::handle:horizontal { width: 14px; margin: -4px 0; background: #246bb2; border-radius: 7px; }
        QComboBox, QFontComboBox, QSpinBox { background: #333333; border: 1px solid #555555; border-radius: 4px; padding: 4px; color: white; }
        QComboBox::drop-down, QFontComboBox::drop-down { border: none; width: 24px; }
        QComboBox::down-arrow, QFontComboBox::down-arrow { 
            image: url(%1);
            width: 16px;
            height: 16px;
        }
        QPushButton { background: #333333; border: 1px solid #555555; border-radius: 4px; padding: 6px; color: white; font-weight: bold; }
        QPushButton:hover { background: #444444; border-color: #246bb2; }
    )").arg(arrowPath));

    QVBoxLayout* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    // ensure scrollarea transparent so widget style applies
    scrollArea->setStyleSheet("QScrollArea { background-color: transparent; } QWidget#PropContainer { background-color: transparent; }");
    
    QWidget* container = new QWidget();
    container->setObjectName("PropContainer");
    m_mainLayout = new QVBoxLayout(container);
    m_mainLayout->setContentsMargins(15, 20, 15, 20);
    m_mainLayout->setSpacing(15);
    
    scrollArea->setWidget(container);
    outerLayout->addWidget(scrollArea);

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
    QVBoxLayout* mainLayout = new QVBoxLayout(m_arrowWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(12);



    // --- TOOL PROPERTIES ---
    mainLayout->addWidget(createSectionTitle("Tool Properties"));
    
    // Color & Shadow
    QHBoxLayout* colorShadowLayout = new QHBoxLayout();
    colorShadowLayout->setSpacing(15);
    colorShadowLayout->addStretch();
    
    colorShadowLayout->addWidget(createPickerButton("Color", m_btnArrowColor));
    updateFillIcon(m_btnArrowColor, QColor("#ef4444"));
    
    QVBoxLayout* shadowLayout = new QVBoxLayout();
    connect(m_btnArrowColor, &QPushButton::clicked, this, [this](){
        pickColor(m_btnArrowColor, m_selectedColor, [this](const QColor& c){
            m_selectedColor = c;
            updateFillIcon(m_btnArrowColor, c);
            emit colorChanged(c);
        }, false); // No transparent option
    });

    m_btnArrowShadow = new ShadowPropertyWidget(m_arrowWidget);
    shadowLayout->addWidget(m_btnArrowShadow, 0, Qt::AlignCenter);
    colorShadowLayout->addLayout(shadowLayout);
    colorShadowLayout->addStretch();
    
    connect(m_btnArrowShadow, &ShadowPropertyWidget::shadowStyleChanged, this, [this](const ShadowStyle& style){
        emit arrowShadowChanged(style);
    });
    
    mainLayout->addLayout(colorShadowLayout);
    
    // Line style row
    QVBoxLayout* lineStyleLayout = new QVBoxLayout();
    
    // Lambda to draw line style icons
    auto createLineIcon = [](Qt::PenStyle style) -> QIcon {
        QPixmap pixmap(100, 20);
        pixmap.fill(Qt::transparent);
        QPainter p(&pixmap);
        p.setRenderHint(QPainter::Antialiasing);
        QPen pen(Qt::white, 3, style, Qt::FlatCap, Qt::MiterJoin);
        p.setPen(pen);
        p.drawLine(10, 10, 90, 10);
        return QIcon(pixmap);
    };

    // Lambda to draw arrow head icons
    auto createArrowIcon = [](const QString& type, bool isStart) -> QIcon {
        QPixmap pixmap(60, 20);
        pixmap.fill(Qt::transparent);
        QPainter p(&pixmap);
        p.setRenderHint(QPainter::Antialiasing);
        
        ArrowStyle style;
        style.color = Qt::white;
        style.lineWidth = 2.0;
        style.penStyle = Qt::SolidLine;
        
        if (isStart) {
            style.startHead = ArrowPainter::stringToArrowHead(type);
            style.endHead = ArrowHead::None;
        } else {
            style.startHead = ArrowHead::None;
            style.endHead = ArrowPainter::stringToArrowHead(type);
        }
        
        ArrowPainter::draw(p, QPointF(10, 10), QPointF(50, 10), style);
        
        return QIcon(pixmap);
    };
    
    auto populateArrowCombo = [&](QComboBox* combo, bool isStart) {
        combo->setIconSize(QSize(60, 20));
        const QStringList styles = {
            "None", "Open", "FilledTriangle", 
            "FilledDiamond", "FilledCircle", 
            "FilledSquare", "Tee"
        };
        for (const QString& style : styles) {
            combo->addItem(createArrowIcon(style, isStart), "", style);
        }
    };
    
    QHBoxLayout* topRow = new QHBoxLayout();
    m_arrowStartStyleCombo = new QComboBox();
    populateArrowCombo(m_arrowStartStyleCombo, true);
    
    m_arrowEndStyleCombo = new QComboBox();
    populateArrowCombo(m_arrowEndStyleCombo, false);
    
    connect(m_arrowStartStyleCombo, &QComboBox::currentIndexChanged, this, [this](int /*index*/) {
        emit arrowStartStyleChanged(m_arrowStartStyleCombo->currentData().toString());
    });
    connect(m_arrowEndStyleCombo, &QComboBox::currentIndexChanged, this, [this](int /*index*/) {
        emit arrowEndStyleChanged(m_arrowEndStyleCombo->currentData().toString());
    });
    
    topRow->addWidget(m_arrowStartStyleCombo);
    topRow->addWidget(m_arrowEndStyleCombo);
    
    m_arrowLineStyleCombo = new QComboBox();
    m_arrowLineStyleCombo->setIconSize(QSize(100, 20));
    m_arrowLineStyleCombo->addItem(createLineIcon(Qt::SolidLine), "", "Solid");
    m_arrowLineStyleCombo->addItem(createLineIcon(Qt::DashLine), "", "Dashed");
    m_arrowLineStyleCombo->addItem(createLineIcon(Qt::DotLine), "", "Dotted");
    m_arrowLineStyleCombo->addItem(createLineIcon(Qt::DashDotLine), "", "DashDot");
    
    connect(m_arrowLineStyleCombo, &QComboBox::currentIndexChanged, this, [this](int /*index*/) {
        emit arrowLineStyleChanged(m_arrowLineStyleCombo->currentData().toString());
    });
    
    // We will repurpose m_arrowTypeCombo as the backend type for backward compatibility,
    // but the UI will interact with the new combos.
    m_arrowTypeCombo = new QComboBox();
    m_arrowTypeCombo->addItems({"Custom"}); // We force it to Custom so it respects our new properties
    m_arrowTypeCombo->hide(); 
    
    lineStyleLayout->addLayout(topRow);
    lineStyleLayout->addWidget(m_arrowLineStyleCombo);
    
    mainLayout->addLayout(lineStyleLayout);

    // Sliders Helper
    auto addSliderRow = [&](const QString& label, QSlider*& slider, QLabel*& valLabel, int min, int max, int val, const QString& suffix) {
        QHBoxLayout* hLayout = new QHBoxLayout();
        QLabel* title = new QLabel(label);
        title->setFixedWidth(60);
        title->setStyleSheet("font-size: 12px; font-weight: normal;");
        hLayout->addWidget(title);
        
        slider = new QSlider(Qt::Horizontal);
        slider->setRange(min, max);
        slider->setValue(val);
        hLayout->addWidget(slider);
        
        valLabel = new QLabel(QString::number(val) + suffix);
        valLabel->setFixedWidth(40);
        valLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        valLabel->setStyleSheet("background-color: #1e1e1e; border: 1px solid #444; border-radius: 4px; padding: 2px; font-size: 12px; font-weight: normal;");
        hLayout->addWidget(valLabel);
        
        mainLayout->addLayout(hLayout);
    };
    
    addSliderRow("Width", m_arrowWidthSlider, m_arrowWidthLabel, 1, 40, m_currentWidth, "");
    connect(m_arrowWidthSlider, &QSlider::valueChanged, this, [this](int value) {
        m_currentWidth = value;
        m_arrowWidthLabel->setText(QString::number(value));
        emit lineWidthChanged(value);
    });
    
    addSliderRow("Opacity", m_arrowOpacitySlider, m_arrowOpacityLabel, 0, 100, 100, "%");
    connect(m_arrowOpacitySlider, &QSlider::valueChanged, this, [this](int value) {
        m_arrowOpacityLabel->setText(QString::number(value) + "%");
        emit arrowOpacityChanged(value);
    });

    addSliderRow("Start Size", m_arrowStartSizeSlider, m_arrowStartSizeLabel, 1, 10, 3, "");
    connect(m_arrowStartSizeSlider, &QSlider::valueChanged, this, [this](int value) {
        m_arrowStartSizeLabel->setText(QString::number(value));
        emit arrowStartSizeChanged(value);
    });

    addSliderRow("End Size", m_arrowEndSizeSlider, m_arrowEndSizeLabel, 1, 10, 3, "");
    connect(m_arrowEndSizeSlider, &QSlider::valueChanged, this, [this](int value) {
        m_arrowEndSizeLabel->setText(QString::number(value));
        emit arrowEndSizeChanged(value);
    });

    m_mainLayout->addWidget(m_arrowWidget);
}

void EditorPropsPanel::createShapeProps() {
    m_shapeWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(m_shapeWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(8);

    mainLayout->addWidget(createSectionTitle("Tool Properties"));

    // Fill, Outline, Shape, Shadow pickers
    QHBoxLayout* pickersLayout = new QHBoxLayout();
    pickersLayout->setSpacing(6);
    
    pickersLayout->addStretch();
    
    pickersLayout->addWidget(createPickerButton("Fill", m_btnShapeFill));
    updateFillIcon(m_btnShapeFill, Qt::transparent); // Default fill
    connect(m_btnShapeFill, &QPushButton::clicked, this, [this](){
        // Fill supports transparent
        pickColor(m_btnShapeFill, m_selectedColor, [this](const QColor& c){
            m_selectedColor = c;
            updateFillIcon(m_btnShapeFill, c);
            emit shapeFillColorChanged(c);
        }, true);
    });

    pickersLayout->addWidget(createPickerButton("Outline", m_btnShapeOutline));
    updateOutlineIcon(m_btnShapeOutline, QColor(255, 59, 48)); // Default outline red
    connect(m_btnShapeOutline, &QPushButton::clicked, this, [this](){
        // Outline supports transparent
        pickColor(m_btnShapeOutline, m_textOutlineColor, [this](const QColor& c){
            m_textOutlineColor = c;
            updateOutlineIcon(m_btnShapeOutline, c);
            emit shapeOutlineColorChanged(c);
        }, true);
    });

    // Shape selector
    QWidget* shapePickerW = new QWidget();
    QVBoxLayout* shapeL = new QVBoxLayout(shapePickerW);
    shapeL->setContentsMargins(0, 0, 0, 0);
    shapeL->setSpacing(2);
    QLabel* shapeLabel = new QLabel("Shape");
    shapeLabel->setAlignment(Qt::AlignCenter);
    shapeLabel->setFixedHeight(20);
    shapeLabel->setStyleSheet("font-weight: normal; color: #ccc; padding: 0px; margin: 0px;");
    shapeL->addWidget(shapeLabel);
    
    m_shapeStyleCombo = new QComboBox();
    m_shapeStyleCombo->setFixedSize(40, 40);
    m_shapeStyleCombo->setIconSize(QSize(32, 32));
    m_shapeStyleCombo->addItem(ScreenCut::createSvgIcon(ScreenCut::SVG_RECT, 32, 32), "", "Rectangle");
    // We don't have SVG_ROUNDED_RECT yet, but we'll use SVG_RECT for now
    m_shapeStyleCombo->addItem(ScreenCut::createSvgIcon(ScreenCut::SVG_RECT, 32, 32), "", "Rounded Rectangle");
    m_shapeStyleCombo->addItem(ScreenCut::createSvgIcon(ScreenCut::SVG_ELLIPSE, 32, 32), "", "Ellipse");
    m_shapeStyleCombo->addItem(ScreenCut::createSvgIcon(ScreenCut::SVG_POLYGON, 32, 32), "", "Polygon");
    
    // Apply styling to hide the text and only show icon in the combobox box (hide the drop-down arrow in normal state if we want, but default is fine)
    m_shapeStyleCombo->setStyleSheet("QComboBox { border: 1px solid #444; border-radius: 4px; background-color: #2b2b2b; padding: 2px; } QComboBox::drop-down { border: 0px; width: 0px; } QComboBox::down-arrow { image: none; } QComboBox QAbstractItemView { icon-size: 32px 32px; }");
    
    connect(m_shapeStyleCombo, &QComboBox::currentIndexChanged, this, [this](int /*index*/) {
        emit shapeStyleChanged(m_shapeStyleCombo->currentData().toString());
    });
    shapeL->addWidget(m_shapeStyleCombo, 0, Qt::AlignCenter);
    pickersLayout->addWidget(shapePickerW);

    m_btnShapeShadow = new ShadowPropertyWidget(m_shapeWidget);
    pickersLayout->addWidget(m_btnShapeShadow);
    connect(m_btnShapeShadow, &ShadowPropertyWidget::shadowStyleChanged, this, [this](const ShadowStyle& style){
        emit shapeShadowChanged(style);
    });

    pickersLayout->addStretch();
    mainLayout->addLayout(pickersLayout);

    createSliderRow(mainLayout, "Thickness", m_shapeThicknessSlider, m_shapeThicknessLabel, 1, 40, 3, "");
    connect(m_shapeThicknessSlider, &QSlider::valueChanged, this, [this](int value) {
        m_shapeThicknessLabel->setText(QString::number(value));
        emit shapeThicknessChanged(value);
    });

    createSliderRow(mainLayout, "Opacity", m_shapeOpacitySlider, m_shapeOpacityLabel, 0, 100, 100, "%");
    connect(m_shapeOpacitySlider, &QSlider::valueChanged, this, [this](int value) {
        m_shapeOpacityLabel->setText(QString::number(value) + "%");
        emit shapeOpacityChanged(value);
    });
    
    // Line style row
    QVBoxLayout* lineStyleLayout = new QVBoxLayout();
    
    // Lambda to draw line style icons
    auto createLineIcon = [](Qt::PenStyle style) -> QIcon {
        QPixmap pixmap(100, 20);
        pixmap.fill(Qt::transparent);
        QPainter p(&pixmap);
        p.setRenderHint(QPainter::Antialiasing);
        QPen pen(Qt::white, 3, style, Qt::FlatCap, Qt::MiterJoin);
        p.setPen(pen);
        p.drawLine(10, 10, 90, 10);
        return QIcon(pixmap);
    };
    
    QHBoxLayout* lineComboLayout = new QHBoxLayout();
    lineComboLayout->addWidget(new QLabel("Line Style"));
    m_lineStyleCombo = new QComboBox();
    m_lineStyleCombo->setIconSize(QSize(100, 20));
    m_lineStyleCombo->addItem(createLineIcon(Qt::SolidLine), "", "Solid");
    m_lineStyleCombo->addItem(createLineIcon(Qt::DashLine), "", "Dashed");
    connect(m_lineStyleCombo, &QComboBox::currentIndexChanged, this, [this](int /*index*/) {
        emit lineStyleChanged(m_lineStyleCombo->currentData().toString());
    });
    lineComboLayout->addWidget(m_lineStyleCombo, 1);
    lineStyleLayout->addLayout(lineComboLayout);
    mainLayout->addLayout(lineStyleLayout);

    m_mainLayout->addWidget(m_shapeWidget);
}

void EditorPropsPanel::createTextProps() {
    m_textWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(m_textWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(8);

    // --- TOOL PROPERTIES ---
    mainLayout->addWidget(createSectionTitle("Tool Properties"));

    // Fill, Outline, Shadow Pickers
    QHBoxLayout* pickersLayout = new QHBoxLayout();
    pickersLayout->setSpacing(15);
    
    pickersLayout->addStretch();
    pickersLayout->addWidget(createPickerButton("Fill", m_btnTextFill));
    pickersLayout->addWidget(createPickerButton("Outline", m_btnTextOutline));
    
    m_btnTextShadow = new ShadowPropertyWidget(m_textWidget);
    pickersLayout->addWidget(m_btnTextShadow);
    
    pickersLayout->addStretch();
    mainLayout->addLayout(pickersLayout);

    // Font Family & Style
    m_fontFamilyCombo = new QFontComboBox();
    mainLayout->addWidget(m_fontFamilyCombo);


    // Font Size
    QHBoxLayout* fontSizeLayout = new QHBoxLayout();
    fontSizeLayout->addWidget(new QLabel("Font size:"));
    m_fontSizeSlider = new QSlider(Qt::Horizontal);
    m_fontSizeSlider->setRange(8, 144);
    m_fontSizeLabel = new QLabel("24 pt");
    m_fontSizeLabel->setFixedWidth(40);
    fontSizeLayout->addWidget(m_fontSizeSlider);
    fontSizeLayout->addWidget(m_fontSizeLabel);
    mainLayout->addLayout(fontSizeLayout);

    // Line Width (Outline Width)
    QHBoxLayout* lineWidthLayout = new QHBoxLayout();
    lineWidthLayout->addWidget(new QLabel("Line width:"));
    m_textLineWidthSlider = new QSlider(Qt::Horizontal);
    m_textLineWidthSlider->setRange(0, 10);
    m_textLineWidthLabel = new QLabel("4 pt");
    m_textLineWidthLabel->setFixedWidth(40);
    lineWidthLayout->addWidget(m_textLineWidthSlider);
    lineWidthLayout->addWidget(m_textLineWidthLabel);
    mainLayout->addLayout(lineWidthLayout);

    // --- ADVANCED SECTION ---
    m_textAdvancedWidget = new QWidget();
    QVBoxLayout* advLayout = new QVBoxLayout(m_textAdvancedWidget);
    advLayout->setContentsMargins(0, 0, 0, 0);
    advLayout->setSpacing(6);

    // Alignment Buttons (Row 1)
    QHBoxLayout* alignLayout = new QHBoxLayout();
    alignLayout->setSpacing(4);
    
    auto createSvgToolBtn = [](const QString& svgString, bool checkable = true) {
        QPushButton* btn = new QPushButton();
        btn->setFixedSize(30, 30);
        btn->setCheckable(checkable);
        btn->setIcon(createSvgIcon(svgString, 18, 18));
        btn->setStyleSheet("QPushButton:checked { background-color: #bfdbfe; border: 1px solid #3b82f6; }");
        return btn;
    };
    
    m_btnAlignLeft = createSvgToolBtn(SVG_ALIGN_LEFT);
    m_btnAlignCenter = createSvgToolBtn(SVG_ALIGN_CENTER);
    m_btnAlignRight = createSvgToolBtn(SVG_ALIGN_RIGHT);
    alignLayout->addWidget(m_btnAlignLeft);
    alignLayout->addWidget(m_btnAlignCenter);
    alignLayout->addWidget(m_btnAlignRight);
    alignLayout->addSpacing(10);
    
    m_btnAlignTop = createSvgToolBtn(SVG_ALIGN_TOP);
    m_btnAlignMiddle = createSvgToolBtn(SVG_ALIGN_MIDDLE);
    m_btnAlignBottom = createSvgToolBtn(SVG_ALIGN_BOTTOM);
    alignLayout->addWidget(m_btnAlignTop);
    alignLayout->addWidget(m_btnAlignMiddle);
    alignLayout->addWidget(m_btnAlignBottom);
    
    advLayout->addLayout(alignLayout);

    // Style Buttons (Row 2)
    QHBoxLayout* styleLayout = new QHBoxLayout();
    styleLayout->setSpacing(4);
    
    m_btnBold = createSvgToolBtn(SVG_FONT_BOLD);
    m_btnItalic = createSvgToolBtn(SVG_FONT_ITALIC);
    m_btnUnderline = createSvgToolBtn(SVG_FONT_UNDERLINE);
    m_btnStrikeOut = createSvgToolBtn(SVG_FONT_STRIKEOUT);
    styleLayout->addWidget(m_btnBold);
    styleLayout->addWidget(m_btnItalic);
    styleLayout->addWidget(m_btnUnderline);
    styleLayout->addWidget(m_btnStrikeOut);
    styleLayout->addStretch();
    
    advLayout->addLayout(styleLayout);

    // Opacity
    QHBoxLayout* opacityLayout = new QHBoxLayout();
    opacityLayout->addWidget(new QLabel("Opacity:"));
    m_opacitySlider = new QSlider(Qt::Horizontal);
    m_opacitySlider->setRange(0, 100);
    m_opacitySlider->setValue(100);
    m_opacityLabel = new QLabel("100%");
    m_opacityLabel->setFixedWidth(40);
    opacityLayout->addWidget(m_opacitySlider);
    opacityLayout->addWidget(m_opacityLabel);
    advLayout->addLayout(opacityLayout);

    // Line Spacing
    QHBoxLayout* spacingLayout = new QHBoxLayout();
    spacingLayout->addWidget(new QLabel("Line Spacing:"));
    m_spacingSlider = new QSlider(Qt::Horizontal);
    m_spacingSlider->setRange(-10, 50);
    m_spacingSlider->setValue(0);
    m_spacingLabel = new QLabel("0 pt");
    m_spacingLabel->setFixedWidth(40);
    spacingLayout->addWidget(m_spacingSlider);
    spacingLayout->addWidget(m_spacingLabel);
    advLayout->addLayout(spacingLayout);

    mainLayout->addWidget(m_textAdvancedWidget);
    m_mainLayout->addWidget(m_textWidget);

    // --- SIGNAL CONNECTIONS ---
    connect(m_fontFamilyCombo, &QFontComboBox::currentFontChanged, this, [this](const QFont& f){ emit fontFamilyChanged(f.family()); });
    
    connect(m_btnBold, &QPushButton::toggled, this, &EditorPropsPanel::textIsBoldChanged);
    connect(m_btnItalic, &QPushButton::toggled, this, &EditorPropsPanel::textIsItalicChanged);

    connect(m_fontSizeSlider, &QSlider::valueChanged, this, [this](int val){
        m_fontSizeLabel->setText(QString("%1 pt").arg(val));
        emit fontSizeChanged(val);
    });

    connect(m_textLineWidthSlider, &QSlider::valueChanged, this, [this](int val){
        m_textLineWidthLabel->setText(QString("%1 pt").arg(val));
        emit textOutlineWidthChanged(val);
    });

    connect(m_opacitySlider, &QSlider::valueChanged, this, [this](int val){
        m_opacityLabel->setText(QString("%1%").arg(val));
        emit textOpacityChanged(val);
    });

    connect(m_spacingSlider, &QSlider::valueChanged, this, [this](int val){
        m_spacingLabel->setText(QString("%1 pt").arg(val));
        emit textLineSpacingChanged(val);
    });

    connect(m_btnTextFill, &QPushButton::clicked, this, [this](){
        pickColor(m_btnTextFill, m_selectedColor, [this](const QColor& c){
            m_selectedColor = c;
            updateFillIcon(m_btnTextFill, c);
            emit colorChanged(c);
        }, false); // No transparent option
    });

    connect(m_btnTextOutline, &QPushButton::clicked, this, [this](){
        pickColor(m_btnTextOutline, m_textOutlineColor, [this](const QColor& c){
            m_textOutlineColor = c;
            updateOutlineIcon(m_btnTextOutline, c);
            emit textOutlineColorChanged(c);
        });
    });

    connect(m_btnTextShadow, &ShadowPropertyWidget::shadowStyleChanged, this, [this](const ShadowStyle& style){
        emit textShadowChanged(style);
    });

    connect(m_btnUnderline, &QPushButton::toggled, this, &EditorPropsPanel::textIsUnderlineChanged);
    connect(m_btnStrikeOut, &QPushButton::toggled, this, &EditorPropsPanel::textIsStrikeOutChanged);

    // Alignment buttons (should be exclusive, but we handle it manually for simplicity or use QButtonGroup)
    connect(m_btnAlignLeft, &QPushButton::clicked, this, [this](){ 
        m_btnAlignCenter->setChecked(false); m_btnAlignRight->setChecked(false); m_btnAlignLeft->setChecked(true);
        emit textHAlignChanged(TextAnnotation::TextAlign::Left); 
    });
    connect(m_btnAlignCenter, &QPushButton::clicked, this, [this](){ 
        m_btnAlignLeft->setChecked(false); m_btnAlignRight->setChecked(false); m_btnAlignCenter->setChecked(true);
        emit textHAlignChanged(TextAnnotation::TextAlign::Center); 
    });
    connect(m_btnAlignRight, &QPushButton::clicked, this, [this](){ 
        m_btnAlignLeft->setChecked(false); m_btnAlignCenter->setChecked(false); m_btnAlignRight->setChecked(true);
        emit textHAlignChanged(TextAnnotation::TextAlign::Right); 
    });

    connect(m_btnAlignTop, &QPushButton::clicked, this, [this](){ 
        m_btnAlignMiddle->setChecked(false); m_btnAlignBottom->setChecked(false); m_btnAlignTop->setChecked(true);
        emit textVAlignChanged(TextAnnotation::VerticalAlign::Top); 
    });
    connect(m_btnAlignMiddle, &QPushButton::clicked, this, [this](){ 
        m_btnAlignTop->setChecked(false); m_btnAlignBottom->setChecked(false); m_btnAlignMiddle->setChecked(true);
        emit textVAlignChanged(TextAnnotation::VerticalAlign::Middle); 
    });
    connect(m_btnAlignBottom, &QPushButton::clicked, this, [this](){ 
        m_btnAlignTop->setChecked(false); m_btnAlignMiddle->setChecked(false); m_btnAlignBottom->setChecked(true);
        emit textVAlignChanged(TextAnnotation::VerticalAlign::Bottom); 
    });
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
        case ToolType::Stamp: title = "Stamp"; break;
        case ToolType::Crop: title = "Crop"; break;
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
        case ToolType::None:
        case ToolType::Stamp:
        case ToolType::Crop:
            showColor = false; showSize = false; break;
        case ToolType::Arrow: 
            m_arrowWidget->setVisible(true); 
            showColor = false;
            showSize = false;
            break;
        case ToolType::Rectangle:
        case ToolType::Ellipse:
        case ToolType::Polygon:
            m_shapeWidget->setVisible(true);
            showColor = false;
            showSize = false;
            break;
        case ToolType::Text: 
            m_textWidget->setVisible(true); 
            showColor = false;
            showSize = false;
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
    m_blurTypeCombo->blockSignals(true);
    m_penStyleCombo->blockSignals(true);
    
    // Text UI blocks
    if (m_fontFamilyCombo) m_fontFamilyCombo->blockSignals(true);
    if (m_fontSizeSlider) m_fontSizeSlider->blockSignals(true);
    if (m_textLineWidthSlider) m_textLineWidthSlider->blockSignals(true);
    if (m_opacitySlider) m_opacitySlider->blockSignals(true);
    if (m_spacingSlider) m_spacingSlider->blockSignals(true);
    if (m_btnBold) m_btnBold->blockSignals(true);
    if (m_btnItalic) m_btnItalic->blockSignals(true);
    if (m_btnUnderline) m_btnUnderline->blockSignals(true);
    if (m_btnStrikeOut) m_btnStrikeOut->blockSignals(true);
    
    if (item->getType() == ToolType::Arrow) {
        auto arrow = std::static_pointer_cast<ArrowAnnotation>(item);
        m_arrowTypeCombo->setCurrentText(arrow->arrowType);
        
        auto getComboIdx = [](QComboBox* combo, const QString& style) {
            int idx = combo->findData(style);
            if (idx == -1) {
                if (style == "Arrow") idx = combo->findData("FilledTriangle");
                else if (style == "Circle") idx = combo->findData("FilledCircle");
                else if (style == "Square") idx = combo->findData("FilledSquare");
            }
            return idx;
        };
        
        m_arrowStartStyleCombo->setCurrentIndex(getComboIdx(m_arrowStartStyleCombo, arrow->startStyle));
        m_arrowEndStyleCombo->setCurrentIndex(getComboIdx(m_arrowEndStyleCombo, arrow->endStyle));
        m_arrowLineStyleCombo->setCurrentIndex(m_arrowLineStyleCombo->findData(arrow->lineStyle));
        
        m_arrowWidthSlider->blockSignals(true);
        m_arrowWidthSlider->setValue(arrow->lineWidth);
        m_arrowWidthLabel->setText(QString::number(arrow->lineWidth));
        m_arrowWidthSlider->blockSignals(false);
        
        m_arrowOpacitySlider->blockSignals(true);
        m_arrowOpacitySlider->setValue(arrow->opacity);
        m_arrowOpacityLabel->setText(QString::number(arrow->opacity) + "%");
        m_arrowOpacitySlider->blockSignals(false);
        
        m_arrowStartSizeSlider->blockSignals(true);
        m_arrowStartSizeSlider->setValue(arrow->startSize);
        m_arrowStartSizeLabel->setText(QString::number(arrow->startSize));
        m_arrowStartSizeSlider->blockSignals(false);
        
        m_arrowEndSizeSlider->blockSignals(true);
        m_arrowEndSizeSlider->setValue(arrow->endSize);
        m_arrowEndSizeLabel->setText(QString::number(arrow->endSize));
        m_arrowEndSizeSlider->blockSignals(false);
        
        m_btnArrowShadow->setShadowStyle(arrow->shadow);
        
        updateFillIcon(m_btnArrowColor, arrow->color);

    } else if (item->getType() == ToolType::Rectangle || item->getType() == ToolType::Ellipse || item->getType() == ToolType::Polygon) {
        std::shared_ptr<AnnotationItem> shapeItem = item;
        
        QColor fillColor = Qt::transparent;
        QColor outlineColor = QColor(255, 59, 48);
        int lineWidth = 3;
        QString shapeStyle = "Rectangle";
        QString lineStyle = "Solid";
        int opacity = 100;
        ShadowStyle shadow;
        
        if (item->getType() == ToolType::Polygon) {
            auto poly = std::static_pointer_cast<PolygonAnnotation>(item);
            fillColor = poly->fillColor;
            outlineColor = poly->outlineColor;
            lineWidth = poly->lineWidth;
            shapeStyle = "Polygon";
            lineStyle = poly->lineStyle;
            opacity = poly->opacity;
            shadow = poly->shadow;
        } else {
            auto shape = std::static_pointer_cast<ShapeAnnotation>(item);
            fillColor = shape->fillColor;
            outlineColor = shape->outlineColor;
            lineWidth = shape->lineWidth;
            shapeStyle = shape->shapeStyle;
            lineStyle = shape->lineStyle;
            opacity = shape->opacity;
            shadow = shape->shadow;
        }
        
        if (m_shapeStyleCombo) m_shapeStyleCombo->setCurrentIndex(m_shapeStyleCombo->findData(shapeStyle));
        if (m_lineStyleCombo) m_lineStyleCombo->setCurrentIndex(m_lineStyleCombo->findData(lineStyle));
        
        if (m_btnShapeFill) {
            m_selectedColor = fillColor;
            updateFillIcon(m_btnShapeFill, fillColor);
        }
        
        if (m_btnShapeOutline) {
            m_textOutlineColor = outlineColor;
            updateOutlineIcon(m_btnShapeOutline, outlineColor);
        }
        
        if (m_shapeThicknessSlider) {
            m_shapeThicknessSlider->blockSignals(true);
            m_shapeThicknessSlider->setValue(lineWidth);
            if (m_shapeThicknessLabel) m_shapeThicknessLabel->setText(QString::number(lineWidth));
            m_shapeThicknessSlider->blockSignals(false);
        }
        
        if (m_shapeOpacitySlider) {
            m_shapeOpacitySlider->blockSignals(true);
            m_shapeOpacitySlider->setValue(opacity);
            if (m_shapeOpacityLabel) m_shapeOpacityLabel->setText(QString::number(opacity) + "%");
            m_shapeOpacitySlider->blockSignals(false);
        }
        
        if (m_btnShapeShadow) {
            m_btnShapeShadow->setShadowStyle(shadow);
        }
    } else if (item->getType() == ToolType::Text) {
        auto txt = std::static_pointer_cast<TextAnnotation>(item);
        if (m_fontFamilyCombo) m_fontFamilyCombo->setCurrentFont(QFont(txt->fontFamily));
        
        if (m_btnBold) m_btnBold->setChecked(txt->isBold);
        if (m_btnItalic) m_btnItalic->setChecked(txt->isItalic);
        
        if (m_fontSizeSlider) {
            m_fontSizeSlider->setValue(txt->fontSize);
            if (m_fontSizeLabel) m_fontSizeLabel->setText(QString("%1 pt").arg(txt->fontSize));
        }
        
        if (m_textLineWidthSlider) {
            m_textLineWidthSlider->setValue(txt->outlineWidth);
            if (m_textLineWidthLabel) m_textLineWidthLabel->setText(QString("%1 pt").arg(txt->outlineWidth));
        }
        
        if (m_opacitySlider) {
            m_opacitySlider->setValue(txt->opacity);
            if (m_opacityLabel) m_opacityLabel->setText(QString("%1%").arg(txt->opacity));
        }
        
        if (m_spacingSlider) {
            m_spacingSlider->setValue(txt->lineSpacing);
            if (m_spacingLabel) m_spacingLabel->setText(QString("%1 pt").arg(txt->lineSpacing));
        }
        
        if (m_btnUnderline) { m_btnUnderline->setChecked(txt->isUnderline); }
        if (m_btnStrikeOut) { m_btnStrikeOut->setChecked(txt->isStrikeOut); }
        
        // Alignment
        if (m_btnAlignLeft) m_btnAlignLeft->setChecked(txt->hAlign == TextAnnotation::TextAlign::Left);
        if (m_btnAlignCenter) m_btnAlignCenter->setChecked(txt->hAlign == TextAnnotation::TextAlign::Center);
        if (m_btnAlignRight) m_btnAlignRight->setChecked(txt->hAlign == TextAnnotation::TextAlign::Right);
        
        if (m_btnAlignTop) m_btnAlignTop->setChecked(txt->vAlign == TextAnnotation::VerticalAlign::Top);
        if (m_btnAlignMiddle) m_btnAlignMiddle->setChecked(txt->vAlign == TextAnnotation::VerticalAlign::Middle);
        if (m_btnAlignBottom) m_btnAlignBottom->setChecked(txt->vAlign == TextAnnotation::VerticalAlign::Bottom);
        
        // Update picker buttons styling (rough mockup)
        if (m_btnTextFill) {
            updateFillIcon(m_btnTextFill, txt->color);
        }
        if (m_btnTextOutline) {
            updateOutlineIcon(m_btnTextOutline, txt->outlineColor);
        }
        if (m_btnTextShadow) {
            m_btnTextShadow->setShadowStyle(txt->shadow);
        }
        
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
    m_blurTypeCombo->blockSignals(false);
    m_penStyleCombo->blockSignals(false);
    
    if (m_fontFamilyCombo) m_fontFamilyCombo->blockSignals(false);
    if (m_btnBold) m_btnBold->blockSignals(false);
    if (m_btnItalic) m_btnItalic->blockSignals(false);
    if (m_fontSizeSlider) m_fontSizeSlider->blockSignals(false);
    if (m_textLineWidthSlider) m_textLineWidthSlider->blockSignals(false);
    if (m_opacitySlider) m_opacitySlider->blockSignals(false);
    if (m_spacingSlider) m_spacingSlider->blockSignals(false);
    if (m_btnUnderline) m_btnUnderline->blockSignals(false);
    if (m_btnStrikeOut) m_btnStrikeOut->blockSignals(false);
}

void EditorPropsPanel::updateFontSizeUI(int size) {
    if (m_fontSizeSlider) {
        m_fontSizeSlider->blockSignals(true);
        m_fontSizeSlider->setValue(size);
        if (m_fontSizeLabel) m_fontSizeLabel->setText(QString("%1 pt").arg(size));
        m_fontSizeSlider->blockSignals(false);
    }
}

void EditorPropsPanel::syncFromSelection(const QColor& color, int width) {
    m_selectedColor = color;
    m_currentWidth = width;

    m_widthSlider->blockSignals(true);
    m_widthSlider->setValue(width);
    m_widthLabel->setText(QString::number(width));
    m_widthSlider->blockSignals(false);
    
    if (m_arrowWidthSlider) {
        m_arrowWidthSlider->blockSignals(true);
        m_arrowWidthSlider->setValue(width);
        m_arrowWidthLabel->setText(QString::number(width));
        m_arrowWidthSlider->blockSignals(false);
    }
    
    if (m_btnArrowColor) {
        m_btnArrowColor->setStyleSheet(QString("border: 1px solid #94a3b8; border-radius: 4px; background-color: %1;").arg(color.name()));
    }

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
