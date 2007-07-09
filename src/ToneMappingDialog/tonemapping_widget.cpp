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
#include <QStatusBar>
#include <QProgressBar>
#include "tonemapping_widget.h"
#include "../options.h"
#include "../config.h"
#include "../Threads/tonemapper_thread.h"

extern int xsize;
extern float pregamma;

TMWidget::TMWidget(QWidget *parent, pfs::Frame* &_OriginalPfsFrame, QString cachepath, QStatusBar *_sb) : QWidget(parent), OriginalPfsFrame(_OriginalPfsFrame), settings("Qtpfsgui", "Qtpfsgui"), cachepath(cachepath), sb(_sb) {
	setupUi(this);

	// ashikhmin02
	contrastGang = 	new Gang(contrastSlider, contrastdsb,	0,	1,	0.5);
	
	// drago03
	biasGang = 	new Gang(biasSlider, biasdsb,		0.5,	1,	0.85);
	
	// durand02
	spatialGang = 	new Gang(spatialSlider, spatialdsb,	0,	60,	8);
	rangeGang = 	new Gang(rangeSlider, rangedsb,		0,	10,	0.4);
	baseGang = 	new Gang(baseSlider, basedsb,		0,	10,	5.0);
	
	// fattal02
	alphaGang = 	new Gang(alphaSlider, alphadsb,		1e-3,	2,	1e-1, true);
	betaGang = 	new Gang(betaSlider, betadsb,		0.6,	1.0,	0.8);
	saturation2Gang = new Gang(saturation2Slider, saturation2dsb, 0,	3,	1);
	
	// pattanaik00
	multiplierGang = new Gang(multiplierSlider, multiplierdsb,	1e-3,	50, 1,    true);
	coneGang = 	new Gang(coneSlider, conedsb,		0,	1,   0.5);
	rodGang = 	new Gang(rodSlider, roddsb,		0,	1,   0.5);
	
	// reinhard02
	keyGang = 	new Gang(keySlider, keydsb,		0,	1,   0.18);
	phiGang = 	new Gang(phiSlider, phidsb,		0,	50,  1);
	range2Gang = 	new Gang(range2Slider, range2dsb,	2, 	15,  8);
	lowerGang = 	new Gang(lowerSlider, lowerdsb,		1,	100, 1);
	upperGang = 	new Gang(upperSlider, upperdsb,		1,	100, 43);
	
	// reinhard04
	brightnessGang =new Gang(brightnessSlider, brightnessdsb,	-35,	10, -10);
	saturationGang =new Gang(saturationSlider, saturationdsb,	 0,	1.2, 0.99);
	
	// pregamma
	pregammagang  = new Gang(pregammaSlider,  pregammadsb,  0, 3, 1);

	connect(ashikhmin02Default, SIGNAL(clicked()),this,SLOT(ashikhminReset()));
	connect(drago03Default,     SIGNAL(clicked()),this,SLOT(dragoReset()));
	connect(durand02Default,    SIGNAL(clicked()),this,SLOT(durandReset()));
	connect(fattal02Default,    SIGNAL(clicked()),this,SLOT(fattalReset()));
	connect(pattanaik00Default, SIGNAL(clicked()),this,SLOT(pattanaikReset()));
	connect(reinhard02Default,  SIGNAL(clicked()),this,SLOT(reinhard02Reset()));
	connect(reinhard04Default,  SIGNAL(clicked()),this,SLOT(reinhard04Reset()));
	connect(pregammadefault,    SIGNAL(clicked()),this,SLOT(preGammaReset()));

	connect(applyButton, SIGNAL(clicked()), this, SLOT(apply_clicked()));
	connect(loadsettingsbutton,SIGNAL(clicked()),this, SLOT(loadsettings()));
	connect(savesettingsbutton,SIGNAL(clicked()),this, SLOT(savesettings()));
	connect(button_fromTxt2Gui,SIGNAL(clicked()),this, SLOT(fromTxt2Gui()));

	RecentPathLoadSaveTmoSettings=settings.value(KEY_RECENT_PATH_LOAD_SAVE_TMO_SETTINGS,QDir::currentPath()).toString();

	// get available sizes
	assert(OriginalPfsFrame!=NULL);
	int height = OriginalPfsFrame->getHeight();
	int width = OriginalPfsFrame->getWidth();
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
	sizeComboBox->clear();
	float r = ( (float)height )/( (float) width);
	for(int i = 0; i < sizes.size(); i++) {
		sizeComboBox->addItem( QString("%1x%2").arg(sizes[i]).arg( (int)(r*sizes[i]) ));
	}

	//swap original frame to hd
	pfs::DOMIO pfsio;
	pfsio.writeFrame(OriginalPfsFrame, cachepath+"/original.pfs");
	pfsio.freeFrame(OriginalPfsFrame);
}

TMWidget::~TMWidget() {
delete contrastGang; delete biasGang; delete spatialGang; delete rangeGang; delete baseGang; delete alphaGang; delete betaGang; delete saturation2Gang; delete multiplierGang; delete coneGang; delete rodGang; delete keyGang; delete phiGang; delete range2Gang; delete lowerGang; delete upperGang; delete brightnessGang; delete saturationGang; delete pregammagang;
	//fetch original frame from hd
	pfs::DOMIO pfsio;
	OriginalPfsFrame=pfsio.readFrame(cachepath+"/original.pfs");
	xsize=-1;
	pregamma=-1;
	QFile::remove(cachepath+"/original.pfs");
	QFile::remove(cachepath+"/after_resize.pfs");
	QFile::remove(cachepath+"/after_pregamma.pfs");
}
void TMWidget::ashikhminReset() {
contrastGang->setDefault();
}
void TMWidget::dragoReset(){
biasGang->setDefault();
}
void TMWidget::durandReset(){
spatialGang->setDefault();
rangeGang->setDefault();
baseGang->setDefault();
}
void TMWidget::fattalReset(){
alphaGang->setDefault();
betaGang->setDefault();
saturation2Gang->setDefault();
}
void TMWidget::pattanaikReset(){
multiplierGang->setDefault();
coneGang->setDefault();
rodGang->setDefault();
}
void TMWidget::reinhard02Reset(){
keyGang->setDefault();
phiGang->setDefault();
range2Gang->setDefault();
lowerGang->setDefault();
upperGang->setDefault();
}
void TMWidget::reinhard04Reset(){
brightnessGang->setDefault();
saturationGang->setDefault();
}
void TMWidget::preGammaReset(){
pregammagang->setDefault();
}

void TMWidget::apply_clicked() {
	FillToneMappingOptions();

	QProgressBar *newprogressbar=new QProgressBar(sb);

	//tone mapper thread needs to know full size of the hdr
	TonemapperThread *thread = new TonemapperThread(sizes[sizes.size()-1], cachepath, newprogressbar);

	connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

	qRegisterMetaType<QImage>("QImage");
	connect(thread, SIGNAL(ImageComputed(const QImage&,tonemapping_options*)), this, SIGNAL(newResult(const QImage&,tonemapping_options*)));
	connect(thread, SIGNAL(removeProgressBar(QProgressBar*)), this, SLOT(removeProgressBar(QProgressBar*)));
	connect(thread, SIGNAL(setMaximumSteps(int)),newprogressbar,SLOT(setMaximum(int)));
	connect(thread, SIGNAL(setCurrentProgress(int)),newprogressbar,SLOT(setValue(int)));

	sb->addWidget(newprogressbar);
	//start thread
	thread->ComputeImage(ToneMappingOptions);
}

void TMWidget::FillToneMappingOptions() {

	ToneMappingOptions.xsize=sizes[sizeComboBox->currentIndex()];
	ToneMappingOptions.pregamma=pregammagang->v();

	switch (operators_tabWidget->currentIndex ()) {
	case 0:
		ToneMappingOptions.tmoperator=fattal;
		ToneMappingOptions.operator_options.fattaloptions.alpha=alphaGang->v();
		ToneMappingOptions.operator_options.fattaloptions.beta=betaGang->v();
		ToneMappingOptions.operator_options.fattaloptions.color=saturation2Gang->v();
	break;
	case 1:
		ToneMappingOptions.tmoperator=ashikhmin;
		ToneMappingOptions.operator_options.ashikhminoptions.simple=simpleCheckBox->isChecked();
		ToneMappingOptions.operator_options.ashikhminoptions.eq2=eq2RadioButton->isChecked();
		ToneMappingOptions.operator_options.ashikhminoptions.lct=contrastGang->v();
	break;
	case 2:
		ToneMappingOptions.tmoperator=durand;
		ToneMappingOptions.operator_options.durandoptions.spatial=spatialGang->v();
		ToneMappingOptions.operator_options.durandoptions.range=rangeGang->v();
		ToneMappingOptions.operator_options.durandoptions.base=baseGang->v();
	break;
	case 3:
		ToneMappingOptions.tmoperator=drago;
		ToneMappingOptions.operator_options.dragooptions.bias=biasGang->v();
	break;
	case 4:
		ToneMappingOptions.tmoperator=pattanaik;
		ToneMappingOptions.operator_options.pattanaikoptions.autolum=autoYcheckbox->isChecked();
		ToneMappingOptions.operator_options.pattanaikoptions.local=pattalocal->isChecked();
		ToneMappingOptions.operator_options.pattanaikoptions.cone=coneGang->v();
		ToneMappingOptions.operator_options.pattanaikoptions.rod=rodGang->v();
		ToneMappingOptions.operator_options.pattanaikoptions.multiplier=multiplierGang->v();
	break;
	case 5:
		ToneMappingOptions.tmoperator=reinhard02;
		ToneMappingOptions.operator_options.reinhard02options.scales=usescalescheckbox->isChecked();
		ToneMappingOptions.operator_options.reinhard02options.key=keyGang->v();
		ToneMappingOptions.operator_options.reinhard02options.phi=phiGang->v();
		ToneMappingOptions.operator_options.reinhard02options.range=(int)range2Gang->v();
		ToneMappingOptions.operator_options.reinhard02options.lower=(int)lowerGang->v();
		ToneMappingOptions.operator_options.reinhard02options.upper=(int)upperGang->v();
	break;
	case 6:
		ToneMappingOptions.tmoperator=reinhard04;
		ToneMappingOptions.operator_options.reinhard04options.brightness=brightnessGang->v();
		ToneMappingOptions.operator_options.reinhard04options.saturation=saturationGang->v();
	break;
	
	}
}


void TMWidget::loadsettings() {
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
		//enable graphical elements
		lineEditTMOsetting->setText(qfi.fileName());
		lineEditTMOsetting->setEnabled(TRUE);
		label_applyTMOSetting->setEnabled(TRUE);
		button_fromTxt2Gui->setEnabled(TRUE);
		//update filename internal field, used by parsing function fromTxt2Gui()
		TMOSettingsFilename=opened;
		//call parsing function
		fromTxt2Gui();
	}
}

void TMWidget::savesettings() {
	QString fname =QFileDialog::getSaveFileName(
			this,
			tr("Save tonemapping settings text file to..."),
			RecentPathLoadSaveTmoSettings,
			tr("Qtpfsgui tonemapping settings text file (*.txt)"));
	if( ! fname.isEmpty() ) {
		QFileInfo qfi(fname);
		// update internal field variable
		RecentPathLoadSaveTmoSettings=qfi.path();
		// if the new dir, the one just chosen by the user, is different from the one stored in the settings, update the settings.
		if (RecentPathLoadSaveTmoSettings != settings.value(KEY_RECENT_PATH_LOAD_SAVE_TMO_SETTINGS,QDir::currentPath()).toString()) {
			settings.setValue(KEY_RECENT_PATH_LOAD_SAVE_TMO_SETTINGS, RecentPathLoadSaveTmoSettings);
		}
		//enable graphical elements
		lineEditTMOsetting->setText(qfi.fileName());
		lineEditTMOsetting->setEnabled(TRUE);
		label_applyTMOSetting->setEnabled(TRUE);
		button_fromTxt2Gui->setEnabled(TRUE);
		//update filename internal field, used by parsing function fromTxt2Gui()
		TMOSettingsFilename=fname;
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
	switch (operators_tabWidget->currentIndex()) {
	case 1:
		out << "TMO=" << "Ashikhmin02" << endl;
		out << "SIMPLE=" << (simpleCheckBox->isChecked() ? "YES" : "NO") << endl;
		out << "EQUATION=" << (eq2RadioButton->isChecked() ? "2" : "4") << endl;
		out << "CONTRAST=" << contrastGang->v() << endl;
		break;
	case 3:
		out << "TMO=" << "Drago03" << endl;
		out << "BIAS=" << biasGang->v() << endl;
		break;
	case 2:
		out << "TMO=" << "Durand02" << endl;
		out << "SPACIAL=" << spatialGang->v() << endl;
		out << "RANGE=" << rangeGang->v() << endl;
		out << "BASE=" << baseGang->v() << endl;
		break;
	case 0:
		out << "TMO=" << "Fattal02" << endl;
		out << "ALPHA=" << alphaGang->v() << endl;
		out << "BETA=" << betaGang->v() << endl;
		out << "COLOR=" << saturation2Gang->v() << endl;
		break;
	case 4:
		out << "TMO=" << "Pattanaik00" << endl;
		out << "MULTIPLIER=" << multiplierGang->v() << endl;
		out << "LOCAL=" << (pattalocal->isChecked() ? "YES" : "NO") << endl;
		out << "AUTOLUMINANCE=" << (autoYcheckbox->isChecked() ? "YES" : "NO") << endl;
		out << "CONE=" << coneGang->v() << endl;
		out << "ROD=" << rodGang->v() << endl;
		break;
	case 5:
		out << "TMO=" << "Reinhard02" << endl;
		out << "KEY=" << keyGang->v() << endl;
		out << "PHI=" << phiGang->v() << endl;
		out << "SCALES=" << (usescalescheckbox->isChecked() ? "YES" : "NO") << endl;
		out << "RANGE=" << range2Gang->v() << endl;
		out << "LOWER=" << lowerGang->v() << endl;
		out << "UPPER=" << upperGang->v() << endl;
		break;
	case 6:
		out << "TMO=" << "Reinhard04" << endl;
		out << "BRIGHTNESS=" << brightnessGang->v() << endl;
		out << "SATURATION=" << saturationGang->v() << endl;
		break;
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
				operators_tabWidget->setCurrentIndex(1);
			} else if (value == "Drago03") {
				operators_tabWidget->setCurrentIndex(3);
			} else if (value == "Durand02") {
				operators_tabWidget->setCurrentIndex(2);
			} else if (value == "Fattal02") {
				operators_tabWidget->setCurrentIndex(0);
			} else if (value == "Pattanaik00") {
				operators_tabWidget->setCurrentIndex(4);
			} else if (value == "Reinhard02") {
				operators_tabWidget->setCurrentIndex(5);
			} else if (value == "Reinhard04") {
				operators_tabWidget->setCurrentIndex(6);
			}
		} else if (field=="SIMPLE") {
			(value == "YES") ? simpleCheckBox->setCheckState(Qt::Checked) : simpleCheckBox->setCheckState(Qt::Unchecked);
		} else if (field=="EQUATION") {
			(value=="2") ? eq2RadioButton->setChecked(true) : eq2RadioButton->setChecked(false);
			(value=="4") ? eq4RadioButton->setChecked(true) : eq4RadioButton->setChecked(false);
		} else if (field=="CONTRAST") {
			contrastSlider->setValue(contrastGang->v2p(value.toFloat()));
		} else if (field=="BIAS") {
			biasSlider->setValue(biasGang->v2p(value.toFloat()));
		} else if (field=="SPACIAL") {
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
		} else if (field=="SATURATION") {
			saturationSlider->setValue(saturationGang->v2p(value.toFloat()));
		} else if (field=="PREGAMMA") {
			pregammaSlider->setValue(pregammagang->v2p(value.toFloat()));
		}
	}
	apply_clicked();
}

void TMWidget::removeProgressBar(QProgressBar* pb) {
	sb->removeWidget(pb);
	delete pb;
}
