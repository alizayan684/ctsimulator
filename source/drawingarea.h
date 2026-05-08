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

#include <QObject>
#include <QImage>
#include <QSize>

#include <complex>
#include <vector>

using Complex = std::complex<double>;

class DrawingArea : public QObject
{
    Q_OBJECT
public:
    explicit DrawingArea(QObject* parent = nullptr);

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

    int  numProjections_  = 0;

    // System matrix A[row][col] where row = projection*detector, col = pixel
    std::vector<std::vector<double>> A_;
    QSize A_size_;

    // Sinogram[projection][detector]
    std::vector<std::vector<double>> sinogram_;
    QSize sinogram_size_;

};
