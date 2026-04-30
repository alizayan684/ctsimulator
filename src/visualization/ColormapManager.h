#pragma once
#include <QObject>
#include <QImage>

namespace visualization {

/**
 * @brief Manages colormaps (e.g., Viridis, Plasma, Grayscale) for image visualization.
 */
class ColormapManager : public QObject {
    Q_OBJECT
public:
    explicit ColormapManager(QObject* parent = nullptr);

    enum class ColormapType {
        Grayscale,
        Viridis,
        Plasma,
        Inferno,
        Magma
    };

    Q_INVOKABLE QImage applyColormap(const QVector<double>& data, int width, int height, ColormapType type);

private:
    QRgb getViridisColor(double t) const;
    // Other color mapping functions will be added here
};

} // namespace visualization
