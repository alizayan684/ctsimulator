/****************************
 * CT Image Provider Implementation
 *****************************/

#include "CTImageProvider.h"
#include "CTController.h"

CTImageProvider::CTImageProvider(CTController* controller)
    : QQuickImageProvider(QQuickImageProvider::Image)
    , controller_(controller)
{
}

QImage CTImageProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize)
{
    // Strip cache-busting query parameter (QML sends "phantom?1234567")
    const QString cleanId = id.section('?', 0, 0);

    QImage image;

    if (cleanId == "phantom") {
        image = controller_->phantomImage();
    } else if (cleanId == "sinogram") {
        image = controller_->sinogramImage();
    } else if (cleanId == "reconstruction") {
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
