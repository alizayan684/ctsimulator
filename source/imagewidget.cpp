/****************************
 * Author: Sanghyeb(Sam) Lee
 * Date: Jan/2013 | Modernized 2024
 * Copyright 2013 Sang hyeb(Sam) Lee MIT License
 *
 * X-ray CT simulation & reconstruction
 *****************************/

#include "imagewidget.h"

#include <QPainter>
#include <QStyleOption>

#include <algorithm>

ImageWidget::ImageWidget(const std::vector<double>& data,
                         int rows, int cols,
                         double zoomFactor,
                         QWidget* parent)
    : QWidget(parent)
    , rows_(rows)
    , cols_(cols)
    , zoomFactor_(zoomFactor)
{
    setStyleSheet("background-color:black;");

    // Build the QImage from the pixel data.
    // Matrix::qimage() normalises to [0,255] automatically.
    Matrix m(data, rows, cols);
    image_ = m.qimage();   // QImage owns its buffer by value — no heap leak
}

void ImageWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);

    // Apply the widget's stylesheet (background colour, etc.)
    QStyleOption opt;
    opt.initFrom(this);   // replaces the removed Qt4 opt.init(this)
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    if (image_.isNull()) return;

    const QSize scaledSize(
        static_cast<int>(image_.width()  * zoomFactor_),
        static_cast<int>(image_.height() * zoomFactor_));
    const QImage scaled = image_.scaled(scaledSize);

    const QPointF origin(width()  / 2.0 - scaled.width()  / 2.0,
                         height() / 2.0 - scaled.height() / 2.0);
    painter.drawImage(origin, scaled);
}
