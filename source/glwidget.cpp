/****************************
 * Author: Sanghyeb(Sam) Lee
 * Date: Jan/2013 | Modernized 2024
 * Copyright 2013 Sang hyeb(Sam) Lee MIT License
 *
 * X-ray CT simulation & reconstruction
 *****************************/

#include "glwidget.h"

GlWidget::GlWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    // Qt6: QGLFormat is gone.
    // Double-buffering and depth buffer are on by default via QSurfaceFormat.
}

void GlWidget::initializeGL()
{
    // Must call initializeOpenGLFunctions() before using any GL function
    // through QOpenGLFunctions — it resolves function pointers for the
    // current context.
    initializeOpenGLFunctions();

    // qglClearColor() was a QGLWidget helper; use the raw GL call instead.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glShadeModel(GL_FLAT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

void GlWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const GLfloat aspect = static_cast<GLfloat>(w) / static_cast<GLfloat>(h);
    glFrustum(-aspect, +aspect, -1.0, +1.0, 4.0, 15.0);
    glMatrixMode(GL_MODELVIEW);
}

void GlWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawTetrahedron();
}

void GlWidget::drawTetrahedron()
{
    // Vertices of a regular tetrahedron centred at the origin
    static constexpr std::array<std::array<GLfloat, 3>, 4> V{{
        { 0.0f,          -1.0f, +2.0f },
        { +1.73205081f,  -1.0f, -1.0f },
        { -1.73205081f,  -1.0f, -1.0f },
        {  0.0f,         +2.0f,  0.0f }
    }};

    // Face index triples into V
    static constexpr std::array<std::array<int, 3>, 4> faces{{
        {0, 1, 2}, {0, 2, 3}, {0, 3, 1}, {1, 3, 2}
    }};

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -10.0f);
    glRotatef(rotationX_, 1.0f, 0.0f, 0.0f);
    glRotatef(rotationY_, 0.0f, 1.0f, 0.0f);
    glRotatef(rotationZ_, 0.0f, 0.0f, 1.0f);

    for (int f = 0; f < 4; ++f) {
        // qglColor() was a QGLWidget helper. Use glColor3f with QColor components.
        const QColor& c = faceColors_[static_cast<std::size_t>(f)];
        glColor3f(static_cast<GLfloat>(c.redF()),
                  static_cast<GLfloat>(c.greenF()),
                  static_cast<GLfloat>(c.blueF()));
        glBegin(GL_TRIANGLES);
        for (int v = 0; v < 3; ++v) {
            const auto& vtx = V[static_cast<std::size_t>(faces[static_cast<std::size_t>(f)][v])];
            glVertex3f(vtx[0], vtx[1], vtx[2]);
        }
        glEnd();
    }
}
