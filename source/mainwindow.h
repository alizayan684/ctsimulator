/****************************
 * Author: Sanghyeb(Sam) Lee
 * Date: Jan/2013 | Modernized 2024
 * Copyright 2013 Sang hyeb(Sam) Lee MIT License
 *
 * X-ray CT simulation & reconstruction
 *****************************/

#pragma once

#include "drawingarea.h"
#include "matrixwidget.h"
#include "sinogramwidget.h"
#include "artthread.h"
#include "sirtthread.h"
#include "imagewidget.h"

#include <QMainWindow>
#include <QFileDialog>
#include <QMdiSubWindow>
#include <QMessageBox>

#include <memory>
#include <vector>

namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void on_phantomExplore_clicked();
    void on_phantomLoad_clicked();
    void on_pushButton_clicked();
    void on_btn_step2_clicked();
    void on_btn_step3_clicked();
    void on_btn_step4_clicked();
    void on_cb_visualizeLine_toggled(bool checked);
    void on_debug_btnpath_clicked();
    void on_debug_close_button_clicked();
    void on_debug_showsino_clicked();
    void on_step5_art_start_2_clicked();
    void on_step5_art_stop_clicked();
    void on_step5_sirt_start_clicked();
    void on_step5_sirt_stop_clicked();
    void on_display_apply_clicked();
    void animationLoaded();

    // ART thread callbacks — image is passed by value (thread-safe copy)
    void artUpdate(int iteration, std::vector<double> image);
    void artFinished();
    void artSingleIteration(int iteration, double residual);

    // SIRT thread callbacks
    void sirtUpdate(int iteration, std::vector<double> image);
    void sirtFinished();
    void sirtSingleIteration(int iteration, double residual);

private:
    std::unique_ptr<Ui::MainWindow> ui_;

    // Non-owning observer: the MDI area owns the widget
    DrawingArea* drawingArea_ = nullptr;

    // Thread objects are owned by unique_ptr; reset() replaces delete
    std::unique_ptr<ArtThread>  artThread_;
    std::unique_ptr<SirtThread> sirtThread_;

    // Helper: add a sub-window to the MDI area and return the new widget ptr
    template<typename W, typename... Args>
    W* addMdiWindow(const QString& title, Args&&... args);
};
