#pragma once
#include <QQuickPaintedItem>
#include <QImage>

namespace visualization {

/**
 * @brief Custom QML Item for rendering high-quality CT images with zoom/pan and colormaps.
 */
class ImageViewer : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(QImage image READ image WRITE setImage NOTIFY imageChanged)

public:
    explicit ImageViewer(QQuickItem* parent = nullptr);

    QImage image() const;
    void setImage(const QImage& img);

    void paint(QPainter* painter) override;

signals:
    void imageChanged();

private:
    QImage m_image;
    // Variables for zoom and pan will be added here
};

} // namespace visualization
