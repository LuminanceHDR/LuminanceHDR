/**
 * This file is a part of Qtpfsgui package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2006,2007 Giuseppe Rota
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ----------------------------------------------------------------------
 *
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 */

#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QDir>
#include <QInputDialog>
#include <QStatusBar>
#include <QProgressBar>
#include <QLineEdit>
#include <QKeyEvent>
#include "tonemapping_widget.h"
#include "../Common/config.h"
#include "../Threads/tonemapperThread.h"

extern int xsize;
extern float pregamma;

TMWidget::TMWidget(QWidget *parent, QStatusBar *_sb) : QWidget(parent), sb(_sb), adding_custom_size(false) {
	setupUi(this);

	cachepath=QtpfsguiOptions::getInstance()->tempfilespath;

	// mantiuk06
	contrastfactorGang = new Gang(contrastFactorSlider,contrastFactordsb,contrastEqualizCheckBox,
		NULL,NULL, 0.001, 10, 0.1);
	connect(contrastfactorGang, SIGNAL(enableUndo(bool)), undoButton, SLOT(setEnabled(bool)));
	connect(contrastfactorGang, SIGNAL(enableRedo(bool)), redoButton, SLOT(setEnabled(bool)));

	saturationfactorGang = new Gang(saturationFactorSlider, saturationFactordsb,
		NULL,NULL,NULL, 0, 2, 0.8);
	detailfactorGang = new Gang(detailFactorSlider, detailFactordsb,NULL,NULL,NULL, 1, 99, 1.0);
	
	// fattal02
	alphaGang = 	  new Gang(alphaSlider, alphadsb, NULL,NULL,NULL, 1e-3, 2, 1e-1, true);
	connect(alphaGang, SIGNAL(enableUndo(bool)), undoButton, SLOT(setEnabled(bool)));
	connect(alphaGang, SIGNAL(enableRedo(bool)), redoButton, SLOT(setEnabled(bool)));
	betaGang = 	  new Gang(betaSlider, betadsb, NULL,NULL,NULL, 0.6, 1.0, 0.8);
	saturation2Gang = new Gang(saturation2Slider, saturation2dsb, NULL,NULL,NULL, 0, 3, 1);
	noiseGang =	  new Gang(noiseSlider, noisedsb, NULL,NULL,NULL, 0, 1, 0);
	oldFattalGang =	  new Gang(NULL,NULL, oldFattalCheckBox);

	// ashikhmin02
	contrastGang = 	new Gang(contrastSlider, contrastdsb,NULL,NULL,NULL, 0, 1, 0.5);
	connect(contrastGang, SIGNAL(enableUndo(bool)), undoButton, SLOT(setEnabled(bool)));
	connect(contrastGang, SIGNAL(enableRedo(bool)), redoButton, SLOT(setEnabled(bool)));
	simpleGang = 	new Gang(NULL, NULL, simpleCheckBox);
	eq2Gang = 	new Gang(NULL, NULL,NULL, NULL, eq2RadioButton);

	// drago03
	biasGang = 	new Gang(biasSlider, biasdsb,NULL,NULL,NULL, 0.5, 1, 0.85);
	connect(biasGang, SIGNAL(enableUndo(bool)), undoButton, SLOT(setEnabled(bool)));
	connect(biasGang, SIGNAL(enableRedo(bool)), redoButton, SLOT(setEnabled(bool)));

	// durand02
	spatialGang = 	new Gang(spatialSlider, spatialdsb,NULL,NULL,NULL, 0, 60, 8);
	connect(spatialGang, SIGNAL(enableUndo(bool)), undoButton, SLOT(setEnabled(bool)));
	connect(spatialGang, SIGNAL(enableRedo(bool)), redoButton, SLOT(setEnabled(bool)));
	rangeGang = 	new Gang(rangeSlider, rangedsb,NULL,NULL,NULL, 0.01, 10, 0.4);
	baseGang = 	new Gang(baseSlider, basedsb,NULL,NULL,NULL, 0, 10, 5.0);

	// pattanaik00
	multiplierGang = new Gang(multiplierSlider, multiplierdsb,NULL,NULL,NULL, 1e-3,50, 1, true);
	connect(multiplierGang, SIGNAL(enableUndo(bool)), undoButton, SLOT(setEnabled(bool)));
	connect(multiplierGang, SIGNAL(enableRedo(bool)), redoButton, SLOT(setEnabled(bool)));
	coneGang = 	 new Gang(coneSlider, conedsb,NULL,NULL,NULL, 0, 1, 0.5);
	rodGang = 	 new Gang(rodSlider, roddsb,NULL,NULL,NULL, 0, 1, 0.5);
	autoYGang =	 new Gang(NULL,NULL, autoYcheckbox);
	pattalocalGang = new Gang(NULL,NULL, pattalocal);

	// reinhard02
	keyGang = 	new Gang(keySlider, keydsb,NULL,NULL,NULL, 0, 1, 0.18);
	connect(keyGang, SIGNAL(enableUndo(bool)), undoButton, SLOT(setEnabled(bool)));
	connect(keyGang, SIGNAL(enableRedo(bool)), redoButton, SLOT(setEnabled(bool)));
	phiGang = 	new Gang(phiSlider, phidsb,NULL,NULL,NULL, 0, 50, 1);
	range2Gang = 	new Gang(range2Slider, range2dsb,NULL,NULL,NULL, 2, 15, 8);
	lowerGang = 	new Gang(lowerSlider, lowerdsb,NULL,NULL,NULL, 1, 100, 1);
	upperGang = 	new Gang(upperSlider, upperdsb,NULL,NULL,NULL, 1, 100, 43);
	usescalesGang = new Gang(NULL,NULL, usescalescheckbox);

	// reinhard05
	brightnessGang = new Gang(brightnessSlider, brightnessdsb,NULL,NULL,NULL, -35, 10, -10);
	connect(brightnessGang, SIGNAL(enableUndo(bool)), undoButton, SLOT(setEnabled(bool)));
	connect(brightnessGang, SIGNAL(enableRedo(bool)), redoButton, SLOT(setEnabled(bool)));
	chromaticGang  = new Gang(chromaticAdaptSlider, chromaticAdaptdsb,NULL,NULL,NULL, -1.7, 1.3, 1);
	lightGang      = new Gang(lightAdaptSlider, lightAdaptdsb,NULL,NULL,NULL,-1, 1, 0);

	// pregamma
	pregammagang =	new Gang(pregammaSlider, pregammadsb,NULL,NULL,NULL, 0, 3, 1);

	connect(stackedWidget_operators, SIGNAL(currentChanged (int)), this, SLOT(updateUndoState(int)));

	RecentPathLoadSaveTmoSettings=settings.value(KEY_RECENT_PATH_LOAD_SAVE_TMO_SETTINGS,QDir::currentPath()).toString();

	//re-read the original frame	
	pfs::DOMIO pfsio;
	originalPfsFrame=pfsio.readFrame( QFile::encodeName(cachepath+"/original.pfs").constData());

	// get available sizes
	assert(originalPfsFrame!=NULL);
	int height = originalPfsFrame->getHeight();
	int width = originalPfsFrame->getWidth();
	sizes.resize(0);
	for(int x = 256; x <= width; x *= 2) {
		if( x >= width )
			break;
		sizes.push_back(x);
		if( 3*(x/2) >= width )
			break;
		sizes.push_back(3*(x/2));
	}
	sizes.push_back(width);
	HeightWidthRatio = ( (float)height )/( (float) width);
	fillCustomSizeComboBox();

	qRegisterMetaType<QImage>("QImage");
}

TMWidget::~TMWidget() {
delete contrastfactorGang; delete saturationfactorGang; delete detailfactorGang; delete contrastGang; delete biasGang; delete spatialGang; delete rangeGang; delete baseGang; delete alphaGang; delete betaGang; delete saturation2Gang; delete noiseGang; delete multiplierGang; delete coneGang; delete rodGang; delete keyGang; delete phiGang; delete range2Gang; delete lowerGang; delete upperGang; delete brightnessGang; delete chromaticGang; delete lightGang; delete pregammagang;
	pfs::DOMIO pfsio;
	xsize = -1;
	pregamma = -1;
	pfsio.freeFrame(originalPfsFrame);
	QFile::remove(cachepath+"/original.pfs");
	QFile::remove(cachepath+"/after_resize.pfs");
	QFile::remove(cachepath+"/after_pregamma.pfs");
}

void TMWidget::on_defaultButton_clicked() {
	QWidget *current_page = stackedWidget_operators->currentWidget();
	if (current_page== page_mantiuk) {
		contrastfactorGang->setDefault();
		saturationfactorGang->setDefault();
		detailfactorGang->setDefault();
		contrastEqualizCheckBox->setChecked(false);
	}
	else if (current_page == page_ashikhmin) {
		contrastGang->setDefault();
		simpleCheckBox->setChecked(false);
		eq2RadioButton->setChecked(true);
	}
	else if (current_page == page_drago) {
		biasGang->setDefault();
	}
	else if (current_page == page_durand) {
		spatialGang->setDefault();
		rangeGang->setDefault();
		baseGang->setDefault();
	}
	else if (current_page == page_fattal) {
		alphaGang->setDefault();
		betaGang->setDefault();
		saturation2Gang->setDefault();
		noiseGang->setDefault();
		oldFattalCheckBox->setChecked(false);
	}
	else if (current_page == page_pattanaik) {
		multiplierGang->setDefault();
		coneGang->setDefault();
		rodGang->setDefault();
		pattalocal->setChecked(false);
		autoYcheckbox->setChecked(false);
	}
	else if (current_page == page_reinhard02) {
		keyGang->setDefault();
		phiGang->setDefault();
		range2Gang->setDefault();
		lowerGang->setDefault();
		upperGang->setDefault();
		usescalescheckbox->setChecked(false);
	}
	else if (current_page == page_reinhard05) {
		brightnessGang->setDefault();
		chromaticGang->setDefault();
		lightGang->setDefault();
	}
}

void TMWidget::updateUndoState(int) {
	QWidget *current_page = stackedWidget_operators->currentWidget();
	if (current_page== page_mantiuk) {
		contrastfactorGang->updateUndoState();
	}
	else if (current_page == page_ashikhmin) {
		contrastGang->updateUndoState();
	}
	else if (current_page == page_drago) {
		biasGang->updateUndoState();
	}
	else if (current_page == page_durand) {
		spatialGang->updateUndoState();
		rangeGang->setDefault();
		baseGang->setDefault();
	}
	else if (current_page == page_fattal) {
		alphaGang->updateUndoState();
	}
	else if (current_page == page_pattanaik) {
		multiplierGang->updateUndoState();
	}
	else if (current_page == page_reinhard02) {
		keyGang->updateUndoState();
	}
	else if (current_page == page_reinhard05) {
		brightnessGang->updateUndoState();
	}
}


void TMWidget::on_pregammadefault_clicked(){
	pregammagang->setDefault();
}

MyProgressBar::MyProgressBar(QWidget *parent) : QProgressBar(parent) {
	((QStatusBar*)parent)->addWidget(this);
	setValue(0);
}

MyProgressBar::~MyProgressBar() {
	((QStatusBar*)parent())->removeWidget(this);
}

void MyProgressBar::mousePressEvent(QMouseEvent *event) {
	if (event->buttons()==Qt::LeftButton)
		emit leftMouseButtonClicked();
}

void MyProgressBar::advanceCurrentProgress() {
	this->setValue(this->value()+1);
}

void TMWidget::on_applyButton_clicked() {
	bool doTonemapping = true;
	// Warning when using size dependent TMOs with smaller sizes
	if (stackedWidget_operators->currentWidget() == page_fattal &&
		(sizeComboBox->currentIndex()+1) < sizeComboBox->count())
	{
		doTonemapping = QMessageBox::Yes ==
			QMessageBox::question(
				this, "Attention",
				tr("This tonemapping operator depends on the size of the input image. Applying this operator on the full size image will most probably result in a different image.\n\nDo you want to continue?"),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes
			);
	}

	if (doTonemapping) {
		fillToneMappingOptions();
		setupUndo();

		MyProgressBar *newprogressbar=new MyProgressBar(sb);
		int w = sb->width();
		int h = (int) (0.9 * sb->height());

		newprogressbar->resize(w,h);
		//tone mapper thread needs to know full size of the hdr
		TonemapperThread *thread = new TonemapperThread(sizes[sizes.size()-1], ToneMappingOptions);

		connect(thread, SIGNAL(imageComputed(const QImage&,tonemapping_options*)), this, SIGNAL(newResult(const QImage&,tonemapping_options*)));
		connect(thread, SIGNAL(finished()), newprogressbar, SLOT(deleteLater()));
		connect(thread, SIGNAL(setMaximumSteps(int)), newprogressbar, SLOT(setMaximum(int)));
		connect(thread, SIGNAL(advanceCurrentProgress()), newprogressbar, SLOT(advanceCurrentProgress()));
		connect(newprogressbar, SIGNAL(leftMouseButtonClicked()), thread, SLOT(terminateRequested()));

		//start thread
		thread->start();
	}
}

void TMWidget::fillToneMappingOptions() {

	ToneMappingOptions.xsize=sizes[sizeComboBox->currentIndex()];
	ToneMappingOptions.pregamma=pregammagang->v();

	QWidget *current_page=stackedWidget_operators->currentWidget();
	if (current_page==page_mantiuk) {
		ToneMappingOptions.tmoperator=mantiuk;
		ToneMappingOptions.operator_options.mantiukoptions.contrastfactor=contrastfactorGang->v();
		ToneMappingOptions.operator_options.mantiukoptions.saturationfactor=saturationfactorGang->v();
		ToneMappingOptions.operator_options.mantiukoptions.detailfactor=detailfactorGang->v();
		ToneMappingOptions.operator_options.mantiukoptions.contrastequalization=contrastfactorGang->isCheckBox1Checked();
	} else if (current_page==page_fattal) {
		ToneMappingOptions.tmoperator=fattal;
		ToneMappingOptions.operator_options.fattaloptions.alpha=alphaGang->v();
		ToneMappingOptions.operator_options.fattaloptions.beta=betaGang->v();
		ToneMappingOptions.operator_options.fattaloptions.color=saturation2Gang->v();
		ToneMappingOptions.operator_options.fattaloptions.noiseredux=noiseGang->v();
		ToneMappingOptions.operator_options.fattaloptions.newfattal=!oldFattalGang->isCheckBox1Checked();
	} else if (current_page==page_ashikhmin) {
		ToneMappingOptions.tmoperator=ashikhmin;
		ToneMappingOptions.operator_options.ashikhminoptions.simple=simpleGang->isCheckBox1Checked();
		ToneMappingOptions.operator_options.ashikhminoptions.eq2=eq2Gang->isRadioButtonChecked();
		ToneMappingOptions.operator_options.ashikhminoptions.lct=contrastGang->v();
	} else if (current_page==page_durand) {
		ToneMappingOptions.tmoperator=durand;
		ToneMappingOptions.operator_options.durandoptions.spatial=spatialGang->v();
		ToneMappingOptions.operator_options.durandoptions.range=rangeGang->v();
		ToneMappingOptions.operator_options.durandoptions.base=baseGang->v();
	} else if (current_page==page_drago) {
		ToneMappingOptions.tmoperator=drago;
		ToneMappingOptions.operator_options.dragooptions.bias=biasGang->v();
	} else if (current_page==page_pattanaik) {
		ToneMappingOptions.tmoperator=pattanaik;
		ToneMappingOptions.operator_options.pattanaikoptions.autolum=autoYGang->isCheckBox1Checked();
		ToneMappingOptions.operator_options.pattanaikoptions.local=pattalocalGang->isCheckBox2Checked();
		ToneMappingOptions.operator_options.pattanaikoptions.cone=coneGang->v();
		ToneMappingOptions.operator_options.pattanaikoptions.rod=rodGang->v();
		ToneMappingOptions.operator_options.pattanaikoptions.multiplier=multiplierGang->v();
	} else if (current_page==page_reinhard02) {
		ToneMappingOptions.tmoperator=reinhard02;
		ToneMappingOptions.operator_options.reinhard02options.scales=usescalesGang->isCheckBox1Checked();
		ToneMappingOptions.operator_options.reinhard02options.key=keyGang->v();
		ToneMappingOptions.operator_options.reinhard02options.phi=phiGang->v();
		ToneMappingOptions.operator_options.reinhard02options.range=(int)range2Gang->v();
		ToneMappingOptions.operator_options.reinhard02options.lower=(int)lowerGang->v();
		ToneMappingOptions.operator_options.reinhard02options.upper=(int)upperGang->v();
	} else if (current_page==page_reinhard05) {
		ToneMappingOptions.tmoperator=reinhard05;
		ToneMappingOptions.operator_options.reinhard05options.brightness=brightnessGang->v();
		ToneMappingOptions.operator_options.reinhard05options.chromaticAdaptation=chromaticGang->v();
		ToneMappingOptions.operator_options.reinhard05options.lightAdaptation=lightGang->v();
	}
}

void TMWidget::setupUndo() {
	QWidget *current_page=stackedWidget_operators->currentWidget();
	if (current_page==page_mantiuk) {
		contrastfactorGang->setupUndo();
		saturationfactorGang->setupUndo();
		detailfactorGang->setupUndo();
	} else if (current_page==page_fattal) {
		alphaGang->setupUndo();
		betaGang->setupUndo();
		saturation2Gang->setupUndo();
		noiseGang->setupUndo();
		oldFattalGang->setupUndo();
	} else if (current_page==page_ashikhmin) {
		simpleGang->setupUndo();
		eq2Gang->setupUndo();
		contrastGang->setupUndo();
	} else if (current_page==page_durand) {
		spatialGang->setupUndo();
		rangeGang->setupUndo();
		baseGang->setupUndo();
	} else if (current_page==page_drago) {
		biasGang->setupUndo();
	} else if (current_page==page_pattanaik) {
		autoYGang->setupUndo();
		pattalocalGang->setupUndo();
		coneGang->setupUndo();
		rodGang->setupUndo();
		multiplierGang->setupUndo();
	} else if (current_page==page_reinhard02) {
		usescalesGang->setupUndo();
		keyGang->setupUndo();
		phiGang->setupUndo();
		range2Gang->setupUndo();
		lowerGang->setupUndo();
		upperGang->setupUndo();
	} else if (current_page==page_reinhard05) {
		brightnessGang->setupUndo();
		chromaticGang->setupUndo();
		lightGang->setupUndo();
	}
}

void TMWidget::on_undoButton_clicked() {
	QWidget *current_page=stackedWidget_operators->currentWidget();
	if (current_page==page_mantiuk) {
		contrastfactorGang->undo();
		saturationfactorGang->undo();
		detailfactorGang->undo();
	} else if (current_page==page_fattal) {
		alphaGang->undo();
		betaGang->undo();
		saturation2Gang->undo();
		noiseGang->undo();
		oldFattalGang->undo();
	} else if (current_page==page_ashikhmin) {
		simpleGang->undo();
		eq2Gang->undo();
		contrastGang->undo();
	} else if (current_page==page_durand) {
		spatialGang->undo();
		rangeGang->undo();
		baseGang->undo();
	} else if (current_page==page_drago) {
		biasGang->undo();
	} else if (current_page==page_pattanaik) {
		autoYGang->undo();
		pattalocalGang->undo();
		coneGang->undo();
		rodGang->undo();
		multiplierGang->undo();
	} else if (current_page==page_reinhard02) {
		usescalesGang->undo();
		keyGang->undo();
		phiGang->undo();
		range2Gang->undo();
		lowerGang->undo();
		upperGang->undo();
	} else if (current_page==page_reinhard05) {
		brightnessGang->undo();
		chromaticGang->undo();
		lightGang->undo();
	}
}

void TMWidget::on_redoButton_clicked() {
	QWidget *current_page=stackedWidget_operators->currentWidget();
	if (current_page==page_mantiuk) {
		contrastfactorGang->redo();
		saturationfactorGang->redo();
		detailfactorGang->redo();
	} else if (current_page==page_fattal) {
		alphaGang->redo();
		betaGang->redo();
		saturation2Gang->redo();
		noiseGang->redo();
		oldFattalGang->redo();
	} else if (current_page==page_ashikhmin) {
		simpleGang->redo();
		eq2Gang->redo();
		contrastGang->redo();
	} else if (current_page==page_durand) {
		spatialGang->redo();
		rangeGang->redo();
		baseGang->redo();
	} else if (current_page==page_drago) {
		biasGang->redo();
	} else if (current_page==page_pattanaik) {
		autoYGang->redo();
		pattalocalGang->redo();
		coneGang->redo();
		rodGang->redo();
		multiplierGang->redo();
	} else if (current_page==page_reinhard02) {
		usescalesGang->redo();
		keyGang->redo();
		phiGang->redo();
		range2Gang->redo();
		lowerGang->redo();
		upperGang->redo();
	} else if (current_page==page_reinhard05) {
		brightnessGang->redo();
		chromaticGang->redo();
		lightGang->redo();
	}
}

void TMWidget::on_loadsettingsbutton_clicked() {
	QString opened = QFileDialog::getOpenFileName(
			this,
			tr("Load a tonemapping settings text file..."),
			RecentPathLoadSaveTmoSettings,
			tr("Qtpfsgui tonemapping settings text file (*.txt)") );
	if( ! opened.isEmpty() ) {
		QFileInfo qfi(opened);
		if (!qfi.isReadable()) {
		QMessageBox::critical(this,tr("Aborting..."),tr("File is not readable (check existence, permissions,...)"),
					QMessageBox::Ok,QMessageBox::NoButton);
		return;
		}
		// update internal field variable
		RecentPathLoadSaveTmoSettings=qfi.path();
		// if the new dir, the one just chosen by the user, is different from the one stored in the settings, update the settings.
		if (RecentPathLoadSaveTmoSettings != settings.value(KEY_RECENT_PATH_LOAD_SAVE_TMO_SETTINGS,QDir::currentPath()).toString()) {
			settings.setValue(KEY_RECENT_PATH_LOAD_SAVE_TMO_SETTINGS, RecentPathLoadSaveTmoSettings);
		}
		//update filename internal field, used by parsing function fromTxt2Gui()
		TMOSettingsFilename=opened;
		//call parsing function
		fromTxt2Gui();
	}
}

void TMWidget::on_savesettingsbutton_clicked() {
	QString fname =QFileDialog::getSaveFileName(
			this,
			tr("Save tonemapping settings text file to..."),
			RecentPathLoadSaveTmoSettings,
			tr("Qtpfsgui tonemapping settings text file (*.txt)"));
	if( ! fname.isEmpty() ) {
		QFileInfo qfi(fname);
		if (qfi.suffix().toUpper() != "TXT") {
			fname+=".txt";
		}
		// update internal field variable
		RecentPathLoadSaveTmoSettings=qfi.path();
		// if the new dir, the one just chosen by the user, is different from the one stored in the settings, update the settings.
		if (RecentPathLoadSaveTmoSettings != settings.value(KEY_RECENT_PATH_LOAD_SAVE_TMO_SETTINGS, QDir::currentPath()).toString()) {
			settings.setValue(KEY_RECENT_PATH_LOAD_SAVE_TMO_SETTINGS, RecentPathLoadSaveTmoSettings);
		}
		//write txt file
		fromGui2Txt(fname);
	}
}

void TMWidget::fromGui2Txt(QString destination) {
	QFile file(destination);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QMessageBox::critical(this,tr("Aborting..."),tr("File is not writable (check permissions, path...)"),
		QMessageBox::Ok,QMessageBox::NoButton);
		return;
	}
	QTextStream out(&file);
	out << "# Qtpfsgui Tonemapping Setting file." << endl;
	out << "# Editing this file by hand is risky, worst case scenario is Qtpfsgui crashing." << endl;
	out << "# Please edit this file by hand only if you know what you're doing, in any case never change the left hand side text (i.e. the part before the ``='')." << endl;
	out << "TMOSETTINGSVERSION=" << TMOSETTINGSVERSION << endl;

	QWidget *current_page=stackedWidget_operators->currentWidget();
	if (current_page==page_mantiuk) {
		out << "TMO=" << "Mantiuk06" << endl;
		out << "CONTRASTFACTOR=" << contrastfactorGang->v() << endl;
		out << "SATURATIONFACTOR=" << saturationfactorGang->v() << endl;
		out << "DETAILFACTOR=" << detailfactorGang->v() << endl;
		out << "CONTRASTEQUALIZATION=" << (contrastEqualizCheckBox->isChecked() ? "YES" : "NO") << endl;
	} else if (current_page==page_fattal) {
		out << "TMO=" << "Fattal02" << endl;
		out << "ALPHA=" << alphaGang->v() << endl;
		out << "BETA=" << betaGang->v() << endl;
		out << "COLOR=" << saturation2Gang->v() << endl;
		out << "NOISE=" << noiseGang->v() << endl;
		out << "OLDFATTAL=" << (oldFattalCheckBox->isChecked() ? "YES" : "NO") << endl;
	} else if (current_page==page_ashikhmin) {
		out << "TMO=" << "Ashikhmin02" << endl;
		out << "SIMPLE=" << (simpleCheckBox->isChecked() ? "YES" : "NO") << endl;
		out << "EQUATION=" << (eq2RadioButton->isChecked() ? "2" : "4") << endl;
		out << "CONTRAST=" << contrastGang->v() << endl;
	} else if (current_page==page_durand) {
		out << "TMO=" << "Durand02" << endl;
		out << "SPATIAL=" << spatialGang->v() << endl;
		out << "RANGE=" << rangeGang->v() << endl;
		out << "BASE=" << baseGang->v() << endl;
	} else if (current_page==page_drago) {
		out << "TMO=" << "Drago03" << endl;
		out << "BIAS=" << biasGang->v() << endl;
	} else if (current_page==page_pattanaik) {
		out << "TMO=" << "Pattanaik00" << endl;
		out << "MULTIPLIER=" << multiplierGang->v() << endl;
		out << "LOCAL=" << (pattalocal->isChecked() ? "YES" : "NO") << endl;
		out << "AUTOLUMINANCE=" << (autoYcheckbox->isChecked() ? "YES" : "NO") << endl;
		out << "CONE=" << coneGang->v() << endl;
		out << "ROD=" << rodGang->v() << endl;
	} else if (current_page==page_reinhard02) {
		out << "TMO=" << "Reinhard02" << endl;
		out << "KEY=" << keyGang->v() << endl;
		out << "PHI=" << phiGang->v() << endl;
		out << "SCALES=" << (usescalescheckbox->isChecked() ? "YES" : "NO") << endl;
		out << "RANGE=" << range2Gang->v() << endl;
		out << "LOWER=" << lowerGang->v() << endl;
		out << "UPPER=" << upperGang->v() << endl;
	} else if (current_page==page_reinhard05) {
		out << "TMO=" << "Reinhard05" << endl;
		out << "BRIGHTNESS=" << brightnessGang->v() << endl;
		out << "CHROMATICADAPTATION=" << chromaticGang->v() << endl;
		out << "LIGHTADAPTATION=" << lightGang->v() << endl;
	}
	out << "PREGAMMA=" << pregammagang->v() << endl;
	file.close();
}

void TMWidget::fromTxt2Gui() {
	QFile file(TMOSettingsFilename);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text) || file.size()==0) {
		QMessageBox::critical(this,tr("Aborting..."),tr("File is not readable (check permissions, path...)"),
		QMessageBox::Ok,QMessageBox::NoButton);
		return;
	}
	QTextStream in(&file);
	QString field,value;

	while (!in.atEnd()) {
		QString line = in.readLine();
		//skip comments
		if (line.startsWith('#'))
			continue;

		field=line.section('=',0,0); //get the field
		value=line.section('=',1,1); //get the value
		if (field=="TMOSETTINGSVERSION") {
			if (value != TMOSETTINGSVERSION) {
				QMessageBox::critical(this,tr("Aborting..."),tr("Error, the tone mapping settings file format has changed. This (old) file cannot be used with this version of Qtpfsgui. Create a new one."),
				QMessageBox::Ok,QMessageBox::NoButton);
				return;
			}
		} else if (field=="TMO") {
			if (value=="Ashikhmin02") {
				stackedWidget_operators->setCurrentWidget(page_ashikhmin);
			} else if (value == "Mantiuk06") {
				stackedWidget_operators->setCurrentWidget(page_mantiuk);
			} else if (value == "Drago03") {
				stackedWidget_operators->setCurrentWidget(page_drago);
			} else if (value == "Durand02") {
				stackedWidget_operators->setCurrentWidget(page_durand);
			} else if (value == "Fattal02") {
				stackedWidget_operators->setCurrentWidget(page_fattal);
			} else if (value == "Pattanaik00") {
				stackedWidget_operators->setCurrentWidget(page_pattanaik);
			} else if (value == "Reinhard02") {
				stackedWidget_operators->setCurrentWidget(page_reinhard02);
			} else if (value == "Reinhard05") {
				stackedWidget_operators->setCurrentWidget(page_reinhard05);
			}
		} else if (field=="CONTRASTFACTOR") {
			contrastFactorSlider->setValue(contrastfactorGang->v2p(value.toFloat()));
		} else if (field=="SATURATIONFACTOR") {
			saturationFactorSlider->setValue(saturationfactorGang->v2p(value.toFloat()));
		} else if (field=="DETAILFACTOR") {
			detailFactorSlider->setValue(detailfactorGang->v2p(value.toFloat()));
		} else if (field=="CONTRASTEQUALIZATION") {
			contrastEqualizCheckBox->setChecked((value=="YES"));
		} else if (field=="SIMPLE") {
			simpleCheckBox->setChecked((value=="YES"));
		} else if (field=="EQUATION") {
			eq2RadioButton->setChecked((value=="2"));
			eq4RadioButton->setChecked((value=="4"));
		} else if (field=="CONTRAST") {
			contrastSlider->setValue(contrastGang->v2p(value.toFloat()));
		} else if (field=="BIAS") {
			biasSlider->setValue(biasGang->v2p(value.toFloat()));
		} else if (field=="SPATIAL") {
			spatialSlider->setValue(spatialGang->v2p(value.toFloat()));
		} else if (field=="RANGE") {
			rangeSlider->setValue(rangeGang->v2p(value.toFloat()));
		} else if (field=="BASE") {
			baseSlider->setValue(baseGang->v2p(value.toFloat()));
		} else if (field=="ALPHA") {
			alphaSlider->setValue(alphaGang->v2p(value.toFloat()));
		} else if (field=="BETA") {
			betaSlider->setValue(betaGang->v2p(value.toFloat()));
		} else if (field=="COLOR") {
			saturation2Slider->setValue(saturation2Gang->v2p(value.toFloat()));
		} else if (field=="NOISE") {
			noiseSlider->setValue(noiseGang->v2p(value.toFloat()));
		} else if (field=="OLDFATTAL") {
			 oldFattalCheckBox->setChecked(value=="YES");
		} else if (field=="MULTIPLIER") {
			multiplierSlider->setValue(multiplierGang->v2p(value.toFloat()));
		} else if (field=="LOCAL") {
			(value=="YES") ? pattalocal->setCheckState(Qt::Checked) : pattalocal->setCheckState(Qt::Unchecked);
		} else if (field=="AUTOLUMINANCE") {
			(value=="YES") ? autoYcheckbox->setCheckState(Qt::Checked) : autoYcheckbox->setCheckState(Qt::Unchecked);
		} else if (field=="CONE") {
			coneSlider->setValue(coneGang->v2p(value.toFloat()));
		} else if (field=="ROD") {
			rodSlider->setValue(rodGang->v2p(value.toFloat()));
		} else if (field=="KEY") {
			keySlider->setValue(keyGang->v2p(value.toFloat()));
		} else if (field=="PHI") {
			phiSlider->setValue(phiGang->v2p(value.toFloat()));
		} else if (field=="SCALES") {
			(value=="YES") ? usescalescheckbox->setCheckState(Qt::Checked) : usescalescheckbox->setCheckState(Qt::Unchecked);
		} else if (field=="RANGE") {
			range2Slider->setValue(range2Gang->v2p(value.toFloat()));
		} else if (field=="LOWER") {
			lowerSlider->setValue(lowerGang->v2p(value.toFloat()));
		} else if (field=="UPPER") {
			upperSlider->setValue(upperGang->v2p(value.toFloat()));
		} else if (field=="BRIGHTNESS") {
			brightnessSlider->setValue(brightnessGang->v2p(value.toFloat()));
		} else if (field=="CHROMATICADAPTATION") {
			chromaticAdaptSlider->setValue(chromaticGang->v2p(value.toFloat()));
		} else if (field=="LIGHTADAPTATION") {
			lightAdaptSlider->setValue(lightGang->v2p(value.toFloat()));
		} else if (field=="PREGAMMA") {
			pregammaSlider->setValue(pregammagang->v2p(value.toFloat()));
		}
	}
// 	on_applyButton_clicked();
}

void TMWidget::on_addCustomSizeButton_clicked(){
	bool ok;
	int i = QInputDialog::getInteger(this, tr("Custom LDR size"),
	                                      tr("Enter the width of the new size:"), 0 , 0, 2147483647, 1, &ok);
	if (ok && i > 0) {
		sizes.push_front(i);
		fillCustomSizeComboBox();
	}
}

void TMWidget::fillCustomSizeComboBox() {
	sizeComboBox->clear();
	for(int i = 0; i < sizes.size(); i++)
		sizeComboBox->addItem( QString("%1x%2").arg(sizes[i]).arg( (int)(HeightWidthRatio*sizes[i]) ));
}

