
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

	startBakingBtn = new QPushButton("Start baking");

	texturePreview = new QLabel();
	QImage tmpImg{1024,1024,QImage::Format_RGB888};
	tmpImg.allGray();
	setImage(tmpImg);
	QScrollArea* scrollArea = new QScrollArea();
	scrollArea->setWidget(texturePreview);

	mainLayout->addLayout(loadPanelLayout);
	mainLayout->addWidget(startBakingBtn);
	mainLayout->addWidget(scrollArea);
	setLayout(mainLayout);

	connect(highPolyLoadBtn,	SIGNAL (clicked()), this, SLOT(loadHighObj()));
	connect(lowPolyLoadBtn,		SIGNAL (clicked()), this, SLOT(loadLowObj()));
	connect(startBakingBtn,		SIGNAL (clicked()), this, SLOT(startBaking()));
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

	std::cout << "DONE" << std::endl;

	highPolyFileLabel->setText(filepath);
}


void MainWindow::loadLowObj() {
	const auto filepath = QFileDialog::getOpenFileName(this, "Open low poly OBJ", "./", "*.obj");

	lowPolyFileLabel->setText("Loading...");
	lowPolyFileLabel->repaint();

	core.loadLowObj(filepath.toUtf8().constData());

	std::cout << "DONE" << std::endl;

	lowPolyFileLabel->setText(filepath);
}

void MainWindow::startBaking() {
	startBakingBtn->setEnabled(false);
	core.generateNormalMap();
	QImage img{1024, 1024, QImage::Format_RGB888};
	uchar* img_bits{img.bits()};
	for(int j = 0; j < core.tex_h; ++j) {
		for(int i = 0; i < core.tex_w; ++i) {
			const int idx_out = i + j*core.tex_w;
			const int idx_in = i + (core.tex_h - j - 1)*core.tex_w;
			img_bits[3*idx_out + 0] = (uchar)(255 * (0.5 * core.tex[3*idx_in + 0] + 0.5));
			img_bits[3*idx_out + 1] = (uchar)(255 * (0.5 * core.tex[3*idx_in + 1] + 0.5));
			img_bits[3*idx_out + 2] = (uchar)(255 * (0.5 * core.tex[3*idx_in + 2] + 0.5));
		}
	}

	setImage(img);

	startBakingBtn->setEnabled(true);

	img.save("out/test.tiff");
}