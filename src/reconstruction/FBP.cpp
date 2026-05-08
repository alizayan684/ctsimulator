/****************************
 * FBP (Filtered Back Projection)
 * Parallel-beam geometry matching DrawingArea setup.
 *****************************/

#include "FBP.h"

#include <cmath>
#include <complex>
#include <algorithm>

FBP::FBP(DrawingArea* area, int sweeps, double relaxation, int showEvery,
         FilterType filter, QObject* parent)
    : ReconstructionAlgorithm(area, sweeps, relaxation, showEvery, parent)
    , filter_(filter)
{
    (void)sweeps;   // FBP is non-iterative; sweeps unused
    (void)relaxation;
    (void)showEvery;
}

QString FBP::filterName() const
{
    switch (filter_) {
        case RamLak:     return QStringLiteral("Ram-Lak");
        case SheppLogan: return QStringLiteral("Shepp-Logan");
        case Cosine:     return QStringLiteral("Cosine");
        case Hamming:    return QStringLiteral("Hamming");
        case Hann:       return QStringLiteral("Hann");
    }
    return QStringLiteral("Unknown");
}

// ── Iterative Cooley–Tukey FFT (power-of-two length required) ───────────
void FBP::fft(std::vector<std::complex<double>>& a, bool invert)
{
    const std::size_t n = a.size();
    if (n <= 1) return;

    // Bit-reversal permutation
    for (std::size_t i = 1, j = 0; i < n; ++i) {
        std::size_t bit = n >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j)
            std::swap(a[i], a[j]);
    }

    for (std::size_t len = 2; len <= n; len <<= 1) {
        const double ang = 2 * M_PI / static_cast<double>(len) * (invert ? -1 : 1);
        const std::complex<double> wlen(std::cos(ang), std::sin(ang));
        for (std::size_t i = 0; i < n; i += len) {
            std::complex<double> w(1.0);
            for (std::size_t j = 0; j < len / 2; ++j) {
                std::complex<double> u = a[i + j];
                std::complex<double> v = a[i + j + len / 2] * w;
                a[i + j] = u + v;
                a[i + j + len / 2] = u - v;
                w *= wlen;
            }
        }
    }

    if (invert) {
        for (auto& x : a)
            x /= static_cast<double>(n);
    }
}

// Build a length-n (power of two) ramp filter in the frequency domain.
// The filter is real and symmetric: index 0 = DC, index n/2 = Nyquist.
std::vector<double> FBP::buildRampFilter(int n, FilterType type)
{
    std::vector<double> filt(static_cast<std::size_t>(n), 0.0);
    const int half = n / 2;

    for (int i = 0; i <= half; ++i) {
        double v = M_PI * static_cast<double>(i) / static_cast<double>(half); // 0 … π
        double h = v; // base ramp |v|

        switch (type) {
            case RamLak:
                h = v;
                break;
            case SheppLogan:
                h = v * (std::sin(v * M_PI / 2.0) / (v * M_PI / 2.0 + 1e-12));
                break;
            case Cosine:
                h = v * std::cos(v * M_PI / 2.0);
                break;
            case Hamming:
                h = v * (0.54 + 0.46 * std::cos(v * M_PI));
                break;
            case Hann:
                h = v * (0.5 + 0.5 * std::cos(v * M_PI));
                break;
        }

        if (i == 0) h = 0.0; // suppress DC

        filt[static_cast<std::size_t>(i)] = h;
        if (i > 0 && i < half)
            filt[static_cast<std::size_t>(n - i)] = h;
    }

    return filt;
}

void FBP::run()
{
    const auto& sinogram = area_->sinogram();
    const int sinoW = area_->sinogramSize().width();   // projections
    const int sinoH = area_->sinogramSize().height();  // detector elements

    if (sinoW <= 0 || sinoH <= 0) {
        return;
    }

    // Power-of-two FFT length >= sinoH
    int nfft = 1;
    while (nfft < sinoH) nfft <<= 1;

    const auto filterFreq = buildRampFilter(nfft, filter_);

    // Prepare filtered sinogram (projections × detectors)
    std::vector<std::vector<double>> filtered;
    filtered.assign(static_cast<std::size_t>(sinoW),
                    std::vector<double>(static_cast<std::size_t>(sinoH), 0.0));

    for (int p = 0; p < sinoW && running.load(); ++p) {
        std::vector<std::complex<double>> buf(static_cast<std::size_t>(nfft));
        for (int d = 0; d < sinoH; ++d)
            buf[static_cast<std::size_t>(d)] = sinogram[static_cast<std::size_t>(p)][static_cast<std::size_t>(d)];
        for (int d = sinoH; d < nfft; ++d)
            buf[static_cast<std::size_t>(d)] = 0.0;

        fft(buf, false);

        for (int k = 0; k < nfft; ++k)
            buf[static_cast<std::size_t>(k)] *= filterFreq[static_cast<std::size_t>(k)];

        fft(buf, true);

        for (int d = 0; d < sinoH; ++d)
            filtered[static_cast<std::size_t>(p)][static_cast<std::size_t>(d)] = buf[static_cast<std::size_t>(d)].real();
    }

    // ── Backprojection ───────────────────────────────────────────────────
    QSize phSize = area_->phantomSize();
    const int W = phSize.width();
    const int H = phSize.height();
    const int nPix = W * H;

    std::vector<double> image(static_cast<std::size_t>(nPix), 0.0);

    // Detector geometry matches DrawingArea::loadSourceAndDetector
    // Detector runs from -R/√2 to +R/√2 ≈ [-W/2, W/2]
    const double R = 1.001 * W / std::sqrt(2.0);
    const double detectorHalfLength = R / std::sqrt(2.0);
    const double detSpacing = 2.0 * detectorHalfLength / (sinoH - 1); // approximate
    const double cx = W / 2.0;
    const double cy = H / 2.0;

    // Precompute angles
    std::vector<double> angles(static_cast<std::size_t>(sinoW));
    const double dAngle = 2.0 * M_PI / sinoW; // assumes full 360° coverage
    for (int p = 0; p < sinoW; ++p)
        angles[static_cast<std::size_t>(p)] = p * dAngle;

    for (int p = 0; p < sinoW && running.load(); ++p) {
        const double theta = angles[static_cast<std::size_t>(p)];
        const double c = std::cos(theta);
        const double s = std::sin(theta);

        for (int y = 0; y < H; ++y) {
            const double dy = y - cy;
            for (int x = 0; x < W; ++x) {
                const double dx = x - cx;
                // Parallel-beam: projection coordinate t = dx*cosθ + dy*sinθ
                const double t = dx * c + dy * s;
                // Map t to detector index
                double detIdx = t / detSpacing + (sinoH - 1) / 2.0;

                int i0 = static_cast<int>(std::floor(detIdx));
                int i1 = i0 + 1;
                double w1 = detIdx - static_cast<double>(i0);
                double w0 = 1.0 - w1;

                double val = 0.0;
                if (i0 >= 0 && i0 < sinoH)
                    val += w0 * filtered[static_cast<std::size_t>(p)][static_cast<std::size_t>(i0)];
                if (i1 >= 0 && i1 < sinoH)
                    val += w1 * filtered[static_cast<std::size_t>(p)][static_cast<std::size_t>(i1)];

                image[static_cast<std::size_t>(y * W + x)] += val;
            }
        }
    }

    // Normalise by number of projections
    const double norm = static_cast<double>(sinoW);
    if (norm > 0.0) {
        for (auto& v : image)
            v /= norm;
    }

    // Compute a simple L1 residual vs phantom if available
    const auto& phantom = area_->pixelData();
    double residual = 0.0;
    if (static_cast<int>(phantom.size()) == nPix) {
        for (int i = 0; i < nPix; ++i)
            residual += std::fabs(phantom[static_cast<std::size_t>(i)] - image[static_cast<std::size_t>(i)]);
    }

    emit updateReady(1, image);
    emit singleIteration(1, residual);
}
