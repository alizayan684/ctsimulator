/****************************
 * CT Controller Implementation
 *****************************/

#include "CTController.h"
#include "utility.h"
#include "src/reconstruction/ART.h"
#include "src/reconstruction/SIRT.h"
#include "src/reconstruction/FBP.h"
#include "src/reconstruction/MLEM.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtConcurrent>
#include <algorithm>
#include <cmath>
#include <tuple>

CTController::CTController(QObject* parent)
    : QObject(parent)
    , drawingArea_(std::make_unique<DrawingArea>())
{
    updateThrottleTimer_.start();
}

CTController::~CTController()
{
    cleanupAlgorithm();
}

// ── Property setters (with undo capture) ─────────────────────────────────────

void CTController::setCurrentAlgorithm(const QString& algorithm)
{
    if (currentAlgorithm_ != algorithm) {
        pushUndoState();
        currentAlgorithm_ = algorithm;
        emit currentAlgorithmChanged();
    }
}

void CTController::setCurrentFilter(const QString& filter)
{
    if (currentFilter_ != filter) {
        pushUndoState();
        currentFilter_ = filter;
        emit currentFilterChanged();
    }
}

void CTController::setIterations(int iterations)
{
    if (iterations_ != iterations) {
        pushUndoState();
        iterations_ = iterations;
        emit iterationsChanged();
    }
}

void CTController::setRelaxation(double relaxation)
{
    if (relaxation_ != relaxation) {
        pushUndoState();
        relaxation_ = relaxation;
        emit relaxationChanged();
    }
}

void CTController::setShowEvery(int showEvery)
{
    if (showEvery_ != showEvery) {
        pushUndoState();
        showEvery_ = showEvery;
        emit showEveryChanged();
    }
}

// ── Phantom generation / loading ─────────────────────────────────────────────

void CTController::generateSheppLogan()
{
    constexpr int N = 256;
    constexpr int M = 256;
    std::vector<double> data(static_cast<std::size_t>(M * N), 0.0);

    const std::vector<std::tuple<double, double, double, double, double, double>> ellipses = {
        { 1.0,   0.0,     0.0,     0.69,   0.92,    0.0   },
        {-0.8,   0.0,    -0.0184,  0.6624, 0.874,   0.0   },
        {-0.2,   0.22,    0.0,     0.11,   0.31,   -18.0  },
        {-0.2,  -0.22,    0.0,     0.16,   0.41,   18.0   },
        { 0.1,   0.0,     0.35,    0.21,   0.25,    0.0   },
        { 0.1,   0.0,     0.1,     0.046,  0.046,   0.0   },
        { 0.1,   0.0,    -0.1,     0.046,  0.046,   0.0   },
        { 0.1,  -0.08,   -0.605,   0.046,  0.023,   0.0   },
        { 0.1,   0.0,    -0.605,   0.023,  0.023,   0.0   },
        { 0.1,   0.06,   -0.605,   0.023,  0.046,   0.0   }
    };

    constexpr double DEG2RAD = 3.14159265358979323846 / 180.0;
    for (int y = 0; y < M; ++y) {
        for (int x = 0; x < N; ++x) {
            const double nx = (x - N / 2.0) / (N / 2.0);
            const double ny = (y - M / 2.0) / (M / 2.0);
            double val = 0.0;
            for (const auto& [intensity, x0, y0, a, b, theta_deg] : ellipses) {
                const double theta = theta_deg * DEG2RAD;
                const double cosT = std::cos(theta);
                const double sinT = std::sin(theta);
                const double dx = cosT * (nx - x0) + sinT * (ny - y0);
                const double dy = -sinT * (nx - x0) + cosT * (ny - y0);
                if ((dx * dx) / (a * a) + (dy * dy) / (b * b) <= 1.0) {
                    val += intensity;
                }
            }
            data[static_cast<std::size_t>(y * N + x)] = val;
        }
    }

    auto matrix = std::make_unique<Matrix>(std::move(data), M, N);
    phantomImage_ = matrix->qimage();
    std::vector<double> pixelData = matrix->toVector();
    drawingArea_->setPhantom(phantomImage_, pixelData);

    phantomLoaded_ = true;
    emit phantomLoadedChanged();
}

void CTController::loadPhantom(const QUrl& fileUrl)
{
    QString filePath = urlToLocalPath(fileUrl);
    if (filePath.isEmpty()) {
        emit error("Invalid file URL");
        return;
    }

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        emit error("File does not exist: " + filePath);
        return;
    }

    auto matrix = Utility::loadPhantom(filePath);
    if (!matrix) {
        emit error("Failed to load phantom from: " + filePath);
        return;
    }

    phantomImage_ = matrix->qimage();
    std::vector<double> pixelData = matrix->toVector();
    drawingArea_->setPhantom(phantomImage_, pixelData);

    phantomLoaded_ = true;
    emit phantomLoadedChanged();
}

// ── Async sinogram generation ────────────────────────────────────────────────

void CTController::generateSinogram(int projections, double voltage, double current)
{
    if (!phantomLoaded_) {
        emit error("No phantom loaded");
        return;
    }

    if (sinogramGenerating_) {
        emit error("Sinogram generation already in progress");
        return;
    }

    if (reconstructionRunning_) {
        emit error("Cannot generate sinogram while reconstruction is running");
        return;
    }

    // Save parameters for the async work
    lastProjections_ = projections;
    lastVoltage_ = voltage;
    lastCurrent_ = current;

    sinogramGenerating_ = true;
    emit sinogramGeneratingChanged();

    // Run the heavy computation on a background thread
    auto future = QtConcurrent::run([this, projections, voltage, current]() {
        drawingArea_->loadSourceAndDetector(128);
        drawingArea_->loadMovement(0.0, 360.0, 360.0 / projections);
        drawingArea_->setupSystemMatrix();

        if (voltage > 0 && current > 0) {
            constexpr double baseStd = 5.0;
            const double stdDev = baseStd * (80.0 / voltage) * std::sqrt(100.0 / current);
            drawingArea_->addNoise(stdDev);
        }
    });

    // Use a watcher to handle completion back on the main thread
    auto* watcher = new QFutureWatcher<void>(this);
    connect(watcher, &QFutureWatcher<void>::finished, this, [this, watcher]() {
        if (drawingArea_->hasError()) {
            emit error(drawingArea_->errorMessage());
            sinogramGenerating_ = false;
            emit sinogramGeneratingChanged();
            drawingArea_->clearError();
            watcher->deleteLater();
            return;
        }

        // Build the sinogram image on the main thread (it's fast, just pixel copying)
        const auto& sinogramData = drawingArea_->sinogram();
        const int sinoWidth = static_cast<int>(sinogramData.size());
        const int sinoHeight = sinoWidth > 0 ? static_cast<int>(sinogramData[0].size()) : 0;

        if (sinoWidth > 0 && sinoHeight > 0) {
            sinogramImage_ = QImage(sinoWidth, sinoHeight, QImage::Format_ARGB32);
            double minVal = std::numeric_limits<double>::max();
            double maxVal = std::numeric_limits<double>::lowest();
            for (const auto& row : sinogramData) {
                for (double val : row) {
                    minVal = std::min(minVal, val);
                    maxVal = std::max(maxVal, val);
                }
            }
            double range = maxVal - minVal;
            if (range < 1e-10) range = 1.0;

            for (int y = 0; y < sinoHeight; ++y) {
                for (int x = 0; x < sinoWidth; ++x) {
                    int gray = static_cast<int>(255.0 * (sinogramData[x][y] - minVal) / range);
                    gray = std::clamp(gray, 0, 255);
                    sinogramImage_.setPixelColor(x, y, QColor(gray, gray, gray));
                }
            }
        }

        sinogramGenerating_ = false;
        emit sinogramGeneratingChanged();
        emit sinogramReady();

        watcher->deleteLater();
    });

    watcher->setFuture(future);
}

// ── Reconstruction ───────────────────────────────────────────────────────────

void CTController::startReconstruction()
{
    if (!phantomLoaded_) {
        emit error("No phantom loaded");
        return;
    }

    if (reconstructionRunning_) {
        emit error("Reconstruction already running");
        return;
    }

    if (drawingArea_->sinogram().empty()) {
        emit error("Please generate a sinogram first");
        return;
    }

    reconstructionRunning_ = true;
    progress_ = 0.0;
    currentIteration_ = 0;
    expectedIterations_ = (currentAlgorithm_ == "FBP") ? 1 : iterations_;
    emit reconstructionRunningChanged();
    emit progressChanged(progress_);
    emit currentIterationChanged(currentIteration_);

    // Create algorithm with smart pointer
    std::unique_ptr<ReconstructionAlgorithm> algo;

    if (currentAlgorithm_ == "ART") {
        algo = std::make_unique<ART>(drawingArea_.get(), iterations_, relaxation_, showEvery_, this);
    } else if (currentAlgorithm_ == "SIRT") {
        algo = std::make_unique<SIRT>(drawingArea_.get(), iterations_, relaxation_, showEvery_, this);
    } else if (currentAlgorithm_ == "FBP") {
        FBP::FilterType ft = FBP::RamLak;
        if (currentFilter_ == "Shepp-Logan") ft = FBP::SheppLogan;
        else if (currentFilter_ == "Cosine") ft = FBP::Cosine;
        else if (currentFilter_ == "Hamming") ft = FBP::Hamming;
        else if (currentFilter_ == "Hann")   ft = FBP::Hann;
        algo = std::make_unique<FBP>(drawingArea_.get(), iterations_, relaxation_, showEvery_, ft, this);
    } else if (currentAlgorithm_ == "MLEM") {
        algo = std::make_unique<MLEM>(drawingArea_.get(), iterations_, relaxation_, showEvery_, this);
    } else {
        emit error("Unknown algorithm: " + currentAlgorithm_);
        reconstructionRunning_ = false;
        emit reconstructionRunningChanged();
        return;
    }

    // Connect with explicit QueuedConnection for thread safety (prevents UI freeze)
    auto* raw = algo.get();
    connect(raw, &ReconstructionAlgorithm::updateReady,
            this, &CTController::onUpdateReady, Qt::QueuedConnection);
    connect(raw, &ReconstructionAlgorithm::singleIteration,
            this, &CTController::onSingleIteration, Qt::QueuedConnection);
    connect(raw, &ReconstructionAlgorithm::finished,
            this, &CTController::onAlgorithmFinished, Qt::QueuedConnection);

    activeAlgorithm_ = std::move(algo);
    raw->start();
}

void CTController::stopReconstruction()
{
    if (activeAlgorithm_) {
        activeAlgorithm_->running = false;
        activeAlgorithm_->wait();
        onAlgorithmFinished();
    }
}

void CTController::cleanupAlgorithm()
{
    if (activeAlgorithm_) {
        activeAlgorithm_->running = false;
        activeAlgorithm_->wait();
        activeAlgorithm_.reset();
    }
}

// ── Algorithm callbacks ──────────────────────────────────────────────────────

void CTController::onUpdateReady(int iteration, std::vector<double> image)
{
    currentIteration_ = iteration;
    reconstructionData_ = std::move(image);

    QSize phantomSize = drawingArea_->phantomSize();
    reconstructionWidth_ = phantomSize.width();
    reconstructionHeight_ = phantomSize.height();

    // Throttle image refresh to avoid flooding QML when showEvery is small
    if (updateThrottleTimer_.elapsed() >= kMinUpdateIntervalMs || iteration == expectedIterations_) {
        updateThrottleTimer_.restart();
        updateReconstructionImage(reconstructionData_, reconstructionWidth_, reconstructionHeight_);
        emit reconstructionUpdated(iteration);
    }
    emit currentIterationChanged(iteration);
}

void CTController::onSingleIteration(int iteration, double residual)
{
    progress_ = static_cast<double>(iteration) / static_cast<double>(expectedIterations_);
    if (progress_ > 1.0) progress_ = 1.0;
    emit progressChanged(progress_);

    currentResidual_ = residual;
    emit currentResidualChanged();
}

void CTController::onAlgorithmFinished()
{
    if (!reconstructionRunning_)
        return;

    reconstructionRunning_ = false;
    progress_ = 1.0;
    emit reconstructionRunningChanged();
    emit progressChanged(progress_);
    emit reconstructionFinished();

    if (activeAlgorithm_) {
        disconnect(activeAlgorithm_.get(), nullptr, this, nullptr);
        activeAlgorithm_.reset();
    }
}

// ── Image helpers ────────────────────────────────────────────────────────────

void CTController::updateReconstructionImage(const std::vector<double>& data, int width, int height)
{
    if (data.empty() || width <= 0 || height <= 0) {
        return;
    }

    reconstructionImage_ = QImage(width, height, QImage::Format_ARGB32);

    double minVal = *std::min_element(data.begin(), data.end());
    double maxVal = *std::max_element(data.begin(), data.end());
    double range = maxVal - minVal;
    if (range < 1e-10) range = 1.0;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const int idx = y * width + x;
            int gray = static_cast<int>(255.0 * (data[idx] - minVal) / range);
            gray = std::clamp(gray, 0, 255);
            reconstructionImage_.setPixelColor(x, y, QColor(gray, gray, gray));
        }
    }
}

QString CTController::urlToLocalPath(const QUrl& url) const
{
    if (url.isLocalFile()) {
        return url.toLocalFile();
    }
    return url.toString();
}

QImage CTController::phantomImage() const
{
    return phantomImage_;
}

QImage CTController::sinogramImage() const
{
    return sinogramImage_;
}

QImage CTController::reconstructionImage() const
{
    return reconstructionImage_;
}

// ── Session save/load (JSON) ─────────────────────────────────────────────────

void CTController::saveSession(const QUrl& fileUrl)
{
    QString filePath = urlToLocalPath(fileUrl);
    if (filePath.isEmpty()) {
        emit error("Invalid file URL for session save");
        return;
    }

    QJsonObject session;
    session["algorithm"] = currentAlgorithm_;
    session["filter"] = currentFilter_;
    session["iterations"] = iterations_;
    session["relaxation"] = relaxation_;
    session["showEvery"] = showEvery_;
    session["projections"] = lastProjections_;
    session["voltage"] = lastVoltage_;
    session["current"] = lastCurrent_;

    QJsonDocument doc(session);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit error("Failed to open file for writing: " + filePath);
        return;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    emit sessionSaved();
}

void CTController::loadSession(const QUrl& fileUrl)
{
    QString filePath = urlToLocalPath(fileUrl);
    if (filePath.isEmpty()) {
        emit error("Invalid file URL for session load");
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit error("Failed to open session file: " + filePath);
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull() || !doc.isObject()) {
        emit error("Invalid session file format");
        return;
    }

    const QJsonObject session = doc.object();

    // Apply all settings (without pushing undo for each)
    if (session.contains("algorithm")) {
        currentAlgorithm_ = session["algorithm"].toString();
        emit currentAlgorithmChanged();
    }
    if (session.contains("filter")) {
        currentFilter_ = session["filter"].toString();
        emit currentFilterChanged();
    }
    if (session.contains("iterations")) {
        iterations_ = session["iterations"].toInt();
        emit iterationsChanged();
    }
    if (session.contains("relaxation")) {
        relaxation_ = session["relaxation"].toDouble();
        emit relaxationChanged();
    }
    if (session.contains("showEvery")) {
        showEvery_ = session["showEvery"].toInt();
        emit showEveryChanged();
    }
    if (session.contains("projections"))
        lastProjections_ = session["projections"].toInt();
    if (session.contains("voltage"))
        lastVoltage_ = session["voltage"].toDouble();
    if (session.contains("current"))
        lastCurrent_ = session["current"].toDouble();

    emit sessionLoaded();
}

// ── Export ────────────────────────────────────────────────────────────────────

void CTController::exportImage(const QUrl& fileUrl)
{
    if (reconstructionImage_.isNull()) {
        emit error("No reconstruction to export");
        return;
    }

    QString filePath = urlToLocalPath(fileUrl);

    // Auto-append .png if no recognised image extension is present
    const QString lower = filePath.toLower();
    if (!lower.endsWith(".png") && !lower.endsWith(".tiff") && !lower.endsWith(".tif")
        && !lower.endsWith(".jpg") && !lower.endsWith(".jpeg") && !lower.endsWith(".bmp")) {
        filePath += ".png";
    }

    if (!reconstructionImage_.save(filePath)) {
        emit error("Failed to save image to: " + filePath);
    }
}

void CTController::exportSinogram(const QUrl& fileUrl)
{
    if (sinogramImage_.isNull()) {
        emit error("No sinogram to export");
        return;
    }

    QString filePath = urlToLocalPath(fileUrl);
    if (!sinogramImage_.save(filePath)) {
        emit error("Failed to save sinogram to: " + filePath);
    }
}

// ── Parameter presets ────────────────────────────────────────────────────────

void CTController::applyPreset(const QString& presetName)
{
    if (presetName == "Quick Preview") {
        setIterations(10);
        setRelaxation(0.25);
        setShowEvery(5);
    } else if (presetName == "Standard") {
        setIterations(50);
        setRelaxation(0.1);
        setShowEvery(5);
    } else if (presetName == "High Quality") {
        setIterations(200);
        setRelaxation(0.05);
        setShowEvery(10);
    } else {
        emit error("Unknown preset: " + presetName);
    }
}

// ── Undo / Redo ──────────────────────────────────────────────────────────────

QJsonObject CTController::captureParameterState() const
{
    QJsonObject state;
    state["algorithm"] = currentAlgorithm_;
    state["filter"] = currentFilter_;
    state["iterations"] = iterations_;
    state["relaxation"] = relaxation_;
    state["showEvery"] = showEvery_;
    return state;
}

void CTController::applyParameterState(const QJsonObject& state)
{
    currentAlgorithm_ = state["algorithm"].toString();
    currentFilter_ = state["filter"].toString();
    iterations_ = state["iterations"].toInt();
    relaxation_ = state["relaxation"].toDouble();
    showEvery_ = state["showEvery"].toInt();

    emit currentAlgorithmChanged();
    emit currentFilterChanged();
    emit iterationsChanged();
    emit relaxationChanged();
    emit showEveryChanged();
}

void CTController::pushUndoState()
{
    undoStack_.push_back(captureParameterState());
    if (static_cast<int>(undoStack_.size()) > kMaxUndoDepth) {
        undoStack_.erase(undoStack_.begin());
    }
    redoStack_.clear();
    emit undoStateChanged();
}

void CTController::undo()
{
    if (undoStack_.empty()) return;

    redoStack_.push_back(captureParameterState());
    QJsonObject previous = undoStack_.back();
    undoStack_.pop_back();
    applyParameterState(previous);
    emit undoStateChanged();
}

void CTController::redo()
{
    if (redoStack_.empty()) return;

    undoStack_.push_back(captureParameterState());
    QJsonObject next = redoStack_.back();
    redoStack_.pop_back();
    applyParameterState(next);
    emit undoStateChanged();
}
