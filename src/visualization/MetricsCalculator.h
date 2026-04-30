#pragma once
#include <QObject>
#include <QVector>

namespace visualization {

/**
 * @brief Calculates image quality metrics such as RMSE, SSIM, and PSNR.
 */
class MetricsCalculator : public QObject {
    Q_OBJECT

public:
    explicit MetricsCalculator(QObject* parent = nullptr);

    // Root Mean Square Error
    Q_INVOKABLE double calculateRMSE(const QVector<double>& reference, const QVector<double>& target);
    
    // Peak Signal-to-Noise Ratio
    Q_INVOKABLE double calculatePSNR(const QVector<double>& reference, const QVector<double>& target, double maxVal = 1.0);
    
    // Structural Similarity Index Measure
    Q_INVOKABLE double calculateSSIM(const QVector<double>& reference, const QVector<double>& target, int width, int height);
};

} // namespace visualization
