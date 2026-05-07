/****************************
 * CT Image Provider
 * Provides phantom, sinogram, and reconstruction images to QML
 *****************************/

#pragma once

#include <QQuickImageProvider>
#include <QImage>

class CTController;

class CTImageProvider : public QQuickImageProvider
{
    Q_OBJECT
public:
    explicit CTImageProvider(CTController* controller, QObject* parent = nullptr);

    [[nodiscard]] QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;

private:
    CTController* controller_;
};
