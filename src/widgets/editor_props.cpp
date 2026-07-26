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
#include <QColorDialog>
#include <QMenu>
#include <QWidgetAction>

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

    // --- QUICK STYLES ---
    mainLayout->addWidget(createSectionTitle("Quick Styles"));
    

    // Quick Styles Grid
    QGridLayout* stylesGrid = new QGridLayout();
    stylesGrid->setSpacing(4);
    
    struct QuickStyle { QColor fill; QColor outline; int outlineWidth; ShadowDirection shadow; };
    std::vector<QuickStyle> quickStyles = {
        {QColor("#ef4444"), Qt::transparent, 0, ShadowDirection::None},      // Flat Red
        {QColor("#3b82f6"), Qt::transparent, 0, ShadowDirection::None},      // Flat Blue
        {QColor("#22c55e"), Qt::transparent, 0, ShadowDirection::None},      // Flat Green
        {QColor("#eab308"), Qt::transparent, 0, ShadowDirection::None},      // Flat Yellow
        
        {Qt::white, QColor("#ef4444"), 2, ShadowDirection::BottomRight},     // White with Red Outline + Shadow
        {Qt::white, QColor("#3b82f6"), 2, ShadowDirection::BottomRight},     // White with Blue Outline + Shadow
        {Qt::white, QColor("#22c55e"), 2, ShadowDirection::BottomRight},     // White with Green Outline + Shadow
        {Qt::white, QColor("#eab308"), 2, ShadowDirection::BottomRight},     // White with Yellow Outline + Shadow
        
        {QColor("#ef4444"), Qt::white, 2, ShadowDirection::BottomRight},     // Red with White Outline + Shadow
        {QColor("#3b82f6"), Qt::white, 2, ShadowDirection::BottomRight},     // Blue with White Outline + Shadow
        {QColor("#22c55e"), Qt::white, 2, ShadowDirection::BottomRight},     // Green with White Outline + Shadow
        {QColor("#eab308"), Qt::white, 2, ShadowDirection::BottomRight}      // Yellow with White Outline + Shadow
    };
    
    for (int i = 0; i < 12; ++i) {
        QPushButton* presetBtn = new QPushButton("A");
        presetBtn->setFixedSize(36, 36);
        
        QString css = QString("QPushButton { font-weight: bold; font-size: 20px; border-radius: 4px; ");
        if (quickStyles[i].fill == Qt::transparent) css += "background-color: white; color: black; ";
        else css += QString("background-color: %1; color: %2; ")
                    .arg(quickStyles[i].fill.name())
                    .arg(quickStyles[i].fill == Qt::white ? "black" : "white");
        
        if (quickStyles[i].outlineWidth > 0 && quickStyles[i].outline != Qt::transparent) {
            css += QString("border: %1px solid %2; ").arg(quickStyles[i].outlineWidth).arg(quickStyles[i].outline.name());
        } else {
            css += "border: 1px solid #ccc; ";
        }
        css += "}";
        
        presetBtn->setStyleSheet(css);
        connect(presetBtn, &QPushButton::clicked, this, [this, i, quickStyles](){
            const auto& qs = quickStyles[i];
            
            // Apply Fill
            m_selectedColor = qs.fill;
            if(m_btnTextFill) m_btnTextFill->setStyleSheet(QString("border: 1px solid #94a3b8; border-radius: 4px; background-color: %1;")
                .arg(qs.fill == Qt::transparent ? "transparent" : qs.fill.name()));
            emit colorChanged(m_selectedColor);
            
            // Apply Outline
            m_textOutlineColor = qs.outline;
            if(m_btnTextOutline) m_btnTextOutline->setStyleSheet(QString("border: 1px solid #94a3b8; border-radius: 4px; background-color: %1;")
                .arg(qs.outline == Qt::transparent ? "transparent" : qs.outline.name()));
            emit textOutlineColorChanged(m_textOutlineColor);
            
            if(m_textLineWidthSlider) m_textLineWidthSlider->setValue(qs.outlineWidth);
            
            // Apply Shadow
            m_textShadowDirection = qs.shadow;
            m_textHasShadow = (qs.shadow != ShadowDirection::None);
            if(m_btnTextShadow) m_btnTextShadow->setStyleSheet(QString("border: 1px solid #94a3b8; border-radius: 4px; background-color: %1;")
                .arg(m_textHasShadow ? "#bfdbfe" : "white"));
            emit textHasShadowChanged(m_textHasShadow);
            emit textShadowDirectionChanged(m_textShadowDirection);
        });
        stylesGrid->addWidget(presetBtn, i / 4, i % 4);
    }
    mainLayout->addLayout(stylesGrid);

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
    m_fontStyleCombo = new QComboBox();
    m_fontStyleCombo->addItems({"Regular", "Bold", "Italic", "Bold Italic"});
    mainLayout->addWidget(m_fontStyleCombo);

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

    // Formatting & Alignment Buttons
    QHBoxLayout* fmtLayout = new QHBoxLayout();
    fmtLayout->setSpacing(4);
    
    auto createToolBtn = [](const QString& text, bool checkable = true) {
        QPushButton* btn = new QPushButton(text);
        btn->setFixedSize(30, 30);
        btn->setCheckable(checkable);
        btn->setStyleSheet("QPushButton:checked { background-color: #bfdbfe; border: 1px solid #3b82f6; }");
        return btn;
    };

    m_btnUnderline = createToolBtn("U");
    m_btnStrikeOut = createToolBtn("S");
    fmtLayout->addWidget(m_btnUnderline);
    fmtLayout->addWidget(m_btnStrikeOut);
    fmtLayout->addSpacing(10);
    
    m_btnAlignLeft = createToolBtn("L");
    m_btnAlignCenter = createToolBtn("C");
    m_btnAlignRight = createToolBtn("R");
    fmtLayout->addWidget(m_btnAlignLeft);
    fmtLayout->addWidget(m_btnAlignCenter);
    fmtLayout->addWidget(m_btnAlignRight);
    fmtLayout->addSpacing(10);
    
    m_btnAlignTop = createToolBtn("T");
    m_btnAlignMiddle = createToolBtn("M");
    m_btnAlignBottom = createToolBtn("B");
    fmtLayout->addWidget(m_btnAlignTop);
    fmtLayout->addWidget(m_btnAlignMiddle);
    fmtLayout->addWidget(m_btnAlignBottom);
    
    advLayout->addLayout(fmtLayout);

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
    
    connect(m_fontStyleCombo, &QComboBox::currentIndexChanged, this, [this](int idx){
        emit textIsBoldChanged(idx == 1 || idx == 3);
        emit textIsItalicChanged(idx == 2 || idx == 3);
    });

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
        case ToolType::Arrow: m_arrowWidget->setVisible(true); break;
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
    if (m_fontStyleCombo) m_fontStyleCombo->blockSignals(true);
    if (m_fontSizeSlider) m_fontSizeSlider->blockSignals(true);
    if (m_textLineWidthSlider) m_textLineWidthSlider->blockSignals(true);
    if (m_opacitySlider) m_opacitySlider->blockSignals(true);
    if (m_spacingSlider) m_spacingSlider->blockSignals(true);
    if (m_btnUnderline) m_btnUnderline->blockSignals(true);
    if (m_btnStrikeOut) m_btnStrikeOut->blockSignals(true);
    
    if (item->getType() == ToolType::Arrow) {
        m_arrowTypeCombo->setCurrentText(std::static_pointer_cast<ArrowAnnotation>(item)->arrowType);
    } else if (item->getType() == ToolType::Rectangle || item->getType() == ToolType::Ellipse) {
        auto shape = std::static_pointer_cast<ShapeAnnotation>(item);
        m_shapeStyleCombo->setCurrentText(shape->shapeStyle);
        m_lineStyleCombo->setCurrentText(shape->lineStyle);
    } else if (item->getType() == ToolType::Text) {
        auto txt = std::static_pointer_cast<TextAnnotation>(item);
        if (m_fontFamilyCombo) m_fontFamilyCombo->setCurrentFont(QFont(txt->fontFamily));
        
        if (m_fontStyleCombo) {
            int styleIdx = 0; // Regular
            if (txt->isBold && txt->isItalic) styleIdx = 3;
            else if (txt->isItalic) styleIdx = 2;
            else if (txt->isBold) styleIdx = 1;
            m_fontStyleCombo->setCurrentIndex(styleIdx);
        }
        
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
    if (m_fontStyleCombo) m_fontStyleCombo->blockSignals(false);
    if (m_fontSizeSlider) m_fontSizeSlider->blockSignals(false);
    if (m_textLineWidthSlider) m_textLineWidthSlider->blockSignals(false);
    if (m_opacitySlider) m_opacitySlider->blockSignals(false);
    if (m_spacingSlider) m_spacingSlider->blockSignals(false);
    if (m_btnUnderline) m_btnUnderline->blockSignals(false);
    if (m_btnStrikeOut) m_btnStrikeOut->blockSignals(false);
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
