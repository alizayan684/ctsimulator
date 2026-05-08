/****************************
 * Author: Sanghyeb(Sam) Lee
 * Date: Jan/2013 | Modernized 2024
 * Copyright 2013 Sang hyeb(Sam) Lee MIT License
 *****************************/

#include "matrix.h"

#include <stdexcept>
#include <algorithm>

Matrix::Matrix(std::vector<double> data, int M, int N)
    : data_(std::move(data)), M_(M), N_(N)
{
    if (M_ <= 0 || N_ <= 0)
        throw std::invalid_argument("Matrix dimensions must be positive");
    if (static_cast<int>(data_.size()) != M_ * N_)
        throw std::invalid_argument("Data size does not match declared dimensions");
}

Matrix::Matrix(const double* data, int M, int N)
    : data_(data, data + M * N), M_(M), N_(N)
{
    if (M_ <= 0 || N_ <= 0)
        throw std::invalid_argument("Matrix dimensions must be positive");
}

double& Matrix::at(int row, int col)
{
    if (row < 0 || row >= M_ || col < 0 || col >= N_)
        throw std::out_of_range("Matrix index out of range");
    return data_[row * N_ + col];
}

double Matrix::at(int row, int col) const
{
    if (row < 0 || row >= M_ || col < 0 || col >= N_)
        throw std::out_of_range("Matrix index out of range");
    return data_[row * N_ + col];
}

QSize Matrix::qsize() const
{
    return QSize(N_, M_); // Qt: width=cols, height=rows
}

QImage Matrix::qimage() const
{
    // Normalise to the full 0–255 range, handling negative values
    const double minVal = *std::min_element(data_.begin(), data_.end());
    const double maxVal = *std::max_element(data_.begin(), data_.end());
    const double range  = maxVal - minVal;
    const double scale  = (range > 0.0) ? 255.0 / range : 0.0;

    QImage image(qsize(), QImage::Format_RGB32);
    for (int i = 0; i < M_; ++i) {
        for (int j = 0; j < N_; ++j) {
            const int intensity = static_cast<int>((data_[i * N_ + j] - minVal) * scale);
            const int clamped   = std::clamp(intensity, 0, 255);
            image.setPixel(j, i, qRgb(clamped, clamped, clamped));
        }
    }
    return image;
}
