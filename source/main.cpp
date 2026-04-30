/****************************
 * Author: Sanghyeb(Sam) Lee
 * Date: Jan/2013 | Modernized 2024
 * Copyright 2013 Sang hyeb(Sam) Lee MIT License
 *
 * X-ray CT simulation & reconstruction
 *****************************/

#include <QApplication>
#include "mainwindow.h"

int main(int argc, char* argv[])
{
    // Qt6: High-DPI scaling is enabled by default — no setAttribute needed.
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    // QApplication::exec() replaces the Qt4-era exec_()
    return QApplication::exec();
}
