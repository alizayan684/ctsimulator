#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickImageProvider>

#include "source/CTController.h"
#include "source/CTImageProvider.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    // Create and register CTController
    CTController controller;
    engine.rootContext()->setContextProperty("ctController", &controller);

    // Register image provider
    engine.addImageProvider("ct", new CTImageProvider(&controller));

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("untitled", "Main");

    return QCoreApplication::exec();
}
