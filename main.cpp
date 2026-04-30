#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickImageProvider>

#include "source/CTController.h"
#include "source/CTImageProvider.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QQmlApplicationEngine engine;

    // Create and register CTController
    CTController controller;
    engine.rootContext()->setContextProperty("ctController", &controller);

    // Register image provider (parented to engine so it is cleaned up)
    auto* provider = new CTImageProvider(&controller, &engine);
    engine.addImageProvider("ct", provider);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("untitled", "Main");

    return QCoreApplication::exec();
}
