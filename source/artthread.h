/****************************
 * Author: Sanghyeb(Sam) Lee
 * Date: Jan/2013 | Modernized 2024
 * Copyright 2013 Sang hyeb(Sam) Lee MIT License
 *
 * X-ray CT simulation & reconstruction
 *****************************/

#pragma once

#include "drawingarea.h"

#include <QThread>

#include <atomic>
#include <vector>

// Runs the ART (Algebraic Reconstruction Technique) algorithm on a
// background thread. All internal buffers are owned via std::vector — no
// manual new/delete. The result snapshot is returned through a
// std::vector<double> that is emitted alongside the iteration number.

class ArtThread : public QThread
{
    Q_OBJECT
    Q_DISABLE_COPY(ArtThread)
public:
    // DrawingArea must outlive this thread. Pass by raw observer pointer
    // (non-owning) because the MDI window owns the widget.
    ArtThread(DrawingArea* area, int sweeps, double relaxation, int showEvery,
              QObject* parent = nullptr);

    // Thread-safe stop flag: main thread sets this to false to request stop.
    std::atomic<bool> running{true};

signals:
    // Emitted every `showEvery` sweeps with the current reconstructed image.
    void updateReady(int iteration, std::vector<double> image);
    // Emitted once per sweep with the L1 residual vs the ground-truth phantom.
    void singleIteration(int iteration, double residual);

protected:
    void run() override;

private:
    DrawingArea* area_;    // non-owning observer
    int          sweeps_;
    double       relaxation_;
    int          showEvery_;
};
