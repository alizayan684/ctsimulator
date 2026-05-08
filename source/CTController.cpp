/****************************
 * CT Controller Implementation
 *****************************/

#include "CTController.h"
#include "utility.h"

#include <QFile>
#include <QFileInfo>
#include <QtConcurrent/QtConcurrent>
#include <algorithm>
#include <cmath>
#include <tuple>

CTController::CTController(QObject* parent)
    : QObject(parent)
    , drawingArea_(std::make_unique<DrawingArea>())
{

}

CTController::~CTController() = default;

void CTController::setCurrentAlgorithm(const QString& algorithm)
{
    if (currentAlgorithm_ != algorithm) {
        currentAlgorithm_ = algorithm;
        emit currentAlgorithmChanged();
    }
}

void CTController::generateSheppLogan()
{
    constexpr int N = 256;
    constexpr int M = 256;
    std::vector<double> data(static_cast<std::size_t>(M * N), 0.0);

    // Standard Shepp-Logan phantom ellipses:
    // {intensity, x0, y0, a, b, rotation_degrees}
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
            // Map pixel to normalized coordinates [-1, 1]
            const double nx = (x - N / 2.0) / (N / 2.0);
            const double ny = (y - M / 2.0) / (M / 2.0);
            double val = 0.0;
            for (const auto& e : ellipses) {
                const double intensity = std::get<0>(e);
                const double x0        = std::get<1>(e);
                const double y0        = std::get<2>(e);
                const double a         = std::get<3>(e);
                const double b         = std::get<4>(e);
                const double theta     = std::get<5>(e) * DEG2RAD;
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

    // Load phantom using existing Utility class
    auto matrix = Utility::loadPhantom(filePath);
    if (!matrix) {
        emit error("Failed to load phantom from: " + filePath);
        return;
    }

    // Convert to QImage for display
    phantomImage_ = matrix->qimage();
    
    // Update drawing area with phantom data
    std::vector<double> pixelData = matrix->toVector();
    drawingArea_->setPhantom(phantomImage_, pixelData);

    phantomLoaded_ = true;
    emit phantomLoadedChanged();
}

void CTController::generateSinogram(int projections, double voltage, double current)
{
    if (!phantomLoaded_) {
        emit error("No phantom loaded");
        return;
    }

    // Configure source and detector
    drawingArea_->loadSourceAndDetector(128);  // Default detector elements

    // Load movement with specified projections
    int numProjections = drawingArea_->loadMovement(0.0, 360.0, 360.0 / projections);
    
    // Setup system matrix and generate sinogram
    drawingArea_->setupSystemMatrix();

    // Apply noise model based on voltage / current (lower mA → more noise)
    if (voltage > 0 && current > 0) {
        const double baseStd = 5.0;
        const double stdDev = baseStd * (80.0 / voltage) * std::sqrt(100.0 / current);
        drawingArea_->addNoise(stdDev);
    }

    // Create sinogram image from the data
    auto& sinogramData = drawingArea_->sinogram();
    int sinoWidth = static_cast<int>(sinogramData.size());
    int sinoHeight = sinoWidth > 0 ? static_cast<int>(sinogramData[0].size()) : 0;

    if (sinoWidth > 0 && sinoHeight > 0) {
        // Create grayscale image from sinogram data
        sinogramImage_ = QImage(sinoWidth, sinoHeight, QImage::Format_ARGB32);
        
        // Find min/max for normalization
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

        // Fill image
        for (int y = 0; y < sinoHeight; ++y) {
            for (int x = 0; x < sinoWidth; ++x) {
                int gray = static_cast<int>(255.0 * (sinogramData[x][y] - minVal) / range);
                gray = std::clamp(gray, 0, 255);
                sinogramImage_.setPixelColor(x, y, QColor(gray, gray, gray));
            }
        }
    }

    emit sinogramReady();
}

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

    // Setup if not already done
    if (drawingArea_->sinogram().empty()) {
        generateSinogram(180, 120.0, 100.0);  // Default values
    }

    reconstructionRunning_ = true;
    progress_ = 0.0;
    currentIteration_ = 0;
    emit reconstructionRunningChanged();
    emit progressChanged(progress_);
    emit currentIterationChanged(currentIteration_);

    if (currentAlgorithm_ == "ART") {
        artThread_ = std::make_unique<ArtThread>(drawingArea_.get(), 50, 0.1, 1, this);
        connect(artThread_.get(), &ArtThread::updateReady, this, &CTController::artUpdate);
        connect(artThread_.get(), &ArtThread::singleIteration, this, &CTController::artSingleIteration);
        artThread_->start();
    } else if (currentAlgorithm_ == "SIRT") {
        sirtThread_ = std::make_unique<SirtThread>(drawingArea_.get(), 50, 0.1, 1, this);
        connect(sirtThread_.get(), &SirtThread::updateReady, this, &CTController::sirtUpdate);
        connect(sirtThread_.get(), &SirtThread::singleIteration, this, &CTController::sirtSingleIteration);
        sirtThread_->start();
    }
}

void CTController::stopReconstruction()
{
    if (artThread_) {
        artThread_->running = false;
        artThread_->wait();
        artFinished();
    }
    if (sirtThread_) {
        sirtThread_->running = false;
        sirtThread_->wait();
        sirtFinished();
    }
}

void CTController::artUpdate(int iteration, std::vector<double> image)
{
    currentIteration_ = iteration;
    reconstructionData_ = std::move(image);
    
    // Determine dimensions from phantom
    QSize phantomSize = drawingArea_->phantomSize();
    reconstructionWidth_ = phantomSize.width();
    reconstructionHeight_ = phantomSize.height();
    
    updateReconstructionImage(reconstructionData_, reconstructionWidth_, reconstructionHeight_);
    emit reconstructionUpdated(iteration);
    emit currentIterationChanged(iteration);
}

void CTController::artFinished()
{
    reconstructionRunning_ = false;
    progress_ = 1.0;
    artThread_.reset();
    emit reconstructionRunningChanged();
    emit progressChanged(progress_);
    emit reconstructionFinished();
}

void CTController::artSingleIteration(int iteration, double residual)
{
    progress_ = static_cast<double>(iteration) / 50.0;  // Assuming 50 sweeps
    emit progressChanged(progress_);
}

void CTController::sirtUpdate(int iteration, std::vector<double> image)
{
    currentIteration_ = iteration;
    reconstructionData_ = std::move(image);
    
    QSize phantomSize = drawingArea_->phantomSize();
    reconstructionWidth_ = phantomSize.width();
    reconstructionHeight_ = phantomSize.height();
    
    updateReconstructionImage(reconstructionData_, reconstructionWidth_, reconstructionHeight_);
    emit reconstructionUpdated(iteration);
    emit currentIterationChanged(iteration);
}

void CTController::sirtFinished()
{
    reconstructionRunning_ = false;
    progress_ = 1.0;
    sirtThread_.reset();
    emit reconstructionRunningChanged();
    emit progressChanged(progress_);
    emit reconstructionFinished();
}

void CTController::sirtSingleIteration(int iteration, double residual)
{
    progress_ = static_cast<double>(iteration) / 50.0;
    emit progressChanged(progress_);
}

void CTController::updateReconstructionImage(const std::vector<double>& data, int width, int height)
{
    if (data.empty() || width <= 0 || height <= 0) {
        return;
    }

    reconstructionImage_ = QImage(width, height, QImage::Format_ARGB32);

    // Normalize
    double minVal = *std::min_element(data.begin(), data.end());
    double maxVal = *std::max_element(data.begin(), data.end());
    double range = maxVal - minVal;
    if (range < 1e-10) range = 1.0;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = y * width + x;
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

// Placeholder implementations for session management and export
void CTController::saveSession(const QUrl& fileUrl)
{
    emit error("Session save not yet implemented");
}

void CTController::loadSession(const QUrl& fileUrl)
{
    emit error("Session load not yet implemented");
}

void CTController::exportImage(const QUrl& fileUrl)
{
    if (reconstructionImage_.isNull()) {
        emit error("No reconstruction to export");
        return;
    }

    QString filePath = urlToLocalPath(fileUrl);
    if (!reconstructionImage_.save(filePath)) {
        emit error("Failed to save image to: " + filePath);
    }
}

void CTController::exportDICOM(const QUrl& fileUrl)
{
    emit error("DICOM export not yet implemented");
}
