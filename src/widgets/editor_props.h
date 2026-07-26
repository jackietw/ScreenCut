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

signals:
    void colorChanged(const QColor& color);
    void lineWidthChanged(int width);
    
    void arrowTypeChanged(const QString& type);
    void shapeStyleChanged(const QString& style);
    void lineStyleChanged(const QString& style);
    void fontFamilyChanged(const QString& family);
    void fontSizeChanged(int size);
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
    QComboBox* m_shapeStyleCombo = nullptr;
    QComboBox* m_lineStyleCombo = nullptr;
    QFontComboBox* m_fontCombo = nullptr;
    QComboBox* m_blurTypeCombo = nullptr;
    QComboBox* m_penStyleCombo = nullptr;
    
    QColor m_selectedColor = QColor(255, 59, 48);
    int m_currentWidth = 3;
    
    std::vector<QPushButton*> m_colorButtons;
};

} // namespace ScreenCut

#endif // EDITOR_PROPS_H
