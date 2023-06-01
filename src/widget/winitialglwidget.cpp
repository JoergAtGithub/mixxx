#include "widget/winitialglwidget.h"

#include <QOpenGLFunctions>

WInitialGLWidget::WInitialGLWidget(QWidget* parent)
        : WGLWidget(parent) {
}

void WInitialGLWidget::paintGL() {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void WInitialGLWidget::initializeGL() {
    emit onInitialized();
}
