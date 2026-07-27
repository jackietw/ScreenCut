/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "editor_props.h"
#include "editor_arrow.h"
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
#include "../resources/IconUtils.h"

namespace ScreenCut {

EditorPropsPanel::EditorPropsPanel(QWidget* parent) : QWidget(parent) {
    setupUI();
}

void EditorPropsPanel::setupUI() {
    setFixedWidth(260);
    setStyleSheet(R"(
        QWidget { background-color: #252525; color: #ffffff; }
        QLabel { color: #dddddd; font-size: 13px; font-weight: bold; }
        QSlider::groove:horizontal { height: 6px; background: #3c3c3c; border-radius: 3px; }
        QSlider::handle:horizontal { width: 14px; margin: -4px 0; background: #246bb2; border-radius: 7px; }
        QComboBox, QFontComboBox, QSpinBox { background: #333333; border: 1px solid #555555; border-radius: 4px; padding: 4px; color: white; }
        QComboBox::drop-down, QFontComboBox::drop-down { border: none; width: 24px; }
        QComboBox::down-arrow, QFontComboBox::down-arrow { 
            image: url("data:image/svg+xml;base64,PHN2ZyB4bWxucz0naHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmcnIHdpZHRoPScxNicgaGVpZ2h0PScxNicgdmlld0JveD0nMCAwIDE2IDE2Jz48cGF0aCBmaWxsPSd3aGl0ZScgZD0nTTQgNmg4bC00IDV6Jy8+PC9zdmc+");
            width: 16px;
            height: 16px;
        }
        QPushButton { background: #333333; border: 1px solid #555555; border-radius: 4px; padding: 6px; color: white; font-weight: bold; }
        QPushButton:hover { background: #444444; border-color: #246bb2; }
    )");

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

    auto createSectionTitle = [](const QString& title) -> QWidget* {
        QWidget* w = new QWidget();
        w->setStyleSheet("background-color: #333333; border-top: 1px solid #444; border-bottom: 1px solid #444;");
        QHBoxLayout* l = new QHBoxLayout(w);
        l->setContentsMargins(8, 4, 8, 4);
        QLabel* label = new QLabel(title);
        label->setStyleSheet("font-weight: bold; color: #ddd; border: none; background: transparent;");
        l->addWidget(label, 0, Qt::AlignCenter);
        return w;
    };

    auto pickColor = [this](QPushButton* sourceBtn, const QColor& initial, std::function<void(const QColor&)> onSelected) {
        QMenu* menu = new QMenu(this);
        QWidget* container = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(container);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(8);

        // Top: Transparent + Basic Colors
        QHBoxLayout* topLayout = new QHBoxLayout();
        topLayout->setSpacing(4);
        
        struct Preset { QString name; QColor color; };
        std::vector<Preset> presets = {
            {"", Qt::black},
            {"", Qt::white},
            {"", QColor("#ef4444")},
            {"", QColor("#22c55e")},
            {"", QColor("#3b82f6")},
            {"", QColor("#eab308")}
        };
        
        for (const auto& p : presets) {
            QPushButton* b = new QPushButton(p.name);
            b->setFixedSize(24, 24);
            QString css = "QPushButton { border: 1px solid #ccc; border-radius: 12px; ";
            css += QString("background-color: %1; }").arg(p.color.name());
            b->setStyleSheet(css);
            connect(b, &QPushButton::clicked, [menu, onSelected, p]() {
                onSelected(p.color);
                menu->close();
            });
            topLayout->addWidget(b);
        }
        topLayout->addStretch();
        layout->addLayout(topLayout);

        // Middle & Bottom: QColorDialog
        QColor startColor = initial;
        if (startColor.alpha() == 0) startColor.setAlpha(255);
        QColorDialog* cd = new QColorDialog(startColor, container);
        cd->setWindowFlags(Qt::Widget);
        cd->setOptions(QColorDialog::DontUseNativeDialog | QColorDialog::ShowAlphaChannel | QColorDialog::NoButtons);
        connect(cd, &QColorDialog::currentColorChanged, [onSelected](const QColor& c){
            if (c.isValid()) onSelected(c);
        });
        layout->addWidget(cd);

        QWidgetAction* action = new QWidgetAction(menu);
        action->setDefaultWidget(container);
        menu->addAction(action);
        
        menu->exec(sourceBtn->mapToGlobal(QPoint(0, sourceBtn->height())));
        menu->deleteLater();
    };

    // --- TOOL PROPERTIES ---
    mainLayout->addWidget(createSectionTitle("Tool Properties"));
    
    // Color & Shadow
    QHBoxLayout* colorShadowLayout = new QHBoxLayout();
    
    QVBoxLayout* colorLayout = new QVBoxLayout();
    colorLayout->addWidget(new QLabel("Color", m_arrowWidget), 0, Qt::AlignCenter);
    m_btnArrowColor = new QPushButton();
    m_btnArrowColor->setFixedSize(40, 40);
    m_btnArrowColor->setStyleSheet("border: 1px solid #94a3b8; border-radius: 4px; background-color: #ef4444;");
    colorLayout->addWidget(m_btnArrowColor, 0, Qt::AlignCenter);
    colorShadowLayout->addLayout(colorLayout);
    
    connect(m_btnArrowColor, &QPushButton::clicked, this, [this, pickColor](){
        pickColor(m_btnArrowColor, m_selectedColor, [this](const QColor& c){
            m_selectedColor = c;
            m_btnArrowColor->setStyleSheet(QString("border: 1px solid #94a3b8; border-radius: 4px; background-color: %1;").arg(c.name()));
            emit colorChanged(c);
        });
    });

    QVBoxLayout* shadowLayout = new QVBoxLayout();
    shadowLayout->addWidget(new QLabel("Shadow", m_arrowWidget), 0, Qt::AlignCenter);
    m_btnArrowShadow = new QPushButton("▦");
    m_btnArrowShadow->setFixedSize(40, 40);
    m_btnArrowShadow->setStyleSheet("QPushButton { font-size: 24px; border: 1px solid #94a3b8; border-radius: 4px; background-color: #333; color: #aaa; }");
    shadowLayout->addWidget(m_btnArrowShadow, 0, Qt::AlignCenter);
    colorShadowLayout->addLayout(shadowLayout);
    
    // Toggle shadow on/off
    connect(m_btnArrowShadow, &QPushButton::clicked, this, [this](){
        // For simplicity, we toggle between None and BottomRight
        bool hasShadow = (m_arrowShadowDirection == ShadowDirection::None);
        m_arrowShadowDirection = hasShadow ? ShadowDirection::BottomRight : ShadowDirection::None;
        
        m_btnArrowShadow->setStyleSheet(QString("QPushButton { font-size: 24px; border: 1px solid #94a3b8; border-radius: 4px; background-color: %1; color: %2; }")
            .arg(hasShadow ? "#bfdbfe" : "#333").arg(hasShadow ? "#000" : "#aaa"));
            
        emit arrowHasShadowChanged(hasShadow);
        emit arrowShadowDirectionChanged(m_arrowShadowDirection);
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
            "None", "Open", "Triangle", "FilledTriangle", 
            "Diamond", "FilledDiamond", "Circle", "FilledCircle", 
            "Square", "FilledSquare", "Tee"
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
    QVBoxLayout* mainLayout = new QVBoxLayout(m_textWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(8);

    // Helper lambda for section titles
    auto createSectionTitle = [](const QString& title) -> QWidget* {
        QWidget* w = new QWidget();
        w->setStyleSheet("background-color: #cbd5e1; border-top: 1px solid #94a3b8; border-bottom: 1px solid #94a3b8;");
        QHBoxLayout* l = new QHBoxLayout(w);
        l->setContentsMargins(8, 4, 8, 4);
        QLabel* label = new QLabel(title);
        label->setStyleSheet("font-weight: bold; color: #334155; border: none; background: transparent;");
        l->addWidget(label, 0, Qt::AlignCenter);
        return w;
    };
    // --- TOOL PROPERTIES ---
    mainLayout->addWidget(createSectionTitle("Tool Properties"));

    // Fill, Outline, Shadow Pickers
    QHBoxLayout* pickersLayout = new QHBoxLayout();
    pickersLayout->setSpacing(12);
    
    auto createPicker = [](const QString& title, QPushButton*& btn) -> QWidget* {
        QWidget* w = new QWidget();
        QVBoxLayout* l = new QVBoxLayout(w);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(2);
        QLabel* label = new QLabel(title);
        label->setAlignment(Qt::AlignCenter);
        btn = new QPushButton();
        btn->setFixedSize(48, 48);
        btn->setStyleSheet("border: 1px solid #94a3b8; border-radius: 4px; background-color: white;");
        l->addWidget(label);
        l->addWidget(btn, 0, Qt::AlignCenter);
        return w;
    };
    
    pickersLayout->addStretch();
    pickersLayout->addWidget(createPicker("Fill", m_btnTextFill));
    pickersLayout->addWidget(createPicker("Outline", m_btnTextOutline));
    pickersLayout->addWidget(createPicker("Shadow", m_btnTextShadow));
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

    auto pickColor = [this](QPushButton* sourceBtn, const QColor& initial, std::function<void(const QColor&)> onSelected) {
        QMenu* menu = new QMenu(this);
        QWidget* container = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(container);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(8);

        // Top: Transparent + Basic Colors
        QHBoxLayout* topLayout = new QHBoxLayout();
        topLayout->setSpacing(4);
        
        struct Preset { QString name; QColor color; };
        std::vector<Preset> presets = {
            {"🚫", Qt::transparent},
            {"", Qt::black},
            {"", Qt::white},
            {"", QColor("#ef4444")},
            {"", QColor("#22c55e")},
            {"", QColor("#3b82f6")},
            {"", QColor("#eab308")}
        };
        
        for (const auto& p : presets) {
            QPushButton* b = new QPushButton(p.name);
            b->setFixedSize(24, 24);
            QString css = "QPushButton { border: 1px solid #ccc; border-radius: 12px; ";
            if (p.color == Qt::transparent) {
                css += "background: white; color: red; font-size: 14px; font-weight: bold; }";
            } else {
                css += QString("background-color: %1; }").arg(p.color.name());
            }
            b->setStyleSheet(css);
            connect(b, &QPushButton::clicked, [menu, onSelected, p]() {
                onSelected(p.color);
                menu->close();
            });
            topLayout->addWidget(b);
        }
        topLayout->addStretch();
        layout->addLayout(topLayout);

        // Middle & Bottom: QColorDialog (Palette + RGB)
        QColor startColor = initial;
        if (startColor.alpha() == 0) startColor.setAlpha(255);
        QColorDialog* cd = new QColorDialog(startColor, container);
        cd->setWindowFlags(Qt::Widget);
        cd->setOptions(QColorDialog::DontUseNativeDialog | QColorDialog::ShowAlphaChannel | QColorDialog::NoButtons);
        connect(cd, &QColorDialog::currentColorChanged, [onSelected](const QColor& c){
            if (c.isValid()) onSelected(c);
        });
        layout->addWidget(cd);

        QWidgetAction* action = new QWidgetAction(menu);
        action->setDefaultWidget(container);
        menu->addAction(action);
        
        // Show below the button
        menu->exec(sourceBtn->mapToGlobal(QPoint(0, sourceBtn->height())));
        menu->deleteLater();
    };

    connect(m_btnTextFill, &QPushButton::clicked, this, [this, pickColor](){
        pickColor(m_btnTextFill, m_selectedColor, [this](const QColor& c){
            m_selectedColor = c;
            m_btnTextFill->setStyleSheet(QString("border: 1px solid #94a3b8; border-radius: 4px; background-color: %1;").arg(c == Qt::transparent ? "transparent" : c.name()));
            emit colorChanged(c);
        });
    });

    connect(m_btnTextOutline, &QPushButton::clicked, this, [this, pickColor](){
        pickColor(m_btnTextOutline, m_textOutlineColor, [this](const QColor& c){
            m_textOutlineColor = c;
            m_btnTextOutline->setStyleSheet(QString("border: 1px solid #94a3b8; border-radius: 4px; background-color: %1;").arg(c == Qt::transparent ? "transparent" : c.name()));
            emit textOutlineColorChanged(c);
        });
    });

    // Shadow 3x3 Grid Menu
    QMenu* shadowMenu = new QMenu(m_btnTextShadow);
    QWidget* shadowGridWidget = new QWidget();
    QGridLayout* shadowGrid = new QGridLayout(shadowGridWidget);
    shadowGrid->setSpacing(2);
    shadowGrid->setContentsMargins(4, 4, 4, 4);

    struct ShadowBtn { int row, col; ShadowDirection dir; QString icon; };
    std::vector<ShadowBtn> shadowBtns = {
        {0, 0, ShadowDirection::TopLeft, "↖"}, {0, 1, ShadowDirection::Top, "↑"}, {0, 2, ShadowDirection::TopRight, "↗"},
        {1, 0, ShadowDirection::Left, "←"}, {1, 1, ShadowDirection::None, "⨯"}, {1, 2, ShadowDirection::Right, "→"},
        {2, 0, ShadowDirection::BottomLeft, "↙"}, {2, 1, ShadowDirection::Bottom, "↓"}, {2, 2, ShadowDirection::BottomRight, "↘"}
    };

    for (const auto& sb : shadowBtns) {
        QPushButton* b = new QPushButton(sb.icon);
        b->setFixedSize(32, 32);
        b->setStyleSheet("QPushButton { font-size: 16px; border: 1px solid #ccc; background: white; } QPushButton:hover { background: #e2e8f0; }");
        connect(b, &QPushButton::clicked, this, [this, sb, shadowMenu](){
            m_textShadowDirection = sb.dir;
            m_textHasShadow = (sb.dir != ShadowDirection::None);
            m_btnTextShadow->setStyleSheet(QString("border: 1px solid #94a3b8; border-radius: 4px; background-color: %1;")
                                            .arg(m_textHasShadow ? "#bfdbfe" : "white"));
            emit textHasShadowChanged(m_textHasShadow);
            emit textShadowDirectionChanged(m_textShadowDirection);
            shadowMenu->close();
        });
        shadowGrid->addWidget(b, sb.row, sb.col);
    }
    
    QWidgetAction* shadowAction = new QWidgetAction(shadowMenu);
    shadowAction->setDefaultWidget(shadowGridWidget);
    shadowMenu->addAction(shadowAction);
    m_btnTextShadow->setMenu(shadowMenu);

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
        case ToolType::Arrow: 
            m_arrowWidget->setVisible(true); 
            showColor = false;
            showSize = false;
            break;
        case ToolType::Rectangle:
        case ToolType::Ellipse: m_shapeWidget->setVisible(true); break;
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
        
        m_arrowShadowDirection = arrow->shadowDirection;
        bool hasShadow = arrow->hasShadow;
        m_btnArrowShadow->setStyleSheet(QString("QPushButton { font-size: 24px; border: 1px solid #94a3b8; border-radius: 4px; background-color: %1; color: %2; }")
            .arg(hasShadow ? "#bfdbfe" : "#333").arg(hasShadow ? "#000" : "#aaa"));
        
        m_btnArrowColor->setStyleSheet(QString("border: 1px solid #94a3b8; border-radius: 4px; background-color: %1;").arg(arrow->color.name()));

    } else if (item->getType() == ToolType::Rectangle || item->getType() == ToolType::Ellipse) {
        auto shape = std::static_pointer_cast<ShapeAnnotation>(item);
        m_shapeStyleCombo->setCurrentText(shape->shapeStyle);
        m_lineStyleCombo->setCurrentText(shape->lineStyle);
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
            m_btnTextFill->setStyleSheet(QString("border: 1px solid #94a3b8; border-radius: 4px; background-color: %1;").arg(txt->color == Qt::transparent ? "transparent" : txt->color.name()));
        }
        if (m_btnTextOutline) {
            QString outColor = txt->outlineColor == Qt::transparent ? "transparent" : txt->outlineColor.name();
            m_btnTextOutline->setStyleSheet(QString("border: 1px solid #94a3b8; border-radius: 4px; background-color: %1;").arg(outColor));
        }
        if (m_btnTextShadow) {
            m_textShadowDirection = txt->shadowDirection;
            m_textHasShadow = txt->hasShadow;
            m_btnTextShadow->setStyleSheet(QString("border: 1px solid #94a3b8; border-radius: 4px; background-color: %1;").arg(txt->hasShadow ? "#bfdbfe" : "white"));
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
