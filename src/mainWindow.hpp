#ifndef _MAIN_WINDOW_HPP_
#define _MAIN_WINDOW_HPP_

#include <QtWidgets>

class MainWindow : public QWidget {
	Q_OBJECT
public:
	explicit MainWindow(QImage& image);

	void setImage(QImage& image);

private:
	QLabel* imageContainer;
};

#endif