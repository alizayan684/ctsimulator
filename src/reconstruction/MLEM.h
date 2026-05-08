/****************************
 * MLEM (Maximum Likelihood Expectation Maximization)
 * Poisson noise model support, configurable iterations, convergence monitoring
 *****************************/

#pragma once

#include "ReconstructionAlgorithm.h"

#include <vector>

class MLEM : public ReconstructionAlgorithm
{
    Q_OBJECT
    Q_DISABLE_COPY(MLEM)
public:
    MLEM(DrawingArea* area, int sweeps, double relaxation, int showEvery,
         QObject* parent = nullptr);

protected:
    void run() override;
};
