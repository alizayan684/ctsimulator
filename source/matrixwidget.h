/****************************
 * Author: Sanghyeb(Sam) Lee
 * Date: Jan/2013 | Modernized 2024
 * Copyright 2013 Sang hyeb(Sam) Lee MIT License
 *
 * X-ray CT simulation & reconstruction
 *****************************/

#pragma once

#include <QWidget>
#include <QPoint>
#include <QSize>

#include <vector>

// Displays a single row of the system matrix A as a colour-coded grid,
// overlaid with the corresponding source (red) and detector (blue) points.
// Stores the matrix data as a std::vector<double> by value so it remains
// valid even after the system matrix is rebuilt.

class MatrixWidget : public QWidget
{
    Q_OBJECT
public:
    MatrixWidget(QPoint source, QPoint detector,
                 QSize matrixSize, std::vector<double> matrixData,
                 QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QPoint              source_;
    QPoint              detector_;
    QSize               matrixSize_;
    std::vector<double> matrixData_; // owns a copy — safe lifetime
};
