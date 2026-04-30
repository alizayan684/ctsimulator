/****************************
 * Author: Sanghyeb(Sam) Lee
 * Date: Jan/2013 | Modernized 2024
 * Copyright 2013 Sang hyeb(Sam) Lee MIT License
 *
 * X-ray CT simulation & reconstruction
 *****************************/

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QMdiSubWindow>

#include <cmath>

// ── Construction / destruction ───────────────────────────────────────────────

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui_(std::make_unique<Ui::MainWindow>())
{
    ui_->setupUi(this);
    ui_->combo_projectionType->addItem("Parallel Projection");
}

MainWindow::~MainWindow()
{
    // If threads are still running when the window closes, the DrawingArea
    // widget (which they hold a raw pointer to) is about to be destroyed.
    // Signal them to stop and wait for them to actually exit before the
    // destructor proceeds.
    if (artThread_ && artThread_->isRunning()) {
        artThread_->running = false;
        artThread_->wait();
    }
    if (sirtThread_ && sirtThread_->isRunning()) {
        sirtThread_->running = false;
        sirtThread_->wait();
    }
    // unique_ptr members (ui_, artThread_, sirtThread_) clean up automatically.
}

// ── MDI helper ───────────────────────────────────────────────────────────────

template<typename W, typename... Args>
W* MainWindow::addMdiWindow(const QString& title, Args&&... args)
{
    auto* widget = new W(std::forward<Args>(args)...);
    ui_->mdiArea->addSubWindow(widget);
    widget->show();
    ui_->mdiArea->currentSubWindow()->adjustSize();
    ui_->mdiArea->currentSubWindow()->setWindowTitle(title);
    return widget;
}

// ── Step 1 — Phantom loading ─────────────────────────────────────────────────

void MainWindow::on_phantomExplore_clicked()
{
    const QString filename = QFileDialog::getOpenFileName();
    if (filename.isNull()) return;
    ui_->phanton_filename->setText(filename);
}

void MainWindow::on_phantomLoad_clicked()
{
    auto phantom = Utility::loadPhantom(ui_->phanton_filename->text());
    if (!phantom) {
        QMessageBox::warning(this,
            tr("Invalid phantom file"),
            tr("File missing or inconsistent column sizes."),
            QMessageBox::Ok);
        return;
    }

    ui_->phantom_size_x->setText(QString::number(phantom->rows()));
    ui_->phantom_size_y->setText(QString::number(phantom->cols()));
    ui_->phantom_location_x->setText("0");
    ui_->phantom_location_y->setText("0");

    // Create the drawing area; MDI takes ownership via addSubWindow
    auto* area = new DrawingArea();
    drawingArea_ = area;

    // Transfer the QImage and pixel data into the drawing area
    area->setPhantom(phantom->qimage(), phantom->toVector());

    ui_->mdiArea->addSubWindow(area);
    area->show();
    area->setMinimumSize(phantom->cols(), phantom->rows());
    ui_->mdiArea->currentSubWindow()->adjustSize();
    ui_->mdiArea->currentSubWindow()->setWindowTitle("mainwindow");

    ui_->btn_step2->setEnabled(true);
}

// ── Noise injection ──────────────────────────────────────────────────────────

void MainWindow::on_pushButton_clicked()
{
    if (!drawingArea_) return;
    const double sd = ui_->misc_sd->text().toDouble();
    drawingArea_->addNoise(sd);
    for (QMdiSubWindow* w : ui_->mdiArea->subWindowList())
        w->update();
}

// ── Step 2 — Source / detector setup ─────────────────────────────────────────

void MainWindow::on_btn_step2_clicked()
{
    if (!drawingArea_) return;
    drawingArea_->loadSourceAndDetector(ui_->txt_numberOfBins->text().toInt());
    ui_->btn_step3->setEnabled(true);
}

// ── Step 3 — Gantry rotation / animation ─────────────────────────────────────

void MainWindow::on_btn_step3_clicked()
{
    if (!drawingArea_) return;

    const double start     = ui_->txt_startAngle->text().toDouble();
    const double finish    = ui_->txt_finishAngle->text().toDouble();
    const double increment = ui_->txt_angleIncrement->text().toDouble();

    drawingArea_->visualizeLine = ui_->cb_visualizeLine->isChecked();

    // New-style connect: compile-time checked, no string macros
    connect(drawingArea_, &DrawingArea::animationLoaded,
            this,         &MainWindow::animationLoaded);

    const int nop = drawingArea_->loadMovement(start, finish, increment);
    ui_->txt_nop->setText(QString::number(nop));
}

void MainWindow::animationLoaded()
{
    ui_->btn_step4->setEnabled(true);
}

void MainWindow::on_cb_visualizeLine_toggled(bool checked)
{
    if (drawingArea_)
        drawingArea_->visualizeLine = checked;
}

// ── Step 4 — System matrix construction ──────────────────────────────────────

void MainWindow::on_btn_step4_clicked()
{
    if (!drawingArea_) return;
    drawingArea_->setupSystemMatrix();

    ui_->step4_matrix_row->setText(
        QString::number(drawingArea_->systemMatrixSize().height()));
    ui_->step4_matrix_column->setText(
        QString::number(drawingArea_->systemMatrixSize().width()));

    ui_->debug_btnpath->setEnabled(true);
    ui_->debug_showsino->setEnabled(true);
    ui_->debug_close_button->setEnabled(true);
    ui_->step5_group->setEnabled(true);
    ui_->misc_group->setEnabled(true);
}

// ── Debug: visualise ray matrix rows ─────────────────────────────────────────

void MainWindow::on_debug_btnpath_clicked()
{
    if (!drawingArea_) return;

    const int start  = ui_->debug_startmatrix->text().toInt();
    const int count  = ui_->debug_numberOfMatrix->text().toInt();
    const int maxRow = drawingArea_->systemMatrixSize().height();

    if (start + count >= maxRow) {
        QMessageBox::warning(this,
            "Matrix outside boundary",
            "Requested matrix row index is out of range.");
        return;
    }

    const int phantomW = drawingArea_->phantomSize().width();
    const int phantomH = drawingArea_->phantomSize().height();
    const int nSrc     = drawingArea_->numSourceElements();
    const auto& srcProj = drawingArea_->sourceProjections();
    const auto& detProj = drawingArea_->detectorProjections();
    const auto& A       = drawingArea_->systemMatrix();

    constexpr int scale = 5;
    const int widgetW = static_cast<int>(phantomW * scale * 1.8);
    const int widgetH = static_cast<int>(phantomH * scale * 1.8);

    for (int i = 0; i < count; ++i) {
        const int rowIdx  = start + i;
        const int proj    = rowIdx / nSrc;
        const int srcIdx  = rowIdx % nSrc;

        const QPoint src(
            static_cast<int>(srcProj[static_cast<std::size_t>(proj)]
                                    [static_cast<std::size_t>(srcIdx)].real()),
            static_cast<int>(srcProj[static_cast<std::size_t>(proj)]
                                    [static_cast<std::size_t>(srcIdx)].imag()));
        const QPoint det(
            static_cast<int>(detProj[static_cast<std::size_t>(proj)]
                                    [static_cast<std::size_t>(srcIdx)].real()),
            static_cast<int>(detProj[static_cast<std::size_t>(proj)]
                                    [static_cast<std::size_t>(srcIdx)].imag()));

        // Copy the row data — MatrixWidget owns its snapshot
        std::vector<double> rowData =
            A[static_cast<std::size_t>(rowIdx)];

        const QString title = QString("Proj(%1), Source(%2)")
            .arg(proj).arg(srcIdx);

        auto* w = addMdiWindow<MatrixWidget>(
            title, src, det,
            QSize(phantomW, phantomH),
            std::move(rowData));
        w->setMinimumSize(widgetW, widgetH);
    }
}

void MainWindow::on_debug_close_button_clicked()
{
    for (QMdiSubWindow* w : ui_->mdiArea->subWindowList())
        if (w->windowTitle() != "mainwindow")
            w->close();
}

void MainWindow::on_debug_showsino_clicked()
{
    if (!drawingArea_) return;

    const QSize sinoSize(drawingArea_->numProjections(),
                         drawingArea_->numDetectorElements());

    auto* w = addMdiWindow<SinogramWidget>(
        "sinogram",
        drawingArea_->sinogram(),
        sinoSize,
        drawingArea_->zoomFactor());

    w->setMinimumSize(
        static_cast<int>(sinoSize.width()  * 2 * drawingArea_->zoomFactor()),
        static_cast<int>(sinoSize.height() * 2 * drawingArea_->zoomFactor()));
}

// ── Step 5 — Reconstruction ───────────────────────────────────────────────────

void MainWindow::on_step5_art_start_2_clicked()
{
    if (!drawingArea_) return;
    // If a previous thread is somehow still alive (e.g. user clicked start
    // twice), stop it and wait for it to exit before creating a new one.
    if (artThread_ && artThread_->isRunning()) {
        artThread_->running = false;
        artThread_->wait();
    }

    const int    sweeps      = ui_->art_sweep_2->text().toInt();
    const double relaxation  = ui_->art_relaxation_2->text().toDouble();
    const int    showEvery   = ui_->art_sw_2->text().toInt();

    // unique_ptr: previous thread is automatically deleted if it exists
    artThread_ = std::make_unique<ArtThread>(drawingArea_, sweeps, relaxation, showEvery);

    // New-style connect: compile-time checked, carries std::vector<double> by value
    connect(artThread_.get(), &ArtThread::updateReady,
            this,             &MainWindow::artUpdate);
    connect(artThread_.get(), &ArtThread::finished,
            this,             &MainWindow::artFinished);
    connect(artThread_.get(), &ArtThread::singleIteration,
            this,             &MainWindow::artSingleIteration);

    artThread_->start();
    ui_->step5_art_stop->setEnabled(true);
    ui_->step5_art_start_2->setEnabled(false);
}

void MainWindow::on_step5_art_stop_clicked()
{
    if (!artThread_) return;
    artThread_->running = false;
    // Do NOT wait() here — artFinished() signal will fire when run() returns,
    // which calls wait() then reset(). Blocking the UI thread here would freeze it.
}

void MainWindow::artUpdate(int iteration, std::vector<double> image)
{
    if (!drawingArea_) return;
    const int rows = drawingArea_->phantomSize().height();
    const int cols = drawingArea_->phantomSize().width();

    auto* w = addMdiWindow<ImageWidget>(
        QString("%1th iteration ART").arg(iteration),
        image, rows, cols, drawingArea_->zoomFactor());

    const int minDim = std::max(128, static_cast<int>(cols * drawingArea_->zoomFactor()));
    w->setMinimumSize(minDim, minDim);
}

void MainWindow::artFinished()
{
    // wait() blocks until the thread's run() has returned.
    // This must happen before reset() calls ~ArtThread(), because destroying
    // a QThread that is still running is undefined behaviour.
    if (artThread_) {
        artThread_->wait();
        artThread_.reset();
    }
    ui_->step5_art_start_2->setEnabled(true);
    ui_->step5_art_stop->setEnabled(false);
}

void MainWindow::artSingleIteration(int iteration, double residual)
{
    ui_->txt_output->setText(
        QString("ART: %1th Iteration complete (residual=%2)\n")
            .arg(iteration).arg(residual)
        + ui_->txt_output->toPlainText());
}

void MainWindow::on_step5_sirt_start_clicked()
{
    if (!drawingArea_) return;
    if (sirtThread_ && sirtThread_->isRunning()) {
        sirtThread_->running = false;
        sirtThread_->wait();
    }

    const int    sweeps     = ui_->sirt_sweep->text().toInt();
    const double relaxation = ui_->sirt_relaxation->text().toDouble();
    const int    showEvery  = ui_->sirt_sw->text().toInt();

    sirtThread_ = std::make_unique<SirtThread>(drawingArea_, sweeps, relaxation, showEvery);

    connect(sirtThread_.get(), &SirtThread::updateReady,
            this,              &MainWindow::sirtUpdate);
    connect(sirtThread_.get(), &SirtThread::finished,
            this,              &MainWindow::sirtFinished);
    connect(sirtThread_.get(), &SirtThread::singleIteration,
            this,              &MainWindow::sirtSingleIteration);

    sirtThread_->start();
    ui_->step5_sirt_stop->setEnabled(true);
    ui_->step5_sirt_start->setEnabled(false);
}

void MainWindow::on_step5_sirt_stop_clicked()
{
    if (!sirtThread_) return;
    sirtThread_->running = false;
}

void MainWindow::sirtUpdate(int iteration, std::vector<double> image)
{
    if (!drawingArea_) return;
    const int rows = drawingArea_->phantomSize().height();
    const int cols = drawingArea_->phantomSize().width();

    auto* w = addMdiWindow<ImageWidget>(
        QString("%1th iteration SIRT").arg(iteration),
        image, rows, cols, drawingArea_->zoomFactor());

    w->setMinimumSize(
        static_cast<int>(cols * drawingArea_->zoomFactor()),
        static_cast<int>(rows * drawingArea_->zoomFactor()));
}

void MainWindow::sirtFinished()
{
    if (sirtThread_) {
        sirtThread_->wait();
        sirtThread_.reset();
    }
    ui_->step5_sirt_start->setEnabled(true);
    ui_->step5_sirt_stop->setEnabled(false);
}

void MainWindow::sirtSingleIteration(int iteration, double residual)
{
    ui_->txt_output->setText(
        QString("SIRT: %1th Iteration complete (residual=%2)\n")
            .arg(iteration).arg(residual)
        + ui_->txt_output->toPlainText());
}

// ── Display settings ─────────────────────────────────────────────────────────

void MainWindow::on_display_apply_clicked()
{
    if (drawingArea_)
        drawingArea_->setZoomFactor(ui_->display_zoomfactor->text().toDouble());

    for (QMdiSubWindow* w : ui_->mdiArea->subWindowList())
        w->update();
}
