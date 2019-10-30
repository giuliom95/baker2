#ifndef _MAIN_WINDOW_HPP_
#define _MAIN_WINDOW_HPP_

#include "core.hpp"

#include <QtWidgets>

class MainWindow : public QWidget {
	Q_OBJECT
public:
	explicit MainWindow();
	~MainWindow();

public slots:
	void loadHighObj();
	void loadLowObj();
	void selectOutFile();
	void setMapSize(QString);
    void generateMap();

signals:
	void startMapGenerationSig();

private:
	QPushButton*	lowPolyLoadBtn;
	QPushButton*	highPolyLoadBtn;
	QPushButton*	outFileChooseBtn;
	QLineEdit*		lowPolyFileLabel;
	QLineEdit*		highPolyFileLabel;
	QLineEdit*		outFileFileLabel;
	QComboBox*		mapSizeCombo;
	QPushButton*	startBakingBtn;
	QProgressBar*	progressBar;

	Core 			core;
    QThread*		workerThread;

	QString			outFilePath;
	bool lowPolyLoaded, highPolyLoaded;

	void checkBakingRequirements();

	bool canUpdate;

};

#endif
