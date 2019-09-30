
#include "mainWindow.hpp"
#include "canvasWidget.hpp"

MainWindow::MainWindow() : QWidget() {
	resize(800,800);
	setWindowTitle("TEST");

	auto* layout = new QHBoxLayout();
	layout->setContentsMargins(0,0,0,0);

	QImage buffer{300, 300, QImage::Format_RGB888};
	auto* bufferData = buffer.bits();
	for(int i = 0; i < 300; ++i)
		for(int j = 0; j < 300; ++j) {
			bufferData[3*(j*300 + i) + 0] = (uchar)(256*(i / 300.f));
			bufferData[3*(j*300 + i) + 1] = (uchar)(256*(j / 300.f));
			bufferData[3*(j*300 + i) + 2] = 0;
		}

	QLabel* imageContainer = new QLabel();
	imageContainer->setPixmap(QPixmap::fromImage(buffer));

	QScrollArea* scrollArea = new QScrollArea();
	scrollArea->setBackgroundRole(QPalette::Dark);
	scrollArea->setWidget(imageContainer);
	
	layout->addWidget(scrollArea);
	setLayout(layout);
}