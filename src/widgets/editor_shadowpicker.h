/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef EDITOR_SHADOWPICKER_H
#define EDITOR_SHADOWPICKER_H

#include <QWidget>
#include <QColor>
#include <QPushButton>
#include "../editor/editor_models.h"

class QDial;
class QSlider;
class QLineEdit;
class QMenu;
class QWidgetAction;

namespace ScreenCut {

// The 3x3 Grid for Shadow Presets
class ShadowGridWidget : public QWidget {
    Q_OBJECT
public:
    explicit ShadowGridWidget(QWidget* parent = nullptr);
    void setShadowStyle(const ShadowStyle& style);
    ShadowStyle shadowStyle() const;

signals:
    void shadowStyleChanged(const ShadowStyle& style);

private:
    void setupUI();
    void updateGridUI();
    
    ShadowStyle m_style;
    QPushButton* m_cells[9];
    bool m_updating = false;
};

// The Advanced Popup
class ShadowAdvancedPopup : public QWidget {
    Q_OBJECT
public:
    explicit ShadowAdvancedPopup(const ShadowStyle& initialStyle, QWidget* parent = nullptr);
    void setShadowStyle(const ShadowStyle& style);
    ShadowStyle shadowStyle() const;

signals:
    void shadowStyleChanged(const ShadowStyle& style);

private:
    void setupUI();
    void updateUI();
    
    ShadowStyle m_style;
    bool m_updating = false;
    
    QPushButton* m_btnColor;
    QDial* m_dialAngle;
    QSlider* m_sliderDistance;
    QSlider* m_sliderOpacity;
    QSlider* m_sliderBlur;
    
    QLineEdit* m_editAngle;
    QLineEdit* m_editDistance;
    QLineEdit* m_editOpacity;
    QLineEdit* m_editBlur;
};

// The Container for the Dropdown Title and the Grid
class ShadowPropertyWidget : public QWidget {
    Q_OBJECT
public:
    explicit ShadowPropertyWidget(QWidget* parent = nullptr);
    void setShadowStyle(const ShadowStyle& style);
    ShadowStyle shadowStyle() const;

signals:
    void shadowStyleChanged(const ShadowStyle& style);

private:
    void setupUI();
    
    ShadowGridWidget* m_grid;
    ShadowStyle m_style;
    QPushButton* m_titleButton;
};

} // namespace ScreenCut
#endif
