#ifndef _CANVAS_WIDGET_HPP_
#define _CANVAS_WIDGET_HPP_

#include <QtWidgets>

class Canvas : public QOpenGLWidget, protected QOpenGLFunctions {
	Q_OBJECT
public:
	explicit Canvas();

protected:
	void initializeGL();
	void paintGL();
	void resizeGL();

};

#endif