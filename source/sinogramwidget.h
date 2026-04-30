/****************************
 * Author: Sanghyeb(Sam) Lee
 * Date: Jan/2013 | Modernized 2024
 * Copyright 2013 Sang hyeb(Sam) Lee MIT License
 *
 * X-ray CT simulation & reconstruction
 *****************************/

#pragma once

#include <QWidget>
#include <QSize>

#include <vector>

// Renders the sinogram as a greyscale image.
// Receives the sinogram as a const reference to the vector-of-vectors stored
// inside DrawingArea, and the zoom factor at construction time. No raw
// double** pointers; no heap allocation in paintEvent.

class SinogramWidget : public QWidget
{
    Q_OBJECT
public:
    // sinogram[projection][detector], size = projections × detectors
    SinogramWidget(const std::vector<std::vector<double>>& sinogram,
                   QSize size, double zoomFactor,
                   QWidget* parent = nullptr);

    [[nodiscard]] QSize sinogramSize() const { return sinogramSize_; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    // Snapshot copy taken at construction so the widget is self-contained
    // and safe to display even after the DrawingArea data is rebuilt.
    std::vector<std::vector<double>> sinogram_;
    QSize  sinogramSize_;
    double zoomFactor_;
};
