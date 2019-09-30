
#include "mainWindow.hpp"
#include "canvasWidget.hpp"

MainWindow::MainWindow() : QWidget() {
	resize(800,800);
	setWindowTitle("TEST");

	auto* layout = new QHBoxLayout();
	layout->setContentsMargins(0,0,0,0);

	auto* canvas = new Canvas();

	layout->addWidget(canvas);
	
	setLayout(layout);
}