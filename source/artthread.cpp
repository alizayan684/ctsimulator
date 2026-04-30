/****************************
 * Author: Sanghyeb(Sam) Lee
 * Date: Jan/2013 | Modernized 2024
 * Copyright 2013 Sang hyeb(Sam) Lee MIT License
 *
 * X-ray CT simulation & reconstruction
 *****************************/

#include "artthread.h"

#include <cmath>
#include <iostream>

ArtThread::ArtThread(DrawingArea* area, int sweeps, double relaxation,
                     int showEvery, QObject* parent)
    : QThread(parent)
    , area_(area)
    , sweeps_(sweeps)
    , relaxation_(relaxation)
    , showEvery_(showEvery)
{}

void ArtThread::run()
{
    std::cout << "ArtThread: started.\n";

    // ── Pull const refs to the data computed in DrawingArea ──────────────────
    // These are read-only during reconstruction; DrawingArea must not be
    // modified while the thread is running.
    const auto& A        = area_->systemMatrix();   // [row][pixel]
    const auto& sinogram = area_->sinogram();       // [projection][detector]
    const auto& phantom  = area_->pixelData();      // ground-truth pixel vector

    const int aRows    = area_->systemMatrixSize().height();
    const int aCols    = area_->systemMatrixSize().width();
    const int sinoW    = area_->sinogramSize().width();   // projections
    const int sinoH    = area_->sinogramSize().height();  // detectors

    // Flatten sinogram into a 1-D vector b of length aRows
    std::vector<double> b;
    b.reserve(static_cast<std::size_t>(sinoW * sinoH));
    for (int i = 0; i < sinoW; ++i)
        for (int j = 0; j < sinoH; ++j)
            b.push_back(sinogram[static_cast<std::size_t>(i)]
                                 [static_cast<std::size_t>(j)]);

    // x is the current estimate of the reconstructed image (all zeros to start)
    std::vector<double> x(static_cast<std::size_t>(aCols), 0.0);
    // x_prev holds the previous iteration's estimate
    std::vector<double> x_prev(static_cast<std::size_t>(aCols), 0.0);

    // ── Main ART iteration ───────────────────────────────────────────────────
    // ART update rule (Kaczmarz):
    //   x_new = x_old + λ * (b[i] - A[i]·x_old) / ||A[i]||² * A[i]
    // where λ is the relaxation factor.

    for (int sweep = 0; sweep < sweeps_ && running.load(); ++sweep) {
        for (int i = 0; i < aRows && running.load(); ++i) {
            // Swap buffers instead of copying (zero-allocation inner loop)
            std::swap(x, x_prev);

            const auto& row = A[static_cast<std::size_t>(i)];

            double nominator   = 0.0;
            double denominator = 0.0;
            for (int h = 0; h < aCols; ++h) {
                nominator   += row[static_cast<std::size_t>(h)] * x_prev[static_cast<std::size_t>(h)];
                denominator += row[static_cast<std::size_t>(h)] * row[static_cast<std::size_t>(h)];
            }

            const double updateFactor = (denominator == 0.0)
                ? 0.0
                : (b[static_cast<std::size_t>(i)] - nominator) / denominator;

            for (int j = 0; j < aCols; ++j) {
                x[static_cast<std::size_t>(j)] =
                    x_prev[static_cast<std::size_t>(j)]
                    + relaxation_ * row[static_cast<std::size_t>(j)] * updateFactor;
            }
        }

        // ── Emit snapshot every N sweeps ─────────────────────────────────────
        if ((sweep + 1) % showEvery_ == 0) {
            // Copy into a new vector — the signal carries ownership to the
            // main thread via Qt's queued connection (thread-safe).
            emit updateReady(sweep + 1, x);
        }

        // ── Compute L1 residual vs ground-truth phantom ───────────────────────
        double residual = 0.0;
        for (int z = 0; z < aCols; ++z)
            residual += std::fabs(phantom[static_cast<std::size_t>(z)]
                                  - x[static_cast<std::size_t>(z)]);

        emit singleIteration(sweep + 1, residual);
    }

    std::cout << "ArtThread: finished.\n";
}
