#include <tiny_obj_loader.h>

#include <QtWidgets>
#include "mainWindow.hpp"

int main(int argc, char** argv) {
	QApplication app(argc, argv);

	MainWindow win;
	win.show();
	// load gui
	// load obj
	// upload data to embree
	// cast
	return app.exec();
}