/****************************
 * CT Image Provider Implementation
 *****************************/

#include "CTImageProvider.h"
#include "CTController.h"

CTImageProvider::CTImageProvider(CTController* controller, QObject* parent)
    : QQuickImageProvider(QQuickImageProvider::Image)
    , controller_(controller)
{
}

QImage CTImageProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize)
{
    QImage image;

    if (id == "phantom") {
        image = controller_->phantomImage();
    } else if (id == "sinogram") {
        image = controller_->sinogramImage();
    } else if (id == "reconstruction") {
        image = controller_->reconstructionImage();
    }

    if (image.isNull()) {
        // Return a placeholder image
        image = QImage(256, 256, QImage::Format_ARGB32);
        image.fill(Qt::darkGray);
    }

    if (size) {
        *size = image.size();
    }

    if (requestedSize.isValid() && !requestedSize.isNull()) {
        image = image.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    return image;
}
