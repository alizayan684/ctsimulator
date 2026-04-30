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

// Runs the SIRT (Simultaneous Iterative Reconstruction Technique) algorithm.
// Unlike ART which updates x one row at a time, SIRT accumulates the full
// gradient (Aᵀ(b - Ax)) before updating x — slower convergence but smoother
// results. All buffers are std::vector — no manual new/delete.

class SirtThread : public QThread
{
    Q_OBJECT
    Q_DISABLE_COPY(SirtThread)
public:
    SirtThread(DrawingArea* area, int sweeps, double relaxation, int showEvery,
               QObject* parent = nullptr);

    std::atomic<bool> running{true};

signals:
    void updateReady(int iteration, std::vector<double> image);
    void singleIteration(int iteration, double residual);

protected:
    void run() override;

private:
    DrawingArea* area_;
    int          sweeps_;
    double       relaxation_;
    int          showEvery_;
};
