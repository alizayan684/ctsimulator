/****************************
 * Author: Sanghyeb(Sam) Lee
 * Date: Jan/2013 | Modernized 2024
 * Copyright 2013 Sang hyeb(Sam) Lee MIT License
 *
 * X-ray CT simulation & reconstruction
 *****************************/

#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QColor>
#include <QPoint>

#include <array>

// Qt6: QGLWidget is gone. QOpenGLWidget renders into an FBO that composites
// cleanly with the rest of the widget hierarchy.
// QOpenGLFunctions provides portable access to OpenGL function pointers.

class GlWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit GlWidget(QWidget* parent = nullptr);

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;

private:
    void drawTetrahedron();

    GLfloat rotationX_ = -21.0f;
    GLfloat rotationY_ = -57.0f;
    GLfloat rotationZ_ =   0.0f;

    std::array<QColor, 4> faceColors_{
        Qt::red, Qt::green, Qt::blue, Qt::yellow
    };
};
