/****************************
 * SIRT (Simultaneous Iterative Reconstruction Technique)
 * Refactored to use ReconstructionAlgorithm base class
 *****************************/

#pragma once

#include "ReconstructionAlgorithm.h"

class SIRT : public ReconstructionAlgorithm
{
    Q_OBJECT
    Q_DISABLE_COPY(SIRT)
public:
    SIRT(DrawingArea* area, int sweeps, double relaxation, int showEvery,
         QObject* parent = nullptr);

protected:
    void run() override;
};
