
#include "mainWindow.hpp"

MainWindow::MainWindow(QImage& image) : 
	QWidget() {
	resize(800,800);
	setWindowTitle("TEST");

	auto* layout = new QHBoxLayout();
	layout->setContentsMargins(0,0,0,0);

	imageContainer = new QLabel();
	setImage(image);

	QScrollArea* scrollArea = new QScrollArea();
	scrollArea->setBackgroundRole(QPalette::Dark);
	scrollArea->setWidget(imageContainer);
	
	layout->addWidget(scrollArea);
	setLayout(layout);
}

void MainWindow::setImage(QImage& image) {
	imageContainer->setPixmap(QPixmap::fromImage(image));
}