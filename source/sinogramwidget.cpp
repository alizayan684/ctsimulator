/****************************
 * Author: Sanghyeb(Sam) Lee
 * Date: Jan/2013 | Modernized 2024
 * Copyright 2013 Sang hyeb(Sam) Lee MIT License
 *
 * X-ray CT simulation & reconstruction
 *****************************/

#include "sinogramwidget.h"

#include <QPainter>
#include <QImage>

#include <algorithm>

SinogramWidget::SinogramWidget(const std::vector<std::vector<double>>& sinogram,
                               QSize size, double zoomFactor, QWidget* parent)
    : QWidget(parent)
    , sinogram_(sinogram)   // snapshot copy — widget owns its data
    , sinogramSize_(size)
    , zoomFactor_(zoomFactor)
{}

void SinogramWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);

    // ── Find the maximum value for normalisation ──────────────────────────────
    double maxVal = 0.0;
    for (const auto& row : sinogram_)
        for (const double v : row)
            maxVal = std::max(maxVal, v);

    if (maxVal == 0.0) return; // nothing to draw

    // ── Build a greyscale QImage (no heap allocation — QImage owns its buffer)
    QImage image(sinogramSize_.width(), sinogramSize_.height(),
                 QImage::Format_RGB32);

    for (int i = 0; i < sinogramSize_.width(); ++i) {
        for (int j = 0; j < sinogramSize_.height(); ++j) {
            const int pv = static_cast<int>(
                sinogram_[static_cast<std::size_t>(i)]
                          [static_cast<std::size_t>(j)] / maxVal * 255.0);
            image.setPixel(i, j, qRgb(pv, pv, pv));
        }
    }

    // ── Scale and centre ──────────────────────────────────────────────────────
    const QSize scaledSize(
        static_cast<int>(sinogramSize_.width()  * zoomFactor_),
        static_cast<int>(sinogramSize_.height() * zoomFactor_));
    const QImage scaled = image.scaled(scaledSize);

    const QPointF origin(width()  / 2.0 - scaled.width()  / 2.0,
                         height() / 2.0 - scaled.height() / 2.0);
    painter.drawImage(origin, scaled);

    resize(scaledSize);
}
