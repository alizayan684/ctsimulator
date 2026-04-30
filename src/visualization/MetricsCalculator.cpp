#include "MetricsCalculator.h"
#include <cmath>

namespace visualization {

MetricsCalculator::MetricsCalculator(QObject* parent) : QObject(parent) {
}

double MetricsCalculator::calculateRMSE(const QVector<double>& reference, const QVector<double>& target) {
    if (reference.size() != target.size() || reference.isEmpty()) return 0.0;
    
    double sumSqError = 0.0;
    for (int i = 0; i < reference.size(); ++i) {
        double diff = reference[i] - target[i];
        sumSqError += diff * diff;
    }
    
    return std::sqrt(sumSqError / reference.size());
}

double MetricsCalculator::calculatePSNR(const QVector<double>& reference, const QVector<double>& target, double maxVal) {
    double rmse = calculateRMSE(reference, target);
    if (rmse == 0.0) return 100.0; // Identical images
    
    return 20.0 * std::log10(maxVal / rmse);
}

double MetricsCalculator::calculateSSIM(const QVector<double>& reference, const QVector<double>& target, int width, int height) {
    // TODO: Implement full SSIM algorithm
    return 1.0; 
}

} // namespace visualization
