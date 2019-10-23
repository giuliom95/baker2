
#include "mainWindow.hpp"

MainWindow::MainWindow() :	QWidget(),
							core() {
	setWindowTitle("Baker");

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

	Worker* worker = new Worker(core);
	worker->moveToThread(&workerThread);

	connect(highPolyLoadBtn,	SIGNAL(clicked()), this,	SLOT(loadHighObj()));
	connect(lowPolyLoadBtn,		SIGNAL(clicked()), this,	SLOT(loadLowObj()));
	connect(startBakingBtn,		SIGNAL(clicked()), this,	SLOT(startMapGeneration()));
	connect(this, SIGNAL(startMapGenerationSig()), worker, SLOT(doWork()));
	connect(worker, SIGNAL(progressUpdate(int)), this, SLOT(mapGenerationProgress(int)), Qt::DirectConnection);
	connect(worker, SIGNAL(finished()), this, SLOT(mapGenerationDone()));

	workerThread.start();

	lowPolyLoaded = false;
	highPolyLoaded = false;
	startBakingBtn->setEnabled(false);
}

MainWindow::~MainWindow() {
	workerThread.quit();
	workerThread.wait();
}

void MainWindow::setImage(QImage& image) {
	texturePreview->setPixmap(QPixmap::fromImage(image));
	texturePreview->setMinimumSize(image.width(), image.height());
}

void MainWindow::loadHighObj() {
	const QString filepath = QFileDialog::getOpenFileName(this, "Open high poly OBJ", "./", "*.obj");
	if(filepath.isEmpty()) return;

	highPolyFileLabel->setText("Loading...");
	highPolyFileLabel->repaint();

	core.loadHighObj(filepath.toUtf8().constData());

	std::cout << "DONE" << std::endl;

	highPolyFileLabel->setText(filepath);

	highPolyLoaded = true;
	if(lowPolyLoaded) startBakingBtn->setEnabled(true);
}


void MainWindow::loadLowObj() {
	const auto filepath = QFileDialog::getOpenFileName(this, "Open low poly OBJ", "./", "*.obj");
	if(filepath.isEmpty()) return;

	lowPolyFileLabel->setText("Loading...");
	lowPolyFileLabel->repaint();

	core.loadLowObj(filepath.toUtf8().constData());

	progressBar->setMaximum(core.getLowTrisNum());

	std::cout << "DONE" << std::endl;

	lowPolyFileLabel->setText(filepath);

	lowPolyLoaded = true;
	if(highPolyLoaded) startBakingBtn->setEnabled(true);
}


void MainWindow::startMapGeneration() {

	// Fixes width so the label change doesn't change the button's size
	startBakingBtn->setMinimumWidth(startBakingBtn->width());
	startBakingBtn->setEnabled(false);
	startBakingBtn->setText("Baking...");
	startBakingBtn->repaint();

	emit startMapGenerationSig();
}

void MainWindow::mapGenerationDone() {
	progressBar->setValue(progressBar->maximum());
	startBakingBtn->setText("Start baking");
	startBakingBtn->setEnabled(true);

	std::cout << "DONE BAKING" << std::endl;
}

void MainWindow::mapGenerationProgress(int progress) {
	//std::cout << progress << std::endl;
	progressBar->setValue(progress);
	QCoreApplication::processEvents(QEventLoop::AllEvents);
}

void Worker::doWork() {

	const auto trinum = core.getLowTrisNum();
	for (int ti = 0; ti < trinum; ++ti) {
		core.generateNormalMapOnTriangle(ti);
		if(ti % 100 == 0) 
			emit progressUpdate(ti);
	}

	core.divideMapByCount();

	emit finished();
}
