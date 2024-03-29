
#include "mainWindow.hpp"

MainWindow::MainWindow() :	QWidget(),
							core() {
	setWindowTitle("Baker");

	QVBoxLayout* mainLayout = new QVBoxLayout();
	
	QLabel* lowPolyLabel	= new QLabel("Low poly model");
	QLabel* highPolyLabel	= new QLabel("High poly model");
	QLabel* outFileLabel	= new QLabel("Out texture file");
	QLabel* mapSizeLabel	= new QLabel("Map size");

	lowPolyFileLabel	= new QLineEdit("No file selected");
	highPolyFileLabel	= new QLineEdit("No file selected");
	outFileFileLabel	= new QLineEdit("No file selected");
	lowPolyLoadBtn		= new QPushButton("Load...");
	highPolyLoadBtn		= new QPushButton("Load...");
	outFileChooseBtn	= new QPushButton("Choose...");
	
	mapSizeCombo		= new QComboBox();
	mapSizeCombo->addItem("256x256");
	mapSizeCombo->addItem("512x512");
	mapSizeCombo->addItem("1024x1024");
	mapSizeCombo->addItem("2048x2048");
	mapSizeCombo->addItem("4096x4096");
	mapSizeCombo->addItem("8192x8192");
	setMapSize("256x256");

	lowPolyFileLabel->setReadOnly(true);
	highPolyFileLabel->setReadOnly(true);
	outFileFileLabel->setReadOnly(true);

	QGridLayout* loadPanelLayout = new QGridLayout();
	loadPanelLayout->addWidget(lowPolyLabel,		0, 0);
	loadPanelLayout->addWidget(lowPolyFileLabel,	0, 1);
	loadPanelLayout->addWidget(lowPolyLoadBtn,		0, 2);
	loadPanelLayout->addWidget(highPolyLabel,		1, 0);
	loadPanelLayout->addWidget(highPolyFileLabel,	1, 1);
	loadPanelLayout->addWidget(highPolyLoadBtn,		1, 2);
	loadPanelLayout->addWidget(outFileLabel,		2, 0);
	loadPanelLayout->addWidget(outFileFileLabel,	2, 1);
	loadPanelLayout->addWidget(outFileChooseBtn,	2, 2);
	loadPanelLayout->addWidget(mapSizeLabel,		3, 0);
	loadPanelLayout->addWidget(mapSizeCombo,		3, 1);

	startBakingBtn = new QPushButton("Start baking");
	progressBar = new QProgressBar();
	progressBar->setMinimum(0);
	progressBar->setMaximum(10);
	progressBar->setValue(0);

	QHBoxLayout* startBar = new QHBoxLayout();
	startBar->addWidget(progressBar);
	startBar->addWidget(startBakingBtn);

	mainLayout->addLayout(loadPanelLayout);
	mainLayout->addLayout(startBar);
	setLayout(mainLayout);

	connect(highPolyLoadBtn,	SIGNAL(clicked()), this,	SLOT(loadHighObj()));
	connect(lowPolyLoadBtn,		SIGNAL(clicked()), this,	SLOT(loadLowObj()));
	connect(outFileChooseBtn,	SIGNAL(clicked()), this,	SLOT(selectOutFile()));
	connect(mapSizeCombo,		SIGNAL(activated(QString)), this, SLOT(setMapSize(QString)));
	connect(startBakingBtn,		SIGNAL(clicked()), this,	SLOT(generateMap()));

	lowPolyLoaded = false;
	highPolyLoaded = false;
	outFilePath = QString();
	startBakingBtn->setEnabled(false);

}

MainWindow::~MainWindow() {
}

void MainWindow::loadHighObj() {
	const QString filepath = QFileDialog::getOpenFileName(this, "Open high poly OBJ", "./", "*.obj");
	if(filepath.isEmpty()) return;

	lockButtons();
	highPolyFileLabel->setText("Loading...");
	repaint();

	core.loadHighObj(filepath.toUtf8().constData());

	std::cout << "DONE" << std::endl;

	highPolyFileLabel->setText(filepath);

	highPolyLoaded = true;
	checkBakingRequirements();
	unlockButtons();
}


void MainWindow::loadLowObj() {
	const auto filepath = QFileDialog::getOpenFileName(this, "Open low poly OBJ", "./", "*.obj");
	if(filepath.isEmpty()) return;

	lockButtons();
	lowPolyFileLabel->setText("Loading...");
	repaint();

	core.loadLowObj(filepath.toUtf8().constData());

	progressBar->setMaximum(core.getLowTrisNum());

	std::cout << "DONE" << std::endl;

	lowPolyFileLabel->setText(filepath);

	lowPolyLoaded = true;
	checkBakingRequirements();
	unlockButtons();
}

void MainWindow::selectOutFile() {
	const auto filepath = QFileDialog::getSaveFileName(this, "Select out file", "./", "*.png");
	if(filepath.isEmpty()) return;
	outFilePath = filepath;
	outFileFileLabel->setText(filepath);

	checkBakingRequirements();
}

void MainWindow::setMapSize(QString s) {
	int w = s.split("x")[0].toInt();
	core.tex_w = w;
	core.tex_h = w;
}

void MainWindow::generateMap() {

	// Fixes width so the label change doesn't change the button's size
	startBakingBtn->setMinimumWidth(startBakingBtn->width());
	lockButtons();
	startBakingBtn->setText("Baking...");
	repaint();

	//emit startMapGenerationSig();

	core.clearBuffers();
	const auto trinum = core.getLowTrisNum();
	
	for (int ti = 0; ti < trinum; ++ti) {
		core.generateNormalMapOnTriangle(ti);
		if(ti % 100 == 0) {
			progressBar->setValue(ti);
		}
	}

	core.divideMapByCount();
	const int w = core.tex_w;
	const int h = core.tex_h;
	QImage img{w, h, QImage::Format_RGB888};
	uchar* img_bits = img.bits();
	for(int i = 0; i < w; ++i) {
		for(int j = 0; j < h; ++j) {
			const int out_idx = 3*(i + (h - j - 1)*w);
			const int im = i - 1 < 	0 ?     0 : i - 1; // i minus
			const int ip = i + 1 >= w ? w - 1 : i + 1; // i plus
			const int jm = j - 1 < 	0 ?     0 : j - 1; // j minus
			const int jp = j + 1 >= h ? h - 1 : j + 1; // j plus

			const int idx_mm = 3*(im + jm*w);
			const int idx_0m = 3*(i  + jm*w);
			const int idx_pm = 3*(ip + jm*w);
			const int idx_m0 = 3*(im + j *w);
			const int idx_00 = 3*(i  + j *w);
			const int idx_p0 = 3*(ip + j *w);
			const int idx_mp = 3*(im + jp*w);
			const int idx_0p = 3*(i  + jp*w);
			const int idx_pp = 3*(ip + jp*w);

			Vec3f blurred = (1/16.0f) * ( 
				1 * Vec3f{core.tex[idx_mm + 0], core.tex[idx_mm + 1], core.tex[idx_mm + 2]} + 
				2 * Vec3f{core.tex[idx_0m + 0], core.tex[idx_0m + 1], core.tex[idx_0m + 2]} + 
				1 * Vec3f{core.tex[idx_pm + 0], core.tex[idx_pm + 1], core.tex[idx_pm + 2]} + 
				2 * Vec3f{core.tex[idx_m0 + 0], core.tex[idx_m0 + 1], core.tex[idx_m0 + 2]} + 
				4 * Vec3f{core.tex[idx_00 + 0], core.tex[idx_00 + 1], core.tex[idx_00 + 2]} + 
				2 * Vec3f{core.tex[idx_p0 + 0], core.tex[idx_p0 + 1], core.tex[idx_p0 + 2]} + 
				1 * Vec3f{core.tex[idx_mp + 0], core.tex[idx_mp + 1], core.tex[idx_mp + 2]} + 
				2 * Vec3f{core.tex[idx_0p + 0], core.tex[idx_0p + 1], core.tex[idx_0p + 2]} + 
				1 * Vec3f{core.tex[idx_pp + 0], core.tex[idx_pp + 1], core.tex[idx_pp + 2]}
			);

			img_bits[out_idx + 0] = 128 * blurred[0] + 127;
			img_bits[out_idx + 1] = 128 * blurred[1] + 127;
			img_bits[out_idx + 2] = 128 * blurred[2] + 127;
		}
	}
	img.save(outFilePath);

	progressBar->setValue(progressBar->maximum());
	startBakingBtn->setText("Start baking");
	unlockButtons();
}

void MainWindow::checkBakingRequirements() {
	if(
		lowPolyLoaded &&
		highPolyLoaded &&
		!outFilePath.isEmpty()
	) startBakingBtn->setEnabled(true);
}

void MainWindow::lockButtons() {
	lowPolyLoadBtn->setEnabled(false);
	highPolyLoadBtn->setEnabled(false);
	outFileChooseBtn->setEnabled(false);
	lowPolyFileLabel->setEnabled(false);
	highPolyFileLabel->setEnabled(false);
	mapSizeCombo->setEnabled(false);
	outFileFileLabel->setEnabled(false);
	startBakingBtn->setEnabled(false);
};

void MainWindow::unlockButtons() {
	startBakingBtn->setEnabled(true);
	lowPolyLoadBtn->setEnabled(true);
	highPolyLoadBtn->setEnabled(true);
	outFileChooseBtn->setEnabled(true);
	lowPolyFileLabel->setEnabled(true);
	highPolyFileLabel->setEnabled(true);
	mapSizeCombo->setEnabled(true);
	outFileFileLabel->setEnabled(true);
};
