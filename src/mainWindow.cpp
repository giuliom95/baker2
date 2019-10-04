
#include "mainWindow.hpp"

MainWindow::MainWindow() :	QWidget(),
							core() {
	resize(800,800);
	setWindowTitle("TEST");

	QVBoxLayout* mainLayout = new QVBoxLayout();
	
	QLabel* lowPolyLabel	= new QLabel("Low poly model");
	QLabel* highPolyLabel	= new QLabel("High poly model");

	lowPolyFileLabel	= new QLineEdit("No file selected");
	highPolyFileLabel	= new QLineEdit("No file selected");
	lowPolyLoadBtn		= new QPushButton("Load...");
	highPolyLoadBtn		= new QPushButton("Load...");

	lowPolyFileLabel->setReadOnly(true);
	highPolyFileLabel->setReadOnly(true);

	QGridLayout* loadPanelLayout = new QGridLayout();
	loadPanelLayout->addWidget(lowPolyLabel,		0, 0);
	loadPanelLayout->addWidget(lowPolyFileLabel,	0, 1);
	loadPanelLayout->addWidget(lowPolyLoadBtn,		0, 2);
	loadPanelLayout->addWidget(highPolyLabel,		1, 0);
	loadPanelLayout->addWidget(highPolyFileLabel,	1, 1);
	loadPanelLayout->addWidget(highPolyLoadBtn,		1, 2);

	texturePreview = new QLabel();
	QImage tmpImg{1024,1024,QImage::Format_RGB888};
	tmpImg.allGray();
	setImage(tmpImg);
	QScrollArea* scrollArea = new QScrollArea();
	scrollArea->setWidget(texturePreview);
	mainLayout->addLayout(loadPanelLayout);
	mainLayout->addWidget(scrollArea);
	setLayout(mainLayout);

	connect(highPolyLoadBtn,	SIGNAL (clicked()), this, SLOT(loadHighObj()));
	connect(lowPolyLoadBtn,		SIGNAL (clicked()), this, SLOT(loadLowObj()));
}

void MainWindow::setImage(QImage& image) {
	texturePreview->setPixmap(QPixmap::fromImage(image));
	texturePreview->setMinimumSize(image.width(), image.height());
}

void MainWindow::loadHighObj() {
	const auto filepath = QFileDialog::getOpenFileName(this, "Open high poly OBJ", "./", "*.obj");

	highPolyFileLabel->setText("Loading...");
	highPolyFileLabel->repaint();

	core.loadHighObj(filepath.toUtf8().constData());

	core.raycast();
	std::cout << "DONE" << std::endl;
	setImage(core.texture);

	highPolyFileLabel->setText(filepath);
}


void MainWindow::loadLowObj() {
	const auto filepath = QFileDialog::getOpenFileName(this, "Open low poly OBJ", "./", "*.obj");

	lowPolyFileLabel->setText("Loading...");
	lowPolyFileLabel->repaint();

	core.loadLowObj(filepath.toUtf8().constData());

	core.splatTriangles();
	std::cout << "DONE" << std::endl;
	setImage(core.texture);

	lowPolyFileLabel->setText(filepath);
}