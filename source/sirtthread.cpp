/****************************
 * Author: Sanghyeb(Sam) Lee
 * Date: Jan/2013 | Modernized 2024
 * Copyright 2013 Sang hyeb(Sam) Lee MIT License
 *
 * X-ray CT simulation & reconstruction
 *****************************/

#include "sirtthread.h"

#include <cmath>
#include <iostream>

SirtThread::SirtThread(DrawingArea* area, int sweeps, double relaxation,
                       int showEvery, QObject* parent)
    : QThread(parent)
    , area_(area)
    , sweeps_(sweeps)
    , relaxation_(relaxation)
    , showEvery_(showEvery)
{}

void SirtThread::run()
{
    std::cout << "SirtThread: started.\n";

    // ── Pull const refs to the shared data (read-only during reconstruction) ─
    const auto& A        = area_->systemMatrix();
    const auto& sinogram = area_->sinogram();
    const auto& phantom  = area_->pixelData();

    const int aRows = area_->systemMatrixSize().height();
    const int aCols = area_->systemMatrixSize().width();
    const int sinoW = area_->sinogramSize().width();
    const int sinoH = area_->sinogramSize().height();

    // Flatten sinogram → b  (length = aRows)
    std::vector<double> b;
    b.reserve(static_cast<std::size_t>(sinoW * sinoH));
    for (int i = 0; i < sinoW; ++i)
        for (int j = 0; j < sinoH; ++j)
            b.push_back(sinogram[static_cast<std::size_t>(i)]
                                 [static_cast<std::size_t>(j)]);

    // Current estimate (all zeros)
    std::vector<double> x(static_cast<std::size_t>(aCols), 0.0);
    std::vector<double> x_prev(static_cast<std::size_t>(aCols), 0.0);

    // diff = b - Ax  (one entry per row of A)
    std::vector<double> diff(static_cast<std::size_t>(aRows), 0.0);

    // ── Main SIRT iteration ───────────────────────────────────────────────────
    // SIRT update rule:
    //   diff  = b - A·x_prev          (residual vector, length=aRows)
    //   x_new = x_prev + λ · Aᵀ·diff  (gradient step)

    for (int sweep = 0; sweep < sweeps_ && running.load(); ++sweep) {
        std::swap(x, x_prev);

        // Step 1: compute diff = b - A*x_prev
        for (int i = 0; i < aRows; ++i) {
            double Ax = 0.0;
            const auto& row = A[static_cast<std::size_t>(i)];
            for (int j = 0; j < aCols; ++j)
                Ax += row[static_cast<std::size_t>(j)] * x_prev[static_cast<std::size_t>(j)];
            diff[static_cast<std::size_t>(i)] = b[static_cast<std::size_t>(i)] - Ax;
        }

        // Step 2: x = x_prev + λ * Aᵀ * diff
        for (int i = 0; i < aCols; ++i) {
            double AtDiff = 0.0;
            for (int j = 0; j < aRows; ++j)
                AtDiff += A[static_cast<std::size_t>(j)][static_cast<std::size_t>(i)]
                          * diff[static_cast<std::size_t>(j)];
            x[static_cast<std::size_t>(i)] =
                x_prev[static_cast<std::size_t>(i)] + relaxation_ * AtDiff;
        }

        // ── Emit snapshot every N sweeps ─────────────────────────────────────
        if ((sweep + 1) % showEvery_ == 0)
            emit updateReady(sweep + 1, x);

        // ── Compute L1 residual vs ground-truth phantom ───────────────────────
        double residual = 0.0;
        for (int z = 0; z < aCols; ++z)
            residual += std::fabs(phantom[static_cast<std::size_t>(z)]
                                  - x[static_cast<std::size_t>(z)]);

        emit singleIteration(sweep + 1, residual);
    }

    std::cout << "SirtThread: finished.\n";
}
