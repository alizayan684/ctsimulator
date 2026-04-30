/****************************
 * CT Controller - QML Bridge
 * Connects QML frontend to CT simulation backend
 *****************************/

#pragma once

#include "drawingarea.h"
#include "artthread.h"
#include "sirtthread.h"
#include "matrix.h"

#include <QObject>
#include <QUrl>
#include <QImage>
#include <memory>
#include <vector>

class CTController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool phantomLoaded READ phantomLoaded NOTIFY phantomLoadedChanged)
    Q_PROPERTY(bool reconstructionRunning READ reconstructionRunning NOTIFY reconstructionRunningChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString currentAlgorithm READ currentAlgorithm WRITE setCurrentAlgorithm NOTIFY currentAlgorithmChanged)
    Q_PROPERTY(int currentIteration READ currentIteration NOTIFY currentIterationChanged)

public:
    explicit CTController(QObject* parent = nullptr);
    ~CTController() override;

    // Property getters
    [[nodiscard]] bool phantomLoaded() const { return phantomLoaded_; }
    [[nodiscard]] bool reconstructionRunning() const { return reconstructionRunning_; }
    [[nodiscard]] double progress() const { return progress_; }
    [[nodiscard]] QString currentAlgorithm() const { return currentAlgorithm_; }
    [[nodiscard]] int currentIteration() const { return currentIteration_; }

    // Property setters
    void setCurrentAlgorithm(const QString& algorithm);

    // QML-invokable methods
    Q_INVOKABLE void loadPhantom(const QUrl& fileUrl);
    Q_INVOKABLE void generateSinogram(int projections, double voltage, double current);
    Q_INVOKABLE void startReconstruction();
    Q_INVOKABLE void stopReconstruction();
    Q_INVOKABLE void saveSession(const QUrl& fileUrl);
    Q_INVOKABLE void loadSession(const QUrl& fileUrl);
    Q_INVOKABLE void exportImage(const QUrl& fileUrl);
    Q_INVOKABLE void exportDICOM(const QUrl& fileUrl);

    // Image access for CTImageProvider
    [[nodiscard]] QImage phantomImage() const;
    [[nodiscard]] QImage sinogramImage() const;
    [[nodiscard]] QImage reconstructionImage() const;

signals:
    void phantomLoadedChanged();
    void sinogramReady();
    void reconstructionRunningChanged();
    void progressChanged(double progress);
    void currentIterationChanged(int iteration);
    void currentAlgorithmChanged();
    void reconstructionUpdated(int iteration);
    void reconstructionFinished();
    void error(const QString& message);

private slots:
    // Thread callbacks
    void artUpdate(int iteration, std::vector<double> image);
    void artFinished();
    void artSingleIteration(int iteration, double residual);
    void sirtUpdate(int iteration, std::vector<double> image);
    void sirtFinished();
    void sirtSingleIteration(int iteration, double residual);

private:
    // Internal state
    bool phantomLoaded_ = false;
    bool reconstructionRunning_ = false;
    double progress_ = 0.0;
    QString currentAlgorithm_ = "ART";
    int currentIteration_ = 0;

    // Backend components
    std::unique_ptr<DrawingArea> drawingArea_;
    std::unique_ptr<ArtThread> artThread_;
    std::unique_ptr<SirtThread> sirtThread_;

    // Image data storage
    QImage phantomImage_;
    QImage sinogramImage_;
    QImage reconstructionImage_;
    std::vector<double> reconstructionData_;
    int reconstructionWidth_ = 0;
    int reconstructionHeight_ = 0;

    // Helpers
    void updateReconstructionImage(const std::vector<double>& data, int width, int height);
    [[nodiscard]] QString urlToLocalPath(const QUrl& url) const;
};
