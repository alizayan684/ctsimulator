#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include "src/visualization/RayPathVisualizer.h"
#include "src/visualization/ColormapManager.h"
#include "src/visualization/ImageViewer.h"
#include "src/visualization/MetricsCalculator.h"
#include "src/visualization/ROITools.h"
#include <QQuickImageProvider>
#include "source/CTController.h"
#include "source/CTImageProvider.h"
void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    QFile outFile("debug_log.txt");
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream ts(&outFile);
    ts << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz ") << msg << "\n";
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(customMessageHandler);
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    QApplication app(argc, argv);

    // Register Visualization Types
    qmlRegisterType<visualization::RayPathVisualizer>("CT.Visualization", 1, 0, "RayPathVisualizer");
    qmlRegisterType<visualization::ImageViewer>("CT.Visualization", 1, 0, "ImageViewer");
    
    // Register Singleton Managers
    qmlRegisterSingletonType<visualization::ColormapManager>("CT.Visualization", 1, 0, "ColormapManager",
        [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject * {
            Q_UNUSED(engine)
            Q_UNUSED(scriptEngine)
            return new visualization::ColormapManager();
        });
        
    qmlRegisterSingletonType<visualization::MetricsCalculator>("CT.Visualization", 1, 0, "MetricsCalculator",
        [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject * {
            Q_UNUSED(engine)
            Q_UNUSED(scriptEngine)
            return new visualization::MetricsCalculator();
        });

    qmlRegisterSingletonType<visualization::ROITools>("CT.Visualization", 1, 0, "ROITools",
        [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject * {
            Q_UNUSED(engine)
            Q_UNUSED(scriptEngine)
            return new visualization::ROITools();
        });

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
        
    // For the tutorial mode, load the TutorialManager
    engine.load(QUrl::fromLocalFile("qml/tutorials/TutorialManager.qml"));

    return QCoreApplication::exec();
}
