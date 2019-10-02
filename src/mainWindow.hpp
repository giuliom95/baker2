#ifndef _MAIN_WINDOW_HPP_
#define _MAIN_WINDOW_HPP_

#include "core.hpp"

#include <QtWidgets>

class MainWindow : public QWidget {
	Q_OBJECT
public:
	explicit MainWindow();

	void setImage(QImage& image);

public slots:
	void loadHighObj();

private:
	QLabel*			texturePreview;
	QPushButton*	lowPolyLoadBtn;
	QPushButton*	highPolyLoadBtn;
	QLineEdit*		lowPolyFileLabel;
	QLineEdit*		highPolyFileLabel;

	Core core;
};

#endif