#include "ImageViewer.h"
#include <QPainter>

namespace visualization {

ImageViewer::ImageViewer(QQuickItem* parent) : QQuickPaintedItem(parent) {
    // Setup smooth rendering
    setAntialiasing(true);
}

QImage ImageViewer::image() const {
    return m_image;
}

void ImageViewer::setImage(const QImage& img) {
    if (m_image != img) {
        m_image = img;
        emit imageChanged();
        update(); // Trigger repaint
    }
}

void ImageViewer::paint(QPainter* painter) {
    if (!m_image.isNull()) {
        // Draw the image scaled to fit the item, keeping aspect ratio
        painter->setRenderHint(QPainter::SmoothPixmapTransform);
        QRectF targetRect = boundingRect();
        QImage scaledImg = m_image.scaled(targetRect.size().toSize(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        
        // Center the image
        QPointF offset((targetRect.width() - scaledImg.width()) / 2.0,
                       (targetRect.height() - scaledImg.height()) / 2.0);
                       
        painter->drawImage(offset, scaledImg);
    }
}

} // namespace visualization
