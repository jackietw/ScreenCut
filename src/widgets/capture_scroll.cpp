/*
* SPDX-FileCopyrightText: 2026 Jackie <jackie.github@outlook.com>
* SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "capture_scroll.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QScreen>
#include <QGuiApplication>
#include <QApplication>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QDebug>
#include <cmath>
#include <algorithm>

#include "../platform/platform.h"

namespace ScreenCut {

// --- ScrollBorderOverlay Implementation ---

ScrollBorderOverlay::ScrollBorderOverlay(const QRect& logicalRect, QWidget* parent)
    : QWidget(parent)
    , m_logicalRect(logicalRect)
    , m_currentLogicalHeight(logicalRect.height())
    , m_padding(2)
    , m_topPadding(25)
{
#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::WindowTransparentForInput | Qt::NoDropShadowWindowHint);
#else
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::WindowTransparentForInput | Qt::NoDropShadowWindowHint | Qt::Tool);
#endif
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    Platform::excludeWindowFromCapture(winId());
    Platform::elevateWindowAboveSystemBars(winId());

    QScreen* screen = QGuiApplication::screenAt(logicalRect.center());
    if (!screen) screen = QGuiApplication::primaryScreen();
    m_ratio = screen ? screen->devicePixelRatio() : 1.0;
    if (m_ratio <= 0) m_ratio = 1.0;

    m_physWidth = qRound(logicalRect.width() * m_ratio);
    m_physHeight = qRound(logicalRect.height() * m_ratio);

    int logicalX = logicalRect.x() - m_padding;
    int logicalY = logicalRect.y() - m_topPadding;
    int totalW = logicalRect.width() + m_padding * 2;
    int totalH = logicalRect.height() + m_topPadding + m_padding;

    setGeometry(logicalX, logicalY, totalW, totalH);
}

ScrollBorderOverlay::~ScrollBorderOverlay() = default;

void ScrollBorderOverlay::setCapturedDimensions(int logicalHeight, int physWidth, int physHeight) {
    m_currentLogicalHeight = logicalHeight;
    m_physWidth = physWidth;
    m_physHeight = physHeight;
    update();
}

void ScrollBorderOverlay::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw red border strictly around the fixed logical viewport rectangle (do not change size during scrolling)
    QPen pen(QColor(255, 0, 0));
    pen.setWidth(2);
    painter.setPen(pen);
    painter.drawRect(m_padding - 1, m_topPadding - 1, m_logicalRect.width() + 1, m_logicalRect.height() + 1);

    // Draw size label showing exact physical pixels (and logical DIPs) of the stitched image
    QString sizeStr = QString("%1 × %2 px").arg(m_physWidth).arg(m_physHeight);
    if (std::abs(m_ratio - 1.0) > 0.01) {
        sizeStr = QString("%1 × %2 px (%3 × %4 DPI)").arg(m_physWidth).arg(m_physHeight).arg(m_logicalRect.width()).arg(m_currentLogicalHeight);
    }
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    QFontMetrics metrics = painter.fontMetrics();
    int tw = metrics.horizontalAdvance(sizeStr);
    int th = metrics.height();

    int textX = m_padding;
    int textY = m_topPadding - th - 6;
    if (textY < 0) {
        textY = m_topPadding + 4;
    }

    painter.setBrush(QColor(0, 0, 0, 180));
    painter.setPen(Qt::NoPen);
    painter.drawRect(textX - 4, textY, tw + 8, th + 4);

    painter.setPen(QColor(255, 255, 255));
    painter.drawText(textX, textY + metrics.ascent() + 2, sizeStr);
}

// --- ScrollCaptureToolbar Implementation ---

ScrollCaptureToolbar::ScrollCaptureToolbar(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    m_statusLabel = new QLabel("Scroll vertically...");
    m_statusLabel->setStyleSheet("color: white; font-weight: bold; font-size: 14px; background: transparent;");

    QHBoxLayout* hboxBtns = new QHBoxLayout();
    hboxBtns->setSpacing(8);

    m_btnFinish = new QPushButton("Finish");
    m_btnFinish->setCursor(Qt::PointingHandCursor);
    m_btnFinish->setStyleSheet(
        "QPushButton { background-color: #3b82f6; color: white; border: none; border-radius: 4px; padding: 6px 15px; font-weight: bold; font-size: 13px; }"
        "QPushButton:hover { background-color: #2563eb; }"
    );
    connect(m_btnFinish, &QPushButton::clicked, this, &ScrollCaptureToolbar::finishRequested);

    m_btnCancel = new QPushButton("Cancel");
    m_btnCancel->setCursor(Qt::PointingHandCursor);
    m_btnCancel->setStyleSheet(
        "QPushButton { background-color: #ef4444; color: white; border: none; border-radius: 4px; padding: 6px 15px; font-weight: bold; font-size: 13px; }"
        "QPushButton:hover { background-color: #dc2626; }"
    );
    connect(m_btnCancel, &QPushButton::clicked, this, &ScrollCaptureToolbar::cancelRequested);

    hboxBtns->addWidget(m_btnFinish);
    hboxBtns->addWidget(m_btnCancel);
    hboxBtns->addStretch();

    layout->addWidget(m_statusLabel);
    layout->addLayout(hboxBtns);
}

ScrollCaptureToolbar::~ScrollCaptureToolbar() = default;

// --- ScrollCaptureManager Implementation ---

ScrollCaptureManager::ScrollCaptureManager(const QRect& logicalRect, QObject* parent)
    : QObject(parent)
    , m_logicalRect(logicalRect)
    , m_currentY(0)
{
    // Build the floating control UI as a standalone top-level QWidget
    m_controlWidget = new QWidget(nullptr);
#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
    m_controlWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
#else
    m_controlWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::Tool | Qt::NoDropShadowWindowHint);
#endif
    m_controlWidget->setAttribute(Qt::WA_TranslucentBackground);
    m_controlWidget->setAttribute(Qt::WA_ShowWithoutActivating);

    excludeFromCapture(m_controlWidget->winId());

    QVBoxLayout* layout = new QVBoxLayout(m_controlWidget);
    layout->setContentsMargins(10, 10, 10, 10);

    QWidget* bg = new QWidget(m_controlWidget);
    bg->setObjectName("ScrollBg");
    bg->setStyleSheet("#ScrollBg { background-color: #2b2b2b; border: 1px solid #555555; border-radius: 8px; }");
    QHBoxLayout* bgLayout = new QHBoxLayout(bg);
    bgLayout->setContentsMargins(15, 8, 15, 8);

    m_previewLabel = new QLabel(bg);
    m_previewLabel->setFixedSize(80, 120);
    m_previewLabel->setStyleSheet("background-color: #111111; border: 1px solid #666666; border-radius: 4px; margin-right: 15px;");
    m_previewLabel->setAlignment(Qt::AlignCenter);

    m_toolbar = new ScrollCaptureToolbar(bg);
    connect(m_toolbar, &ScrollCaptureToolbar::finishRequested, this, &ScrollCaptureManager::onFinishCapture);
    connect(m_toolbar, &ScrollCaptureToolbar::cancelRequested, this, &ScrollCaptureManager::onCancelCapture);

    bgLayout->addWidget(m_previewLabel);
    bgLayout->addWidget(m_toolbar);
    layout->addWidget(bg);

    m_controlWidget->adjustSize();
    int tw = m_controlWidget->width();
    int th = m_controlWidget->height();

    QScreen* screen = QGuiApplication::screenAt(logicalRect.center());
    if (!screen) screen = QGuiApplication::primaryScreen();
    m_ratio = screen ? screen->devicePixelRatio() : 1.0;
    if (m_ratio <= 0) m_ratio = 1.0;

    int logicalCenterX = logicalRect.x() + logicalRect.width() / 2;
    int logicalBottom = logicalRect.y() + logicalRect.height();
    int logicalTop = logicalRect.y();

    int x = logicalCenterX - tw / 2;
    int y = logicalBottom + 15;

    QRect sg = screen ? screen->geometry() : QRect(0, 0, 1920, 1080);
    if (y + th > sg.bottom()) {
        y = logicalTop - th - 15;
        if (y < sg.top()) {
            y = sg.bottom() - th - 15;
        }
    }
    if (x < sg.left() + 5) x = sg.left() + 5;
    else if (x + tw > sg.right() - 5) x = sg.right() - tw - 5;

    m_controlWidget->move(x, y);

    m_borderOverlay = new ScrollBorderOverlay(logicalRect);
    m_borderOverlay->show();

    m_lastFrame = grabFrame();
    m_accumulatedImage = m_lastFrame.copy();
    updatePreview();

    m_timer = new QTimer(this);
    m_timer->setInterval(33);
    connect(m_timer, &QTimer::timeout, this, &ScrollCaptureManager::onProcessFrame);
    m_timer->start();
}

void ScrollCaptureManager::show() {
    if (m_controlWidget) {
        m_controlWidget->show();
    }
}

ScrollCaptureManager::~ScrollCaptureManager() {
    if (m_timer && m_timer->isActive()) {
        m_timer->stop();
    }
    if (m_borderOverlay) {
        m_borderOverlay->close();
        m_borderOverlay->deleteLater();
        m_borderOverlay = nullptr;
    }
    if (m_controlWidget) {
        m_controlWidget->close();
        m_controlWidget->deleteLater();
        m_controlWidget = nullptr;
    }
}

void ScrollCaptureManager::excludeFromCapture(WId winId) {
    Platform::excludeWindowFromCapture(winId);
}

QImage ScrollCaptureManager::grabFrame() {
    QScreen* screen = QGuiApplication::screenAt(m_logicalRect.center());
    if (!screen) screen = QGuiApplication::primaryScreen();
    if (!screen) return QImage();

    // QScreen::grabWindow expects coordinates and size in logical coordinates relative to the target screen
    int localLogicalX = m_logicalRect.x() - screen->geometry().x();
    int localLogicalY = m_logicalRect.y() - screen->geometry().y();
    int logicalW = m_logicalRect.width();
    int logicalH = m_logicalRect.height();

    QPixmap pix = screen->grabWindow(0, localLogicalX, localLogicalY, logicalW, logicalH);
    QImage img = pix.toImage();
    img.setDevicePixelRatio(m_ratio);
    return img.convertToFormat(QImage::Format_RGB32);
}

static QImage appendBottomRows(const QImage& base, const QImage& addition, int rowsToAdd) {
    if (rowsToAdd <= 0 || rowsToAdd > addition.height()) return base;
    int w = base.width();
    int newH = base.height() + rowsToAdd;
    QImage result(w, newH, base.format());

    for (int y = 0; y < base.height(); ++y) {
        memcpy(result.scanLine(y), base.constScanLine(y), static_cast<size_t>(base.bytesPerLine()));
    }
    int startY = addition.height() - rowsToAdd;
    for (int i = 0; i < rowsToAdd; ++i) {
        memcpy(result.scanLine(base.height() + i), addition.constScanLine(startY + i), static_cast<size_t>(base.bytesPerLine()));
    }
    return result;
}

static QImage appendTopRows(const QImage& base, const QImage& addition, int rowsToAdd) {
    if (rowsToAdd <= 0 || rowsToAdd > addition.height()) return base;
    int w = base.width();
    int newH = base.height() + rowsToAdd;
    QImage result(w, newH, base.format());

    for (int i = 0; i < rowsToAdd; ++i) {
        memcpy(result.scanLine(i), addition.constScanLine(i), static_cast<size_t>(base.bytesPerLine()));
    }
    for (int y = 0; y < base.height(); ++y) {
        memcpy(result.scanLine(rowsToAdd + y), base.constScanLine(y), static_cast<size_t>(base.bytesPerLine()));
    }
    return result;
}

void ScrollCaptureManager::onProcessFrame() {
    QImage currentFrame = grabFrame();
    if (currentFrame.isNull() || m_lastFrame.isNull() || currentFrame.width() != m_lastFrame.width() || currentFrame.height() != m_lastFrame.height()) {
        m_lastFrame = currentFrame;
        return;
    }

    int H = currentFrame.height();
    int W = currentFrame.width();
    int templateHeight = std::min(300, H / 3);
    if (templateHeight < 10 || W < 10) return;

    int midY = H / 2;
    int startY = midY - (templateHeight / 2);
    int endY = startY + templateHeight;

    // Check template variance (SAD against template average color) to skip flat regions
    quint64 sumR = 0, sumG = 0, sumB = 0;
    int sampleCount = 0;
    for (int y = startY; y < endY; y += 4) {
        const QRgb* line = reinterpret_cast<const QRgb*>(m_lastFrame.constScanLine(y));
        for (int x = 0; x < W; x += 8) {
            QRgb c = line[x];
            sumR += qRed(c);
            sumG += qGreen(c);
            sumB += qBlue(c);
            sampleCount++;
        }
    }
    if (sampleCount <= 0) sampleCount = 1;
    int avgR = static_cast<int>(sumR / sampleCount);
    int avgG = static_cast<int>(sumG / sampleCount);
    int avgB = static_cast<int>(sumB / sampleCount);

    quint64 varSum = 0;
    for (int y = startY; y < endY; y += 4) {
        const QRgb* line = reinterpret_cast<const QRgb*>(m_lastFrame.constScanLine(y));
        for (int x = 0; x < W; x += 8) {
            QRgb c = line[x];
            varSum += std::abs(qRed(c) - avgR) + std::abs(qGreen(c) - avgG) + std::abs(qBlue(c) - avgB);
        }
    }
    qreal avgDev = static_cast<qreal>(varSum) / sampleCount;
    if (avgDev < 5.0) {
        m_lastFrame = currentFrame;
        return;
    }

    // Coarse search for matching Y position of template within currentFrame
    int minYSearch = std::max(0, startY - 300);
    int maxYSearch = std::min(H - templateHeight, startY + 300);

    int bestCoarseY = startY;
    qreal minCoarseDiff = 1e9;

    for (int candY = minYSearch; candY <= maxYSearch; candY += 2) {
        quint64 diffSum = 0;
        int count = 0;
        for (int ty = 0; ty < templateHeight; ty += 4) {
            const QRgb* lineTempl = reinterpret_cast<const QRgb*>(m_lastFrame.constScanLine(startY + ty));
            const QRgb* lineCand = reinterpret_cast<const QRgb*>(currentFrame.constScanLine(candY + ty));
            for (int x = 0; x < W; x += 8) {
                QRgb c1 = lineTempl[x];
                QRgb c2 = lineCand[x];
                diffSum += std::abs(qRed(c1) - qRed(c2)) + std::abs(qGreen(c1) - qGreen(c2)) + std::abs(qBlue(c1) - qBlue(c2));
                count++;
            }
        }
        if (count <= 0) count = 1;
        qreal avgDiff = static_cast<qreal>(diffSum) / (count * 3.0);
        if (avgDiff < minCoarseDiff) {
            minCoarseDiff = avgDiff;
            bestCoarseY = candY;
        }
    }

    // Fine search around bestCoarseY
    int bestY = bestCoarseY;
    qreal minFineDiff = minCoarseDiff;
    for (int candY = std::max(minYSearch, bestCoarseY - 2); candY <= std::min(maxYSearch, bestCoarseY + 2); ++candY) {
        quint64 diffSum = 0;
        int count = 0;
        for (int ty = 0; ty < templateHeight; ty += 2) {
            const QRgb* lineTempl = reinterpret_cast<const QRgb*>(m_lastFrame.constScanLine(startY + ty));
            const QRgb* lineCand = reinterpret_cast<const QRgb*>(currentFrame.constScanLine(candY + ty));
            for (int x = 0; x < W; x += 4) {
                QRgb c1 = lineTempl[x];
                QRgb c2 = lineCand[x];
                diffSum += std::abs(qRed(c1) - qRed(c2)) + std::abs(qGreen(c1) - qGreen(c2)) + std::abs(qBlue(c1) - qBlue(c2));
                count++;
            }
        }
        if (count <= 0) count = 1;
        qreal avgDiff = static_cast<qreal>(diffSum) / (count * 3.0);
        if (avgDiff < minFineDiff) {
            minFineDiff = avgDiff;
            bestY = candY;
        }
    }

    // High correlation check (average channel difference < 18 out of 255)
    if (minFineDiff < 18.0) {
        int scrolledPixels = startY - bestY;
        if (scrolledPixels != 0) {
            m_currentY += scrolledPixels;
            int frameH = currentFrame.height();

            if (m_currentY + frameH > m_accumulatedImage.height()) {
                int pixelsToAdd = (m_currentY + frameH) - m_accumulatedImage.height();
                m_accumulatedImage = appendBottomRows(m_accumulatedImage, currentFrame, pixelsToAdd);
            }

            if (m_currentY < 0) {
                int pixelsToAdd = std::abs(m_currentY);
                m_accumulatedImage = appendTopRows(m_accumulatedImage, currentFrame, pixelsToAdd);
                m_currentY = 0;
            }

            m_lastFrame = currentFrame;
            updatePreview();

            if (m_accumulatedImage.height() > 20000) {
                m_timer->stop();
                onFinishCapture();
            }
        }
    } else {
        m_lastFrame = currentFrame;
        m_currentY = std::max(0, m_accumulatedImage.height() - currentFrame.height());
    }
}

void ScrollCaptureManager::updatePreview() {
    if (m_accumulatedImage.isNull()) return;

    int physH = m_accumulatedImage.height();
    int physW = m_accumulatedImage.width();
    int logicalH = qRound(physH / m_ratio);

    if (m_borderOverlay) {
        m_borderOverlay->setCapturedDimensions(logicalH, physW, physH);
    }

    if (m_toolbar && m_toolbar->statusLabel()) {
        m_toolbar->statusLabel()->setText(QString("Scrolling... (%1 × %2 px)").arg(physW).arg(physH));
    }

    QPixmap pix = QPixmap::fromImage(m_accumulatedImage).scaled(
        80, 120,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );
    if (m_previewLabel) {
        m_previewLabel->setPixmap(pix);
    }
}

void ScrollCaptureManager::onFinishCapture() {
    if (m_timer && m_timer->isActive()) {
        m_timer->stop();
    }
    QPixmap finalPix = QPixmap::fromImage(m_accumulatedImage);
    finalPix.setDevicePixelRatio(m_ratio);
    if (m_borderOverlay) {
        m_borderOverlay->close();
        m_borderOverlay->deleteLater();
        m_borderOverlay = nullptr;
    }
    if (m_controlWidget) {
        m_controlWidget->close();
        m_controlWidget->deleteLater();
        m_controlWidget = nullptr;
    }
    emit completed(finalPix);
    deleteLater();
}

void ScrollCaptureManager::onCancelCapture() {
    if (m_timer && m_timer->isActive()) {
        m_timer->stop();
    }
    if (m_borderOverlay) {
        m_borderOverlay->close();
        m_borderOverlay->deleteLater();
        m_borderOverlay = nullptr;
    }
    if (m_controlWidget) {
        m_controlWidget->close();
        m_controlWidget->deleteLater();
        m_controlWidget = nullptr;
    }
    emit cancelled();
    deleteLater();
}

} // namespace ScreenCut
