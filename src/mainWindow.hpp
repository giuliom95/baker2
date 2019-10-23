#ifndef _MAIN_WINDOW_HPP_
#define _MAIN_WINDOW_HPP_

#include "core.hpp"

#include <QtWidgets>

class MainWindow : public QWidget {
	Q_OBJECT
public:
	explicit MainWindow();
	~MainWindow();

	void setImage(QImage& image);

public slots:
	void loadHighObj();
	void loadLowObj();
	void startMapGeneration();
	void mapGenerationDone();
	void mapGenerationProgress(int);

signals:
	void startMapGenerationSig();

private:
	QLabel*			texturePreview;
	QPushButton*	lowPolyLoadBtn;
	QPushButton*	highPolyLoadBtn;
	QLineEdit*		lowPolyFileLabel;
	QLineEdit*		highPolyFileLabel;
	QPushButton*	startBakingBtn;
	QProgressBar*	progressBar;

	Core 			core;
	QThread			workerThread;
};

class Worker : public QObject {
	Q_OBJECT
public slots:
	void doWork();

public:
	Core& core;
	Worker(Core& c) : core{c} {}

signals:
	void progressUpdate(int);
	void finished();
};

#endif