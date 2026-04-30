/****************************
 * Author: Sanghyeb(Sam) Lee
 * Date: Jan/2013 | Modernized 2024
 * Copyright 2013 Sang hyeb(Sam) Lee MIT License
 *
 * X-ray CT simulation & reconstruction
 *****************************/

#include "utility.h"

#include <QFile>
#include <QTextStream>
#include <QStringList>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

// ── loadPhantom ──────────────────────────────────────────────────────────────

std::unique_ptr<Matrix> Utility::loadPhantom(const QString& filename)
{
    QFile inputFile(filename);
    if (!inputFile.open(QFile::ReadOnly))
        return nullptr;

    QTextStream stream(&inputFile);

    std::vector<std::vector<double>> rows;

    while (!stream.atEnd()) {
        const QString line = stream.readLine();
        if (line.isEmpty())
            continue;

        const QStringList tokens = line.split('\t');
        std::vector<double> row;
        row.reserve(static_cast<std::size_t>(tokens.size()));
        for (const QString& tok : tokens)
            row.push_back(tok.toDouble());

        // Validate that every row has the same number of columns
        if (!rows.empty() && row.size() != rows.front().size())
            return nullptr; // inconsistent column sizes

        rows.push_back(std::move(row));
    }

    if (rows.empty())
        return nullptr;

    const int M = static_cast<int>(rows.size());
    const int N = static_cast<int>(rows.front().size());

    std::vector<double> flat;
    flat.reserve(static_cast<std::size_t>(M * N));
    for (const auto& r : rows)
        flat.insert(flat.end(), r.begin(), r.end());

    return std::make_unique<Matrix>(std::move(flat), M, N);
}

// ── siddonRayTrace2D ─────────────────────────────────────────────────────────

// Alias for readability inside the algorithm
using Point2D = std::pair<double, double>;

std::vector<double> Utility::siddonRayTrace2D(
    double x_s, double y_s,
    double x_d, double y_d,
    double M,   double N)
{
    constexpr double tolerance = 1e-8;

    const std::size_t totalPixels = static_cast<std::size_t>(M * N);
    std::vector<double> rays(totalPixels, 0.0);

    if (std::fabs(x_s) < N / 2.0 && std::fabs(y_s) < M / 2.0) {
        std::cerr << "Error: source inside the grid\n";
        return rays;
    }
    if (std::fabs(x_d) < N / 2.0 && std::fabs(y_d) < M / 2.0) {
        std::cerr << "Error: detector inside the grid\n";
        return rays;
    }

    double x_sd = x_d - x_s;
    double y_sd = y_d - y_s;

    // Normalise so y component is always positive (simplifies later logic)
    if (y_sd < 0) { y_sd = -y_sd; x_sd = -x_sd; }

    const double rayLength       = std::sqrt(x_sd * x_sd + y_sd * y_sd);
    const double cosAngleRayH    = (x_sd * N) / (rayLength * std::fabs(N));

    // Use local vectors (stack RAII, no heap pointer ownership needed)
    std::vector<Point2D> crossingPoints;
    std::vector<Point2D> lineIndices;
    std::vector<Point2D> crossingH; // horizontal-line crossing points
    std::vector<Point2D> crossingV; // vertical-line crossing points

    // ── (1) Vertical ray (nearly 90°) ────────────────────────────────────────
    if (std::fabs(cosAngleRayH) < tolerance) {
        if (x_s >= -N / 2.0 && x_s <= N / 2.0) {
            for (double i = -M / 2.0; i <= M / 2.0; i += 1.0)
                crossingPoints.emplace_back(x_s, i);

            int nr = 0;
            for (int i = static_cast<int>(-M / 2.0); i <= static_cast<int>(M / 2.0); ++i) {
                if (x_s >= i) ++nr; else break;
            }
            nr = std::min(nr, static_cast<int>(M)) - 1;
            for (double y = 0; y <= M; y += 1.0)
                lineIndices.emplace_back(static_cast<double>(nr), y);
        }

    // ── (2) Horizontal ray (nearly 0°) ───────────────────────────────────────
    } else if (std::fabs(cosAngleRayH) > 1.0 - tolerance) {
        if (y_s >= -M / 2.0 && y_s <= M / 2.0) {
            for (double i = -N / 2.0; i <= N / 2.0; i += 1.0)
                crossingPoints.emplace_back(i, y_s);

            int nr = 0;
            for (double i = -N / 2.0; i <= N / 2.0; i += 1.0) {
                if (y_s >= i) ++nr; else break;
            }
            nr = std::min(nr, static_cast<int>(N)) - 1;
            for (double x = 0; x <= N; x += 1.0)
                lineIndices.emplace_back(x, static_cast<double>(nr));
        }

    // ── (3) General diagonal ray ─────────────────────────────────────────────
    } else {
        const double angle     = std::acos(cosAngleRayH);
        const double tanAngle  = std::tan(angle);

        // Find where ray crosses each horizontal grid line
        for (double y = -M / 2.0; y <= M / 2.0; y += 1.0) {
            double x = ((y - y_s) + tanAngle * x_s) / tanAngle;
            crossingH.emplace_back(x, y);
        }

        // Find where ray crosses each vertical grid line
        for (double x = -N / 2.0; x <= N / 2.0; x += 1.0) {
            double y = tanAngle * x + (y_s - tanAngle * x_s);
            crossingV.emplace_back(x, y);
        }

        if (tanAngle < 0)
            std::reverse(crossingV.begin(), crossingV.end());

        // Merge the two sorted lists of crossing points in y-order
        auto itH = crossingH.begin();
        auto itV = crossingV.begin();
        int idxV = 0, idxH = 0;

        while (itH != crossingH.end() && itV != crossingV.end()) {
            const double eps = std::numeric_limits<double>::epsilon();
            if (std::fabs(itH->second - itV->second) < eps) {
                crossingPoints.push_back(*itH);
                lineIndices.emplace_back(static_cast<double>(idxH),
                                         static_cast<double>(idxV));
                ++itH; ++itV; ++idxH; ++idxV;
            } else if (itH->second < itV->second) {
                crossingPoints.push_back(*itH);
                lineIndices.emplace_back(static_cast<double>(idxV - 1),
                                         static_cast<double>(idxH));
                ++itH; ++idxH;
            } else {
                crossingPoints.push_back(*itV);
                lineIndices.emplace_back(static_cast<double>(idxV),
                                         static_cast<double>(idxH - 1));
                ++itV; ++idxV;
            }
        }

        // Append any remaining horizontal crossing points
        for (; itH != crossingH.end(); ++itH, ++idxH) {
            crossingPoints.push_back(*itH);
            lineIndices.emplace_back(static_cast<double>(idxV - 1),
                                     static_cast<double>(idxH));
        }

        // Append any remaining vertical crossing points
        for (; itV != crossingV.end(); ++itV, ++idxV) {
            crossingPoints.push_back(*itV);
            lineIndices.emplace_back(static_cast<double>(idxV),
                                     static_cast<double>(idxH - 1));
        }

        // Discard crossing points that lie outside the grid boundary
        const double eps = std::numeric_limits<double>::epsilon();
        auto cpIt = crossingPoints.begin();
        auto liIt = lineIndices.begin();
        while (cpIt != crossingPoints.end()) {
            const bool outside =
                cpIt->first  < -N / 2.0 - eps || cpIt->first  > N / 2.0 + eps ||
                cpIt->second < -M / 2.0 - eps || cpIt->second > M / 2.0 + eps;
            if (outside) {
                cpIt = crossingPoints.erase(cpIt);
                liIt = lineIndices.erase(liIt);
            } else {
                ++cpIt; ++liIt;
            }
        }

        // Correct vertical line numbering for downward (negative-slope) rays
        if (tanAngle < 0) {
            for (auto& li : lineIndices)
                li.first = N - li.first;
        }
    }

    // ── Accumulate chord lengths into the ray vector ──────────────────────────
    if (crossingPoints.size() >= 2) {
        auto liIt = lineIndices.begin();
        for (std::size_t k = 1; k < crossingPoints.size(); ++k, ++liIt) {
            const auto& prev = crossingPoints[k - 1];
            const auto& curr = crossingPoints[k];
            const double dx       = curr.first  - prev.first;
            const double dy       = curr.second - prev.second;
            const double distance = std::sqrt(dx * dx + dy * dy);

            const auto col = static_cast<int>(liIt->first);
            const auto row = static_cast<int>(liIt->second);

            if (col >= 0 && col < static_cast<int>(N) &&
                row >= 0 && row < static_cast<int>(M)) {
                rays[static_cast<std::size_t>(row * N + col)] = distance;
            }
        }
    }

    return rays;
}
