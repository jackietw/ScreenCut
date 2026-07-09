/*
* SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
* SPDX-License-Identifier: LGPL-2.1-or-later
*/

#ifndef COMMON_SCROLL_CAPTURE_H
#define COMMON_SCROLL_CAPTURE_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QImage>
#include <QPixmap>
#include <QRect>

namespace ScreenCut {

class ScrollBorderOverlay : public QWidget {
    Q_OBJECT
public:
    explicit ScrollBorderOverlay(const QRect& logicalRect, QWidget* parent = nullptr);
    ~ScrollBorderOverlay() override;

    void setCapturedDimensions(int logicalHeight, int physWidth, int physHeight);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QRect m_logicalRect;
    int m_currentLogicalHeight;
    int m_physWidth;
    int m_physHeight;
    int m_padding;
    int m_topPadding;
    qreal m_ratio;
};

class ScrollCaptureToolbar : public QWidget {
    Q_OBJECT
public:
    explicit ScrollCaptureToolbar(QWidget* parent = nullptr);
    ~ScrollCaptureToolbar() override;

    QLabel* statusLabel() const { return m_statusLabel; }

signals:
    void finishRequested();
    void cancelRequested();

private:
    QLabel* m_statusLabel;
    QPushButton* m_btnFinish;
    QPushButton* m_btnCancel;
};

class ScrollCaptureManager : public QWidget {
    Q_OBJECT
public:
    explicit ScrollCaptureManager(const QRect& logicalRect, QWidget* parent = nullptr);
    ~ScrollCaptureManager() override;

signals:
    void completed(const QPixmap& stitchedPixmap);
    void cancelled();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onProcessFrame();
    void onFinishCapture();
    void onCancelCapture();

private:
    QImage grabFrame();
    void updatePreview();
    void excludeFromCapture(WId winId);

    QRect m_logicalRect;
    qreal m_ratio;
    QTimer* m_timer;
    ScrollBorderOverlay* m_borderOverlay;
    QLabel* m_previewLabel;
    ScrollCaptureToolbar* m_toolbar;

    QImage m_lastFrame;
    QImage m_accumulatedImage;
    int m_currentY;
};

} // namespace ScreenCut

#endif // COMMON_SCROLL_CAPTURE_H
