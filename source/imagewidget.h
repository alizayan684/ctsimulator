/****************************
 * Author: Sanghyeb(Sam) Lee
 * Date: Jan/2013 | Modernized 2024
 * Copyright 2013 Sang hyeb(Sam) Lee MIT License
 *
 * X-ray CT simulation & reconstruction
 *****************************/

#pragma once

#include "matrix.h"

#include <QWidget>
#include <QImage>

// Displays a reconstructed image produced by ART or SIRT.
// Takes a snapshot of the pixel data vector at construction time so it is
// completely self-contained and safe after the thread buffers are recycled.

class ImageWidget : public QWidget
{
    Q_OBJECT
public:
    // data: flat row-major pixel vector (length = rows * cols)
    // zoomFactor: inherited from the main drawing area at call time
    ImageWidget(const std::vector<double>& data,
                int rows, int cols,
                double zoomFactor,
                QWidget* parent = nullptr);

    [[nodiscard]] int imageRows() const { return rows_; }
    [[nodiscard]] int imageCols() const { return cols_; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QImage image_;        // owns its pixel buffer — no raw pointer needed
    int    rows_;
    int    cols_;
    double zoomFactor_;
};
