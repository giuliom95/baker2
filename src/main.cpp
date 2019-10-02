#define TINYOBJLOADER_IMPLEMENTATION

#define VERBOSE 1
#include <iostream>

#include "mainWindow.hpp"
#include <QtWidgets>


int main(int argc, char** argv) {
	QApplication app(argc, argv);

	// load gui
	MainWindow win;
	win.show();

	return app.exec();
}

