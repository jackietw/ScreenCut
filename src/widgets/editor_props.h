/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef EDITOR_PROPS_H
#define EDITOR_PROPS_H

#include <QWidget>
#include <QColor>
#include <QString>
#include "../editor/editor_models.h"

class QVBoxLayout;
class QLabel;
class QSlider;
class QPushButton;
class QComboBox;
class QFontComboBox;

namespace ScreenCut {

class EditorPropsPanel : public QWidget {
    Q_OBJECT
public:
    explicit EditorPropsPanel(QWidget* parent = nullptr);
    ~EditorPropsPanel() override = default;

    void setCurrentTool(ToolType type);

public slots:
    void syncFromSelection(const std::shared_ptr<AnnotationItem>& item);
    void syncFromSelection(const QColor& color, int width); // Fallback
    void updateFontSizeUI(int size);

signals:
    void colorChanged(const QColor& color);
    void lineWidthChanged(int width);
    
    void arrowTypeChanged(const QString& type);
    
    // New arrow property signals
    void arrowStartStyleChanged(const QString& style);
    void arrowEndStyleChanged(const QString& style);
    void arrowLineStyleChanged(const QString& style);
    void arrowOpacityChanged(int opacity);
    void arrowStartSizeChanged(int size);
    void arrowEndSizeChanged(int size);
    void arrowHasShadowChanged(bool hasShadow);
    void arrowShadowDirectionChanged(ScreenCut::ShadowDirection dir);
    
    void shapeStyleChanged(const QString& style);
    void lineStyleChanged(const QString& style);
    void fontFamilyChanged(const QString& family);
    void fontSizeChanged(int size);
    
    // New text property signals
    void textIsBoldChanged(bool bold);
    void textIsItalicChanged(bool italic);
    void textIsUnderlineChanged(bool underline);
    void textIsStrikeOutChanged(bool strike);
    void textHAlignChanged(TextAnnotation::TextAlign align);
    void textVAlignChanged(TextAnnotation::VerticalAlign align);
    void textOpacityChanged(int opacity);
    void textLineSpacingChanged(int spacing);
    void textOutlineColorChanged(const QColor& color);
    void textHasShadowChanged(bool hasShadow);
    void textShadowDirectionChanged(ShadowDirection direction);
    void textOutlineWidthChanged(int width);

    void blurTypeChanged(ToolType type);
    void blurIntensityChanged(int intensity);
    void penStyleChanged(const QString& style);
    void resetStepCounter();

private:
    void setupUI();
    void createQuickStyles();
    void createColorPalette();
    void createSizeSlider();
    
    void createArrowProps();
    void createShapeProps();
    void createTextProps();
    void createBlurProps();
    void createPenProps();
    void createStepProps();

    void updateVisibility(ToolType type);

    QVBoxLayout* m_mainLayout = nullptr;
    QLabel* m_toolNameLabel = nullptr;
    
    QWidget* m_quickStylesWidget = nullptr;
    QWidget* m_colorWidget = nullptr;
    QWidget* m_sizeWidget = nullptr;
    QWidget* m_arrowWidget = nullptr;
    QWidget* m_shapeWidget = nullptr;
    QWidget* m_textWidget = nullptr;
    QWidget* m_blurWidget = nullptr;
    QWidget* m_penWidget = nullptr;
    QWidget* m_stepWidget = nullptr;
    
    QSlider* m_widthSlider = nullptr;
    QLabel* m_widthLabel = nullptr;
    QLabel* m_widthTitle = nullptr;
    
    QComboBox* m_arrowTypeCombo = nullptr;
    QComboBox* m_arrowLineStyleCombo = nullptr;
    QComboBox* m_arrowStartStyleCombo = nullptr;
    QComboBox* m_arrowEndStyleCombo = nullptr;
    
    QSlider* m_arrowWidthSlider = nullptr;
    QLabel*  m_arrowWidthLabel = nullptr;
    QSlider* m_arrowOpacitySlider = nullptr;
    QLabel*  m_arrowOpacityLabel = nullptr;
    QSlider* m_arrowStartSizeSlider = nullptr;
    QLabel*  m_arrowStartSizeLabel = nullptr;
    QSlider* m_arrowEndSizeSlider = nullptr;
    QLabel*  m_arrowEndSizeLabel = nullptr;
    
    QPushButton* m_btnArrowColor = nullptr;
    QPushButton* m_btnArrowShadow = nullptr;
    
    QComboBox* m_shapeStyleCombo = nullptr;
    QComboBox* m_lineStyleCombo = nullptr;
    QComboBox* m_blurTypeCombo = nullptr;
    QComboBox* m_penStyleCombo = nullptr;
    
    // Text UI elements
    QWidget* m_textAdvancedWidget = nullptr;
    QFontComboBox* m_fontFamilyCombo = nullptr;
    QSlider* m_fontSizeSlider = nullptr;
    QLabel* m_fontSizeLabel = nullptr;
    QSlider* m_textLineWidthSlider = nullptr;
    QLabel* m_textLineWidthLabel = nullptr;
    
    QPushButton* m_btnTextFill = nullptr;
    QPushButton* m_btnTextOutline = nullptr;
    QPushButton* m_btnTextShadow = nullptr;
    
    QPushButton* m_btnBold = nullptr;
    QPushButton* m_btnItalic = nullptr;
    QPushButton* m_btnUnderline = nullptr;
    QPushButton* m_btnStrikeOut = nullptr;
    
    QPushButton* m_btnAlignLeft = nullptr;
    QPushButton* m_btnAlignCenter = nullptr;
    QPushButton* m_btnAlignRight = nullptr;
    QPushButton* m_btnAlignTop = nullptr;
    QPushButton* m_btnAlignMiddle = nullptr;
    QPushButton* m_btnAlignBottom = nullptr;
    
    QSlider* m_opacitySlider = nullptr;
    QLabel* m_opacityLabel = nullptr;
    QSlider* m_spacingSlider = nullptr;
    QLabel* m_spacingLabel = nullptr;
    
    QColor m_textOutlineColor = Qt::transparent;
    bool m_textHasShadow = false;
    ShadowDirection m_textShadowDirection = ShadowDirection::BottomRight;
    ShadowDirection m_arrowShadowDirection = ShadowDirection::None;
    
    QColor m_selectedColor = QColor(255, 59, 48);
    int m_currentWidth = 3;
    
    std::vector<QPushButton*> m_colorButtons;
};

} // namespace ScreenCut

#endif // EDITOR_PROPS_H
