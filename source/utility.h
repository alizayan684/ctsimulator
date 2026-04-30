/****************************
 * Author: Sanghyeb(Sam) Lee
 * Date: Jan/2013 | Modernized 2024
 * Copyright 2013 Sang hyeb(Sam) Lee MIT License
 *
 * X-ray CT simulation & reconstruction
 *****************************/

#pragma once

#include "matrix.h"

#include <memory>
#include <vector>
#include <QString>

// Utility is a collection of pure functions. It is intentionally not
// instantiable — use the static methods directly.
class Utility
{
public:
    Utility() = delete;  // not instantiable

    [[nodiscard]] static std::unique_ptr<Matrix> loadPhantom(const QString& filename);

    [[nodiscard]] static std::vector<double> siddonRayTrace2D(
        double x_s, double y_s,
        double x_d, double y_d,
        double rows, double cols);
};
