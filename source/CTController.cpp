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

CTController::CTController(QObject* parent)
    : QObject(parent)
    , drawingArea_(std::make_unique<DrawingArea>())
{
    drawingArea_->setZoomFactor(1.0);
}

CTController::~CTController() = default;

void CTController::setCurrentAlgorithm(const QString& algorithm)
{
    if (currentAlgorithm_ != algorithm) {
        currentAlgorithm_ = algorithm;
        emit currentAlgorithmChanged();
    }
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
    artThread_.reset();
    emit reconstructionRunningChanged();
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
    sirtThread_.reset();
    emit reconstructionRunningChanged();
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
