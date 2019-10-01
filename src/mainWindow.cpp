
#include "mainWindow.hpp"

MainWindow::MainWindow(QImage& image) : 
	QWidget(), buffer{image} {
	resize(800,800);
	setWindowTitle("TEST");

	auto* layout = new QHBoxLayout();
	layout->setContentsMargins(0,0,0,0);

	QLabel* imageContainer = new QLabel();
	imageContainer->setPixmap(QPixmap::fromImage(buffer));

	QScrollArea* scrollArea = new QScrollArea();
	scrollArea->setBackgroundRole(QPalette::Dark);
	scrollArea->setWidget(imageContainer);
	
	layout->addWidget(scrollArea);
	setLayout(layout);
}