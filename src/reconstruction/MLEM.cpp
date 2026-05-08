/****************************
 * MLEM (Maximum Likelihood Expectation Maximization)
 * Poisson noise model. Update:
 *   x^{new}_j = x^{old}_j * (A^T (b / (A x^{old})))_j / (A^T 1)_j
 *****************************/

#include "MLEM.h"

#include <cmath>
#include <limits>

MLEM::MLEM(DrawingArea* area, int sweeps, double relaxation, int showEvery,
           QObject* parent)
    : ReconstructionAlgorithm(area, sweeps, relaxation, showEvery, parent)
{
}

void MLEM::run()
{
    const auto& A        = area_->systemMatrix();
    const auto& sinogram = area_->sinogram();
    const auto& phantom  = area_->pixelData();

    const int aRows = area_->systemMatrixSize().height();
    const int aCols = area_->systemMatrixSize().width();
    const int sinoW = area_->sinogramSize().width();
    const int sinoH = area_->sinogramSize().height();

    // Flatten sinogram → b
    std::vector<double> b;
    b.reserve(static_cast<std::size_t>(sinoW * sinoH));
    for (int i = 0; i < sinoW; ++i)
        for (int j = 0; j < sinoH; ++j)
            b.push_back(sinogram[static_cast<std::size_t>(i)]
                                 [static_cast<std::size_t>(j)]);

    // Initial uniform estimate (must be strictly positive)
    std::vector<double> x(static_cast<std::size_t>(aCols), 1.0);
    std::vector<double> x_prev(static_cast<std::size_t>(aCols), 1.0);

    // Precompute sensitivity image s = A^T * 1  (one entry per pixel)
    std::vector<double> sensitivity(static_cast<std::size_t>(aCols), 0.0);
    for (int j = 0; j < aCols; ++j) {
        double sum = 0.0;
        for (int i = 0; i < aRows; ++i)
            sum += A[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)];
        sensitivity[static_cast<std::size_t>(j)] = sum;
        if (sum < 1e-12)
            sensitivity[static_cast<std::size_t>(j)] = 1.0; // avoid division by zero
    }

    // Workspace vectors
    std::vector<double> ax(static_cast<std::size_t>(aRows), 0.0);
    std::vector<double> ratio(static_cast<std::size_t>(aRows), 0.0);
    std::vector<double> backproj(static_cast<std::size_t>(aCols), 0.0);

    double prevResidual = std::numeric_limits<double>::infinity();

    for (int sweep = 0; sweep < iterations_ && running.load(); ++sweep) {
        std::swap(x, x_prev);

        // 1) Forward project: Ax = A * x_prev
        for (int i = 0; i < aRows; ++i) {
            double sum = 0.0;
            const auto& row = A[static_cast<std::size_t>(i)];
            for (int j = 0; j < aCols; ++j)
                sum += row[static_cast<std::size_t>(j)] * x_prev[static_cast<std::size_t>(j)];
            ax[static_cast<std::size_t>(i)] = sum;
        }

        // 2) Ratio b / (Ax + epsilon)  — Poisson-likelihood correction
        for (int i = 0; i < aRows; ++i) {
            const double denom = ax[static_cast<std::size_t>(i)] + 1e-12;
            ratio[static_cast<std::size_t>(i)] = b[static_cast<std::size_t>(i)] / denom;
        }

        // 3) Backproject ratio: A^T * ratio
        for (int j = 0; j < aCols; ++j) {
            double sum = 0.0;
            for (int i = 0; i < aRows; ++i)
                sum += A[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)]
                       * ratio[static_cast<std::size_t>(i)];
            backproj[static_cast<std::size_t>(j)] = sum;
        }

        // 4) Update: x_j = x_prev_j * backproj_j / sensitivity_j * relaxation
        for (int j = 0; j < aCols; ++j) {
            double update = x_prev[static_cast<std::size_t>(j)]
                            * backproj[static_cast<std::size_t>(j)]
                            / sensitivity[static_cast<std::size_t>(j)];
            // Apply relaxation as a dampening factor (slightly away from 1.0)
            update = x_prev[static_cast<std::size_t>(j)]
                     + relaxation_ * (update - x_prev[static_cast<std::size_t>(j)]);
            // Enforce positivity
            if (update < 1e-12) update = 1e-12;
            x[static_cast<std::size_t>(j)] = update;
        }

        if ((sweep + 1) % showEvery_ == 0)
            emit updateReady(sweep + 1, x);

        // Convergence monitoring: L1 residual vs phantom
        double residual = 0.0;
        for (int z = 0; z < aCols; ++z)
            residual += std::fabs(phantom[static_cast<std::size_t>(z)]
                                  - x[static_cast<std::size_t>(z)]);

        emit singleIteration(sweep + 1, residual);

        // Early stopping criterion
        if (prevResidual != std::numeric_limits<double>::infinity()) {
            const double relChange = std::fabs(prevResidual - residual) / (prevResidual + 1e-12);
            if (relChange < 1e-6) {
                break;
            }
        }
        prevResidual = residual;
    }

    emit updateReady(iterations_, x);
}
