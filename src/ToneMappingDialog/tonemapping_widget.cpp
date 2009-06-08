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
	contrastfactorGang = new Gang(contrastFactorSlider, contrastFactordsb,0.001,10,0.1);
	saturationfactorGang = new Gang(saturationFactorSlider, saturationFactordsb,0,2,0.8);
	detailfactorGang = new Gang(detailFactorSlider, detailFactordsb,1,99,1.0);

	// ashikhmin02
	contrastGang = 	new Gang(contrastSlider, contrastdsb,	0,	1,	0.5);

	// drago03
	biasGang = 	new Gang(biasSlider, biasdsb,		0.5,	1,	0.85);

	// durand02
	spatialGang = 	new Gang(spatialSlider, spatialdsb,	0,	60,	8);
	rangeGang = 	new Gang(rangeSlider, rangedsb,		0.01,	10,	0.4);
	baseGang = 	new Gang(baseSlider, basedsb,		0,	10,	5.0);

	// fattal02
	alphaGang = 	new Gang(alphaSlider, alphadsb,		1e-3,	2,	1e-1, true);
	betaGang = 	new Gang(betaSlider, betadsb,		0.6,	1.0,	0.8);
	saturation2Gang = new Gang(saturation2Slider, saturation2dsb, 0,3,	1);
	noiseGang =	new Gang(noiseSlider, noisedsb, 	0, 	1, 	0);

	// pattanaik00
	multiplierGang = new Gang(multiplierSlider, multiplierdsb, 1e-3,50, 1, true);
	coneGang = 	new Gang(coneSlider, conedsb,		0,	1,   0.5);
	rodGang = 	new Gang(rodSlider, roddsb,		0,	1,   0.5);

	// reinhard02
	keyGang = 	new Gang(keySlider, keydsb,		0,	1,   0.18);
	phiGang = 	new Gang(phiSlider, phidsb,		0,	50,  1);
	range2Gang = 	new Gang(range2Slider, range2dsb,	2, 	15,  8);
	lowerGang = 	new Gang(lowerSlider, lowerdsb,		1,	100, 1);
	upperGang = 	new Gang(upperSlider, upperdsb,		1,	100, 43);

	// reinhard05
	brightnessGang =new Gang(brightnessSlider, brightnessdsb,	-35,	10, -10);
	chromaticGang  =new Gang(chromaticAdaptSlider, chromaticAdaptdsb, -1.7,	1.3, 1);
	lightGang      =new Gang(lightAdaptSlider, lightAdaptdsb,         -1,	1, 0);

	// pregamma
	pregammagang  = new Gang(pregammaSlider,  pregammadsb,  0, 3, 1);

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
	xsize=-1;
	pregamma=-1;
	pfsio.freeFrame(originalPfsFrame);
	QFile::remove(cachepath+"/original.pfs");
	QFile::remove(cachepath+"/after_resize.pfs");
	QFile::remove(cachepath+"/after_pregamma.pfs");
}

void TMWidget::on_MantiukDefault_clicked() {
	contrastfactorGang->setDefault();
	saturationfactorGang->setDefault();
	detailfactorGang->setDefault();
	ContrastEqualizCheckBox->setChecked(false);
}
void TMWidget::on_ashikhmin02Default_clicked() {
	contrastGang->setDefault();
	simpleCheckBox->setChecked(false);
	eq2RadioButton->setChecked(true);
}
void TMWidget::on_drago03Default_clicked(){
	biasGang->setDefault();
}
void TMWidget::on_durand02Default_clicked(){
	spatialGang->setDefault();
	rangeGang->setDefault();
	baseGang->setDefault();
}
void TMWidget::on_fattal02Default_clicked(){
	alphaGang->setDefault();
	betaGang->setDefault();
	saturation2Gang->setDefault();
	noiseGang->setDefault();
	oldFattalCheckBox->setChecked(false);
}
void TMWidget::on_pattanaik00Default_clicked(){
	multiplierGang->setDefault();
	coneGang->setDefault();
	rodGang->setDefault();
	pattalocal->setChecked(false);
	autoYcheckbox->setChecked(false);
}
void TMWidget::on_reinhard02Default_clicked(){
	keyGang->setDefault();
	phiGang->setDefault();
	range2Gang->setDefault();
	lowerGang->setDefault();
	upperGang->setDefault();
	usescalescheckbox->setChecked(false);
}
void TMWidget::on_reinhard05Default_clicked(){
	brightnessGang->setDefault();
	chromaticGang->setDefault();
	lightGang->setDefault();
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
		FillToneMappingOptions();

		MyProgressBar *newprogressbar=new MyProgressBar(sb);

		//tone mapper thread needs to know full size of the hdr
		TonemapperThread *thread = new TonemapperThread(sizes[sizes.size()-1], ToneMappingOptions);

		connect(thread, SIGNAL(ImageComputed(const QImage&,tonemapping_options*)), this, SIGNAL(newResult(const QImage&,tonemapping_options*)));
		connect(thread, SIGNAL(finished()), newprogressbar, SLOT(deleteLater()));
		connect(thread, SIGNAL(setMaximumSteps(int)), newprogressbar, SLOT(setMaximum(int)));
		connect(thread, SIGNAL(advanceCurrentProgress()), newprogressbar, SLOT(advanceCurrentProgress()));
		connect(newprogressbar, SIGNAL(leftMouseButtonClicked()), thread, SLOT(terminateRequested()));

		//start thread
		thread->start();
	}
}

void TMWidget::FillToneMappingOptions() {

	ToneMappingOptions.xsize=sizes[sizeComboBox->currentIndex()];
	ToneMappingOptions.pregamma=pregammagang->v();

	QWidget *current_page=stackedWidget_operators->currentWidget();
	if (current_page==page_mantiuk) {
		ToneMappingOptions.tmoperator=mantiuk;
		ToneMappingOptions.operator_options.mantiukoptions.contrastfactor=contrastfactorGang->v();
		ToneMappingOptions.operator_options.mantiukoptions.saturationfactor=saturationfactorGang->v();
		ToneMappingOptions.operator_options.mantiukoptions.detailfactor=detailfactorGang->v();
		ToneMappingOptions.operator_options.mantiukoptions.contrastequalization=ContrastEqualizCheckBox->isChecked();
	} else if (current_page==page_fattal) {
		ToneMappingOptions.tmoperator=fattal;
		ToneMappingOptions.operator_options.fattaloptions.alpha=alphaGang->v();
		ToneMappingOptions.operator_options.fattaloptions.beta=betaGang->v();
		ToneMappingOptions.operator_options.fattaloptions.color=saturation2Gang->v();
		ToneMappingOptions.operator_options.fattaloptions.noiseredux=noiseGang->v();
		ToneMappingOptions.operator_options.fattaloptions.newfattal=!oldFattalCheckBox->isChecked();
	} else if (current_page==page_ashikhmin) {
		ToneMappingOptions.tmoperator=ashikhmin;
		ToneMappingOptions.operator_options.ashikhminoptions.simple=simpleCheckBox->isChecked();
		ToneMappingOptions.operator_options.ashikhminoptions.eq2=eq2RadioButton->isChecked();
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
		ToneMappingOptions.operator_options.pattanaikoptions.autolum=autoYcheckbox->isChecked();
		ToneMappingOptions.operator_options.pattanaikoptions.local=pattalocal->isChecked();
		ToneMappingOptions.operator_options.pattanaikoptions.cone=coneGang->v();
		ToneMappingOptions.operator_options.pattanaikoptions.rod=rodGang->v();
		ToneMappingOptions.operator_options.pattanaikoptions.multiplier=multiplierGang->v();
	} else if (current_page==page_reinhard02) {
		ToneMappingOptions.tmoperator=reinhard02;
		ToneMappingOptions.operator_options.reinhard02options.scales=usescalescheckbox->isChecked();
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
		out << "CONTRASTEQUALIZATION=" << (ContrastEqualizCheckBox->isChecked() ? "YES" : "NO") << endl;
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
			ContrastEqualizCheckBox->setChecked((value=="YES"));
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

