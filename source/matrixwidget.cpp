/****************************
 * Author: Sanghyeb(Sam) Lee
 * Date: Jan/2013 | Modernized 2024
 * Copyright 2013 Sang hyeb(Sam) Lee MIT License
 *
 * X-ray CT simulation & reconstruction
 *****************************/

#include "matrixwidget.h"

#include <QPainter>
#include <QPen>
#include <QBrush>

MatrixWidget::MatrixWidget(QPoint source, QPoint detector,
                           QSize matrixSize, std::vector<double> matrixData,
                           QWidget* parent)
    : QWidget(parent)
    , source_(source)
    , detector_(detector)
    , matrixSize_(matrixSize)
    , matrixData_(std::move(matrixData))   // take ownership, no copy
{}

void MatrixWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);

    // Centre the matrix grid inside the widget
    constexpr int cellSize = 5;
    const int originX = width()  / 2 - (matrixSize_.width()  * cellSize / 2);
    const int originY = height() / 2 - (matrixSize_.height() * cellSize / 2);

    // Draw each matrix cell: orange if non-zero (ray passes through), empty otherwise
    for (int i = 0; i < matrixSize_.height(); ++i) {
        for (int j = 0; j < matrixSize_.width(); ++j) {
            const double val = matrixData_[static_cast<std::size_t>(i * matrixSize_.width() + j)];
            if (val != 0.0)
                painter.setBrush(QBrush(QColor(0xc5, 0x6c, 0x00)));
            else
                painter.setBrush(Qt::NoBrush);
            painter.drawRect(originX + j * cellSize,
                             originY + i * cellSize,
                             cellSize, cellSize);
        }
    }

    const int cx = width()  / 2;
    const int cy = height() / 2;

    // Source point (red)
    painter.setPen(QPen(Qt::red, 5, Qt::SolidLine));
    painter.drawPoint(cx + source_.x() * cellSize,
                      cy + source_.y() * cellSize);

    // Detector point (blue)
    painter.setPen(QPen(Qt::blue, 5, Qt::SolidLine));
    painter.drawPoint(cx + detector_.x() * cellSize,
                      cy + detector_.y() * cellSize);
}
