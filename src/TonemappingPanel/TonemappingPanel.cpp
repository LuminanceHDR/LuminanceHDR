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
#include "Common/config.h"
#include "TonemappingPanel/TonemappingPanel.h"
#include "TonemappingPanel/TMOProgressIndicator.h"
#include "TonemappingPanel/SavedParametersDialog.h"
#include "TonemappingPanel/SavingParametersDialog.h"
#include "TonemappingOperators/pfstmdefaultparams.h"
#include "UI/Gang.h"
#include "ui_TonemappingPanel.h"

TonemappingPanel::TonemappingPanel(QWidget *parent):
    QWidget(parent),
	adding_custom_size(false),
    m_Ui(new Ui::TonemappingPanel)
{
    m_Ui->setupUi(this);

    currentTmoOperator = mantiuk06; // from Qt Designer

    // mantiuk06
    contrastfactorGang = new Gang(m_Ui->contrastFactorSlider,m_Ui->contrastFactordsb,m_Ui->contrastEqualizCheckBox,NULL,NULL, NULL, 0.001f, 1.0f, MANTIUK06_CONTRAST_FACTOR);

    connect(contrastfactorGang, SIGNAL(enableUndo(bool)), m_Ui->undoButton, SLOT(setEnabled(bool)));
    connect(contrastfactorGang, SIGNAL(enableRedo(bool)), m_Ui->redoButton, SLOT(setEnabled(bool)));

    saturationfactorGang = new Gang(m_Ui->saturationFactorSlider, m_Ui->saturationFactordsb, NULL, NULL, NULL, NULL, 0.0f, 2.0f, MANTIUK06_SATURATION_FACTOR);
    detailfactorGang = new Gang(m_Ui->detailFactorSlider, m_Ui->detailFactordsb,NULL,NULL,NULL,NULL, 1.0f, 99.0f, MANTIUK06_DETAIL_FACTOR);

    // mantiuk08
    colorSaturationGang = new Gang(m_Ui->colorSaturationSlider,m_Ui->colorSaturationDSB, NULL, NULL, NULL, NULL, 0.f, 2.f, MANTIUK08_COLOR_SATURATION);

    connect(colorSaturationGang, SIGNAL(enableUndo(bool)), m_Ui->undoButton, SLOT(setEnabled(bool)));
    connect(colorSaturationGang, SIGNAL(enableRedo(bool)), m_Ui->redoButton, SLOT(setEnabled(bool)));

    contrastEnhancementGang = new Gang(m_Ui->contrastEnhancementSlider, m_Ui->contrastEnhancementDSB, NULL, NULL, NULL, NULL, .01f, 10.f, MANTIUK08_CONTRAST_ENHANCEMENT);
    luminanceLevelGang = new Gang(m_Ui->luminanceLevelSlider, m_Ui->luminanceLevelDSB, m_Ui->luminanceLevelCheckBox, NULL, NULL, NULL, 1.f, 100.0f, MANTIUK08_LUMINANCE_LEVEL);

    // fattal02
    alphaGang = new Gang(m_Ui->alphaSlider, m_Ui->alphadsb, NULL,NULL,NULL,NULL, 1e-4, 2.f, FATTAL02_ALPHA, true);

    connect(alphaGang, SIGNAL(enableUndo(bool)), m_Ui->undoButton, SLOT(setEnabled(bool)));
    connect(alphaGang, SIGNAL(enableRedo(bool)), m_Ui->redoButton, SLOT(setEnabled(bool)));

    betaGang = new Gang(m_Ui->betaSlider, m_Ui->betadsb, NULL,NULL,NULL,NULL, 0.1f, 2.f, FATTAL02_BETA);
    saturation2Gang = new Gang(m_Ui->saturation2Slider, m_Ui->saturation2dsb, NULL,NULL,NULL,NULL, 0.f, 1.f, FATTAL02_COLOR);
    noiseGang = new Gang(m_Ui->noiseSlider, m_Ui->noisedsb, NULL,NULL,NULL,NULL, 0, 1.f, FATTAL02_NOISE_REDUX);
    oldFattalGang = new Gang(NULL,NULL, m_Ui->oldFattalCheckBox);

    // ashikhmin02
    contrastGang = new Gang(m_Ui->contrastSlider, m_Ui->contrastdsb,NULL,NULL,NULL,NULL, 0.f, 1.f, 0.5f);

    connect(contrastGang, SIGNAL(enableUndo(bool)), m_Ui->undoButton, SLOT(setEnabled(bool)));
    connect(contrastGang, SIGNAL(enableRedo(bool)), m_Ui->redoButton, SLOT(setEnabled(bool)));

    simpleGang = new Gang(NULL, NULL, m_Ui->simpleCheckBox);
    eq2Gang = new Gang(NULL, NULL,NULL, NULL, m_Ui->eq2RadioButton, m_Ui->eq4RadioButton);

    // drago03
    biasGang = new Gang(m_Ui->biasSlider, m_Ui->biasdsb,NULL,NULL,NULL,NULL, 0.f, 1.f, DRAGO03_BIAS);

    connect(biasGang, SIGNAL(enableUndo(bool)), m_Ui->undoButton, SLOT(setEnabled(bool)));
    connect(biasGang, SIGNAL(enableRedo(bool)), m_Ui->redoButton, SLOT(setEnabled(bool)));

    // durand02
    spatialGang = new Gang(m_Ui->spatialSlider, m_Ui->spatialdsb, NULL, NULL, NULL, NULL, 0.f, 100.f, DURAND02_SPATIAL);

    connect(spatialGang, SIGNAL(enableUndo(bool)), m_Ui->undoButton, SLOT(setEnabled(bool)));
    connect(spatialGang, SIGNAL(enableRedo(bool)), m_Ui->redoButton, SLOT(setEnabled(bool)));

    rangeGang = new Gang(m_Ui->rangeSlider, m_Ui->rangedsb,NULL,NULL,NULL,NULL, 0.01f, 10.f, DURAND02_RANGE);
    baseGang = new Gang(m_Ui->baseSlider, m_Ui->basedsb,NULL,NULL,NULL,NULL, 0.f, 10.f, DURAND02_BASE);

    // pattanaik00
    multiplierGang = new Gang(m_Ui->multiplierSlider, m_Ui->multiplierdsb,NULL,NULL,NULL,NULL, 1e-3,1000.f, PATTANAIK00_MULTIPLIER, true);

    connect(multiplierGang, SIGNAL(enableUndo(bool)), m_Ui->undoButton, SLOT(setEnabled(bool)));
    connect(multiplierGang, SIGNAL(enableRedo(bool)), m_Ui->redoButton, SLOT(setEnabled(bool)));

    coneGang = new Gang(m_Ui->coneSlider, m_Ui->conedsb,NULL,NULL,NULL,NULL, 0.f, 1.f, PATTANAIK00_CONE);
    rodGang = new Gang(m_Ui->rodSlider, m_Ui->roddsb,NULL,NULL,NULL,NULL, 0.f, 1.f, PATTANAIK00_ROD);
    autoYGang = new Gang(NULL,NULL, m_Ui->autoYcheckbox);
    pattalocalGang = new Gang(NULL,NULL, m_Ui->pattalocal);

    // reinhard02
    keyGang = new Gang(m_Ui->keySlider, m_Ui->keydsb,NULL,NULL,NULL,NULL, 0.f, 1.f, 0.18f);

    connect(keyGang, SIGNAL(enableUndo(bool)), m_Ui->undoButton, SLOT(setEnabled(bool)));
    connect(keyGang, SIGNAL(enableRedo(bool)), m_Ui->redoButton, SLOT(setEnabled(bool)));

    phiGang = new Gang(m_Ui->phiSlider, m_Ui->phidsb, NULL,NULL,NULL,NULL, 0.f, 100.f, REINHARD02_PHI);
    range2Gang = new Gang(m_Ui->range2Slider, m_Ui->range2dsb,NULL,NULL,NULL,NULL, 1.f, 32.f, REINHARD02_RANGE);
    lowerGang = new Gang(m_Ui->lowerSlider, m_Ui->lowerdsb,NULL,NULL,NULL,NULL, 1.f, 100.f, REINHARD02_LOWER);
    upperGang = new Gang(m_Ui->upperSlider, m_Ui->upperdsb,NULL,NULL,NULL,NULL, 1.f, 100.f, REINHARD02_UPPER);
    usescalesGang = new Gang(NULL,NULL, m_Ui->usescalescheckbox);

    // reinhard05
    brightnessGang = new Gang(m_Ui->brightnessSlider, m_Ui->brightnessdsb,NULL,NULL,NULL,NULL, -20.f, 20.f, REINHARD05_BRIGHTNESS);

    connect(brightnessGang, SIGNAL(enableUndo(bool)), m_Ui->undoButton, SLOT(setEnabled(bool)));
    connect(brightnessGang, SIGNAL(enableRedo(bool)), m_Ui->redoButton, SLOT(setEnabled(bool)));

    chromaticGang = new Gang(m_Ui->chromaticAdaptSlider, m_Ui->chromaticAdaptdsb,NULL,NULL,NULL,NULL, 0.f, 1.f, REINHARD05_CHROMATIC_ADAPTATION);
    lightGang = new Gang(m_Ui->lightAdaptSlider, m_Ui->lightAdaptdsb,NULL,NULL,NULL,NULL, 0.f, 1.f, REINHARD05_LIGHT_ADAPTATION);

    // pregamma
    pregammaGang = new Gang(m_Ui->pregammaSlider, m_Ui->pregammadsb,NULL,NULL,NULL,NULL, 0, 3, 1);

    //--
    connect(m_Ui->stackedWidget_operators, SIGNAL(currentChanged(int)), this, SLOT(updateCurrentTmoOperator(int)));

    connect(m_Ui->loadButton, SIGNAL(clicked()), this, SLOT(loadParameters()));
    connect(m_Ui->saveButton, SIGNAL(clicked()), this, SLOT(saveParameters()));
    connect(m_Ui->loadCommentsButton, SIGNAL(clicked()), this, SLOT(loadComments()));
	
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

void TonemappingPanel::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange)
		 m_Ui->retranslateUi(this);
	QWidget::changeEvent(event);
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
    m_Ui->sizeComboBox->setCurrentIndex(m_Ui->sizeComboBox->count() - 1);
}

void TonemappingPanel::on_defaultButton_clicked()
{
    switch (currentTmoOperator)
    {
    case ashikhmin:
        contrastGang->setDefault();
        m_Ui->simpleCheckBox->setChecked(false);
        m_Ui->eq2RadioButton->setChecked(true);
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
        m_Ui->oldFattalCheckBox->setChecked(false);
        break;
    case mantiuk06:
        contrastfactorGang->setDefault();
        saturationfactorGang->setDefault();
        detailfactorGang->setDefault();
        m_Ui->contrastEqualizCheckBox->setChecked(false);
        break;
    case mantiuk08:
        colorSaturationGang->setDefault();
        contrastEnhancementGang->setDefault();
        luminanceLevelGang->setDefault();
        m_Ui->luminanceLevelCheckBox->setChecked(false);
        break;
    case pattanaik:
        multiplierGang->setDefault();
        coneGang->setDefault();
        rodGang->setDefault();
        m_Ui->pattalocal->setChecked(false);
        m_Ui->autoYcheckbox->setChecked(true);
        break;
    case reinhard02:
        keyGang->setDefault();
        phiGang->setDefault();
        range2Gang->setDefault();
        lowerGang->setDefault();
        upperGang->setDefault();
        m_Ui->usescalescheckbox->setChecked(false);
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

void TonemappingPanel::on_applyButton_clicked()
{
    fillToneMappingOptions();
    setupUndo();

    emit startTonemapping(&toneMappingOptions);
}

void TonemappingPanel::fillToneMappingOptions()
{
    toneMappingOptions.origxsize = sizes[0];
    toneMappingOptions.xsize = sizes[m_Ui->sizeComboBox->currentIndex()];
    toneMappingOptions.pregamma = pregammaGang->v();
    //toneMappingOptions.tonemapSelection = checkBoxSelection->isChecked();
    //toneMappingOptions.tonemapOriginal = checkBoxOriginal->isChecked();
    switch (currentTmoOperator) {
    case ashikhmin:
        toneMappingOptions.tmoperator = ashikhmin;
        toneMappingOptions.operator_options.ashikhminoptions.simple=simpleGang->isCheckBox1Checked();
        toneMappingOptions.operator_options.ashikhminoptions.eq2=eq2Gang->isRadioButton1Checked();
        toneMappingOptions.operator_options.ashikhminoptions.lct=contrastGang->v();
        break;
    case drago:
        toneMappingOptions.tmoperator = drago;
        toneMappingOptions.operator_options.dragooptions.bias=biasGang->v();
        break;
    case durand:
        toneMappingOptions.tmoperator = durand;
        toneMappingOptions.operator_options.durandoptions.spatial=spatialGang->v();
        toneMappingOptions.operator_options.durandoptions.range=rangeGang->v();
        toneMappingOptions.operator_options.durandoptions.base=baseGang->v();
        break;
    case fattal:
        toneMappingOptions.tmoperator = fattal;
        toneMappingOptions.operator_options.fattaloptions.alpha=alphaGang->v();
        toneMappingOptions.operator_options.fattaloptions.beta=betaGang->v();
        toneMappingOptions.operator_options.fattaloptions.color=saturation2Gang->v();
        toneMappingOptions.operator_options.fattaloptions.noiseredux=noiseGang->v();
        toneMappingOptions.operator_options.fattaloptions.newfattal=!oldFattalGang->isCheckBox1Checked();
        break;
    case mantiuk06:
        toneMappingOptions.tmoperator = mantiuk06;
        toneMappingOptions.operator_options.mantiuk06options.contrastfactor=contrastfactorGang->v();
        toneMappingOptions.operator_options.mantiuk06options.saturationfactor=saturationfactorGang->v();
        toneMappingOptions.operator_options.mantiuk06options.detailfactor=detailfactorGang->v();
        toneMappingOptions.operator_options.mantiuk06options.contrastequalization=contrastfactorGang->isCheckBox1Checked();
        break;
    case mantiuk08:
        toneMappingOptions.tmoperator = mantiuk08;
        toneMappingOptions.operator_options.mantiuk08options.colorsaturation=colorSaturationGang->v();
        toneMappingOptions.operator_options.mantiuk08options.contrastenhancement=contrastEnhancementGang->v();
        toneMappingOptions.operator_options.mantiuk08options.luminancelevel=luminanceLevelGang->v();
        toneMappingOptions.operator_options.mantiuk08options.setluminance=luminanceLevelGang->isCheckBox1Checked();
        break;
    case pattanaik:
        toneMappingOptions.tmoperator = pattanaik;
        toneMappingOptions.operator_options.pattanaikoptions.autolum=autoYGang->isCheckBox1Checked();
        toneMappingOptions.operator_options.pattanaikoptions.local=pattalocalGang->isCheckBox1Checked();
        toneMappingOptions.operator_options.pattanaikoptions.cone=coneGang->v();
        toneMappingOptions.operator_options.pattanaikoptions.rod=rodGang->v();
        toneMappingOptions.operator_options.pattanaikoptions.multiplier=multiplierGang->v();
        break;
    case reinhard02:
        toneMappingOptions.tmoperator = reinhard02;
        toneMappingOptions.operator_options.reinhard02options.scales=usescalesGang->isCheckBox1Checked();
        toneMappingOptions.operator_options.reinhard02options.key=keyGang->v();
        toneMappingOptions.operator_options.reinhard02options.phi=phiGang->v();
        toneMappingOptions.operator_options.reinhard02options.range=(int)range2Gang->v();
        toneMappingOptions.operator_options.reinhard02options.lower=(int)lowerGang->v();
        toneMappingOptions.operator_options.reinhard02options.upper=(int)upperGang->v();
        break;
    case reinhard05:
        toneMappingOptions.tmoperator = reinhard05;
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
                                                  lum_options.getDefaultPathTmoSettings(),
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
        lum_options.setDefaultPathTmoSettings( qfi.path() );

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
                                                 lum_options.getDefaultPathTmoSettings(),
                                                 tr("LuminanceHDR tonemapping settings text file (*.txt)"));
    if( ! fname.isEmpty() )
    {
        QFileInfo qfi(fname);
        if (qfi.suffix().toUpper() != "TXT")
        {
            fname+=".txt";
        }

        lum_options.setDefaultPathTmoSettings( qfi.path() );

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
    out << "XSIZE=" << sizes[m_Ui->sizeComboBox->currentIndex()] << endl;
    out << "QUALITY=" << LuminanceOptions().getBatchTmDefaultOutputQuality() << endl;

    QWidget *current_page = m_Ui->stackedWidget_operators->currentWidget();
    if (current_page == m_Ui->page_mantiuk06)
    {
        out << "TMO=" << "Mantiuk06" << endl;
        out << "CONTRASTFACTOR=" << contrastfactorGang->v() << endl;
        out << "SATURATIONFACTOR=" << saturationfactorGang->v() << endl;
        out << "DETAILFACTOR=" << detailfactorGang->v() << endl;
        out << "CONTRASTEQUALIZATION=" << (m_Ui->contrastEqualizCheckBox->isChecked() ? "YES" : "NO") << endl;
    }
    else if (current_page == m_Ui->page_mantiuk08)
    {
        out << "TMO=" << "Mantiuk08" << endl;
        out << "COLORSATURATION=" << colorSaturationGang->v() << endl;
        out << "CONTRASTENHANCEMENT=" << contrastEnhancementGang->v() << endl;
        out << "LUMINANCELEVEL=" << luminanceLevelGang->v() << endl;
        out << "SETLUMINANCE=" << (m_Ui->luminanceLevelCheckBox->isChecked() ? "YES" : "NO") << endl;
    }
    else if (current_page == m_Ui->page_fattal)
    {
        out << "TMO=" << "Fattal02" << endl;
        out << "ALPHA=" << alphaGang->v() << endl;
        out << "BETA=" << betaGang->v() << endl;
        out << "COLOR=" << saturation2Gang->v() << endl;
        out << "NOISE=" << noiseGang->v() << endl;
        out << "OLDFATTAL=" << (m_Ui->oldFattalCheckBox->isChecked() ? "YES" : "NO") << endl;
    }
    else if (current_page == m_Ui->page_ashikhmin)
    {
        out << "TMO=" << "Ashikhmin02" << endl;
        out << "SIMPLE=" << (m_Ui->simpleCheckBox->isChecked() ? "YES" : "NO") << endl;
        out << "EQUATION=" << (m_Ui->eq2RadioButton->isChecked() ? "2" : "4") << endl;
        out << "CONTRAST=" << contrastGang->v() << endl;
    }
    else if (current_page == m_Ui->page_durand)
    {
        out << "TMO=" << "Durand02" << endl;
        out << "SPATIAL=" << spatialGang->v() << endl;
        out << "RANGE=" << rangeGang->v() << endl;
        out << "BASE=" << baseGang->v() << endl;
    }
    else if (current_page == m_Ui->page_drago)
    {
        out << "TMO=" << "Drago03" << endl;
        out << "BIAS=" << biasGang->v() << endl;
    }
    else if (current_page == m_Ui->page_pattanaik)
    {
        out << "TMO=" << "Pattanaik00" << endl;
        out << "MULTIPLIER=" << multiplierGang->v() << endl;
        out << "LOCAL=" << (m_Ui->pattalocal->isChecked() ? "YES" : "NO") << endl;
        out << "AUTOLUMINANCE=" << (m_Ui->autoYcheckbox->isChecked() ? "YES" : "NO") << endl;
        out << "CONE=" << coneGang->v() << endl;
        out << "ROD=" << rodGang->v() << endl;
    }
    else if (current_page == m_Ui->page_reinhard02)
    {
        out << "TMO=" << "Reinhard02" << endl;
        out << "KEY=" << keyGang->v() << endl;
        out << "PHI=" << phiGang->v() << endl;
        out << "SCALES=" << (m_Ui->usescalescheckbox->isChecked() ? "YES" : "NO") << endl;
        out << "RANGE=" << range2Gang->v() << endl;
        out << "LOWER=" << lowerGang->v() << endl;
        out << "UPPER=" << upperGang->v() << endl;
    }
    else if (current_page == m_Ui->page_reinhard05)
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
            for (idx = 0; idx < m_Ui->sizeComboBox->count(); idx++)
			{
					if (sizes[idx] == value.toInt())
						break;
			}
            if (idx == m_Ui->sizeComboBox->count()) // Custom XSIZE
			{
				sizes.push_back(value.toInt());
				fillCustomSizeComboBox();
                m_Ui->sizeComboBox->setCurrentIndex(m_Ui->sizeComboBox->count() - 1);
			}	
			else
                m_Ui->sizeComboBox->setCurrentIndex(idx);
		}
        //else if ( field == "QUALITY" å)
        //{
        //    m_Ui->qualityHS->setValue(value.toInt());
        //    m_Ui->qualitySB->setValue(value.toInt());
        //}
        else if (field == "TMO")
        {
            if (value == "Ashikhmin02") {
                m_Ui->stackedWidget_operators->setCurrentWidget(m_Ui->page_ashikhmin);
            } else if (value == "Mantiuk06") {
                m_Ui->stackedWidget_operators->setCurrentWidget(m_Ui->page_mantiuk06);
            } else if (value == "Mantiuk08") {
                m_Ui->stackedWidget_operators->setCurrentWidget(m_Ui->page_mantiuk08);
            } else if (value == "Drago03") {
                m_Ui->stackedWidget_operators->setCurrentWidget(m_Ui->page_drago);
            } else if (value == "Durand02") {
                m_Ui->stackedWidget_operators->setCurrentWidget(m_Ui->page_durand);
            } else if (value == "Fattal02") {
                m_Ui->stackedWidget_operators->setCurrentWidget(m_Ui->page_fattal);
            } else if (value == "Pattanaik00") {
                m_Ui->stackedWidget_operators->setCurrentWidget(m_Ui->page_pattanaik);
            } else if (value == "Reinhard02") {
                m_Ui->stackedWidget_operators->setCurrentWidget(m_Ui->page_reinhard02);
            } else if (value == "Reinhard05") {
                m_Ui->stackedWidget_operators->setCurrentWidget(m_Ui->page_reinhard05);
            }
        } else if (field == "CONTRASTFACTOR") {
            m_Ui->contrastFactorSlider->setValue(contrastfactorGang->v2p(value.toFloat()));
        } else if (field == "SATURATIONFACTOR") {
            m_Ui->saturationFactorSlider->setValue(saturationfactorGang->v2p(value.toFloat()));
        } else if (field == "DETAILFACTOR") {
            m_Ui->detailFactorSlider->setValue(detailfactorGang->v2p(value.toFloat()));
        } else if (field == "CONTRASTEQUALIZATION") {
            m_Ui->contrastEqualizCheckBox->setChecked((value == "YES"));
        } else if (field == "COLORSATURATION") {
            m_Ui->contrastFactorSlider->setValue(colorSaturationGang->v2p(value.toFloat()));
        } else if (field == "CONTRASTENHANCEMENT") {
            m_Ui->saturationFactorSlider->setValue(contrastEnhancementGang->v2p(value.toFloat()));
        } else if (field == "LUMINANCELEVEL") {
            m_Ui->detailFactorSlider->setValue(luminanceLevelGang->v2p(value.toFloat()));
        } else if (field == "SIMPLE") {
            m_Ui->simpleCheckBox->setChecked((value == "YES"));
        } else if (field == "EQUATION") {
            m_Ui->eq2RadioButton->setChecked((value=="2"));
            m_Ui->eq4RadioButton->setChecked((value=="4"));
        } else if (field == "CONTRAST") {
            m_Ui->contrastSlider->setValue(contrastGang->v2p(value.toFloat()));
        } else if (field == "BIAS") {
            m_Ui->biasSlider->setValue(biasGang->v2p(value.toFloat()));
        } else if (field == "SPATIAL") {
            m_Ui->spatialSlider->setValue(spatialGang->v2p(value.toFloat()));
        } else if (field == "RANGE") {
            m_Ui->rangeSlider->setValue(rangeGang->v2p(value.toFloat()));
        } else if (field == "BASE") {
            m_Ui->baseSlider->setValue(baseGang->v2p(value.toFloat()));
        } else if (field == "ALPHA") {
            m_Ui->alphaSlider->setValue(alphaGang->v2p(value.toFloat()));
        } else if (field == "BETA") {
            m_Ui->betaSlider->setValue(betaGang->v2p(value.toFloat()));
        } else if (field == "COLOR") {
            m_Ui->saturation2Slider->setValue(saturation2Gang->v2p(value.toFloat()));
        } else if (field == "NOISE") {
            m_Ui->noiseSlider->setValue(noiseGang->v2p(value.toFloat()));
        } else if (field == "OLDFATTAL") {
            m_Ui->oldFattalCheckBox->setChecked(value == "YES");
        } else if (field == "MULTIPLIER") {
            m_Ui->multiplierSlider->setValue(multiplierGang->v2p(value.toFloat()));
        } else if (field == "LOCAL") {
            (value == "YES") ? m_Ui->pattalocal->setChecked(value == "YES") : m_Ui->pattalocal->setChecked(value=="NO");
        } else if (field == "AUTOLUMINANCE") {
            (value == "YES") ? m_Ui->autoYcheckbox->setChecked(value == "YES") : m_Ui->autoYcheckbox->setChecked(value=="NO");
        } else if (field == "CONE") {
            m_Ui->coneSlider->setValue(coneGang->v2p(value.toFloat()));
        } else if (field == "ROD") {
            m_Ui->rodSlider->setValue(rodGang->v2p(value.toFloat()));
        } else if (field == "KEY") {
            m_Ui->keySlider->setValue(keyGang->v2p(value.toFloat()));
        } else if (field == "PHI") {
            m_Ui->phiSlider->setValue(phiGang->v2p(value.toFloat()));
        } else if (field == "SCALES") {
            (value == "YES") ? m_Ui->usescalescheckbox->setChecked(value == "YES") : m_Ui->usescalescheckbox->setChecked(value=="NO");
        } else if (field == "RANGE") {
            m_Ui->range2Slider->setValue(range2Gang->v2p(value.toFloat()));
        } else if (field == "LOWER") {
            m_Ui->lowerSlider->setValue(lowerGang->v2p(value.toFloat()));
        } else if (field == "UPPER") {
            m_Ui->upperSlider->setValue(upperGang->v2p(value.toFloat()));
        } else if (field == "BRIGHTNESS") {
            m_Ui->brightnessSlider->setValue(brightnessGang->v2p(value.toFloat()));
        } else if (field == "CHROMATICADAPTATION") {
            m_Ui->chromaticAdaptSlider->setValue(chromaticGang->v2p(value.toFloat()));
        } else if (field == "LIGHTADAPTATION") {
            m_Ui->lightAdaptSlider->setValue(lightGang->v2p(value.toFloat()));
        } else if (field == "PREGAMMA") {
            m_Ui->pregammaSlider->setValue(pregammaGang->v2p(value.toFloat()));
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
        m_Ui->sizeComboBox->setCurrentIndex(m_Ui->sizeComboBox->count() - 1);
    }
}

void TonemappingPanel::fillCustomSizeComboBox()
{
    m_Ui->sizeComboBox->clear();
    for (int i = 0; i < sizes.size(); i++)
    {
        m_Ui->sizeComboBox->addItem( QString("%1x%2").arg(sizes[i]).arg( (int)(heightToWidthRatio*sizes[i]) ));
    }
}

void TonemappingPanel::setEnabled(bool b)
{
    // Operator select
    m_Ui->cmbOperators->setEnabled(b);
    m_Ui->stackedWidget_operators->setEnabled(b);

    // Load/Store/Reset
    m_Ui->loadsettingsbutton->setEnabled(b);
    m_Ui->savesettingsbutton->setEnabled(b);
    m_Ui->defaultButton->setEnabled(b);
    if (b) 
    {
    	updateUndoState();
    }
    else
    {
        m_Ui->undoButton->setEnabled(false);
        m_Ui->redoButton->setEnabled(false);
    }
    // Size
    m_Ui->sizeComboBox->setEnabled(b);
    m_Ui->addCustomSizeButton->setEnabled(b);

    // Gamma
    m_Ui->pregammadefault->setEnabled(b);
    m_Ui->pregammaSlider->setEnabled(b);
    m_Ui->pregammadsb->setEnabled(b);

    // Tonemap
    m_Ui->applyButton->setEnabled(b);

	// DB
    m_Ui->loadCommentsButton->setEnabled(b);
    m_Ui->loadButton->setEnabled(b);
    m_Ui->saveButton->setEnabled(b);
	
    //m_Ui->qualityHS->setEnabled(b);
    //m_Ui->qualitySB->setEnabled(b);

    m_Ui->replaceLdrCheckBox->setEnabled(b);

	// Labels
    m_Ui->lblOperators->setEnabled(b);
    m_Ui->groupSaveLoadTMOsetting->setEnabled(b);
    m_Ui->label->setEnabled(b);
    m_Ui->pregammaLabel->setEnabled(b);
    m_Ui->pregammaGroup->setEnabled(b);
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
    m_Ui->cmbOperators->setCurrentIndex(opts->tmoperator);
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
                m_Ui->simpleCheckBox->setChecked(simple);
				if (eq2)
                    m_Ui->eq2RadioButton->setChecked(true);
				else
                    m_Ui->eq4RadioButton->setChecked(true);
                m_Ui->contrastSlider->setValue(lct);
                m_Ui->contrastdsb->setValue(lct);
                m_Ui->pregammaSlider->setValue(pregamma);
                m_Ui->pregammadsb->setValue(pregamma);
			break;
			case drago:
				bias = model->record(selectedRow).value("bias").toFloat();
				pregamma = model->record(selectedRow).value("pregamma").toFloat();
                m_Ui->biasSlider->setValue(bias);
                m_Ui->biasdsb->setValue(bias);
                m_Ui->pregammaSlider->setValue(pregamma);
                m_Ui->pregammadsb->setValue(pregamma);
			break;
			case durand:
				spatial = model->record(selectedRow).value("spatial").toFloat();
				range = model->record(selectedRow).value("range").toFloat();
				base = model->record(selectedRow).value("base").toFloat();
				pregamma = model->record(selectedRow).value("pregamma").toFloat();
                m_Ui->spatialSlider->setValue(spatial);
                m_Ui->spatialdsb->setValue(spatial);
                m_Ui->rangeSlider->setValue(range);
                m_Ui->rangedsb->setValue(range);
                m_Ui->baseSlider->setValue(base);
                m_Ui->basedsb->setValue(base);
                m_Ui->pregammaSlider->setValue(pregamma);
                m_Ui->pregammadsb->setValue(pregamma);
			break;
			case fattal:
				alpha = model->record(selectedRow).value("alpha").toFloat();
				beta = model->record(selectedRow).value("beta").toFloat();
				colorSat = model->record(selectedRow).value("colorSaturation").toFloat();
				noiseReduction = model->record(selectedRow).value("noiseReduction").toFloat();
				oldFattal = model->record(selectedRow).value("oldFattal").toBool();
				pregamma = model->record(selectedRow).value("pregamma").toFloat();
                m_Ui->alphaSlider->setValue(alpha);
                m_Ui->alphadsb->setValue(alpha);
                m_Ui->betaSlider->setValue(beta);
                m_Ui->betadsb->setValue(beta);
                m_Ui->saturation2Slider->setValue(colorSat);
                m_Ui->saturation2dsb->setValue(colorSat);
                m_Ui->noiseSlider->setValue(noiseReduction);
                m_Ui->noisedsb->setValue(noiseReduction);
                m_Ui->oldFattalCheckBox->setChecked(oldFattal);
                m_Ui->pregammaSlider->setValue(pregamma);
                m_Ui->pregammadsb->setValue(pregamma);
			break;
    		case mantiuk06:
				contrastEqualization = model->record(selectedRow).value("contrastEqualization").toBool();		
				contrastFactor =  model->record(selectedRow).value("contrastFactor").toFloat();	
				saturationFactor =  model->record(selectedRow).value("saturationFactor").toFloat();	
				detailFactor =  model->record(selectedRow).value("detailFactor").toFloat();
				pregamma = model->record(selectedRow).value("pregamma").toFloat();
                m_Ui->contrastEqualizCheckBox->setChecked(contrastEqualization);
                m_Ui->contrastFactorSlider->setValue(contrastFactor);
                m_Ui->contrastFactordsb->setValue(contrastFactor);
                m_Ui->saturationFactorSlider->setValue(saturationFactor);
                m_Ui->saturationFactordsb->setValue(saturationFactor);
                m_Ui->detailFactorSlider->setValue(detailFactor);
                m_Ui->detailFactordsb->setValue(detailFactor);
                m_Ui->pregammaSlider->setValue(pregamma);
                m_Ui->pregammadsb->setValue(pregamma);
			break;
    		case mantiuk08:
				colorSaturation = model->record(selectedRow).value("colorSaturation").toFloat();
				contrastEnhancement = model->record(selectedRow).value("contrastEnhancement").toFloat();
				luminanceLevel = model->record(selectedRow).value("luminanceLevel").toFloat();
				manualLuminanceLevel = model->record(selectedRow).value("manualLuminanceLevel").toBool();
				pregamma = model->record(selectedRow).value("pregamma").toFloat();
                m_Ui->colorSaturationSlider->setValue(colorSaturation);
                m_Ui->colorSaturationDSB->setValue(colorSaturation);
                m_Ui->contrastEnhancementSlider->setValue(contrastEnhancement);
                m_Ui->contrastEnhancementDSB->setValue(contrastEnhancement);
                m_Ui->luminanceLevelSlider->setValue(luminanceLevel);
                m_Ui->luminanceLevelDSB->setValue(luminanceLevel);
                m_Ui->luminanceLevelCheckBox->setChecked(manualLuminanceLevel);
                m_Ui->pregammaSlider->setValue(pregamma);
                m_Ui->pregammadsb->setValue(pregamma);
			break;
			case pattanaik:
				multiplier = model->record(selectedRow).value("multiplier").toFloat();
				rod = model->record(selectedRow).value("rod").toFloat();
				cone = model->record(selectedRow).value("cone").toFloat();
				autolum = model->record(selectedRow).value("autolum").toBool();
				local = model->record(selectedRow).value("local").toBool();
				pregamma = model->record(selectedRow).value("pregamma").toFloat();
                m_Ui->multiplierSlider->setValue(multiplier);
                m_Ui->multiplierdsb->setValue(multiplier);
                m_Ui->coneSlider->setValue(cone);
                m_Ui->conedsb->setValue(cone);
                m_Ui->rodSlider->setValue(rod);
                m_Ui->roddsb->setValue(rod);
                m_Ui->pattalocal->setChecked(local);
                m_Ui->autoYcheckbox->setChecked(autolum);
                m_Ui->pregammaSlider->setValue(pregamma);
                m_Ui->pregammadsb->setValue(pregamma);
			break;
			case reinhard02:
				scales = model->record(selectedRow).value("scales").toBool();
				key = model->record(selectedRow).value("key").toFloat();
				phi = model->record(selectedRow).value("phi").toFloat();
				irange = model->record(selectedRow).value("range").toInt();
				lower = model->record(selectedRow).value("lower").toInt();
				upper = model->record(selectedRow).value("upper").toInt();
				pregamma = model->record(selectedRow).value("pregamma").toFloat();
                m_Ui->usescalescheckbox->setChecked(scales);
                m_Ui->keySlider->setValue(key);
                m_Ui->keydsb->setValue(key);
                m_Ui->phiSlider->setValue(phi);
                m_Ui->phidsb->setValue(phi);
                m_Ui->range2Slider->setValue(irange);
                m_Ui->range2dsb->setValue(irange);
                m_Ui->lowerSlider->setValue(lower);
                m_Ui->lowerdsb->setValue(lower);
                m_Ui->upperSlider->setValue(upper);
                m_Ui->upperdsb->setValue(upper);
                m_Ui->pregammaSlider->setValue(pregamma);
                m_Ui->pregammadsb->setValue(pregamma);
			break;
			case reinhard05:
				brightness = model->record(selectedRow).value("brightness").toFloat();
				chromaticAdaptation = model->record(selectedRow).value("chromaticAdaptation").toFloat();
				lightAdaptation = model->record(selectedRow).value("lightAdaptation").toFloat();
				pregamma = model->record(selectedRow).value("pregamma").toFloat();
                m_Ui->brightnessSlider->setValue(brightness);
                m_Ui->brightnessdsb->setValue(brightness);
                m_Ui->chromaticAdaptSlider->setValue(chromaticAdaptation);
                m_Ui->chromaticAdaptdsb->setValue(chromaticAdaptation);
                m_Ui->lightAdaptSlider->setValue(lightAdaptation);
                m_Ui->lightAdaptdsb->setValue(lightAdaptation);
                m_Ui->pregammaSlider->setValue(pregamma);
                m_Ui->pregammadsb->setValue(pregamma);
			break;
		}
	}
}

void TonemappingPanel::loadComments()
{
    SavedParameters dialog(this);
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
            m_Ui->stackedWidget_operators->setCurrentIndex(ashikhmin);
			updateCurrentTmoOperator(ashikhmin);
			while (query.next())
			{
                m_Ui->simpleCheckBox->setChecked(query.value(0).toBool());
				if (query.value(1).toBool())
                    m_Ui->eq2RadioButton->setChecked(true);
				else
                    m_Ui->eq4RadioButton->setChecked(true);
                m_Ui->contrastSlider->setValue(query.value(2).toFloat());
                m_Ui->contrastdsb->setValue(query.value(2).toFloat());
                m_Ui->pregammaSlider->setValue(query.value(3).toFloat());
                m_Ui->pregammadsb->setValue(query.value(3).toFloat());
			}
		}
		else if (tmOperator == "drago")
		{
            m_Ui->stackedWidget_operators->setCurrentIndex(drago);
			updateCurrentTmoOperator(drago);
			while (query.next())
			{
                m_Ui->biasSlider->setValue(query.value(0).toFloat());
                m_Ui->biasdsb->setValue(query.value(0).toFloat());
                m_Ui->pregammaSlider->setValue(query.value(1).toFloat());
                m_Ui->pregammadsb->setValue(query.value(1).toFloat());
			}
		}
		else if (tmOperator == "durand")
		{
            m_Ui->stackedWidget_operators->setCurrentIndex(durand);
			updateCurrentTmoOperator(durand);
			while (query.next())
			{
                m_Ui->spatialSlider->setValue(query.value(0).toFloat());
                m_Ui->spatialdsb->setValue(query.value(0).toFloat());
                m_Ui->rangeSlider->setValue(query.value(1).toFloat());
                m_Ui->rangedsb->setValue(query.value(1).toFloat());
                m_Ui->baseSlider->setValue(query.value(2).toFloat());
                m_Ui->basedsb->setValue(query.value(2).toFloat());
                m_Ui->pregammaSlider->setValue(query.value(3).toFloat());
                m_Ui->pregammadsb->setValue(query.value(3).toFloat());
			}
		}
		else if (tmOperator == "fattal")
		{
            m_Ui->stackedWidget_operators->setCurrentIndex(fattal);
			updateCurrentTmoOperator(fattal);
			while (query.next())
			{
                m_Ui->alphaSlider->setValue(query.value(0).toFloat());
                m_Ui->alphadsb->setValue(query.value(0).toFloat());
                m_Ui->betaSlider->setValue(query.value(1).toFloat());
                m_Ui->betadsb->setValue(query.value(1).toFloat());
                m_Ui->saturation2Slider->setValue(query.value(2).toFloat());
                m_Ui->saturation2dsb->setValue(query.value(2).toFloat());
                m_Ui->noiseSlider->setValue(query.value(3).toFloat());
                m_Ui->noisedsb->setValue(query.value(3).toFloat());
                m_Ui->oldFattalCheckBox->setChecked(query.value(4).toBool());
                m_Ui->pregammaSlider->setValue(query.value(5).toFloat());
                m_Ui->pregammadsb->setValue(query.value(5).toFloat());
			}
		}
		else if (tmOperator == "mantiuk06")
		{
            m_Ui->stackedWidget_operators->setCurrentIndex(mantiuk06);
			updateCurrentTmoOperator(mantiuk06);
			while (query.next())
			{
                m_Ui->contrastEqualizCheckBox->setChecked(query.value(0).toBool());
                m_Ui->contrastFactorSlider->setValue(query.value(1).toFloat());
                m_Ui->contrastFactordsb->setValue(query.value(1).toFloat());
                m_Ui->saturationFactorSlider->setValue(query.value(2).toFloat());
                m_Ui->saturationFactordsb->setValue(query.value(2).toFloat());
                m_Ui->detailFactorSlider->setValue(query.value(3).toFloat());
                m_Ui->detailFactordsb->setValue(query.value(3).toFloat());
                m_Ui->pregammaSlider->setValue(query.value(4).toFloat());
                m_Ui->pregammadsb->setValue(query.value(4).toFloat());
			}
		}
		else if (tmOperator == "mantiuk08")
		{
            m_Ui->stackedWidget_operators->setCurrentIndex(mantiuk08);
			updateCurrentTmoOperator(mantiuk08);
			while (query.next())
			{
                m_Ui->colorSaturationSlider->setValue(query.value(0).toFloat());
                m_Ui->colorSaturationDSB->setValue(query.value(0).toFloat());
                m_Ui->contrastEnhancementSlider->setValue(query.value(1).toFloat());
                m_Ui->contrastEnhancementDSB->setValue(query.value(1).toFloat());
                m_Ui->luminanceLevelSlider->setValue(query.value(2).toFloat());
                m_Ui->luminanceLevelDSB->setValue(query.value(2).toFloat());
                m_Ui->luminanceLevelCheckBox->setChecked(query.value(3).toBool());
                m_Ui->pregammaSlider->setValue(query.value(4).toFloat());
                m_Ui->pregammadsb->setValue(query.value(4).toFloat());
			}
		}
		else if (tmOperator == "pattanaik")
		{
            m_Ui->stackedWidget_operators->setCurrentIndex(pattanaik);
			updateCurrentTmoOperator(pattanaik);
			while (query.next())
			{
                m_Ui->multiplierSlider->setValue(query.value(0).toFloat());
                m_Ui->multiplierdsb->setValue(query.value(0).toFloat());
                m_Ui->coneSlider->setValue(query.value(1).toFloat());
                m_Ui->conedsb->setValue(query.value(1).toFloat());
                m_Ui->rodSlider->setValue(query.value(2).toFloat());
                m_Ui->roddsb->setValue(query.value(2).toFloat());
                m_Ui->pattalocal->setChecked(query.value(3).toBool());
                m_Ui->autoYcheckbox->setChecked(query.value(3).toFloat());
                m_Ui->pregammaSlider->setValue(query.value(4).toFloat());
                m_Ui->pregammadsb->setValue(query.value(4).toFloat());
			}
		}
		else if (tmOperator == "reinhard02")
		{
            m_Ui->stackedWidget_operators->setCurrentIndex(reinhard02);
			updateCurrentTmoOperator(reinhard02);
			while (query.next())
			{
                m_Ui->usescalescheckbox->setChecked(query.value(0).toBool());
                m_Ui->keySlider->setValue(query.value(1).toFloat());
                m_Ui->keydsb->setValue(query.value(1).toFloat());
                m_Ui->phiSlider->setValue(query.value(2).toFloat());
                m_Ui->phidsb->setValue(query.value(2).toFloat());
                m_Ui->range2Slider->setValue(query.value(3).toInt());
                m_Ui->range2dsb->setValue(query.value(3).toInt());
                m_Ui->lowerSlider->setValue(query.value(4).toInt());
                m_Ui->lowerdsb->setValue(query.value(4).toInt());
                m_Ui->upperSlider->setValue(query.value(5).toInt());
                m_Ui->upperdsb->setValue(query.value(5).toInt());
                m_Ui->pregammaSlider->setValue(query.value(6).toFloat());
                m_Ui->pregammadsb->setValue(query.value(6).toFloat());
			}
		}
		else if (tmOperator == "reinhard05")
		{
            m_Ui->stackedWidget_operators->setCurrentIndex(reinhard05);
			updateCurrentTmoOperator(reinhard05);
			while (query.next())
			{
                m_Ui->brightnessSlider->setValue(query.value(0).toFloat());
                m_Ui->brightnessdsb->setValue(query.value(0).toFloat());
                m_Ui->chromaticAdaptSlider->setValue(query.value(1).toFloat());
                m_Ui->chromaticAdaptdsb->setValue(query.value(1).toFloat());
                m_Ui->lightAdaptSlider->setValue(query.value(2).toFloat());
                m_Ui->lightAdaptdsb->setValue(query.value(2).toFloat());
                m_Ui->pregammaSlider->setValue(query.value(3).toFloat());
                m_Ui->pregammadsb->setValue(query.value(3).toFloat());
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
    float pregamma = m_Ui->pregammadsb->value();
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
    float pregamma = m_Ui->pregammadsb->value();
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
    float pregamma = m_Ui->pregammadsb->value();
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
    float pregamma = m_Ui->pregammadsb->value();
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
    float pregamma = m_Ui->pregammadsb->value();
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
    float pregamma = m_Ui->pregammadsb->value();
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
    float pregamma = m_Ui->pregammadsb->value();
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
    float pregamma = m_Ui->pregammadsb->value();
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
    float pregamma = m_Ui->pregammadsb->value();
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
    return m_Ui->replaceLdrCheckBox->isChecked();
}

// ------------------------- // END FILE


