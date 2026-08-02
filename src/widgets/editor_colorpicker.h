/*
 * SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef EDITOR_COLORPICKER_H
#define EDITOR_COLORPICKER_H

#include <QWidget>
#include <QColor>

class QLineEdit;

namespace ScreenCut {

class SVPickerArea : public QWidget {
    Q_OBJECT
public:
    explicit SVPickerArea(QWidget* parent = nullptr);
    void setHue(int hue);
    void setSV(int s, int v);

signals:
    void svChanged(int s, int v);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    void updateFromMouse(const QPoint& pos);
    int m_hue = 0;
    int m_sat = 0;
    int m_val = 255;
};

class HueSliderArea : public QWidget {
    Q_OBJECT
public:
    explicit HueSliderArea(QWidget* parent = nullptr);
    void setHue(int hue);

signals:
    void hueChanged(int hue);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    void updateFromMouse(const QPoint& pos);
    int m_hue = 0;
};

class EditorColorPicker : public QWidget {
    Q_OBJECT
public:
    explicit EditorColorPicker(const QColor& initialColor = Qt::white, QWidget* parent = nullptr, bool allowTransparent = true);
    ~EditorColorPicker() override = default;

    QColor color() const;

public slots:
    void setColor(const QColor& color);

signals:
    void colorChanged(const QColor& color);

private:
    void setupUI();
    void updateInputs();
    void onPresetClicked(const QColor& color);

    QColor m_color;
    bool m_updating = false;
    bool m_allowTransparent = true;

    SVPickerArea* m_svPicker = nullptr;
    HueSliderArea* m_hueSlider = nullptr;
    QLineEdit* m_rInput = nullptr;
    QLineEdit* m_gInput = nullptr;
    QLineEdit* m_bInput = nullptr;
};

} // namespace ScreenCut

#endif // EDITOR_COLORPICKER_H
