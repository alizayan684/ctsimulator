#pragma once
#include <QObject>
#include <QRectF>
#include <QVector>

namespace visualization {

/**
 * @brief Provides Region of Interest (ROI) analysis tools, including line profiles and histograms.
 */
class ROITools : public QObject {
    Q_OBJECT

public:
    explicit ROITools(QObject* parent = nullptr);

    // Calculates histogram for a specific region
    Q_INVOKABLE QVector<int> calculateHistogram(const QVector<double>& data, int width, int height, QRectF region, int bins = 256);
    
    // Calculates line profile between two points
    Q_INVOKABLE QVector<double> calculateLineProfile(const QVector<double>& data, int width, int height, QPointF start, QPointF end);
};

} // namespace visualization
