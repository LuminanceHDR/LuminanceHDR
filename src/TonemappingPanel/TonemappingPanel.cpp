/*
 * This file is a part of Luminance HDR package.
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
 *
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
#include <QDebug>
#include <QSqlTableModel>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QSqlError>

#include "Common/LuminanceOptions.h"
#include "TonemappingPanel.h"
#include "TMOProgressIndicator.h"
#include "TonemappingWarnDialog.h"
#include "SavedParametersDialog.h"
#include "SavingParametersDialog.h"



TonemappingPanel::TonemappingPanel(QWidget *parent)
    : QWidget(parent), adding_custom_size(false)
{
    setupUi(this);

	// This is hided at moment 
	//checkBoxOriginal->hide();	

    currentTmoOperator = mantiuk06; // from Qt Designer

    // mantiuk06
    contrastfactorGang = new Gang(contrastFactorSlider,contrastFactordsb,contrastEqualizCheckBox,NULL,NULL, NULL, 0.001f, 1.0f, 0.1f);

    connect(contrastfactorGang, SIGNAL(enableUndo(bool)), undoButton, SLOT(setEnabled(bool)));
    connect(contrastfactorGang, SIGNAL(enableRedo(bool)), redoButton, SLOT(setEnabled(bool)));

    saturationfactorGang = new Gang(saturationFactorSlider, saturationFactordsb, NULL, NULL, NULL, NULL, 0.0f, 2.0f, 0.8f);
    detailfactorGang = new Gang(detailFactorSlider, detailFactordsb,NULL,NULL,NULL,NULL, 1.0f, 99.0f, 1.0f);

    // mantiuk08
    colorSaturationGang = new Gang(colorSaturationSlider,colorSaturationDSB, NULL, NULL, NULL, NULL, 0.f, 2.f, 1.f);

    connect(colorSaturationGang, SIGNAL(enableUndo(bool)), undoButton, SLOT(setEnabled(bool)));
    connect(colorSaturationGang, SIGNAL(enableRedo(bool)), redoButton, SLOT(setEnabled(bool)));

    contrastEnhancementGang = new Gang(contrastEnhancementSlider, contrastEnhancementDSB, NULL, NULL, NULL, NULL, .01f, 10.f, 1.f);
    luminanceLevelGang = new Gang(luminanceLevelSlider, luminanceLevelDSB, luminanceLevelCheckBox, NULL, NULL, NULL, 1.f, 100.0f, 1.f);

    // fattal02
    alphaGang = new Gang(alphaSlider, alphadsb, NULL,NULL,NULL,NULL, 1e-4, 2.f, 1.f, true);

    connect(alphaGang, SIGNAL(enableUndo(bool)), undoButton, SLOT(setEnabled(bool)));
    connect(alphaGang, SIGNAL(enableRedo(bool)), redoButton, SLOT(setEnabled(bool)));

    betaGang = new Gang(betaSlider, betadsb, NULL,NULL,NULL,NULL, 0.1f, 2.f, 0.9f);
    saturation2Gang = new Gang(saturation2Slider, saturation2dsb, NULL,NULL,NULL,NULL, 0.f, 1.f, .8f);
    noiseGang = new Gang(noiseSlider, noisedsb, NULL,NULL,NULL,NULL, 0, 1.f, 0.f);
    oldFattalGang = new Gang(NULL,NULL, oldFattalCheckBox);

    // ashikhmin02
    contrastGang = new Gang(contrastSlider, contrastdsb,NULL,NULL,NULL,NULL, 0.f, 1.f, 0.5f);

    connect(contrastGang, SIGNAL(enableUndo(bool)), undoButton, SLOT(setEnabled(bool)));
    connect(contrastGang, SIGNAL(enableRedo(bool)), redoButton, SLOT(setEnabled(bool)));

    simpleGang = new Gang(NULL, NULL, simpleCheckBox);
    eq2Gang = new Gang(NULL, NULL,NULL, NULL, eq2RadioButton, eq4RadioButton);

    // drago03
    biasGang = new Gang(biasSlider, biasdsb,NULL,NULL,NULL,NULL, 0.f, 1.f, 0.85f);

    connect(biasGang, SIGNAL(enableUndo(bool)), undoButton, SLOT(setEnabled(bool)));
    connect(biasGang, SIGNAL(enableRedo(bool)), redoButton, SLOT(setEnabled(bool)));

    // durand02
    spatialGang = new Gang(spatialSlider, spatialdsb, NULL, NULL, NULL, NULL, 0.f, 100.f, 2.f);

    connect(spatialGang, SIGNAL(enableUndo(bool)), undoButton, SLOT(setEnabled(bool)));
    connect(spatialGang, SIGNAL(enableRedo(bool)), redoButton, SLOT(setEnabled(bool)));

    rangeGang = new Gang(rangeSlider, rangedsb,NULL,NULL,NULL,NULL, 0.01f, 10.f, 0.4f);
    baseGang = new Gang(baseSlider, basedsb,NULL,NULL,NULL,NULL, 0.f, 10.f, 5.0f);

    // pattanaik00
    multiplierGang = new Gang(multiplierSlider, multiplierdsb,NULL,NULL,NULL,NULL, 1e-3,1000.f, 1.f, true);

    connect(multiplierGang, SIGNAL(enableUndo(bool)), undoButton, SLOT(setEnabled(bool)));
    connect(multiplierGang, SIGNAL(enableRedo(bool)), redoButton, SLOT(setEnabled(bool)));

    coneGang = new Gang(coneSlider, conedsb,NULL,NULL,NULL,NULL, 0.f, 1.f, 0.5f);
    rodGang = new Gang(rodSlider, roddsb,NULL,NULL,NULL,NULL, 0.f, 1.f, 0.5f);
    autoYGang = new Gang(NULL,NULL, autoYcheckbox);
    pattalocalGang = new Gang(NULL,NULL, pattalocal);

    // reinhard02
    keyGang = new Gang(keySlider, keydsb,NULL,NULL,NULL,NULL, 0.f, 1.f, 0.18f);

    connect(keyGang, SIGNAL(enableUndo(bool)), undoButton, SLOT(setEnabled(bool)));
    connect(keyGang, SIGNAL(enableRedo(bool)), redoButton, SLOT(setEnabled(bool)));

    phiGang = new Gang(phiSlider, phidsb,NULL,NULL,NULL,NULL, 0.f, 100.f, 1.f);
    range2Gang = new Gang(range2Slider, range2dsb,NULL,NULL,NULL,NULL, 1.f, 32.f, 8.f);
    lowerGang = new Gang(lowerSlider, lowerdsb,NULL,NULL,NULL,NULL, 1.f, 100.f, 1.f);
    upperGang = new Gang(upperSlider, upperdsb,NULL,NULL,NULL,NULL, 1.f, 100.f, 43.f);
    usescalesGang = new Gang(NULL,NULL, usescalescheckbox);

    // reinhard05
    brightnessGang = new Gang(brightnessSlider, brightnessdsb,NULL,NULL,NULL,NULL, -20.f, 20.f, 0.f);

    connect(brightnessGang, SIGNAL(enableUndo(bool)), undoButton, SLOT(setEnabled(bool)));
    connect(brightnessGang, SIGNAL(enableRedo(bool)), redoButton, SLOT(setEnabled(bool)));

    chromaticGang = new Gang(chromaticAdaptSlider, chromaticAdaptdsb,NULL,NULL,NULL,NULL, 0.f, 1.f, 0.f);
    lightGang = new Gang(lightAdaptSlider, lightAdaptdsb,NULL,NULL,NULL,NULL, 0.f, 1.f, 1.f);

    // pregamma
    pregammaGang = new Gang(pregammaSlider, pregammadsb,NULL,NULL,NULL,NULL, 0, 3, 1);

    //--
    connect(stackedWidget_operators, SIGNAL(currentChanged(int)), this, SLOT(updateCurrentTmoOperator(int)));

    recentPathLoadSaveTmoSettings = LuminanceOptions().getDefaultPathTmoSettings();

    connect(loadButton, SIGNAL(clicked()), this, SLOT(loadParameters()));
    connect(saveButton, SIGNAL(clicked()), this, SLOT(saveParameters()));
    connect(loadCommentsButton, SIGNAL(clicked()), this, SLOT(loadComments()));
	
	createDatabase();
}

TonemappingPanel::~TonemappingPanel()
{
    qDebug() << "TonemappingPanel::~TonemappingPanel()";

    delete contrastfactorGang;
    delete saturationfactorGang;
    delete detailfactorGang;
    delete contrastGang;
    delete colorSaturationGang;
    delete contrastEnhancementGang;
    delete luminanceLevelGang;
    delete simpleGang;
    delete eq2Gang;
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
    delete autoYGang;
    delete pattalocalGang;
    delete keyGang;
    delete phiGang;
    delete range2Gang;
    delete lowerGang;
    delete upperGang;
    delete usescalesGang;
    delete brightnessGang;
    delete chromaticGang;
    delete lightGang;
    delete pregammaGang;

	QSqlDatabase db = QSqlDatabase::database();
	db.close();
}

void TonemappingPanel::createDatabase()
{
    QDir dir(QDir::homePath());
	
	QString filename = dir.absolutePath();
#ifdef WIN32
	filename += "/LuminanceHDR";
#else
	filename += "/.LuminanceHDR";
#endif
	
	filename += "/saved_parameters.db";

	qDebug() << filename;

	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
	db.setDatabaseName(filename);
	db.setHostName("localhost");
	bool ok = db.open();
	if (!ok)
	{
		QMessageBox::warning(this,tr("TM Database Problem"),
                                  tr("The database used for saving TM parameters cannot be opened.\n"
									"Error: %1").arg(db.lastError().databaseText()),
                                  QMessageBox::Ok,QMessageBox::NoButton);
		return;
	}
	qDebug() << "Database opened";

	QSqlQuery query;
	// Mantiuk 06
	bool res = query.exec(" CREATE TABLE IF NOT EXISTS mantiuk06 (contrastEqualization boolean NOT NULL, contrastFactor real, saturationFactor real, detailFactor real, pregamma real, comment varchar(150));");
	if (res == false)
		qDebug() << query.lastError();
	// Mantiuk 08
	res = query.exec(" CREATE TABLE IF NOT EXISTS mantiuk08 (colorSaturation real, contrastEnhancement real, luminanceLevel real, manualLuminanceLevel boolean NOT NULL, pregamma real, comment varchar(150));");
	if (res == false)
		qDebug() << query.lastError();
	// Ashikhmin
	res = query.exec(" CREATE TABLE IF NOT EXISTS ashikhmin (simple boolean NOT NULL, eq2 boolean NOT NULL, lct real, pregamma real, comment varchar(150));");
	if (res == false)
		qDebug() << query.lastError();
	// Drago
	res = query.exec(" CREATE TABLE IF NOT EXISTS drago (bias real, pregamma real, comment varchar(150));");
	if (res == false)
		qDebug() << query.lastError();
	// Durand
	res = query.exec(" CREATE TABLE IF NOT EXISTS durand (spatial real, range real, base real, pregamma real, comment varchar(150));");
	if (res == false)
		qDebug() << query.lastError();
	// Fattal
	res = query.exec(" CREATE TABLE IF NOT EXISTS fattal (alpha real, beta real, colorSaturation real, noiseReduction real, oldFattal boolean NOT NULL, pregamma real, comment varchar(150));");
	if (res == false)
		qDebug() << query.lastError();
	// Pattanaik
	res = query.exec(" CREATE TABLE IF NOT EXISTS pattanaik (autolum boolean NOT NULL, local boolean NOT NULL, cone real, rod real, multiplier real, pregamma real, comment varchar(150));");
	if (res == false)
		qDebug() << query.lastError();
	// Reinhard02
	res = query.exec(" CREATE TABLE IF NOT EXISTS reinhard02 (scales boolean NOT NULL, key real, phi real, range int, lower int, upper int, pregamma real, comment varchar(150));");
	if (res == false)
		qDebug() << query.lastError();
	// Reinhard05
	res = query.exec(" CREATE TABLE IF NOT EXISTS reinhard05 (brightness real, chromaticAdaptation real, lightAdaptation real, pregamma real, comment varchar(150));");
	if (res == false)
		qDebug() << query.lastError();
}

void TonemappingPanel::setSizes(int width, int height)
{
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

void TonemappingPanel::on_defaultButton_clicked()
{
    switch (currentTmoOperator)
    {
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

// TODO : if you change the position of the operator inside the TMOperator enum
// you will screw up this function!!!
void TonemappingPanel::updateCurrentTmoOperator(int idx)
{
    currentTmoOperator = TMOperator(idx);
    updateUndoState();
}

void TonemappingPanel::updateUndoState()
{
    switch (currentTmoOperator)
    {
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

void TonemappingPanel::on_pregammadefault_clicked()
{
    pregammaGang->setDefault();
}

// TODO : check this function
void TonemappingPanel::on_applyButton_clicked()
{
    // TODO: fix it!!!
    /*
    if (luminance_options->tmowarning_fattalsmall)
    {
        bool doTonemapping = true;

        // Warning when using size dependent TMOs with smaller sizes
        if (currentTmoOperator == fattal && (sizeComboBox->currentIndex() != 0 ))
        {
            TonemappingWarningDialog *warn = new TonemappingWarningDialog(this);
            warn->exec();
            doTonemapping = warn->wasAccepted();
            delete warn;
        }
        if (doTonemapping == false)
            return;
    }
    */

    fillToneMappingOptions();
    setupUndo();

    emit startTonemapping(&toneMappingOptions);
}

void TonemappingPanel::fillToneMappingOptions()
{
    toneMappingOptions.origxsize = sizes[0];
    toneMappingOptions.xsize = sizes[sizeComboBox->currentIndex()];
    toneMappingOptions.pregamma = pregammaGang->v();
    //toneMappingOptions.tonemapSelection = checkBoxSelection->isChecked();
    //toneMappingOptions.tonemapOriginal = checkBoxOriginal->isChecked();
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

void TonemappingPanel::setupUndo()
{
    switch (currentTmoOperator)
    {
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

void TonemappingPanel::on_undoButton_clicked()
{
    switch (currentTmoOperator)
    {
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

void TonemappingPanel::on_redoButton_clicked()
{
    switch (currentTmoOperator)
    {
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

void TonemappingPanel::on_loadsettingsbutton_clicked()
{
    LuminanceOptions lum_options;

    QString opened = QFileDialog::getOpenFileName(this,
                                                  tr("Load a tonemapping settings text file..."),
                                                  recentPathLoadSaveTmoSettings,
                                                  tr("LuminanceHDR tonemapping settings text file (*.txt)") );
    if ( !opened.isEmpty() )
    {
        QFileInfo qfi(opened);
        if (!qfi.isReadable())
        {
            QMessageBox::critical(this,tr("Aborting..."),
                                  tr("File is not readable (check existence, permissions,...)"),
                                  QMessageBox::Ok,QMessageBox::NoButton);
            return;
        }
        // update internal field variable
        recentPathLoadSaveTmoSettings = qfi.path();
        // if the new dir, the one just chosen by the user, is different from the one stored in the settings, update the settings->
        if (recentPathLoadSaveTmoSettings != lum_options.getDefaultPathTmoSettings())
        {
            lum_options.setDefaultPathTmoSettings( recentPathLoadSaveTmoSettings );
        }
        //update filename internal field, used by parsing function fromTxt2Gui()
        tmoSettingsFilename = opened;
        //call parsing function
        fromTxt2Gui();
    }
}

void TonemappingPanel::on_savesettingsbutton_clicked()
{
    LuminanceOptions lum_options;

    QString fname = QFileDialog::getSaveFileName(this,
                                                 tr("Save tonemapping settings text file to..."),
                                                 recentPathLoadSaveTmoSettings,
                                                 tr("LuminanceHDR tonemapping settings text file (*.txt)"));
    if( ! fname.isEmpty() )
    {
        QFileInfo qfi(fname);
        if (qfi.suffix().toUpper() != "TXT")
        {
            fname+=".txt";
        }
        // update internal field variable
        recentPathLoadSaveTmoSettings = qfi.path();
        // if the new dir, the one just chosen by the user, is different from the one stored in the settings, update the settings->
        if (recentPathLoadSaveTmoSettings != lum_options.getDefaultPathTmoSettings() )
        {
            lum_options.setDefaultPathTmoSettings(recentPathLoadSaveTmoSettings);
        }
        //write txt file
        fromGui2Txt(fname);
    }
}

void TonemappingPanel::fromGui2Txt(QString destination)
{
    QFile file(destination);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::critical(this,tr("Aborting..."),tr("File is not writable (check permissions, path...)"),
                              QMessageBox::Ok,QMessageBox::NoButton);
        return;
    }
    QTextStream out(&file);
    out << "# LuminanceHDR Tonemapping Setting file." << endl;
    out << "# Editing this file by hand is risky, worst case scenario is Luminance crashing." << endl;
    out << "# Please edit this file by hand only if you know what you're doing, in any case never change the left hand side text (i.e. the part before the ``='')." << endl;
    out << "TMOSETTINGSVERSION=" << TMOSETTINGSVERSION << endl;
	out << "XSIZE=" << sizes[sizeComboBox->currentIndex()] << endl;
	out << "QUALITY=" << qualitySB->value() << endl;

    QWidget *current_page = stackedWidget_operators->currentWidget();
    if (current_page == page_mantiuk06)
    {
        out << "TMO=" << "Mantiuk06" << endl;
        out << "CONTRASTFACTOR=" << contrastfactorGang->v() << endl;
        out << "SATURATIONFACTOR=" << saturationfactorGang->v() << endl;
        out << "DETAILFACTOR=" << detailfactorGang->v() << endl;
        out << "CONTRASTEQUALIZATION=" << (contrastEqualizCheckBox->isChecked() ? "YES" : "NO") << endl;
    }
    else if (current_page == page_mantiuk08)
    {
        out << "TMO=" << "Mantiuk08" << endl;
        out << "COLORSATURATION=" << colorSaturationGang->v() << endl;
        out << "CONTRASTENHANCEMENT=" << contrastEnhancementGang->v() << endl;
        out << "LUMINANCELEVEL=" << luminanceLevelGang->v() << endl;
        out << "SETLUMINANCE=" << (luminanceLevelCheckBox->isChecked() ? "YES" : "NO") << endl;
    }
    else if (current_page == page_fattal)
    {
        out << "TMO=" << "Fattal02" << endl;
        out << "ALPHA=" << alphaGang->v() << endl;
        out << "BETA=" << betaGang->v() << endl;
        out << "COLOR=" << saturation2Gang->v() << endl;
        out << "NOISE=" << noiseGang->v() << endl;
        out << "OLDFATTAL=" << (oldFattalCheckBox->isChecked() ? "YES" : "NO") << endl;
    }
    else if (current_page == page_ashikhmin)
    {
        out << "TMO=" << "Ashikhmin02" << endl;
        out << "SIMPLE=" << (simpleCheckBox->isChecked() ? "YES" : "NO") << endl;
        out << "EQUATION=" << (eq2RadioButton->isChecked() ? "2" : "4") << endl;
        out << "CONTRAST=" << contrastGang->v() << endl;
    }
    else if (current_page == page_durand)
    {
        out << "TMO=" << "Durand02" << endl;
        out << "SPATIAL=" << spatialGang->v() << endl;
        out << "RANGE=" << rangeGang->v() << endl;
        out << "BASE=" << baseGang->v() << endl;
    }
    else if (current_page == page_drago)
    {
        out << "TMO=" << "Drago03" << endl;
        out << "BIAS=" << biasGang->v() << endl;
    }
    else if (current_page == page_pattanaik)
    {
        out << "TMO=" << "Pattanaik00" << endl;
        out << "MULTIPLIER=" << multiplierGang->v() << endl;
        out << "LOCAL=" << (pattalocal->isChecked() ? "YES" : "NO") << endl;
        out << "AUTOLUMINANCE=" << (autoYcheckbox->isChecked() ? "YES" : "NO") << endl;
        out << "CONE=" << coneGang->v() << endl;
        out << "ROD=" << rodGang->v() << endl;
    }
    else if (current_page == page_reinhard02)
    {
        out << "TMO=" << "Reinhard02" << endl;
        out << "KEY=" << keyGang->v() << endl;
        out << "PHI=" << phiGang->v() << endl;
        out << "SCALES=" << (usescalescheckbox->isChecked() ? "YES" : "NO") << endl;
        out << "RANGE=" << range2Gang->v() << endl;
        out << "LOWER=" << lowerGang->v() << endl;
        out << "UPPER=" << upperGang->v() << endl;
    }
    else if (current_page == page_reinhard05)
    {
        out << "TMO=" << "Reinhard05" << endl;
        out << "BRIGHTNESS=" << brightnessGang->v() << endl;
        out << "CHROMATICADAPTATION=" << chromaticGang->v() << endl;
        out << "LIGHTADAPTATION=" << lightGang->v() << endl;
    }
    out << "PREGAMMA=" << pregammaGang->v() << endl;
    file.close();
}

void TonemappingPanel::fromTxt2Gui()
{
    QFile file(tmoSettingsFilename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text) || file.size()==0)
    {
        QMessageBox::critical(this,tr("Aborting..."),tr("File is not readable (check permissions, path...)"),
                              QMessageBox::Ok,QMessageBox::NoButton);
        return;
    }
    QTextStream in(&file);
    QString field,value;

    while (!in.atEnd())
    {
        QString line = in.readLine();
        //skip comments
        if (line.startsWith('#'))
            continue;

        field=line.section('=',0,0); //get the field
        value=line.section('=',1,1); //get the value
        if (field == "TMOSETTINGSVERSION")
        {
            if (value != TMOSETTINGSVERSION)
            {
                QMessageBox::critical(this,tr("Aborting..."),tr("Error, the tone mapping settings file format has changed. This (old) file cannot be used with this version of LuminanceHDR. Create a new one."),
                                      QMessageBox::Ok,QMessageBox::NoButton);
                return;
            }
        }
		else if (field == "XSIZE")
		{
			int idx;
			for (idx = 0; idx < sizeComboBox->count(); idx++)
			{
					if (sizes[idx] == value.toInt())
						break;
			}
			if (idx == sizeComboBox->count()) // Custom XSIZE
			{
				sizes.push_back(value.toInt());
				fillCustomSizeComboBox();
				sizeComboBox->setCurrentIndex(sizeComboBox->count() - 1);
			}	
			else
				sizeComboBox->setCurrentIndex(idx);
		}
		else if (field == "QUALITY")
		{
			qualityHS->setValue(value.toInt());
			qualitySB->setValue(value.toInt());
		}
        else if (field == "TMO")
        {
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

void TonemappingPanel::on_addCustomSizeButton_clicked()
{
    bool ok;
    int i = QInputDialog::getInteger(this,
                                     tr("Custom LDR size"),
                                     tr("Enter the width of the new size:"), 0 , 0, 2147483647, 1, &ok);
    if (ok && i > 0)
    {
        sizes.push_back(i);
        fillCustomSizeComboBox();
        sizeComboBox->setCurrentIndex(sizeComboBox->count() - 1);
    }
}

void TonemappingPanel::fillCustomSizeComboBox()
{
    sizeComboBox->clear();
    for (int i = 0; i < sizes.size(); i++)
    {
        sizeComboBox->addItem( QString("%1x%2").arg(sizes[i]).arg( (int)(heightToWidthRatio*sizes[i]) ));
    }
}

void TonemappingPanel::setEnabled(bool b)
{
    // Operator select
    cmbOperators->setEnabled(b);
    stackedWidget_operators->setEnabled(b);

    // Load/Store/Reset
    loadsettingsbutton->setEnabled(b);
    savesettingsbutton->setEnabled(b);
    defaultButton->setEnabled(b);
    if (b) 
    {
    	updateUndoState();
    }
    else
    {
        undoButton->setEnabled(false);
        redoButton->setEnabled(false);
    }
    // Size
    sizeComboBox->setEnabled(b);
    addCustomSizeButton->setEnabled(b);

    // Gamma
    pregammadefault->setEnabled(b);
    pregammaSlider->setEnabled(b);
    pregammadsb->setEnabled(b);

    // Tonemap
    applyButton->setEnabled(b);

	// DB
	loadCommentsButton->setEnabled(b);
	loadButton->setEnabled(b);
	saveButton->setEnabled(b);
	
	qualityHS->setEnabled(b);
	qualitySB->setEnabled(b);

	replaceLdrCheckBox->setEnabled(b);

	// Labels
	lblOperators->setEnabled(b);
	groupSaveLoadTMOsetting->setEnabled(b);
	label->setEnabled(b);
	pregammaLabel->setEnabled(b);
	pregammaGroup->setEnabled(b);
}

void TonemappingPanel::updatedHDR(pfs::Frame* f)
{
    setSizes(f->getWidth(), f->getHeight());
}

/*
 * This function should set the entire status.
 * Currently I'm only interested in changing the TM operator
 */
void TonemappingPanel::updateTonemappingParams(TonemappingOptions *opts)
{
    qDebug() << "TonemappingPanel::updateTonemappingParams(TonemappingOptions *opts)";
//    currentTmoOperator = opts->tmoperator;
//    updateUndoState();
    cmbOperators->setCurrentIndex(opts->tmoperator);
}

void TonemappingPanel::saveParameters()
{
	SavingParameters dialog;
	if (dialog.exec())
	{
		// Ashikhmin
		float lct;
		bool simple, 
				eq2; 
		// Drago
		float bias;
		// Durand
		float spatial,
				range,
				base;
		// Fattal
		float alpha,
				beta,
				colorSat,
				noiseReduction;
		bool oldFattal;
		// Mantiuk 06
		float contrastFactor,
				saturationFactor,
				detailFactor;
		bool contrastEqualization;
		// Mantiuk 08
		float colorSaturation,
				contrastEnhancement,
				luminanceLevel;
		bool manualLuminanceLevel;
		// Pattanaik
		float multiplier,
				rod,
				cone;
		bool autolum,
				local;
		// Reinhard 02
		bool scales;
		float key,
				phi;
		int irange,
			lower,
			upper;
		// Reinhard 05
		float brightness,
				chromaticAdaptation,
				lightAdaptation;

		QString comment = dialog.getComment();

    	switch (currentTmoOperator) {
    		case ashikhmin:
        		simple = simpleGang->isCheckBox1Checked();
		        eq2 = eq2Gang->isRadioButton1Checked();
        		lct = contrastGang->v();
				execAshikhminQuery(simple, eq2, lct, comment);
	        break;
    		case drago:
		        bias = biasGang->v();
				execDragoQuery(bias, comment);
	        break;
    		case durand:
    		    spatial = spatialGang->v();
        		range = rangeGang->v();
	        	base = baseGang->v();
				execDurandQuery(spatial, range, base, comment);
    	    break;
		    case fattal:
        		alpha = alphaGang->v();
		        beta = betaGang->v();
        		colorSat = saturation2Gang->v();
		        noiseReduction = noiseGang->v();
        		oldFattal = oldFattalGang->isCheckBox1Checked();
				execFattalQuery(alpha, beta, colorSat, noiseReduction, oldFattal, comment);
	        break;
    		case mantiuk06:
				contrastFactor = contrastfactorGang->v();
				saturationFactor = saturationfactorGang->v();
				detailFactor = detailfactorGang->v();
				contrastEqualization = contrastfactorGang->isCheckBox1Checked();
				execMantiuk06Query(contrastEqualization, contrastFactor, saturationFactor, detailFactor, comment);
	        break;
    		case mantiuk08:
		        colorSaturation = colorSaturationGang->v();
        		contrastEnhancement = contrastEnhancementGang->v();
		        luminanceLevel = luminanceLevelGang->v();
        		manualLuminanceLevel = luminanceLevelGang->isCheckBox1Checked();
				execMantiuk08Query(colorSaturation, contrastEnhancement, luminanceLevel, manualLuminanceLevel, comment);
	        break;
		    case pattanaik:
        		autolum=autoYGang->isCheckBox1Checked();
		        local=pattalocalGang->isCheckBox1Checked();
        		cone=coneGang->v();
		        rod=rodGang->v();
        		multiplier=multiplierGang->v();
				execPattanaikQuery(autolum, local, cone, rod, multiplier, comment);
	        break;
    		case reinhard02:
		        scales = usescalesGang->isCheckBox1Checked();
        		key = keyGang->v();
		        phi = phiGang->v();
        		irange = (int) range2Gang->v();
		        lower = (int) lowerGang->v();
        		upper = (int) upperGang->v();
				execReinhard02Query(scales, key, phi, irange, lower, upper, comment);
	        break;
    		case reinhard05:
		        brightness = brightnessGang->v();
        		chromaticAdaptation = chromaticGang->v();
		        lightAdaptation = lightGang->v();
				execReinhard05Query(brightness, chromaticAdaptation, lightAdaptation, comment);
        	break;
	    }	
	}
}

void TonemappingPanel::loadParameters()
{
	SavedParameters dialog(currentTmoOperator);
	if (dialog.exec())
	{
		QSqlTableModel *model = dialog.getModel();
		if (model->rowCount() == 0)
			return;
		int selectedRow = dialog.getCurrentIndex().row();
		// Ashikhmin
		bool simple,
				eq2;
		float lct;
		// Drago
		float bias;
		// Durand
		float spatial,
				range,
				base;
		// Fattal
		float alpha,
				beta,
				colorSat,
				noiseReduction;
		bool oldFattal;
		// Mantiuk 06
		bool contrastEqualization;
		float contrastFactor;
		float saturationFactor;
		float detailFactor;
		// Mantiuk 08
		float colorSaturation,
				contrastEnhancement,
				luminanceLevel;
		bool manualLuminanceLevel;
		// Pattanaik
		float multiplier,
				rod,
				cone;
		bool autolum,
				local;
		// Reinhard 02
		bool scales;
		float key,
				phi;
		int irange,
			lower,
			upper;
		// Reinhard 05
		float brightness,
				chromaticAdaptation,
				lightAdaptation;
		// Pre-gamma
		float pregamma;

    	switch (currentTmoOperator) {
			case ashikhmin:
				simple = model->record(selectedRow).value("simple").toBool();
				eq2 = model->record(selectedRow).value("eq2").toBool();
				lct = model->record(selectedRow).value("lct").toFloat();
				pregamma = model->record(selectedRow).value("pregamma").toFloat();
				simpleCheckBox->setChecked(simple);
				if (eq2)
					eq2RadioButton->setChecked(true);
				else
					eq4RadioButton->setChecked(true);
				contrastSlider->setValue(lct);
				contrastdsb->setValue(lct);
				pregammaSlider->setValue(pregamma);
				pregammadsb->setValue(pregamma);
			break;
			case drago:
				bias = model->record(selectedRow).value("bias").toFloat();
				pregamma = model->record(selectedRow).value("pregamma").toFloat();
				biasSlider->setValue(bias);
				biasdsb->setValue(bias);
				pregammaSlider->setValue(pregamma);
				pregammadsb->setValue(pregamma);
			break;
			case durand:
				spatial = model->record(selectedRow).value("spatial").toFloat();
				range = model->record(selectedRow).value("range").toFloat();
				base = model->record(selectedRow).value("base").toFloat();
				pregamma = model->record(selectedRow).value("pregamma").toFloat();
				spatialSlider->setValue(spatial);
				spatialdsb->setValue(spatial);
				rangeSlider->setValue(range);
				rangedsb->setValue(range);
				baseSlider->setValue(base);
				basedsb->setValue(base);
				pregammaSlider->setValue(pregamma);
				pregammadsb->setValue(pregamma);
			break;
			case fattal:
				alpha = model->record(selectedRow).value("alpha").toFloat();
				beta = model->record(selectedRow).value("beta").toFloat();
				colorSat = model->record(selectedRow).value("colorSaturation").toFloat();
				noiseReduction = model->record(selectedRow).value("noiseReduction").toFloat();
				oldFattal = model->record(selectedRow).value("oldFattal").toBool();
				pregamma = model->record(selectedRow).value("pregamma").toFloat();
				alphaSlider->setValue(alpha);
				alphadsb->setValue(alpha);
				betaSlider->setValue(beta);
				betadsb->setValue(beta);
				saturation2Slider->setValue(colorSat);
				saturation2dsb->setValue(colorSat);
				noiseSlider->setValue(noiseReduction);
				noisedsb->setValue(noiseReduction);
				oldFattalCheckBox->setChecked(oldFattal);
				pregammaSlider->setValue(pregamma);
				pregammadsb->setValue(pregamma);
			break;
    		case mantiuk06:
				contrastEqualization = model->record(selectedRow).value("contrastEqualization").toBool();		
				contrastFactor =  model->record(selectedRow).value("contrastFactor").toFloat();	
				saturationFactor =  model->record(selectedRow).value("saturationFactor").toFloat();	
				detailFactor =  model->record(selectedRow).value("detailFactor").toFloat();
				pregamma = model->record(selectedRow).value("pregamma").toFloat();
				contrastEqualizCheckBox->setChecked(contrastEqualization);					
				contrastFactorSlider->setValue(contrastFactor);
				contrastFactordsb->setValue(contrastFactor);
				saturationFactorSlider->setValue(saturationFactor);
				saturationFactordsb->setValue(saturationFactor);
				detailFactorSlider->setValue(detailFactor);
				detailFactordsb->setValue(detailFactor);
				pregammaSlider->setValue(pregamma);
				pregammadsb->setValue(pregamma);
			break;
    		case mantiuk08:
				colorSaturation = model->record(selectedRow).value("colorSaturation").toFloat();
				contrastEnhancement = model->record(selectedRow).value("contrastEnhancement").toFloat();
				luminanceLevel = model->record(selectedRow).value("luminanceLevel").toFloat();
				manualLuminanceLevel = model->record(selectedRow).value("manualLuminanceLevel").toBool();
				pregamma = model->record(selectedRow).value("pregamma").toFloat();
				colorSaturationSlider->setValue(colorSaturation);
				colorSaturationDSB->setValue(colorSaturation);
				contrastEnhancementSlider->setValue(contrastEnhancement);
				contrastEnhancementDSB->setValue(contrastEnhancement);
				luminanceLevelSlider->setValue(luminanceLevel);
				luminanceLevelDSB->setValue(luminanceLevel);
				luminanceLevelCheckBox->setChecked(manualLuminanceLevel);	
				pregammaSlider->setValue(pregamma);
				pregammadsb->setValue(pregamma);
			break;
			case pattanaik:
				multiplier = model->record(selectedRow).value("multiplier").toFloat();
				rod = model->record(selectedRow).value("rod").toFloat();
				cone = model->record(selectedRow).value("cone").toFloat();
				autolum = model->record(selectedRow).value("autolum").toBool();
				local = model->record(selectedRow).value("local").toBool();
				pregamma = model->record(selectedRow).value("pregamma").toFloat();
				multiplierSlider->setValue(multiplier);
				multiplierdsb->setValue(multiplier);
				coneSlider->setValue(cone);
				conedsb->setValue(cone);
				rodSlider->setValue(rod);
				roddsb->setValue(rod);
				pattalocal->setChecked(local);
				autoYcheckbox->setChecked(autolum);
				pregammaSlider->setValue(pregamma);
				pregammadsb->setValue(pregamma);
			break;
			case reinhard02:
				scales = model->record(selectedRow).value("scales").toBool();
				key = model->record(selectedRow).value("key").toFloat();
				phi = model->record(selectedRow).value("phi").toFloat();
				irange = model->record(selectedRow).value("range").toInt();
				lower = model->record(selectedRow).value("lower").toInt();
				upper = model->record(selectedRow).value("upper").toInt();
				pregamma = model->record(selectedRow).value("pregamma").toFloat();
				usescalescheckbox->setChecked(scales);
				keySlider->setValue(key);
				keydsb->setValue(key);
				phiSlider->setValue(phi);
				phidsb->setValue(phi);
				range2Slider->setValue(irange);
				range2dsb->setValue(irange);
				lowerSlider->setValue(lower);
				lowerdsb->setValue(lower);
				upperSlider->setValue(upper);
				upperdsb->setValue(upper);
				pregammaSlider->setValue(pregamma);
				pregammadsb->setValue(pregamma);
			break;
			case reinhard05:
				brightness = model->record(selectedRow).value("brightness").toFloat();
				chromaticAdaptation = model->record(selectedRow).value("chromaticAdaptation").toFloat();
				lightAdaptation = model->record(selectedRow).value("lightAdaptation").toFloat();
				pregamma = model->record(selectedRow).value("pregamma").toFloat();
				brightnessSlider->setValue(brightness);
				brightnessdsb->setValue(brightness);
				chromaticAdaptSlider->setValue(chromaticAdaptation);
				chromaticAdaptdsb->setValue(chromaticAdaptation);
				lightAdaptSlider->setValue(lightAdaptation);
				lightAdaptdsb->setValue(lightAdaptation);	
				pregammaSlider->setValue(pregamma);
				pregammadsb->setValue(pregamma);
			break;
		}
	}
}

void TonemappingPanel::loadComments()
{
	SavedParameters dialog(all);
	if (dialog.exec())
	{
		QSqlTableModel *model = dialog.getModel();
		int selectedRow = dialog.getCurrentIndex().row();
		QString comment, tmOperator;
		comment = model->record(selectedRow).value("comment").toString();
		tmOperator = model->record(selectedRow).value("operator").toString();

		QSqlTableModel *temp_model = new QSqlTableModel;
		temp_model->setTable(tmOperator);
		temp_model->select();
		QSqlQuery query("SELECT * from " + tmOperator + " WHERE comment = '" + comment + "'");
		if (tmOperator == "ashikhmin")
		{
			stackedWidget_operators->setCurrentIndex(ashikhmin);
			updateCurrentTmoOperator(ashikhmin);
			while (query.next())
			{
				simpleCheckBox->setChecked(query.value(0).toBool());
				if (query.value(1).toBool())
					eq2RadioButton->setChecked(true);
				else
					eq4RadioButton->setChecked(true);
				contrastSlider->setValue(query.value(2).toFloat());
				contrastdsb->setValue(query.value(2).toFloat());
				pregammaSlider->setValue(query.value(3).toFloat());
				pregammadsb->setValue(query.value(3).toFloat());
			}
		}
		else if (tmOperator == "drago")
		{
			stackedWidget_operators->setCurrentIndex(drago);
			updateCurrentTmoOperator(drago);
			while (query.next())
			{
				biasSlider->setValue(query.value(0).toFloat());
				biasdsb->setValue(query.value(0).toFloat());
				pregammaSlider->setValue(query.value(1).toFloat());
				pregammadsb->setValue(query.value(1).toFloat());
			}
		}
		else if (tmOperator == "durand")
		{
			stackedWidget_operators->setCurrentIndex(durand);
			updateCurrentTmoOperator(durand);
			while (query.next())
			{
				spatialSlider->setValue(query.value(0).toFloat());
				spatialdsb->setValue(query.value(0).toFloat());
				rangeSlider->setValue(query.value(1).toFloat());
				rangedsb->setValue(query.value(1).toFloat());
				baseSlider->setValue(query.value(2).toFloat());
				basedsb->setValue(query.value(2).toFloat());
				pregammaSlider->setValue(query.value(3).toFloat());
				pregammadsb->setValue(query.value(3).toFloat());
			}
		}
		else if (tmOperator == "fattal")
		{
			stackedWidget_operators->setCurrentIndex(fattal);
			updateCurrentTmoOperator(fattal);
			while (query.next())
			{
				alphaSlider->setValue(query.value(0).toFloat());
				alphadsb->setValue(query.value(0).toFloat());
				betaSlider->setValue(query.value(1).toFloat());
				betadsb->setValue(query.value(1).toFloat());
				saturation2Slider->setValue(query.value(2).toFloat());
				saturation2dsb->setValue(query.value(2).toFloat());
				noiseSlider->setValue(query.value(3).toFloat());
				noisedsb->setValue(query.value(3).toFloat());
				oldFattalCheckBox->setChecked(query.value(4).toBool());
				pregammaSlider->setValue(query.value(5).toFloat());
				pregammadsb->setValue(query.value(5).toFloat());
			}
		}
		else if (tmOperator == "mantiuk06")
		{
			stackedWidget_operators->setCurrentIndex(mantiuk06);
			updateCurrentTmoOperator(mantiuk06);
			while (query.next())
			{
				contrastEqualizCheckBox->setChecked(query.value(0).toBool());					
				contrastFactorSlider->setValue(query.value(1).toFloat());
				contrastFactordsb->setValue(query.value(1).toFloat());
				saturationFactorSlider->setValue(query.value(2).toFloat());
				saturationFactordsb->setValue(query.value(2).toFloat());
				detailFactorSlider->setValue(query.value(3).toFloat());
				detailFactordsb->setValue(query.value(3).toFloat());
				pregammaSlider->setValue(query.value(4).toFloat());
				pregammadsb->setValue(query.value(4).toFloat());
			}
		}
		else if (tmOperator == "mantiuk08")
		{
			stackedWidget_operators->setCurrentIndex(mantiuk08);
			updateCurrentTmoOperator(mantiuk08);
			while (query.next())
			{
				colorSaturationSlider->setValue(query.value(0).toFloat());
				colorSaturationDSB->setValue(query.value(0).toFloat());
				contrastEnhancementSlider->setValue(query.value(1).toFloat());
				contrastEnhancementDSB->setValue(query.value(1).toFloat());
				luminanceLevelSlider->setValue(query.value(2).toFloat());
				luminanceLevelDSB->setValue(query.value(2).toFloat());
				luminanceLevelCheckBox->setChecked(query.value(3).toBool());	
				pregammaSlider->setValue(query.value(4).toFloat());
				pregammadsb->setValue(query.value(4).toFloat());
			}
		}
		else if (tmOperator == "pattanaik")
		{
			stackedWidget_operators->setCurrentIndex(pattanaik);
			updateCurrentTmoOperator(pattanaik);
			while (query.next())
			{
				multiplierSlider->setValue(query.value(0).toFloat());
				multiplierdsb->setValue(query.value(0).toFloat());
				coneSlider->setValue(query.value(1).toFloat());
				conedsb->setValue(query.value(1).toFloat());
				rodSlider->setValue(query.value(2).toFloat());
				roddsb->setValue(query.value(2).toFloat());
				pattalocal->setChecked(query.value(3).toBool());
				autoYcheckbox->setChecked(query.value(3).toFloat());
				pregammaSlider->setValue(query.value(4).toFloat());
				pregammadsb->setValue(query.value(4).toFloat());
			}
		}
		else if (tmOperator == "reinhard02")
		{
			stackedWidget_operators->setCurrentIndex(reinhard02);
			updateCurrentTmoOperator(reinhard02);
			while (query.next())
			{
				usescalescheckbox->setChecked(query.value(0).toBool());
				keySlider->setValue(query.value(1).toFloat());
				keydsb->setValue(query.value(1).toFloat());
				phiSlider->setValue(query.value(2).toFloat());
				phidsb->setValue(query.value(2).toFloat());
				range2Slider->setValue(query.value(3).toInt());
				range2dsb->setValue(query.value(3).toInt());
				lowerSlider->setValue(query.value(4).toInt());
				lowerdsb->setValue(query.value(4).toInt());
				upperSlider->setValue(query.value(5).toInt());
				upperdsb->setValue(query.value(5).toInt());
				pregammaSlider->setValue(query.value(6).toFloat());
				pregammadsb->setValue(query.value(6).toFloat());
			}
		}
		else if (tmOperator == "reinhard05")
		{
			stackedWidget_operators->setCurrentIndex(reinhard05);
			updateCurrentTmoOperator(reinhard05);
			while (query.next())
			{
				brightnessSlider->setValue(query.value(0).toFloat());
				brightnessdsb->setValue(query.value(0).toFloat());
				chromaticAdaptSlider->setValue(query.value(1).toFloat());
				chromaticAdaptdsb->setValue(query.value(1).toFloat());
				lightAdaptSlider->setValue(query.value(2).toFloat());
				lightAdaptdsb->setValue(query.value(2).toFloat());	
				pregammaSlider->setValue(query.value(3).toFloat());
				pregammadsb->setValue(query.value(3).toFloat());
			}
		}
		delete temp_model;
	}
}

void TonemappingPanel::execMantiuk06Query(bool contrastEqualization, float contrastFactor, float saturationFactor,
	float detailFactor, QString comment)
{
	qDebug() << "TonemappingPanel::execMantiuk06Query";
	QSqlDatabase db = QSqlDatabase::database();
	QSqlQuery query(db);
	float pregamma = pregammadsb->value();
	query.prepare("INSERT INTO mantiuk06 (contrastEqualization, contrastFactor, saturationFactor, detailFactor, pregamma, comment) "
				"VALUES (:contrastEqualization, :contrastFactor, :saturationFactor, :detailFactor, :pregamma, :comment)");
	query.bindValue(":contrastEqualization", contrastEqualization);
	query.bindValue(":contransFactor", contrastFactor);
	query.bindValue(":saturationFactor", saturationFactor);
	query.bindValue(":detailFactor", detailFactor);
	query.bindValue(":pregamma", pregamma);
	query.bindValue(":comment", comment);
	bool res = query.exec();
	if (res == false)
		qDebug() << query.lastError();
}

void TonemappingPanel::execMantiuk08Query(float colorSaturation, float contrastEnhancement, float luminanceLevel,
	bool manualLuminanceLevel, QString comment)
{
	qDebug() << "TonemappingPanel::execMantiuk08Query";
	QSqlDatabase db = QSqlDatabase::database();
	QSqlQuery query(db);
	float pregamma = pregammadsb->value();
	query.prepare("INSERT INTO mantiuk08 (colorSaturation, contrastEnhancement, luminanceLevel, manualLuminanceLevel, pregamma, comment) "
				"VALUES (:colorSaturation, :contrastEnhancement, :luminanceLevel, :manualLuminanceLevel, :pregamma, :comment)");
	query.bindValue(":colorSaturation", colorSaturation);
	query.bindValue(":contrastEnhancement", contrastEnhancement);
	query.bindValue(":luminanceLevel", luminanceLevel);
	query.bindValue(":manualLuminanceLevel", manualLuminanceLevel);
	query.bindValue(":pregamma", pregamma);
	query.bindValue(":comment", comment);
	bool res = query.exec();
	if (res == false)
		qDebug() << query.lastError();
}

void TonemappingPanel::execAshikhminQuery(bool simple, bool eq2, float lct, QString comment)
{
	qDebug() << "TonemappingPanel::execAshikhminQuery";
	QSqlDatabase db = QSqlDatabase::database();
	QSqlQuery query(db);
	float pregamma = pregammadsb->value();
	query.prepare("INSERT INTO ashikhmin (simple, eq2, lct, pregamma, comment) "
				"VALUES (:simple, :eq2, :lct, :pregamma, :comment)");
	query.bindValue(":simple", simple);
	query.bindValue(":eq2", eq2);
	query.bindValue(":lct", lct);
	query.bindValue(":pregamma", pregamma);
	query.bindValue(":comment", comment);
	bool res = query.exec();
	if (res == false)
		qDebug() << query.lastError();
}

void TonemappingPanel::execDragoQuery(float bias, QString comment)
{
	qDebug() << "TonemappingPanel::execDragoQuery";
	QSqlDatabase db = QSqlDatabase::database();
	QSqlQuery query(db);
	float pregamma = pregammadsb->value();
	query.prepare("INSERT INTO drago (bias, pregamma, comment) "
				"VALUES (:bias, :pregamma, :comment)");
	query.bindValue(":bias", bias);
	query.bindValue(":pregamma", pregamma);
	query.bindValue(":comment", comment);
	bool res = query.exec();
	if (res == false)
		qDebug() << query.lastError();
}

void TonemappingPanel::execDurandQuery(float spatial, float range, float base, QString comment)
{
	qDebug() << "TonemappingPanel::execDurandQuery";
	QSqlDatabase db = QSqlDatabase::database();
	QSqlQuery query(db);
	float pregamma = pregammadsb->value();
	query.prepare("INSERT INTO durand (spatial, range, base, pregamma, comment) "
				"VALUES (:spatial, :range, :base, :pregamma, :comment)");
	query.bindValue(":spatial", spatial);
	query.bindValue(":base", base);
	query.bindValue(":range", range);
	query.bindValue(":pregamma", pregamma);
	query.bindValue(":comment", comment);
	bool res = query.exec();
	if (res == false)
		qDebug() << query.lastError();
}

void TonemappingPanel::execFattalQuery(float alpha, float beta, float colorSaturation, float noiseReduction, bool oldFattal, QString comment)
{
	qDebug() << "TonemappingPanel::execFattalQuery";
	QSqlDatabase db = QSqlDatabase::database();
	QSqlQuery query(db);
	float pregamma = pregammadsb->value();
	query.prepare("INSERT INTO fattal (alpha, beta, colorSaturation, noiseReduction, oldFattal, pregamma, comment) "
				"VALUES (:alpha, :beta, :colorSaturation, :noiseReduction, :oldFattal, :pregamma, :comment)");
	query.bindValue(":alpha", alpha);
	query.bindValue(":beta", beta);
	query.bindValue(":colorSaturation", colorSaturation);
	query.bindValue(":noiseReduction", noiseReduction);
	query.bindValue(":oldFattal", oldFattal);
	query.bindValue(":pregamma", pregamma);
	query.bindValue(":comment", comment);
	bool res = query.exec();
	if (res == false)
		qDebug() << query.lastError();
}

void TonemappingPanel::execPattanaikQuery(bool autolum, bool local, float cone, float rod, float multiplier, QString comment)
{
	qDebug() << "TonemappingPanel::execPattanaikQuery";
	QSqlDatabase db = QSqlDatabase::database();
	QSqlQuery query(db);
	float pregamma = pregammadsb->value();
	query.prepare("INSERT INTO pattanaik (autolum, local, cone, rod, multiplier, pregamma, comment) "
				"VALUES (:autolum, :local, :cone, :rod, :multiplier, :pregamma, :comment)");
	query.bindValue(":autolum", autolum);
	query.bindValue(":local", local);
	query.bindValue(":cone", cone);
	query.bindValue(":rod", rod);
	query.bindValue(":multiplier", multiplier);
	query.bindValue(":pregamma", pregamma);
	query.bindValue(":comment", comment);
	bool res = query.exec();
	if (res == false)
		qDebug() << query.lastError();
}

void TonemappingPanel::execReinhard02Query(bool scales, float key, float phi, int range, int lower, int upper, QString comment)
{
	qDebug() << "TonemappingPanel::execReinhard02Query";
	QSqlDatabase db = QSqlDatabase::database();
	QSqlQuery query(db);
	float pregamma = pregammadsb->value();
	query.prepare("INSERT INTO reinhard02 (scales, key, phi, range, lower, upper, pregamma, comment) "
				"VALUES (:scales, :key, :phi, :range, :lower, :upper, :pregamma, :comment)");
	query.bindValue(":scales", scales);
	query.bindValue(":key", key);
	query.bindValue(":phi", phi);
	query.bindValue(":range", range);
	query.bindValue(":lower", lower);
	query.bindValue(":upper", upper);
	query.bindValue(":pregamma", pregamma);
	query.bindValue(":comment", comment);
	bool res = query.exec();
	if (res == false)
		qDebug() << query.lastError();
}

void TonemappingPanel::execReinhard05Query(float brightness, float chromaticAdaptation, float lightAdaptation, QString comment)
{
	qDebug() << "TonemappingPanel::execReinhard05Query";
	QSqlDatabase db = QSqlDatabase::database();
	QSqlQuery query(db);
	float pregamma = pregammadsb->value();
	query.prepare("INSERT INTO reinhard05 (brightness, chromaticAdaptation, lightAdaptation, pregamma, comment) "
				"VALUES (:brightness, :chromaticAdaptation, :lightAdaptation, :pregamma, :comment)");
	query.bindValue(":brightness", brightness);
	query.bindValue(":chromaticAdaptation", chromaticAdaptation);
	query.bindValue(":lightAdaptation", lightAdaptation);
	query.bindValue(":pregamma", pregamma);
	query.bindValue(":comment", comment);
	bool res = query.exec();
	if (res == false)
		qDebug() << query.lastError();
}

bool TonemappingPanel::replaceLdr()
{
	return replaceLdrCheckBox->isChecked();
}

// ------------------------- // END FILE


