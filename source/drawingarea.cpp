/****************************
 * Author: Sanghyeb(Sam) Lee
 * Date: Jan/2013 | Modernized 2024
 * Copyright 2013 Sang hyeb(Sam) Lee MIT License
 *
 * X-ray CT simulation & reconstruction
 *****************************/

#include "drawingarea.h"

#include <cmath>
#include <complex>
#include <random>

DrawingArea::DrawingArea(QObject* parent)
    : QObject(parent)
{
}

// ── Public setup API ─────────────────────────────────────────────────────────

void DrawingArea::setPhantom(QImage image, std::vector<double> matrixData)
{
    phantomImage_ = std::move(image);
    pixelData_    = std::move(matrixData);
}

QSize DrawingArea::phantomSize() const
{
    return phantomImage_.isNull() ? QSize(0, 0) : phantomImage_.size();
}

void DrawingArea::loadSourceAndDetector(int q)
{
    // R = distance from iso-centre to source/detector along the x-axis
    const double R = 1.001 * phantomImage_.width() / std::sqrt(2.0);
    const double h = R / (std::sqrt(2.0) * q);

    numSourceElements_   = 2 * q + 1;
    numDetectorElements_ = 2 * q + 1;

    source_.resize(static_cast<std::size_t>(numSourceElements_));
    detector_.resize(static_cast<std::size_t>(numDetectorElements_));

    for (int i = 0; i < numSourceElements_; ++i) {
        const double imag = -h * q + i * h;
        source_[static_cast<std::size_t>(i)] = Complex(R, imag);
    }
    for (int i = 0; i < numDetectorElements_; ++i) {
        const double imag = -h * q + i * h;
        detector_[static_cast<std::size_t>(i)] = Complex(-R, imag);
    }
}

int DrawingArea::loadMovement(double startAngle, double endAngle, double incrementAngle)
{
    numProjections_ = static_cast<int>((endAngle - startAngle + 1.0) / incrementAngle);

    sourceProjections_.assign(static_cast<std::size_t>(numProjections_),
                               std::vector<Complex>(static_cast<std::size_t>(numSourceElements_)));
    detectorProjections_.assign(static_cast<std::size_t>(numProjections_),
                                 std::vector<Complex>(static_cast<std::size_t>(numDetectorElements_)));

    for (int i = 0; i < numProjections_; ++i) {
        const double angle          = startAngle + incrementAngle * i;
        const double angleInRadian  = angle * PI / 180.0;
        const Complex rotation       = std::exp(Complex(0.0, angleInRadian));

        for (int j = 0; j < numSourceElements_; ++j)
            sourceProjections_[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] =
                source_[static_cast<std::size_t>(j)] * rotation;

        for (int z = 0; z < numDetectorElements_; ++z)
            detectorProjections_[static_cast<std::size_t>(i)][static_cast<std::size_t>(z)] =
                detector_[static_cast<std::size_t>(z)] * rotation;
    }

    return numProjections_;
}


void DrawingArea::addNoise(double standardDeviation)
{
    // Properly seeded Mersenne Twister — no global state, no rand()
    std::mt19937 rng(std::random_device{}());
    std::normal_distribution<double> dist(0.0, standardDeviation);

    const int sinoW = sinogram_size_.width();   // projections
    const int sinoH = sinogram_size_.height();  // detectors

    // Apply independent Gaussian sample to every sinogram element.
    // The original Box-Muller implementation had a bug where it wrote
    // normal1 to sinogram[i][j] twice instead of normal2 to sinogram[i][j+1].
    // std::normal_distribution handles the pairing internally and correctly.
    for (int i = 0; i < sinoW; ++i)
        for (int j = 0; j < sinoH; ++j)
            sinogram_[static_cast<std::size_t>(i)]
                     [static_cast<std::size_t>(j)] += dist(rng);
}

void DrawingArea::setupSystemMatrix()
{
    const int totalRows    = numDetectorElements_ * numProjections_;
    const int totalPixels  = phantomImage_.height() * phantomImage_.width();

    A_.assign(static_cast<std::size_t>(totalRows),
              std::vector<double>(static_cast<std::size_t>(totalPixels), 0.0));

    A_size_.setWidth(totalPixels);
    A_size_.setHeight(numDetectorElements_ * numProjections_);

    sinogram_.assign(static_cast<std::size_t>(numProjections_),
                     std::vector<double>(static_cast<std::size_t>(numDetectorElements_), 0.0));

    sinogram_size_.setHeight(numDetectorElements_);
    sinogram_size_.setWidth(numProjections_);

    for (int i = 0; i < numProjections_; ++i) {
        for (int j = 0; j < numDetectorElements_; ++j) {
            const auto& src = sourceProjections_[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)];
            const auto& det = detectorProjections_[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)];

            // siddonRayTrace2D returns a std::vector<double> — no raw pointer
            std::vector<double> ray = Utility::siddonRayTrace2D(
                src.real(), src.imag(),
                det.real(), det.imag(),
                static_cast<double>(phantomImage_.height()),
                static_cast<double>(phantomImage_.width()));

            if (ray.empty()) {
                hasError_ = true;
                errorMessage_ = QStringLiteral("Ray trace failed for projection %1, detector %2").arg(i).arg(j);
                return;
            }

            const std::size_t rowIdx = static_cast<std::size_t>(i * numDetectorElements_ + j);
            A_[rowIdx] = ray; // move into the matrix

            // Forward projection: sino[i][j] = sum(ray * pixel)
            double sum = 0.0;
            for (int k = 0; k < totalPixels; ++k)
                sum += ray[static_cast<std::size_t>(k)] * pixelData_[static_cast<std::size_t>(k)];

            sinogram_[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] = sum;
        }
    }
}

