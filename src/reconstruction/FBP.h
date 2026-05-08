/****************************
 * FBP (Filtered Back Projection)
 * Supports: Ram-Lak, Shepp-Logan, Cosine, Hamming, Hann filters
 *****************************/

#pragma once

#include "ReconstructionAlgorithm.h"

#include <QString>

class FBP : public ReconstructionAlgorithm
{
    Q_OBJECT
    Q_DISABLE_COPY(FBP)
public:
    enum FilterType {
        RamLak,
        SheppLogan,
        Cosine,
        Hamming,
        Hann
    };

    FBP(DrawingArea* area, int sweeps, double relaxation, int showEvery,
        FilterType filter = RamLak, QObject* parent = nullptr);

    void setFilter(FilterType f) { filter_ = f; }
    [[nodiscard]] QString filterName() const;

protected:
    void run() override;

private:
    FilterType filter_;

    static void fft(std::vector<std::complex<double>>& a, bool invert);
    static std::vector<double> buildRampFilter(int n, FilterType type);
};
