#include "canvasWidget.hpp"

Canvas::Canvas() : QOpenGLWidget{} {
	QSurfaceFormat format;
	format.setProfile(QSurfaceFormat::CoreProfile);
	format.setVersion(4,5);
	setFormat(format);

	resize(300,300);
}

void Canvas::initializeGL() {
	initializeOpenGLFunctions();
	glClearColor(0,0,0,1);
	glEnable(GL_TEXTURE_2D);
}

void Canvas::resizeGL() {
	
}

void Canvas::paintGL() {
	glClear(GL_COLOR_BUFFER_BIT);
}