/****************************
 * ART (Algebraic Reconstruction Technique)
 * Refactored to use ReconstructionAlgorithm base class
 *****************************/

#pragma once

#include "ReconstructionAlgorithm.h"

class ART : public ReconstructionAlgorithm
{
    Q_OBJECT
    Q_DISABLE_COPY(ART)
public:
    ART(DrawingArea* area, int sweeps, double relaxation, int showEvery,
        QObject* parent = nullptr);

protected:
    void run() override;
};
