/****************************
 * Author: Sanghyeb(Sam) Lee
 * Date: Jan/2013 | Modernized 2024
 * Copyright 2013 Sang hyeb(Sam) Lee MIT License
 *
 * X-ray CT simulation & reconstruction
 *****************************/

#pragma once

#include <vector>
#include <memory>
#include <QString>
#include <QSize>
#include <QImage>

// Owns a flat row-major M×N matrix of doubles and can render it as a QImage.
// Data is stored in a std::vector<double> (RAII, no manual new/delete).
class Matrix
{
public:
    // Takes ownership of data. M = rows, N = columns.
    Matrix(std::vector<double> data, int M, int N);

    // Convenience: construct from a raw pointer (copies the data).
    Matrix(const double* data, int M, int N);

    [[nodiscard]] QSize  qsize()  const;
    [[nodiscard]] QImage qimage() const;   // returns by value — no heap allocation

    // Element access with bounds checking in debug builds
    [[nodiscard]] double& at(int row, int col);
    [[nodiscard]] double  at(int row, int col) const;

    // Safe copy of the underlying data — preferred over data() for most callers
    [[nodiscard]] std::vector<double> toVector() const { return data_; }

    // Raw data pointer — use only when interfacing with C APIs
    [[nodiscard]] double*       data()       { return data_.data(); }
    [[nodiscard]] const double* data() const { return data_.data(); }
    [[nodiscard]] int rows() const { return M_; }
    [[nodiscard]] int cols() const { return N_; }
    [[nodiscard]] std::size_t size() const { return data_.size(); }

private:
    std::vector<double> data_;
    int M_; // rows
    int N_; // columns
};
