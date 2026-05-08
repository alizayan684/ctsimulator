/****************************
 * CT Controller - QML Bridge
 * Connects QML frontend to CT simulation backend
 *****************************/

#pragma once

#include "drawingarea.h"
#include "matrix.h"
#include "src/reconstruction/ReconstructionAlgorithm.h"

#include <QObject>
#include <QUrl>
#include <QImage>
#include <QJsonObject>
#include <QElapsedTimer>
#include <memory>
#include <vector>

class CTController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool phantomLoaded READ phantomLoaded NOTIFY phantomLoadedChanged)
    Q_PROPERTY(bool reconstructionRunning READ reconstructionRunning NOTIFY reconstructionRunningChanged)
    Q_PROPERTY(bool sinogramGenerating READ sinogramGenerating NOTIFY sinogramGeneratingChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString currentAlgorithm READ currentAlgorithm WRITE setCurrentAlgorithm NOTIFY currentAlgorithmChanged)
    Q_PROPERTY(int currentIteration READ currentIteration NOTIFY currentIterationChanged)
    Q_PROPERTY(QString currentFilter READ currentFilter WRITE setCurrentFilter NOTIFY currentFilterChanged)
    Q_PROPERTY(int iterations READ iterations WRITE setIterations NOTIFY iterationsChanged)
    Q_PROPERTY(double relaxation READ relaxation WRITE setRelaxation NOTIFY relaxationChanged)
    Q_PROPERTY(int showEvery READ showEvery WRITE setShowEvery NOTIFY showEveryChanged)
    Q_PROPERTY(double currentResidual READ currentResidual NOTIFY currentResidualChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY undoStateChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY undoStateChanged)

public:
    explicit CTController(QObject* parent = nullptr);
    ~CTController() override;

    // Property getters
    [[nodiscard]] bool phantomLoaded() const { return phantomLoaded_; }
    [[nodiscard]] bool reconstructionRunning() const { return reconstructionRunning_; }
    [[nodiscard]] bool sinogramGenerating() const { return sinogramGenerating_; }
    [[nodiscard]] double progress() const { return progress_; }
    [[nodiscard]] QString currentAlgorithm() const { return currentAlgorithm_; }
    [[nodiscard]] int currentIteration() const { return currentIteration_; }
    [[nodiscard]] QString currentFilter() const { return currentFilter_; }
    [[nodiscard]] int iterations() const { return iterations_; }
    [[nodiscard]] double relaxation() const { return relaxation_; }
    [[nodiscard]] int showEvery() const { return showEvery_; }
    [[nodiscard]] double currentResidual() const { return currentResidual_; }
    [[nodiscard]] bool canUndo() const { return !undoStack_.empty(); }
    [[nodiscard]] bool canRedo() const { return !redoStack_.empty(); }

    // Property setters
    void setCurrentAlgorithm(const QString& algorithm);
    void setCurrentFilter(const QString& filter);
    void setIterations(int iterations);
    void setRelaxation(double relaxation);
    void setShowEvery(int showEvery);

    // QML-invokable methods
    Q_INVOKABLE void loadPhantom(const QUrl& fileUrl);
    Q_INVOKABLE void generateSheppLogan();
    Q_INVOKABLE void generateSinogram(int projections, double voltage, double current);
    Q_INVOKABLE void startReconstruction();
    Q_INVOKABLE void stopReconstruction();
    Q_INVOKABLE void saveSession(const QUrl& fileUrl);
    Q_INVOKABLE void loadSession(const QUrl& fileUrl);
    Q_INVOKABLE void exportImage(const QUrl& fileUrl);
    Q_INVOKABLE void exportSinogram(const QUrl& fileUrl);
    Q_INVOKABLE void applyPreset(const QString& presetName);
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();

    // Image access for CTImageProvider
    [[nodiscard]] QImage phantomImage() const;
    [[nodiscard]] QImage sinogramImage() const;
    [[nodiscard]] QImage reconstructionImage() const;

signals:
    void phantomLoadedChanged();
    void sinogramReady();
    void sinogramGeneratingChanged();
    void reconstructionRunningChanged();
    void progressChanged(double progress);
    void currentIterationChanged(int iteration);
    void currentAlgorithmChanged();
    void currentFilterChanged();
    void iterationsChanged();
    void relaxationChanged();
    void showEveryChanged();
    void reconstructionUpdated(int iteration);
    void reconstructionFinished();
    void error(const QString& message);
    void currentResidualChanged();
    void undoStateChanged();
    void sessionSaved();
    void sessionLoaded();

private slots:
    // Unified algorithm callbacks
    void onUpdateReady(int iteration, std::vector<double> image);
    void onSingleIteration(int iteration, double residual);
    void onAlgorithmFinished();

private:
    // Internal state
    bool phantomLoaded_ = false;
    bool reconstructionRunning_ = false;
    bool sinogramGenerating_ = false;
    double progress_ = 0.0;
    QString currentAlgorithm_ = "ART";
    QString currentFilter_ = "Ram-Lak";
    int currentIteration_ = 0;
    int expectedIterations_ = 50;
    int iterations_ = 50;
    double relaxation_ = 0.1;
    int showEvery_ = 1;
    double currentResidual_ = 0.0;

    // Sinogram generation parameters (saved for async)
    int lastProjections_ = 180;
    double lastVoltage_ = 120.0;
    double lastCurrent_ = 100.0;

    // Backend components
    std::unique_ptr<DrawingArea> drawingArea_;
    std::unique_ptr<ReconstructionAlgorithm> activeAlgorithm_;

    // Image data storage
    QImage phantomImage_;
    QImage sinogramImage_;
    QImage reconstructionImage_;
    std::vector<double> reconstructionData_;
    int reconstructionWidth_ = 0;
    int reconstructionHeight_ = 0;

    // Undo/redo stacks for parameter changes
    std::vector<QJsonObject> undoStack_;
    std::vector<QJsonObject> redoStack_;
    static constexpr int kMaxUndoDepth = 50;

    // Helpers
    void updateReconstructionImage(const std::vector<double>& data, int width, int height);
    [[nodiscard]] QString urlToLocalPath(const QUrl& url) const;
    void cleanupAlgorithm();
    [[nodiscard]] QJsonObject captureParameterState() const;
    void applyParameterState(const QJsonObject& state);
    void pushUndoState();

    // Throttle rapid reconstruction updates (min 50 ms between image refreshes)
    QElapsedTimer updateThrottleTimer_;
    static constexpr int kMinUpdateIntervalMs = 50;
};
