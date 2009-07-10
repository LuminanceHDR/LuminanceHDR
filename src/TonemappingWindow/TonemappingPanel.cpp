/*
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
 * Original Work
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * Improvements, bugfixing
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
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

#include "Common/config.h"
#include "TonemappingPanel.h"
#include "TMOProgressIndicator.h"

TonemappingPanel::TonemappingPanel(QWidget *parent) : QWidget(parent), adding_custom_size(false) {
	setupUi(this);

	currentTmoOperator = mantiuk06; // from Qt Designer

	// mantiuk06
	contrastfactorGang = new Gang(contrastFactorSlider,contrastFactordsb,contrastEqualizCheckBox,
		NULL,NULL, NULL, 0.001f, 1.0f, 0.1f);
	connect(contrastfactorGang, SIGNAL(enableUndo(bool)), undoButton, SLOT(setEnabled(bool)));
	connect(contrastfactorGang, SIGNAL(enableRedo(bool)), redoButton, SLOT(setEnabled(bool)));

	saturationfactorGang = new Gang(saturationFactorSlider, saturationFactordsb,
		NULL,NULL,NULL,NULL, 0.0f, 1.0f, 0.8f);
	detailfactorGang = new Gang(detailFactorSlider, detailFactordsb,NULL,NULL,NULL,NULL, 1.0f, 99.0f, 1.0f);

	// mantiuk08
	colorSaturationGang = new Gang(colorSaturationSlider,colorSaturationDSB,
		NULL,NULL,NULL,NULL, 0.f, 2.f, 1.f);
	connect(colorSaturationGang, SIGNAL(enableUndo(bool)), undoButton, SLOT(setEnabled(bool)));
	connect(colorSaturationGang, SIGNAL(enableRedo(bool)), redoButton, SLOT(setEnabled(bool)));

	contrastEnhancementGang = new Gang(contrastEnhancementSlider, contrastEnhancementDSB,
		NULL,NULL,NULL,NULL, .01f, 10.f, 1.f);
	luminanceLevelGang = new Gang(luminanceLevelSlider, luminanceLevelDSB, luminanceLevelCheckBox,
		NULL,NULL,NULL, 1.f, 100.0f, 1.f);

	// fattal02
	alphaGang = 	  new Gang(alphaSlider, alphadsb, NULL,NULL,NULL,NULL, 1e-4, 2.f, 1.f, true);
	connect(alphaGang, SIGNAL(enableUndo(bool)), undoButton, SLOT(setEnabled(bool)));
	connect(alphaGang, SIGNAL(enableRedo(bool)), redoButton, SLOT(setEnabled(bool)));

	betaGang = 	  new Gang(betaSlider, betadsb, NULL,NULL,NULL,NULL, 0.1f, 2.f, 0.9f);
	saturation2Gang = new Gang(saturation2Slider, saturation2dsb, NULL,NULL,NULL,NULL, 0.f, 1.f, .8f);
	noiseGang =	  new Gang(noiseSlider, noisedsb, NULL,NULL,NULL,NULL, 0, 1.f, 0.f);
	oldFattalGang =	  new Gang(NULL,NULL, oldFattalCheckBox);

	// ashikhmin02
	contrastGang = 	new Gang(contrastSlider, contrastdsb,NULL,NULL,NULL,NULL, 0.f, 1.f, 0.5f);
	connect(contrastGang, SIGNAL(enableUndo(bool)), undoButton, SLOT(setEnabled(bool)));
	connect(contrastGang, SIGNAL(enableRedo(bool)), redoButton, SLOT(setEnabled(bool)));
	simpleGang = 	new Gang(NULL, NULL, simpleCheckBox);
	eq2Gang = 	new Gang(NULL, NULL,NULL, NULL, eq2RadioButton, eq4RadioButton);

	// drago03
	biasGang = 	new Gang(biasSlider, biasdsb,NULL,NULL,NULL,NULL, 0.f, 1.f, 0.85f);
	connect(biasGang, SIGNAL(enableUndo(bool)), undoButton, SLOT(setEnabled(bool)));
	connect(biasGang, SIGNAL(enableRedo(bool)), redoButton, SLOT(setEnabled(bool)));

	// durand02
	spatialGang = 	new Gang(spatialSlider, spatialdsb,NULL,NULL,NULL,NULL, 0.f, 100.f, 2.f);
	connect(spatialGang, SIGNAL(enableUndo(bool)), undoButton, SLOT(setEnabled(bool)));
	connect(spatialGang, SIGNAL(enableRedo(bool)), redoButton, SLOT(setEnabled(bool)));
	rangeGang = 	new Gang(rangeSlider, rangedsb,NULL,NULL,NULL,NULL, 0.01f, 10.f, 0.4f);
	baseGang = 	new Gang(baseSlider, basedsb,NULL,NULL,NULL,NULL, 0.f, 10.f, 5.0f);

	// pattanaik00
	multiplierGang = new Gang(multiplierSlider, multiplierdsb,NULL,NULL,NULL,NULL, 1e-3,1000.f, 1.f, true);
	connect(multiplierGang, SIGNAL(enableUndo(bool)), undoButton, SLOT(setEnabled(bool)));
	connect(multiplierGang, SIGNAL(enableRedo(bool)), redoButton, SLOT(setEnabled(bool)));
	coneGang = 	 new Gang(coneSlider, conedsb,NULL,NULL,NULL,NULL, 0.f, 1.f, 0.5f);
	rodGang = 	 new Gang(rodSlider, roddsb,NULL,NULL,NULL,NULL, 0.f, 1.f, 0.5f);
	autoYGang =	 new Gang(NULL,NULL, autoYcheckbox);
	pattalocalGang = new Gang(NULL,NULL, pattalocal);

	// reinhard02
	keyGang = 	new Gang(keySlider, keydsb,NULL,NULL,NULL,NULL, 0.f, 1.f, 0.18f);
	connect(keyGang, SIGNAL(enableUndo(bool)), undoButton, SLOT(setEnabled(bool)));
	connect(keyGang, SIGNAL(enableRedo(bool)), redoButton, SLOT(setEnabled(bool)));
	phiGang = 	new Gang(phiSlider, phidsb,NULL,NULL,NULL,NULL, 0.f, 100.f, 1.f);
	range2Gang = 	new Gang(range2Slider, range2dsb,NULL,NULL,NULL,NULL, 1.f, 32.f, 8.f);
	lowerGang = 	new Gang(lowerSlider, lowerdsb,NULL,NULL,NULL,NULL, 1.f, 100.f, 1.f);
	upperGang = 	new Gang(upperSlider, upperdsb,NULL,NULL,NULL,NULL, 1.f, 100.f, 43.f);
	usescalesGang = new Gang(NULL,NULL, usescalescheckbox);

	// reinhard05
	brightnessGang = new Gang(brightnessSlider, brightnessdsb,NULL,NULL,NULL,NULL, -8.f, 8.f, 0.f);
	connect(brightnessGang, SIGNAL(enableUndo(bool)), undoButton, SLOT(setEnabled(bool)));
	connect(brightnessGang, SIGNAL(enableRedo(bool)), redoButton, SLOT(setEnabled(bool)));
	chromaticGang  = new Gang(chromaticAdaptSlider, chromaticAdaptdsb,NULL,NULL,NULL,NULL, 0.f, 1.f, 0.f);
	lightGang      = new Gang(lightAdaptSlider, lightAdaptdsb,NULL,NULL,NULL,NULL, 0.f, 1.f, 1.f);

	// pregamma
	pregammaGang =	new Gang(pregammaSlider, pregammadsb,NULL,NULL,NULL,NULL, 0, 3, 1);

	//--
	connect(stackedWidget_operators, SIGNAL(currentChanged (int)), this, SLOT(updateCurrentTmoOperator(int)));
	recentPathLoadSaveTmoSettings=settings.value(KEY_RECENT_PATH_LOAD_SAVE_TMO_SETTINGS,QDir::currentPath()).toString();

}

TonemappingPanel::~TonemappingPanel() {
	delete contrastfactorGang;
	delete saturationfactorGang;
	delete detailfactorGang;
	delete contrastGang;
	delete colorSaturationGang;
	delete contrastEnhancementGang;
	delete luminanceLevelGang;
	delete biasGang;
	delete spatialGang;
	delete rangeGang;
	delete baseGang;
	delete alphaGang;
	delete betaGang;
	delete saturation2Gang;
	delete noiseGang;
	delete multiplierGang;
	delete coneGang;
	delete rodGang;
	delete keyGang;
	delete phiGang;
	delete range2Gang;
	delete lowerGang;
	delete upperGang;
	delete brightnessGang;
	delete chromaticGang;
	delete lightGang;
	delete pregammaGang;

}

void TonemappingPanel::setSizes(int width, int height) {
	sizes.resize(0);
	for(int x = 256; x <= width; x *= 2) {
		if( x >= width )
			break;
		sizes.push_front(x);
		if( 3*(x/2) >= width )
			break;
		sizes.push_front(3*(x/2));
	}
	sizes.push_front(width);
	heightToWidthRatio = ( (float)height )/( (float) width);
	fillCustomSizeComboBox();
	sizeComboBox->setCurrentIndex(sizeComboBox->count() - 1);
}

void TonemappingPanel::on_defaultButton_clicked() {
	switch (currentTmoOperator) {
		case ashikhmin:
			contrastGang->setDefault();
			simpleCheckBox->setChecked(false);
			eq2RadioButton->setChecked(true);
			break;
		case drago:
			biasGang->setDefault();
			break;
		case durand:
			spatialGang->setDefault();
			rangeGang->setDefault();
			baseGang->setDefault();
			break;
		case fattal:
			alphaGang->setDefault();
			betaGang->setDefault();
			saturation2Gang->setDefault();
			noiseGang->setDefault();
			oldFattalCheckBox->setChecked(false);
			break;
		case mantiuk06:
			contrastfactorGang->setDefault();
			saturationfactorGang->setDefault();
			detailfactorGang->setDefault();
			contrastEqualizCheckBox->setChecked(false);
			break;
		case mantiuk08:
			colorSaturationGang->setDefault();
			contrastEnhancementGang->setDefault();
			luminanceLevelGang->setDefault();
			luminanceLevelCheckBox->setChecked(false);
			break;
		case pattanaik:
			multiplierGang->setDefault();
			coneGang->setDefault();
			rodGang->setDefault();
			pattalocal->setChecked(false);
			autoYcheckbox->setChecked(true);
			break;
		case reinhard02:
			keyGang->setDefault();
			phiGang->setDefault();
			range2Gang->setDefault();
			lowerGang->setDefault();
			upperGang->setDefault();
			usescalescheckbox->setChecked(false);
			break;
		case reinhard05:
			brightnessGang->setDefault();
			chromaticGang->setDefault();
			lightGang->setDefault();
			break;
	}
}

void TonemappingPanel::updateCurrentTmoOperator(int idx) {
	currentTmoOperator = TMOperator(idx);
	updateUndoState();
}

void TonemappingPanel::updateUndoState() {
	switch (currentTmoOperator) {
		case ashikhmin:
			contrastGang->updateUndoState();
			break;
		case drago:
			biasGang->updateUndoState();
			break;
		case durand:
			spatialGang->updateUndoState();
			break;
		case fattal:
			alphaGang->updateUndoState();
			break;
		case mantiuk06:
			contrastfactorGang->updateUndoState();
			break;
		case mantiuk08:
			colorSaturationGang->updateUndoState();
			break;
		case pattanaik:
			multiplierGang->updateUndoState();
			break;
		case reinhard02:
			keyGang->updateUndoState();
			break;
		case reinhard05:
			brightnessGang->updateUndoState();
			break;
	}
}

void TonemappingPanel::on_pregammadefault_clicked(){
	pregammaGang->setDefault();
}

void TonemappingPanel::on_applyButton_clicked() {
	bool doTonemapping = true;
	// Warning when using size dependent TMOs with smaller sizes
	if (currentTmoOperator == fattal && (sizeComboBox->currentIndex() != 0 ))
	{
		doTonemapping = QMessageBox::Yes ==
			QMessageBox::question(
				this, tr("Attention"),
				tr("This tonemapping operator depends on the size of the input image. Applying this operator on the full size image will most probably result in a different image.\n\nDo you want to continue?"),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes
			);
	}

	if (!doTonemapping)
		return;

	fillToneMappingOptions();
	setupUndo();

	emit startTonemapping(toneMappingOptions);
}

void TonemappingPanel::fillToneMappingOptions() {
	toneMappingOptions.origxsize = sizes[0];
	toneMappingOptions.xsize = sizes[sizeComboBox->currentIndex()];
	toneMappingOptions.pregamma = pregammaGang->v();
	toneMappingOptions.tonemapSelection = checkBox->isChecked();
	switch (currentTmoOperator) {
		case ashikhmin:
			toneMappingOptions.tmoperator = ashikhmin;
			toneMappingOptions.tmoperator_str = "Ashikhmin";
			toneMappingOptions.operator_options.ashikhminoptions.simple=simpleGang->isCheckBox1Checked();
			toneMappingOptions.operator_options.ashikhminoptions.eq2=eq2Gang->isRadioButton1Checked();
			toneMappingOptions.operator_options.ashikhminoptions.lct=contrastGang->v();
			break;
		case drago:
			toneMappingOptions.tmoperator = drago;
			toneMappingOptions.tmoperator_str = "Drago";
			toneMappingOptions.operator_options.dragooptions.bias=biasGang->v();
			break;
		case durand:
			toneMappingOptions.tmoperator = durand;
			toneMappingOptions.tmoperator_str = "Durand";
			toneMappingOptions.operator_options.durandoptions.spatial=spatialGang->v();
			toneMappingOptions.operator_options.durandoptions.range=rangeGang->v();
			toneMappingOptions.operator_options.durandoptions.base=baseGang->v();
			break;
		case fattal:
			toneMappingOptions.tmoperator = fattal;
			toneMappingOptions.tmoperator_str = "Fattal";
			toneMappingOptions.operator_options.fattaloptions.alpha=alphaGang->v();
			toneMappingOptions.operator_options.fattaloptions.beta=betaGang->v();
			toneMappingOptions.operator_options.fattaloptions.color=saturation2Gang->v();
			toneMappingOptions.operator_options.fattaloptions.noiseredux=noiseGang->v();
			toneMappingOptions.operator_options.fattaloptions.newfattal=!oldFattalGang->isCheckBox1Checked();
			break;
		case mantiuk06:
			toneMappingOptions.tmoperator = mantiuk06;
			toneMappingOptions.tmoperator_str = "Mantiuk '06";
			toneMappingOptions.operator_options.mantiuk06options.contrastfactor=contrastfactorGang->v();
			toneMappingOptions.operator_options.mantiuk06options.saturationfactor=saturationfactorGang->v();
			toneMappingOptions.operator_options.mantiuk06options.detailfactor=detailfactorGang->v();
			toneMappingOptions.operator_options.mantiuk06options.contrastequalization=contrastfactorGang->isCheckBox1Checked();
			break;
		case mantiuk08:
			toneMappingOptions.tmoperator = mantiuk08;
			toneMappingOptions.tmoperator_str = "Mantiuk '08";
			toneMappingOptions.operator_options.mantiuk08options.colorsaturation=colorSaturationGang->v();
			toneMappingOptions.operator_options.mantiuk08options.contrastenhancement=contrastEnhancementGang->v();
			toneMappingOptions.operator_options.mantiuk08options.luminancelevel=luminanceLevelGang->v();
			toneMappingOptions.operator_options.mantiuk08options.setluminance=luminanceLevelGang->isCheckBox1Checked();
			break;
		case pattanaik:
			toneMappingOptions.tmoperator = pattanaik;
			toneMappingOptions.tmoperator_str = "Pattanaik";
			toneMappingOptions.operator_options.pattanaikoptions.autolum=autoYGang->isCheckBox1Checked();
			toneMappingOptions.operator_options.pattanaikoptions.local=pattalocalGang->isCheckBox1Checked();
			toneMappingOptions.operator_options.pattanaikoptions.cone=coneGang->v();
			toneMappingOptions.operator_options.pattanaikoptions.rod=rodGang->v();
			toneMappingOptions.operator_options.pattanaikoptions.multiplier=multiplierGang->v();
			break;
		case reinhard02:
			toneMappingOptions.tmoperator = reinhard02;
			toneMappingOptions.tmoperator_str = "Reinhard '02";
			toneMappingOptions.operator_options.reinhard02options.scales=usescalesGang->isCheckBox1Checked();
			toneMappingOptions.operator_options.reinhard02options.key=keyGang->v();
			toneMappingOptions.operator_options.reinhard02options.phi=phiGang->v();
			toneMappingOptions.operator_options.reinhard02options.range=(int)range2Gang->v();
			toneMappingOptions.operator_options.reinhard02options.lower=(int)lowerGang->v();
			toneMappingOptions.operator_options.reinhard02options.upper=(int)upperGang->v();
			break;
		case reinhard05:
			toneMappingOptions.tmoperator = reinhard05;
			toneMappingOptions.tmoperator_str = "Reinhard '05";
			toneMappingOptions.operator_options.reinhard05options.brightness=brightnessGang->v();
			toneMappingOptions.operator_options.reinhard05options.chromaticAdaptation=chromaticGang->v();
			toneMappingOptions.operator_options.reinhard05options.lightAdaptation=lightGang->v();
			break;
	}
}

void TonemappingPanel::setupUndo() {
	switch (currentTmoOperator) {
		case ashikhmin:
			simpleGang->setupUndo();
			eq2Gang->setupUndo();
			contrastGang->setupUndo();
			break;
		case drago:
			biasGang->setupUndo();
			break;
		case durand:
			spatialGang->setupUndo();
			rangeGang->setupUndo();
			baseGang->setupUndo();
			break;
		case fattal:
			alphaGang->setupUndo();
			betaGang->setupUndo();
			saturation2Gang->setupUndo();
			noiseGang->setupUndo();
			oldFattalGang->setupUndo();
			break;
		case mantiuk06:
			contrastfactorGang->setupUndo();
			saturationfactorGang->setupUndo();
			detailfactorGang->setupUndo();
			break;
		case mantiuk08:
			colorSaturationGang->setupUndo();
			contrastEnhancementGang->setupUndo();
			luminanceLevelGang->setupUndo();
			break;
		case pattanaik:
			autoYGang->setupUndo();
			pattalocalGang->setupUndo();
			coneGang->setupUndo();
			rodGang->setupUndo();
			multiplierGang->setupUndo();
			break;
		case reinhard02:
			usescalesGang->setupUndo();
			keyGang->setupUndo();
			phiGang->setupUndo();
			range2Gang->setupUndo();
			lowerGang->setupUndo();
			upperGang->setupUndo();
			break;
		case reinhard05:
			brightnessGang->setupUndo();
			chromaticGang->setupUndo();
			lightGang->setupUndo();
			break;
	}
}

void TonemappingPanel::on_undoButton_clicked() {
	switch (currentTmoOperator) {
		case ashikhmin:
			simpleGang->undo();
			eq2Gang->undo();
			contrastGang->undo();
			break;
		case drago:
			biasGang->undo();
			break;
		case durand:
			spatialGang->undo();
			rangeGang->undo();
			baseGang->undo();
			break;
		case fattal:
			alphaGang->undo();
			betaGang->undo();
			saturation2Gang->undo();
			noiseGang->undo();
			oldFattalGang->undo();
			break;
		case mantiuk06:
			contrastfactorGang->undo();
			saturationfactorGang->undo();
			detailfactorGang->undo();
			break;
		case mantiuk08:
			colorSaturationGang->undo();
			contrastEnhancementGang->undo();
			luminanceLevelGang->undo();
			break;
		case pattanaik:
			autoYGang->undo();
			pattalocalGang->undo();
			coneGang->undo();
			rodGang->undo();
			multiplierGang->undo();
			break;
		case reinhard02:
			usescalesGang->undo();
			keyGang->undo();
			phiGang->undo();
			range2Gang->undo();
			lowerGang->undo();
			upperGang->undo();
			break;
		case reinhard05:
			brightnessGang->undo();
			chromaticGang->undo();
			lightGang->undo();
			break;
	}
}

void TonemappingPanel::on_redoButton_clicked() {
	switch (currentTmoOperator) {
		case ashikhmin:
			simpleGang->redo();
			eq2Gang->redo();
			contrastGang->redo();
			break;
		case drago:
			biasGang->redo();
			break;
		case durand:
			spatialGang->redo();
			rangeGang->redo();
			baseGang->redo();
			break;
		case fattal:
			alphaGang->redo();
			betaGang->redo();
			saturation2Gang->redo();
			noiseGang->redo();
			oldFattalGang->redo();
			break;
		case mantiuk06:
			contrastfactorGang->redo();
			saturationfactorGang->redo();
			detailfactorGang->redo();
			break;
		case mantiuk08:
			colorSaturationGang->redo();
			contrastEnhancementGang->redo();
			luminanceLevelGang->redo();
			break;
		case pattanaik:
			autoYGang->redo();
			pattalocalGang->redo();
			coneGang->redo();
			rodGang->redo();
			multiplierGang->redo();
			break;
		case reinhard02:
			usescalesGang->redo();
			keyGang->redo();
			phiGang->redo();
			range2Gang->redo();
			lowerGang->redo();
			upperGang->redo();
			break;
		case reinhard05:
			brightnessGang->redo();
			chromaticGang->redo();
			lightGang->redo();
			break;
	}
}

void TonemappingPanel::on_loadsettingsbutton_clicked() {
	QString opened = QFileDialog::getOpenFileName(
			this,
			tr("Load a tonemapping settings text file..."),
			recentPathLoadSaveTmoSettings,
			tr("Qtpfsgui tonemapping settings text file (*.txt)") );
	if( ! opened.isEmpty() ) {
		QFileInfo qfi(opened);
		if (!qfi.isReadable()) {
		QMessageBox::critical(this,tr("Aborting..."),
			tr("File is not readable (check existence, permissions,...)"),
			QMessageBox::Ok,QMessageBox::NoButton);
		return;
		}
		// update internal field variable
		recentPathLoadSaveTmoSettings=qfi.path();
		// if the new dir, the one just chosen by the user, is different from the one stored in the settings, update the settings.
		if (recentPathLoadSaveTmoSettings != settings.value(KEY_RECENT_PATH_LOAD_SAVE_TMO_SETTINGS,QDir::currentPath()).toString()) {
			settings.setValue(KEY_RECENT_PATH_LOAD_SAVE_TMO_SETTINGS, recentPathLoadSaveTmoSettings);
		}
		//update filename internal field, used by parsing function fromTxt2Gui()
		tmoSettingsFilename = opened;
		//call parsing function
		fromTxt2Gui();
	}
}

void TonemappingPanel::on_savesettingsbutton_clicked() {
	QString fname = QFileDialog::getSaveFileName(
			this,
			tr("Save tonemapping settings text file to..."),
			recentPathLoadSaveTmoSettings,
			tr("Qtpfsgui tonemapping settings text file (*.txt)"));
	if( ! fname.isEmpty() ) {
		QFileInfo qfi(fname);
		if (qfi.suffix().toUpper() != "TXT") {
			fname+=".txt";
		}
		// update internal field variable
		recentPathLoadSaveTmoSettings=qfi.path();
		// if the new dir, the one just chosen by the user, is different from the one stored in the settings, update the settings.
		if (recentPathLoadSaveTmoSettings != settings.value(KEY_RECENT_PATH_LOAD_SAVE_TMO_SETTINGS, QDir::currentPath()).toString()) {
			settings.setValue(KEY_RECENT_PATH_LOAD_SAVE_TMO_SETTINGS, recentPathLoadSaveTmoSettings);
		}
		//write txt file
		fromGui2Txt(fname);
	}
}

void TonemappingPanel::fromGui2Txt(QString destination) {
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
	if (current_page == page_mantiuk06) {
		out << "TMO=" << "Mantiuk06" << endl;
		out << "CONTRASTFACTOR=" << contrastfactorGang->v() << endl;
		out << "SATURATIONFACTOR=" << saturationfactorGang->v() << endl;
		out << "DETAILFACTOR=" << detailfactorGang->v() << endl;
		out << "CONTRASTEQUALIZATION=" << (contrastEqualizCheckBox->isChecked() ? "YES" : "NO") << endl;
	}
	if (current_page == page_mantiuk08) {
		out << "TMO=" << "Mantiuk08" << endl;
		out << "COLORSATURATION=" << colorSaturationGang->v() << endl;
		out << "CONTRASTENHANCEMENT=" << contrastEnhancementGang->v() << endl;
		out << "LUMINANCELEVEL=" << luminanceLevelGang->v() << endl;
		out << "SETLUMINANCE=" << (luminanceLevelCheckBox->isChecked() ? "YES" : "NO") << endl;
	}
	else if (current_page == page_fattal) {
		out << "TMO=" << "Fattal02" << endl;
		out << "ALPHA=" << alphaGang->v() << endl;
		out << "BETA=" << betaGang->v() << endl;
		out << "COLOR=" << saturation2Gang->v() << endl;
		out << "NOISE=" << noiseGang->v() << endl;
		out << "OLDFATTAL=" << (oldFattalCheckBox->isChecked() ? "YES" : "NO") << endl;
	}
	else if (current_page == page_ashikhmin) {
		out << "TMO=" << "Ashikhmin02" << endl;
		out << "SIMPLE=" << (simpleCheckBox->isChecked() ? "YES" : "NO") << endl;
		out << "EQUATION=" << (eq2RadioButton->isChecked() ? "2" : "4") << endl;
		out << "CONTRAST=" << contrastGang->v() << endl;
	}
	else if (current_page == page_durand) {
		out << "TMO=" << "Durand02" << endl;
		out << "SPATIAL=" << spatialGang->v() << endl;
		out << "RANGE=" << rangeGang->v() << endl;
		out << "BASE=" << baseGang->v() << endl;
	}
	else if (current_page == page_drago) {
		out << "TMO=" << "Drago03" << endl;
		out << "BIAS=" << biasGang->v() << endl;
	}
	else if (current_page == page_pattanaik) {
		out << "TMO=" << "Pattanaik00" << endl;
		out << "MULTIPLIER=" << multiplierGang->v() << endl;
		out << "LOCAL=" << (pattalocal->isChecked() ? "YES" : "NO") << endl;
		out << "AUTOLUMINANCE=" << (autoYcheckbox->isChecked() ? "YES" : "NO") << endl;
		out << "CONE=" << coneGang->v() << endl;
		out << "ROD=" << rodGang->v() << endl;
	}
	else if (current_page == page_reinhard02) {
		out << "TMO=" << "Reinhard02" << endl;
		out << "KEY=" << keyGang->v() << endl;
		out << "PHI=" << phiGang->v() << endl;
		out << "SCALES=" << (usescalescheckbox->isChecked() ? "YES" : "NO") << endl;
		out << "RANGE=" << range2Gang->v() << endl;
		out << "LOWER=" << lowerGang->v() << endl;
		out << "UPPER=" << upperGang->v() << endl;
	}
	else if (current_page == page_reinhard05) {
		out << "TMO=" << "Reinhard05" << endl;
		out << "BRIGHTNESS=" << brightnessGang->v() << endl;
		out << "CHROMATICADAPTATION=" << chromaticGang->v() << endl;
		out << "LIGHTADAPTATION=" << lightGang->v() << endl;
	}
	out << "PREGAMMA=" << pregammaGang->v() << endl;
	file.close();
}

void TonemappingPanel::fromTxt2Gui() {
	QFile file(tmoSettingsFilename);
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
		if (field == "TMOSETTINGSVERSION") {
			if (value != TMOSETTINGSVERSION) {
				QMessageBox::critical(this,tr("Aborting..."),tr("Error, the tone mapping settings file format has changed. This (old) file cannot be used with this version of Qtpfsgui. Create a new one."),
				QMessageBox::Ok,QMessageBox::NoButton);
				return;
			}
		} else if (field == "TMO") {
			if (value == "Ashikhmin02") {
				stackedWidget_operators->setCurrentWidget(page_ashikhmin);
			} else if (value == "Mantiuk06") {
				stackedWidget_operators->setCurrentWidget(page_mantiuk06);
			} else if (value == "Mantiuk08") {
				stackedWidget_operators->setCurrentWidget(page_mantiuk08);
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
		} else if (field == "CONTRASTFACTOR") {
			contrastFactorSlider->setValue(contrastfactorGang->v2p(value.toFloat()));
		} else if (field == "SATURATIONFACTOR") {
			saturationFactorSlider->setValue(saturationfactorGang->v2p(value.toFloat()));
		} else if (field == "DETAILFACTOR") {
			detailFactorSlider->setValue(detailfactorGang->v2p(value.toFloat()));
		} else if (field == "CONTRASTEQUALIZATION") {
			contrastEqualizCheckBox->setChecked((value == "YES"));
		} else if (field == "COLORSATURATION") {
			contrastFactorSlider->setValue(colorSaturationGang->v2p(value.toFloat()));
		} else if (field == "CONTRASTENHANCEMENT") {
			saturationFactorSlider->setValue(contrastEnhancementGang->v2p(value.toFloat()));
		} else if (field == "LUMINANCELEVEL") {
			detailFactorSlider->setValue(luminanceLevelGang->v2p(value.toFloat()));
		} else if (field == "SIMPLE") {
			simpleCheckBox->setChecked((value == "YES"));
		} else if (field == "EQUATION") {
			eq2RadioButton->setChecked((value=="2"));
			eq4RadioButton->setChecked((value=="4"));
		} else if (field == "CONTRAST") {
			contrastSlider->setValue(contrastGang->v2p(value.toFloat()));
		} else if (field == "BIAS") {
			biasSlider->setValue(biasGang->v2p(value.toFloat()));
		} else if (field == "SPATIAL") {
			spatialSlider->setValue(spatialGang->v2p(value.toFloat()));
		} else if (field == "RANGE") {
			rangeSlider->setValue(rangeGang->v2p(value.toFloat()));
		} else if (field == "BASE") {
			baseSlider->setValue(baseGang->v2p(value.toFloat()));
		} else if (field == "ALPHA") {
			alphaSlider->setValue(alphaGang->v2p(value.toFloat()));
		} else if (field == "BETA") {
			betaSlider->setValue(betaGang->v2p(value.toFloat()));
		} else if (field == "COLOR") {
			saturation2Slider->setValue(saturation2Gang->v2p(value.toFloat()));
		} else if (field == "NOISE") {
			noiseSlider->setValue(noiseGang->v2p(value.toFloat()));
		} else if (field == "OLDFATTAL") {
			 oldFattalCheckBox->setChecked(value == "YES");
		} else if (field == "MULTIPLIER") {
			multiplierSlider->setValue(multiplierGang->v2p(value.toFloat()));
		} else if (field == "LOCAL") {
			(value == "YES") ? pattalocal->setChecked(value == "YES") : pattalocal->setChecked(value=="NO");
		} else if (field == "AUTOLUMINANCE") {
			(value == "YES") ? autoYcheckbox->setChecked(value == "YES") : autoYcheckbox->setChecked(value=="NO");
		} else if (field == "CONE") {
			coneSlider->setValue(coneGang->v2p(value.toFloat()));
		} else if (field == "ROD") {
			rodSlider->setValue(rodGang->v2p(value.toFloat()));
		} else if (field == "KEY") {
			keySlider->setValue(keyGang->v2p(value.toFloat()));
		} else if (field == "PHI") {
			phiSlider->setValue(phiGang->v2p(value.toFloat()));
		} else if (field == "SCALES") {
			(value == "YES") ? usescalescheckbox->setChecked(value == "YES") : usescalescheckbox->setChecked(value=="NO");
		} else if (field == "RANGE") {
			range2Slider->setValue(range2Gang->v2p(value.toFloat()));
		} else if (field == "LOWER") {
			lowerSlider->setValue(lowerGang->v2p(value.toFloat()));
		} else if (field == "UPPER") {
			upperSlider->setValue(upperGang->v2p(value.toFloat()));
		} else if (field == "BRIGHTNESS") {
			brightnessSlider->setValue(brightnessGang->v2p(value.toFloat()));
		} else if (field == "CHROMATICADAPTATION") {
			chromaticAdaptSlider->setValue(chromaticGang->v2p(value.toFloat()));
		} else if (field == "LIGHTADAPTATION") {
			lightAdaptSlider->setValue(lightGang->v2p(value.toFloat()));
		} else if (field == "PREGAMMA") {
			pregammaSlider->setValue(pregammaGang->v2p(value.toFloat()));
		}
	}
}

void TonemappingPanel::on_addCustomSizeButton_clicked(){
	bool ok;
	int i = QInputDialog::getInteger(this, tr("Custom LDR size"),
	                                      tr("Enter the width of the new size:"), 0 , 0, 2147483647, 1, &ok);
	if (ok && i > 0) {
		sizes.push_front(i);
		fillCustomSizeComboBox();
	}
}

void TonemappingPanel::fillCustomSizeComboBox() {
	sizeComboBox->clear();
	for(int i = 0; i < sizes.size(); i++)
		sizeComboBox->addItem( QString("%1x%2").arg(sizes[i]).arg( (int)(heightToWidthRatio*sizes[i]) ));
}

void TonemappingPanel::setLogoText(const char *txt) {
	labelWorking->setText(txt);
}

void TonemappingPanel::setLogoPixmap(const QString &framename) {
	//labelWorking->setPixmap(QPixmap(framename));
	labelLogo->setPixmap(QPixmap(framename));
}

