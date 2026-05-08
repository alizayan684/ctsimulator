/****************************
 * ReconstructionAlgorithm — base implementation
 *****************************/

#include "ReconstructionAlgorithm.h"

ReconstructionAlgorithm::ReconstructionAlgorithm(DrawingArea* area, int sweeps,
                                                 double relaxation, int showEvery,
                                                 QObject* parent)
    : QThread(parent)
    , area_(area)
    , iterations_(sweeps)
    , relaxation_(relaxation)
    , showEvery_(showEvery)
{
}
