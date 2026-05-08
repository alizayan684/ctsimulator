/****************************
 * Abstract Base Class for all reconstruction algorithms
 * Unified interface: ART, SIRT, FBP, MLEM
 *****************************/

#pragma once

#include "source/drawingarea.h"

#include <QThread>
#include <atomic>
#include <vector>

class ReconstructionAlgorithm : public QThread
{
    Q_OBJECT
    Q_DISABLE_COPY(ReconstructionAlgorithm)

public:
    explicit ReconstructionAlgorithm(DrawingArea* area, int sweeps,
                                   double relaxation, int showEvery,
                                   QObject* parent = nullptr);
    ~ReconstructionAlgorithm() override = default;

    std::atomic<bool> running{true};

    void setIterations(int sweeps)     { iterations_ = sweeps; }
    void setRelaxation(double lambda)  { relaxation_ = lambda; }
    void setShowEvery(int n)           { showEvery_ = n; }

signals:
    void updateReady(int iteration, std::vector<double> image);
    void singleIteration(int iteration, double residual);

protected:
    void run() override = 0;

    DrawingArea* area_;
    int          iterations_;
    double       relaxation_;
    int          showEvery_;
};
