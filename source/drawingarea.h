/****************************
 * Author: Sanghyeb(Sam) Lee
 * Date: Jan/2013 | Modernized 2024
 * Copyright 2013 Sang hyeb(Sam) Lee MIT License
 *
 * X-ray CT simulation & reconstruction
 *****************************/

#pragma once

#include "utility.h"
#include "matrix.h"

#include <QWidget>
#include <QPainter>
#include <QStyleOption>
#include <QTimer>

#include <complex>
#include <memory>
#include <vector>

using Complex = std::complex<double>;

class DrawingArea : public QWidget
{
    Q_OBJECT
public:
    explicit DrawingArea(QWidget* parent = nullptr);

    void setPhantom(QImage image, std::vector<double> matrixData);
    void loadSourceAndDetector(int q);
    int  loadMovement(double startAngle, double endAngle, double incrementAngle);
    void setupSystemMatrix();
    void addNoise(double standardDeviation);  // Gaussian noise via Box-Muller, applied in-place

    // Read-only access for threads and sibling widgets
    [[nodiscard]] const std::vector<std::vector<double>>& systemMatrix() const { return A_; }
    [[nodiscard]] const std::vector<std::vector<double>>& sinogram()     const { return sinogram_; }
    [[nodiscard]] QSize  phantomSize()    const;
    [[nodiscard]] QSize  systemMatrixSize() const { return A_size_; }
    [[nodiscard]] QSize  sinogramSize()     const { return sinogram_size_; }
    [[nodiscard]] const std::vector<double>& pixelData() const { return pixelData_; }
    [[nodiscard]] int numProjections()       const { return numProjections_; }
    [[nodiscard]] int numSourceElements()    const { return numSourceElements_; }
    [[nodiscard]] int numDetectorElements()  const { return numDetectorElements_; }
    [[nodiscard]] const std::vector<std::vector<Complex>>& sourceProjections()   const { return sourceProjections_; }
    [[nodiscard]] const std::vector<std::vector<Complex>>& detectorProjections() const { return detectorProjections_; }

    bool visualizeLine = false;

    [[nodiscard]] double zoomFactor() const { return zoomFactor_; }
    void setZoomFactor(double z) { zoomFactor_ = z; update(); }

signals:
    void animationLoaded();

public slots:
    void animate();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    static constexpr double PI = 3.14159265358979323846;

    // Phantom image and flat pixel data
    QImage phantomImage_;
    std::vector<double> pixelData_;

    // Source / detector geometry
    std::vector<Complex> source_;
    std::vector<Complex> detector_;
    int numSourceElements_   = 0;
    int numDetectorElements_ = 0;

    // Gantry projections (indexed [projection][element])
    std::vector<std::vector<Complex>> sourceProjections_;
    std::vector<std::vector<Complex>> detectorProjections_;

    // Current frame pointers (non-owning views into the projection arrays)
    const Complex* currentSource_   = nullptr;
    const Complex* currentDetector_ = nullptr;

    // Animation state
    int  numProjections_  = 0;
    int  animationIndex_  = 0;
    std::unique_ptr<QTimer> timer_;

    // System matrix A[row][col] where row = projection*detector, col = pixel
    std::vector<std::vector<double>> A_;
    QSize A_size_;

    // Sinogram[projection][detector]
    std::vector<std::vector<double>> sinogram_;
    QSize sinogram_size_;

    // Widget canvas size
    int canvasWidth_  = 512;
    int canvasHeight_ = 512;
    double zoomFactor_ = 1.0;
};
