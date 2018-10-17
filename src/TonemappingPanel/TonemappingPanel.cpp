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

#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlTableModel>
#include <QStatusBar>
#include <QTextStream>

#include <Common/LuminanceOptions.h>
#include <Common/config.h>
#include <PreviewPanel/PreviewLabel.h>
#include <TonemappingOperators/pfstmdefaultparams.h>
#include <TonemappingPanel/SavingParametersDialog.h>
#include <TonemappingPanel/TonemappingPanel.h>
#include <TonemappingPanel/TonemappingSettings.h>
#include <TonemappingPanel/ui_TonemappingPanel.h>
#include <UI/Gang.h>

#include <contrib/qtwaitingspinner/QtWaitingSpinner.h>

int TonemappingPanel::sm_counter = 0;

TonemappingPanel::TonemappingPanel(int mainWinNumber, PreviewPanel *panel,
                                   QWidget *parent)
    : QWidget(parent),
      adding_custom_size(false),
      m_previewPanel(panel),
      m_mainWinNumber(mainWinNumber),
      m_autolevelThreshold(0.985f),
      m_thd(new ThresholdWidget(this)),
      m_Ui(new Ui::TonemappingPanel) {
    m_Ui->setupUi(this);

    sm_counter++;

    m_databaseconnection = QStringLiteral("connection_") + QString("%1").arg(sm_counter);

    connect(m_thd.data(), &ThresholdWidget::ready, this,
            &TonemappingPanel::thresholdReady);

    if (!QIcon::hasThemeIcon(QStringLiteral("edit-download")))
        m_Ui->saveButton->setIcon(QIcon(":/program-icons/edit-download"));
    if (!QIcon::hasThemeIcon(QStringLiteral("cloud-upload")))
        m_Ui->loadButton->setIcon(QIcon(":/program-icons/cloud-upload"));

    m_currentTmoOperator = mantiuk06;  // from Qt Designer

    // mantiuk06
    contrastfactorGang =
        new Gang(m_Ui->contrastFactorSlider, m_Ui->contrastFactordsb,
                 m_Ui->contrastEqualizCheckBox, NULL, NULL, NULL,
                 0.01f /*0.001f*/, 1.0f, MANTIUK06_CONTRAST_FACTOR);

    connect(contrastfactorGang, &Gang::enableUndo, m_Ui->undoButton,
            &QWidget::setEnabled);
    connect(contrastfactorGang, &Gang::enableRedo, m_Ui->redoButton,
            &QWidget::setEnabled);

    saturationfactorGang =
        new Gang(m_Ui->saturationFactorSlider, m_Ui->saturationFactordsb, NULL,
                 NULL, NULL, NULL, 0.0f, 2.0f, MANTIUK06_SATURATION_FACTOR);
    detailfactorGang =
        new Gang(m_Ui->detailFactorSlider, m_Ui->detailFactordsb, NULL, NULL,
                 NULL, NULL, 1.0f, 99.0f, MANTIUK06_DETAIL_FACTOR);

    // mantiuk08
    colorSaturationGang =
        new Gang(m_Ui->colorSaturationSlider, m_Ui->colorSaturationDSB, NULL,
                 NULL, NULL, NULL, 0.f, 2.f, MANTIUK08_COLOR_SATURATION);

    connect(colorSaturationGang, &Gang::enableUndo, m_Ui->undoButton,
            &QWidget::setEnabled);
    connect(colorSaturationGang, &Gang::enableRedo, m_Ui->redoButton,
            &QWidget::setEnabled);

    contrastEnhancementGang = new Gang(
        m_Ui->contrastEnhancementSlider, m_Ui->contrastEnhancementDSB, NULL,
        NULL, NULL, NULL, .01f, 10.f, MANTIUK08_CONTRAST_ENHANCEMENT);
    luminanceLevelGang =
        new Gang(m_Ui->luminanceLevelSlider, m_Ui->luminanceLevelDSB,
                 m_Ui->luminanceLevelCheckBox, NULL, NULL, NULL, 1.f, 100.0f,
                 MANTIUK08_LUMINANCE_LEVEL);

    // fattal02
    alphaGang = new Gang(m_Ui->alphaSlider, m_Ui->alphadsb, NULL, NULL, NULL,
                         NULL, 0.05, 2.f, FATTAL02_ALPHA, true);

    connect(alphaGang, &Gang::enableUndo, m_Ui->undoButton,
            &QWidget::setEnabled);
    connect(alphaGang, &Gang::enableRedo, m_Ui->redoButton,
            &QWidget::setEnabled);

    betaGang = new Gang(m_Ui->betaSlider, m_Ui->betadsb, NULL, NULL, NULL, NULL,
                        0.1f, 2.f, FATTAL02_BETA);
    saturation2Gang =
        new Gang(m_Ui->saturation2Slider, m_Ui->saturation2dsb, NULL, NULL,
                 NULL, NULL, 0.f, 1.5f, FATTAL02_COLOR);
    noiseGang = new Gang(m_Ui->noiseSlider, m_Ui->noisedsb, NULL, NULL, NULL,
                         NULL, 0, 1.f, FATTAL02_NOISE_REDUX);
    // oldFattalGang = new Gang(NULL,NULL, m_Ui->oldFattalCheckBox);
    fftSolverGang = new Gang(NULL, NULL, m_Ui->fftVersionCheckBox);

    // ferradans11
    rhoGang = new Gang(m_Ui->rhoSlider, m_Ui->rhodsb, NULL, NULL, NULL, NULL,
                       -10.f, 10.f, FERRADANS11_RHO);

    connect(rhoGang, &Gang::enableUndo, m_Ui->undoButton, &QWidget::setEnabled);
    connect(rhoGang, &Gang::enableRedo, m_Ui->redoButton, &QWidget::setEnabled);

    inv_alphaGang =
        new Gang(m_Ui->inv_alphaSlider, m_Ui->inv_alphadsb, NULL, NULL, NULL,
                 NULL, 0.1f, 10.f, FERRADANS11_INV_ALPHA);

    // ashikhmin02
    contrastGang = new Gang(m_Ui->contrastSlider, m_Ui->contrastdsb, NULL, NULL,
                            NULL, NULL, 0.f, 1.f, 0.5f);

    connect(contrastGang, &Gang::enableUndo, m_Ui->undoButton,
            &QWidget::setEnabled);
    connect(contrastGang, &Gang::enableRedo, m_Ui->redoButton,
            &QWidget::setEnabled);

    simpleGang = new Gang(NULL, NULL, m_Ui->simpleCheckBox);
    eq2Gang = new Gang(NULL, NULL, NULL, NULL, m_Ui->eq2RadioButton,
                       m_Ui->eq4RadioButton);

    // drago03
    biasGang = new Gang(m_Ui->biasSlider, m_Ui->biasdsb, NULL, NULL, NULL, NULL,
                        0.f, 1.f, DRAGO03_BIAS);

    connect(biasGang, &Gang::enableUndo, m_Ui->undoButton,
            &QWidget::setEnabled);
    connect(biasGang, &Gang::enableRedo, m_Ui->redoButton,
            &QWidget::setEnabled);

    // durand02
    spatialGang = new Gang(m_Ui->spatialSlider, m_Ui->spatialdsb, NULL, NULL,
                           NULL, NULL, 0.f, 100.f, DURAND02_SPATIAL);

    connect(spatialGang, &Gang::enableUndo, m_Ui->undoButton,
            &QWidget::setEnabled);
    connect(spatialGang, &Gang::enableRedo, m_Ui->redoButton,
            &QWidget::setEnabled);

    rangeGang = new Gang(m_Ui->rangeSlider, m_Ui->rangedsb, NULL, NULL, NULL,
                         NULL, 0.01f, 10.f, DURAND02_RANGE);
    baseGang = new Gang(m_Ui->baseSlider, m_Ui->basedsb, NULL, NULL, NULL, NULL,
                        0.f, 10.f, DURAND02_BASE);

    // pattanaik00
    multiplierGang =
        new Gang(m_Ui->multiplierSlider, m_Ui->multiplierdsb, NULL, NULL, NULL,
                 NULL, 1e-3, 1000.f, PATTANAIK00_MULTIPLIER, true);

    connect(multiplierGang, &Gang::enableUndo, m_Ui->undoButton,
            &QWidget::setEnabled);
    connect(multiplierGang, &Gang::enableRedo, m_Ui->redoButton,
            &QWidget::setEnabled);

    coneGang = new Gang(m_Ui->coneSlider, m_Ui->conedsb, NULL, NULL, NULL, NULL,
                        1e-3, 1.f, PATTANAIK00_CONE, true);
    rodGang = new Gang(m_Ui->rodSlider, m_Ui->roddsb, NULL, NULL, NULL, NULL,
                       1e-3, 1.f, PATTANAIK00_ROD, true);
    autoYGang = new Gang(NULL, NULL, m_Ui->autoYcheckbox);
    pattalocalGang = new Gang(NULL, NULL, m_Ui->pattalocal);

    // reinhard02
    keyGang = new Gang(m_Ui->keySlider, m_Ui->keydsb, NULL, NULL, NULL, NULL,
                       0.f, 1.f, 0.18f);

    connect(keyGang, &Gang::enableUndo, m_Ui->undoButton, &QWidget::setEnabled);
    connect(keyGang, &Gang::enableRedo, m_Ui->redoButton, &QWidget::setEnabled);

    phiGang = new Gang(m_Ui->phiSlider, m_Ui->phidsb, NULL, NULL, NULL, NULL,
                       0.f, 100.f, REINHARD02_PHI);
    range2Gang = new Gang(m_Ui->range2Slider, m_Ui->range2dsb, NULL, NULL, NULL,
                          NULL, 1.f, 32.f, REINHARD02_RANGE);
    lowerGang = new Gang(m_Ui->lowerSlider, m_Ui->lowerdsb, NULL, NULL, NULL,
                         NULL, 1.f, 100.f, REINHARD02_LOWER);
    upperGang = new Gang(m_Ui->upperSlider, m_Ui->upperdsb, NULL, NULL, NULL,
                         NULL, 1.f, 100.f, REINHARD02_UPPER);
    usescalesGang = new Gang(NULL, NULL, m_Ui->usescalescheckbox);

    // reinhard05
    brightnessGang =
        new Gang(m_Ui->brightnessSlider, m_Ui->brightnessdsb, NULL, NULL, NULL,
                 NULL, -20.f, 20.f, REINHARD05_BRIGHTNESS);

    connect(brightnessGang, &Gang::enableUndo, m_Ui->undoButton,
            &QWidget::setEnabled);
    connect(brightnessGang, &Gang::enableRedo, m_Ui->redoButton,
            &QWidget::setEnabled);

    chromaticGang =
        new Gang(m_Ui->chromaticAdaptSlider, m_Ui->chromaticAdaptdsb, NULL,
                 NULL, NULL, NULL, 0.f, 1.f, REINHARD05_CHROMATIC_ADAPTATION);
    lightGang =
        new Gang(m_Ui->lightAdaptSlider, m_Ui->lightAdaptdsb, NULL, NULL, NULL,
                 NULL, 0.f, 1.f, REINHARD05_LIGHT_ADAPTATION);

    // ferwerda96
    ferwerdamultiplierGang =
        new Gang(m_Ui->ferwerdaMultiplierSlider, m_Ui->ferwerdaMultiplierDsb, NULL, NULL, NULL,
                 NULL, 0.001f, 2.f, FERWERDA96_MULTIPLIER);

    connect(ferwerdamultiplierGang, &Gang::enableUndo, m_Ui->undoButton,
            &QWidget::setEnabled);
    connect(ferwerdamultiplierGang, &Gang::enableRedo, m_Ui->redoButton,
            &QWidget::setEnabled);

    adaptationGang =
        new Gang(m_Ui->adaptationLuminanceSlider, m_Ui->adaptationLuminanceDsb, NULL,
                 NULL, NULL, NULL, 0.001f, 1.f, FERWERDA96_ADAPTATION_LUMINANCE);

    // kimkautz08
    kimkautzc1Gang =
        new Gang(m_Ui->kimkautzC1Slider, m_Ui->kimkautzC1Dsb, NULL,
                 NULL, NULL, NULL, 0.001f, 10.f, KIMKAUTZ08_C1);

    connect(kimkautzc1Gang, &Gang::enableUndo, m_Ui->undoButton,
            &QWidget::setEnabled);
    connect(kimkautzc1Gang, &Gang::enableRedo, m_Ui->redoButton,
            &QWidget::setEnabled);

    kimkautzc2Gang =
        new Gang(m_Ui->kimkautzC2Slider, m_Ui->kimkautzC2Dsb, NULL,
                 NULL, NULL, NULL, 0.001f, 5.f, KIMKAUTZ08_C2);

    // vanhateren06
    vanhaterenPupilAreaGang =
        new Gang(m_Ui->pupil_areaSlider, m_Ui->pupil_areaDsb, NULL,
                 NULL, NULL, NULL, 0.0f, 100.f, VANHATEREN06_PUPIL_AREA);

    connect(vanhaterenPupilAreaGang, &Gang::enableUndo, m_Ui->undoButton,
            &QWidget::setEnabled);

    // pregamma
    pregammaGang = new Gang(m_Ui->pregammaSlider, m_Ui->pregammadsb, NULL, NULL,
                            NULL, NULL, 0.1f, 5.f, 1.0f, true);

    // postgamma
    postgammaGang = new Gang(m_Ui->postgammaSlider, m_Ui->postgammadsb, NULL, NULL,
                            NULL, NULL, 0.1f, 5.f, 1.0f, true);

    // postsaturation
    postsaturationGang = new Gang(m_Ui->postsaturationSlider, m_Ui->postsaturationdsb, NULL, NULL,
                            NULL, NULL, 0.1f, 5.f, 1.0f, true);

    //--
    connect(m_Ui->stackedWidget_operators, &QStackedWidget::currentChanged,
            this, &TonemappingPanel::updateCurrentTmoOperator);

    connect(m_Ui->loadButton, &QAbstractButton::clicked, this,
            &TonemappingPanel::loadParameters);
    connect(m_Ui->saveButton, &QAbstractButton::clicked, this,
            &TonemappingPanel::saveParameters);

    connect(m_Ui->autoLevelsCheckBox, SIGNAL(toggled(bool)), this,
            SLOT(autoLevels(bool)));

    m_spinner = new QtWaitingSpinner(this);

    m_spinner->setRoundness(70.0);
    m_spinner->setMinimumTrailOpacity(15.0);
    m_spinner->setTrailFadePercentage(70.0);
    m_spinner->setNumberOfLines(12);
    m_spinner->setLineLength(5);
    m_spinner->setLineWidth(2);
    m_spinner->setInnerRadius(5);
    m_spinner->setRevolutionsPerSecond(1);

    m_spinner->start();

    m_Ui->frameSpinner->addWidget(m_spinner);
    m_spinner->setVisible(false);

    createDatabase();
}

void TonemappingPanel::setExportQueueSize(int size) {
    m_spinner->setVisible(size > 0);
    m_Ui->lblQueueSize->setText(QString(tr("Queue size: %1")).arg(size));
}

TonemappingPanel::~TonemappingPanel() {
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
    delete rhoGang;
    delete inv_alphaGang;
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
    delete ferwerdamultiplierGang;
    delete adaptationGang;
    delete kimkautzc1Gang;
    delete kimkautzc2Gang;
    delete vanhaterenPupilAreaGang;
    delete pregammaGang;
    delete postgammaGang;
    delete postsaturationGang;
    delete m_spinner;

    qDeleteAll(m_toneMappingOptionsToDelete);

    if (m_mainWinNumber == 0) {
        QSqlDatabase db = QSqlDatabase::database(m_databaseconnection);
        db.close();
    }
}

void TonemappingPanel::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange) m_Ui->retranslateUi(this);
    QWidget::changeEvent(event);
}

void TonemappingPanel::createDatabase() {
    LuminanceOptions options;

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
            m_databaseconnection);
    db.setDatabaseName(options.getDatabaseFileName());
    db.setHostName(QStringLiteral("localhost"));
    bool ok = db.open();
    if (!ok) {
        QMessageBox::warning(
            this, tr("TM Database Problem"),
            tr("The database used for saving TM parameters cannot be opened.\n"
               "Error: %1")
                .arg(db.lastError().databaseText()),
            QMessageBox::Ok, QMessageBox::NoButton);
        return;
    }
    QSqlQuery query(db);
    // Mantiuk 06
    bool res = query.exec(QStringLiteral(
        " CREATE TABLE IF NOT EXISTS mantiuk06 (contrastEqualization boolean \
        NOT \
        NULL, contrastFactor real, saturationFactor real, detailFactor real, \
        pregamma real, comment varchar(150), postsaturation real, postgamma real);"));
    if (res == false) qDebug() << query.lastError();

    res = query.exec(QStringLiteral(
                " SELECT postsaturation FROM mantiuk06; "));
    if (res == false) {
        res = query.exec(QStringLiteral(
                " ALTER TABLE mantiuk06 ADD COLUMN postsaturation real NOT NULL DEFAULT 1;"));
        res = query.exec(QStringLiteral(
                " ALTER TABLE mantiuk06 ADD COLUMN postgamma real NOT NULL DEFAULT 1;"));
    }
    // Mantiuk 08
    res = query.exec(QStringLiteral(
        " CREATE TABLE IF NOT EXISTS mantiuk08 (colorSaturation real, \
        contrastEnhancement real, luminanceLevel real, manualLuminanceLevel \
        boolean NOT NULL, pregamma real, comment varchar(150), \
        postsaturation real, postgamma real);"));
    if (res == false) qDebug() << query.lastError();

    res = query.exec(QStringLiteral(
                " SELECT postsaturation FROM mantiuk08; "));
    if (res == false) {
        res = query.exec(QStringLiteral(
                " ALTER TABLE mantiuk08 ADD COLUMN postsaturation real NOT NULL DEFAULT 1;"));
        res = query.exec(QStringLiteral(
                " ALTER TABLE mantiuk08 ADD COLUMN postgamma real NOT NULL DEFAULT 1;"));
    }
    // Ashikhmin
    res = query.exec(QStringLiteral(
        " CREATE TABLE IF NOT EXISTS ashikhmin (simple boolean NOT NULL, eq2 \
        boolean NOT NULL, lct real, pregamma real, comment varchar(150), postsaturation real, postgamma real);"));
    if (res == false) qDebug() << query.lastError();

    res = query.exec(QStringLiteral(
                " SELECT postsaturation FROM ashikhmin; "));
    if (res == false) {
        res = query.exec(QStringLiteral(
                " ALTER TABLE ashikhmin ADD COLUMN postsaturation real NOT NULL DEFAULT 1;"));
        res = query.exec(QStringLiteral(
                " ALTER TABLE ashikhmin ADD COLUMN postgamma real NOT NULL DEFAULT 1;"));
    }
    // Drago
    res = query.exec(
        QStringLiteral(" CREATE TABLE IF NOT EXISTS drago (bias real, \
                       pregamma real, comment varchar(150), postsaturation real, postgamma real);"));
    if (res == false) qDebug() << query.lastError();

    res = query.exec(QStringLiteral(
                " SELECT postsaturation FROM drago; "));
    if (res == false) {
        res = query.exec(QStringLiteral(
                " ALTER TABLE drago ADD COLUMN postsaturation real NOT NULL DEFAULT 1;"));
        res = query.exec(QStringLiteral(
                " ALTER TABLE drago ADD COLUMN postgamma real NOT NULL DEFAULT 1;"));
    }
    // Durand
    res = query.exec(QStringLiteral(
        " CREATE TABLE IF NOT EXISTS durand (spatial real, range \
        real, base real, pregamma real, comment varchar(150), postsaturation real, postgamma real);"));
    if (res == false) qDebug() << query.lastError();

    res = query.exec(QStringLiteral(
                " SELECT postsaturation FROM durand; "));
    if (res == false) {
        res = query.exec(QStringLiteral(
                " ALTER TABLE durand ADD COLUMN postsaturation real NOT NULL DEFAULT 1;"));
        res = query.exec(QStringLiteral(
                " ALTER TABLE durand ADD COLUMN postgamma real NOT NULL DEFAULT 1;"));
    }
    // Fattal
    res = query.exec(QStringLiteral(
        " CREATE TABLE IF NOT EXISTS fattal (alpha real, beta real, \
        colorSaturation real, noiseReduction real, oldFattal boolean NOT \
        NULL, \
        pregamma real, comment varchar(150), postsaturation real, postgamma real);"));
    if (res == false) qDebug() << query.lastError();

    res = query.exec(QStringLiteral(
                " SELECT postsaturation FROM fattal; "));
    if (res == false) {
        res = query.exec(QStringLiteral(
                " ALTER TABLE fattal ADD COLUMN postsaturation real NOT NULL DEFAULT 1;"));
        res = query.exec(QStringLiteral(
                " ALTER TABLE fattal ADD COLUMN postgamma real NOT NULL DEFAULT 1;"));
    }
    // Ferradans
    res = query.exec(QStringLiteral(
        " CREATE TABLE IF NOT EXISTS ferradans (rho real, \
        inv_alpha real, pregamma real, comment varchar(150), postsaturation real, postgamma real);"));
    if (res == false) qDebug() << query.lastError();

    res = query.exec(QStringLiteral(
                " SELECT postsaturation FROM ferradans; "));
    if (res == false) {
        res = query.exec(QStringLiteral(
                " ALTER TABLE ferradans ADD COLUMN postsaturation real NOT NULL DEFAULT 1;"));
        res = query.exec(QStringLiteral(
                " ALTER TABLE ferradans ADD COLUMN postgamma real NOT NULL DEFAULT 1;"));
    }
    // Ferwerda
    res = query.exec(QStringLiteral(
        " CREATE TABLE IF NOT EXISTS ferwerda (maxlum real, \
        adaptlum real, pregamma real, comment varchar(150), postsaturation real, postgamma real);"));
    if (res == false) qDebug() << query.lastError();

    res = query.exec(QStringLiteral(
                " SELECT postsaturation FROM ferwerda; "));
    if (res == false) {
        res = query.exec(QStringLiteral(
                " ALTER TABLE ferwerda ADD COLUMN postsaturation real NOT NULL DEFAULT 1;"));
        res = query.exec(QStringLiteral(
                " ALTER TABLE ferwerda ADD COLUMN postgamma real NOT NULL DEFAULT 1;"));
    }
    // KimKautz
    res = query.exec(QStringLiteral(
        " CREATE TABLE IF NOT EXISTS kimkautz (\
        kk_c1 real, kk_c2 real, pregamma real, comment varchar(150), postsaturation real, postgamma real);"));
    if (res == false) qDebug() << query.lastError();

    res = query.exec(QStringLiteral(
                " SELECT postsaturation FROM kimkautz; "));
    if (res == false) {
        res = query.exec(QStringLiteral(
                " ALTER TABLE kimkautz ADD COLUMN postsaturation real NOT NULL DEFAULT 1;"));
        res = query.exec(QStringLiteral(
                " ALTER TABLE kimkautz ADD COLUMN postgamma real NOT NULL DEFAULT 1;"));
    }
    // Pattanaik
    res = query.exec(QStringLiteral(
        " CREATE TABLE IF NOT EXISTS pattanaik (autolum boolean \
        NOT NULL, local boolean NOT NULL, cone real, rod real, \
        multiplier real, pregamma real, comment varchar(150), postsaturation real, postgamma real);"));
    if (res == false) qDebug() << query.lastError();

    res = query.exec(QStringLiteral(
                " SELECT postsaturation FROM pattanaik; "));
    if (res == false) {
        res = query.exec(QStringLiteral(
                " ALTER TABLE pattanaik ADD COLUMN postsaturation real NOT NULL DEFAULT 1;"));
        res = query.exec(QStringLiteral(
                " ALTER TABLE pattanaik ADD COLUMN postgamma real NOT NULL DEFAULT 1;"));
    }
    // Reinhard02
    res = query.exec(
        QStringLiteral(" CREATE TABLE IF NOT EXISTS reinhard02 (scales boolean \
                       NOT NULL, key real, phi real, range int, lower int, \
                       upper int, pregamma real, comment varchar(150), postsaturation real, postgamma real);"));
    if (res == false) qDebug() << query.lastError();

    res = query.exec(QStringLiteral(
                " SELECT postsaturation FROM reinhard02; "));
    if (res == false) {
        res = query.exec(QStringLiteral(
                " ALTER TABLE reinhard02 ADD COLUMN postsaturation real NOT NULL DEFAULT 1;"));
        res = query.exec(QStringLiteral(
                " ALTER TABLE reinhard02 ADD COLUMN postgamma real NOT NULL DEFAULT 1;"));
    }
    // Reinhard05
    res = query.exec(QStringLiteral(
        " CREATE TABLE IF NOT EXISTS reinhard05 (brightness real, \
        chromaticAdaptation real, lightAdaptation real, pregamma \
        real, comment varchar(150), postsaturation real, postgamma real);"));
    if (res == false) qDebug() << query.lastError();

    res = query.exec(QStringLiteral(
                " SELECT postsaturation FROM reinhard05; "));
    if (res == false) {
        res = query.exec(QStringLiteral(
                " ALTER TABLE reinhard05 ADD COLUMN postsaturation real NOT NULL DEFAULT 1;"));
        res = query.exec(QStringLiteral(
                " ALTER TABLE reinhard05 ADD COLUMN postgamma real NOT NULL DEFAULT 1;"));
    }
    // VanHateren
    res = query.exec(QStringLiteral(
        " CREATE TABLE IF NOT EXISTS vanhateren (\
        pupil_area real, pregamma real, comment varchar(150), postsaturation real, postgamma real);"));
    if (res == false) qDebug() << query.lastError();

    res = query.exec(QStringLiteral(
                " SELECT postsaturation FROM vanhateren; "));
    if (res == false) {
        res = query.exec(QStringLiteral(
                " ALTER TABLE vanhateren ADD COLUMN postsaturation real NOT NULL DEFAULT 1;"));
        res = query.exec(QStringLiteral(
                " ALTER TABLE vanhateren ADD COLUMN postgamma real NOT NULL DEFAULT 1;"));
    }
    // Hdr creation custom config parameters
    res = query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS parameters (weight integer, response \
        integer, model integer, filename varchar(150));"));
    if (res == false) qDebug() << query.lastError();
}

void TonemappingPanel::setSizes(int width, int height) {
    sizes.resize(0);
    for (int x = 256; x <= width; x *= 2) {
        if (x >= width) break;
        sizes.push_front(x);
        if (3 * (x / 2) >= width) break;
        sizes.push_front(3 * (x / 2));
    }
    sizes.push_front(width);
    heightToWidthRatio = ((float)height) / ((float)width);
    fillCustomSizeComboBox();
    m_Ui->sizeComboBox->setCurrentIndex(m_Ui->sizeComboBox->count() - 1);
}

void TonemappingPanel::on_defaultButton_clicked() {
    switch (m_currentTmoOperator) {
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
            fftSolverGang->setDefault();
            m_Ui->fftVersionCheckBox->setChecked(true);
            break;
        case ferradans:
            rhoGang->setDefault();
            inv_alphaGang->setDefault();
            break;
        case mai:  // no options
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
        case ferwerda:
            ferwerdamultiplierGang->setDefault();
            adaptationGang->setDefault();
            break;
        case kimkautz:
            kimkautzc1Gang->setDefault();
            kimkautzc2Gang->setDefault();
            break;
        case vanhateren:
            vanhaterenPupilAreaGang->setDefault();
            break;
    }
}

// TODO : if you change the position of the operator inside the TMOperator enum
// you will screw up this function!!!
void TonemappingPanel::updateCurrentTmoOperator(int idx) {
    m_currentTmoOperator = TMOperator(idx);
    updateUndoState();
}

void TonemappingPanel::updateUndoState() {
    switch (m_currentTmoOperator) {
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
        case ferradans:
            rhoGang->updateUndoState();
            break;
        case mai:  // no options
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
        case ferwerda:
            ferwerdamultiplierGang->updateUndoState();
            break;
        case kimkautz:
            kimkautzc1Gang->updateUndoState();
            break;
        case vanhateren:
            vanhaterenPupilAreaGang->updateUndoState();
            break;
    }
}

void TonemappingPanel::on_pregammadefault_clicked() {
    pregammaGang->setDefault();
}

void TonemappingPanel::on_postgammadefault_clicked() {
    postgammaGang->setDefault();
}

void TonemappingPanel::on_postsaturationdefault_clicked() {
    postsaturationGang->setDefault();
}

void TonemappingPanel::on_applyButton_clicked() {
    fillToneMappingOptions(false);
    setupUndo();

    emit startTonemapping(m_toneMappingOptions);
}

void TonemappingPanel::on_queueButton_clicked() {
    fillToneMappingOptions(true);
    // setupUndo();

    emit startExport(m_toneMappingOptions);
}

void TonemappingPanel::fillToneMappingOptions(bool exportMode) {
    m_toneMappingOptions = new TonemappingOptions;
    if (!exportMode) {
        m_toneMappingOptionsToDelete.push_back(m_toneMappingOptions);
    }
    if (!exportMode && sizes.size()) {
        m_toneMappingOptions->origxsize = sizes[0];
        m_toneMappingOptions->xsize = sizes[m_Ui->sizeComboBox->currentIndex()];
    } else {
        m_toneMappingOptions->origxsize = 0;
        m_toneMappingOptions->xsize = 0;
    }
    m_toneMappingOptions->pregamma = pregammaGang->v();
    m_toneMappingOptions->postgamma = postgammaGang->v();
    m_toneMappingOptions->postsaturation = postsaturationGang->v();
    // toneMappingOptions->tonemapSelection = checkBoxSelection->isChecked();
    // toneMappingOptions->tonemapOriginal = checkBoxOriginal->isChecked();
    switch (m_currentTmoOperator) {
        case ashikhmin:
            m_toneMappingOptions->tmoperator = ashikhmin;
            m_toneMappingOptions->operator_options.ashikhminoptions.simple =
                simpleGang->isCheckBox1Checked();
            m_toneMappingOptions->operator_options.ashikhminoptions.eq2 =
                eq2Gang->isRadioButton1Checked();
            m_toneMappingOptions->operator_options.ashikhminoptions.lct =
                contrastGang->v();
            break;
        case drago:
            m_toneMappingOptions->tmoperator = drago;
            m_toneMappingOptions->operator_options.dragooptions.bias =
                biasGang->v();
            break;
        case durand:
            m_toneMappingOptions->tmoperator = durand;
            m_toneMappingOptions->operator_options.durandoptions.spatial =
                spatialGang->v();
            m_toneMappingOptions->operator_options.durandoptions.range =
                rangeGang->v();
            m_toneMappingOptions->operator_options.durandoptions.base =
                baseGang->v();
            break;
        case fattal:
            m_toneMappingOptions->tmoperator = fattal;
            m_toneMappingOptions->operator_options.fattaloptions.alpha =
                alphaGang->v();
            m_toneMappingOptions->operator_options.fattaloptions.beta =
                betaGang->v();
            m_toneMappingOptions->operator_options.fattaloptions.color =
                saturation2Gang->v();
            m_toneMappingOptions->operator_options.fattaloptions.noiseredux =
                noiseGang->v();
            //        m_toneMappingOptions->operator_options.fattaloptions.newfattal=!oldFattalGang->isCheckBox1Checked();
            m_toneMappingOptions->operator_options.fattaloptions.fftsolver =
                fftSolverGang->isCheckBox1Checked();
            break;
        case ferradans:
            m_toneMappingOptions->tmoperator = ferradans;
            m_toneMappingOptions->operator_options.ferradansoptions.rho =
                rhoGang->v();
            m_toneMappingOptions->operator_options.ferradansoptions.inv_alpha =
                inv_alphaGang->v();
            break;
        case mai:
            m_toneMappingOptions->tmoperator = mai;
            break;
        case mantiuk06:
            m_toneMappingOptions->tmoperator = mantiuk06;
            m_toneMappingOptions->operator_options.mantiuk06options
                .contrastfactor = contrastfactorGang->v();
            m_toneMappingOptions->operator_options.mantiuk06options
                .saturationfactor = saturationfactorGang->v();
            m_toneMappingOptions->operator_options.mantiuk06options.detailfactor =
                detailfactorGang->v();
            m_toneMappingOptions->operator_options.mantiuk06options
                .contrastequalization =
                contrastfactorGang->isCheckBox1Checked();
            break;
        case mantiuk08:
            m_toneMappingOptions->tmoperator = mantiuk08;
            m_toneMappingOptions->operator_options.mantiuk08options
                .colorsaturation = colorSaturationGang->v();
            m_toneMappingOptions->operator_options.mantiuk08options
                .contrastenhancement = contrastEnhancementGang->v();
            m_toneMappingOptions->operator_options.mantiuk08options
                .luminancelevel = luminanceLevelGang->v();
            m_toneMappingOptions->operator_options.mantiuk08options.setluminance =
                luminanceLevelGang->isCheckBox1Checked();
            break;
        case pattanaik:
            m_toneMappingOptions->tmoperator = pattanaik;
            m_toneMappingOptions->operator_options.pattanaikoptions.autolum =
                autoYGang->isCheckBox1Checked();
            m_toneMappingOptions->operator_options.pattanaikoptions.local =
                pattalocalGang->isCheckBox1Checked();
            m_toneMappingOptions->operator_options.pattanaikoptions.cone =
                coneGang->v();
            m_toneMappingOptions->operator_options.pattanaikoptions.rod =
                rodGang->v();
            m_toneMappingOptions->operator_options.pattanaikoptions.multiplier =
                multiplierGang->v();
            break;
        case reinhard02:
            m_toneMappingOptions->tmoperator = reinhard02;
            m_toneMappingOptions->operator_options.reinhard02options.scales =
                usescalesGang->isCheckBox1Checked();
            m_toneMappingOptions->operator_options.reinhard02options.key =
                keyGang->v();
            m_toneMappingOptions->operator_options.reinhard02options.phi =
                phiGang->v();
            m_toneMappingOptions->operator_options.reinhard02options.range =
                (int)range2Gang->v();
            m_toneMappingOptions->operator_options.reinhard02options.lower =
                (int)lowerGang->v();
            m_toneMappingOptions->operator_options.reinhard02options.upper =
                (int)upperGang->v();
            break;
        case reinhard05:
            m_toneMappingOptions->tmoperator = reinhard05;
            m_toneMappingOptions->operator_options.reinhard05options.brightness =
                brightnessGang->v();
            m_toneMappingOptions->operator_options.reinhard05options
                .chromaticAdaptation = chromaticGang->v();
            m_toneMappingOptions->operator_options.reinhard05options
                .lightAdaptation = lightGang->v();
            break;
        case ferwerda:
            m_toneMappingOptions->tmoperator = ferwerda;
            m_toneMappingOptions->operator_options.ferwerdaoptions.multiplier =
                ferwerdamultiplierGang->v();
            m_toneMappingOptions->operator_options.ferwerdaoptions
                .adaptationluminance = adaptationGang->v();
            break;
        case kimkautz:
            m_toneMappingOptions->tmoperator = kimkautz;
            m_toneMappingOptions->operator_options.kimkautzoptions.c1 =
                kimkautzc1Gang->v();
            m_toneMappingOptions->operator_options.kimkautzoptions.c2 =
                kimkautzc2Gang->v();
            break;
        case vanhateren:
            m_toneMappingOptions->tmoperator = vanhateren;
            m_toneMappingOptions->operator_options.vanhaterenoptions.pupil_area =
                vanhaterenPupilAreaGang->v();
            break;
    }
}

void TonemappingPanel::setupUndo() {
    switch (m_currentTmoOperator) {
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
            //        oldFattalGang->setupUndo();
            fftSolverGang->setupUndo();
            break;
        case ferradans:
            rhoGang->setupUndo();
            inv_alphaGang->setupUndo();
            break;
        case mai:  // no options
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
        case ferwerda:
            ferwerdamultiplierGang->setupUndo();
            adaptationGang->setupUndo();
            break;
        case kimkautz:
            kimkautzc1Gang->setupUndo();
            kimkautzc2Gang->setupUndo();
            break;
        case vanhateren:
            vanhaterenPupilAreaGang->setupUndo();
            break;
    }
}

void TonemappingPanel::on_undoButton_clicked() { onUndoRedo(true); }

void TonemappingPanel::on_redoButton_clicked() { onUndoRedo(false); }

void TonemappingPanel::onUndoRedo(bool undo) {
    typedef void (Gang::*REDO_UNDO)();
    REDO_UNDO redoUndo = undo ? &Gang::undo : &Gang::redo;
    switch (m_currentTmoOperator) {
        case ashikhmin:
            (simpleGang->*redoUndo)();
            (eq2Gang->*redoUndo)();
            (contrastGang->*redoUndo)();
            break;
        case drago:
            (biasGang->*redoUndo)();
            break;
        case durand:
            (spatialGang->*redoUndo)();
            (rangeGang->*redoUndo)();
            (baseGang->*redoUndo)();
            break;
        case fattal:
            (alphaGang->*redoUndo)();
            (betaGang->*redoUndo)();
            (saturation2Gang->*redoUndo)();
            (noiseGang->*redoUndo)();
            //      (oldFattalGang->*redoUndo)();
            (fftSolverGang->*redoUndo)();
            break;
        case ferradans:
            (rhoGang->*redoUndo)();
            (inv_alphaGang->*redoUndo)();
            break;
        case mai:
            break;
        case mantiuk06:
            (contrastfactorGang->*redoUndo)();
            (saturationfactorGang->*redoUndo)();
            (detailfactorGang->*redoUndo)();
            break;
        case mantiuk08:
            (colorSaturationGang->*redoUndo)();
            (contrastEnhancementGang->*redoUndo)();
            (luminanceLevelGang->*redoUndo)();
            break;
        case pattanaik:
            (autoYGang->*redoUndo)();
            (pattalocalGang->*redoUndo)();
            (coneGang->*redoUndo)();
            (rodGang->*redoUndo)();
            (multiplierGang->*redoUndo)();
            break;
        case reinhard02:
            (usescalesGang->*redoUndo)();
            (keyGang->*redoUndo)();
            (phiGang->*redoUndo)();
            (range2Gang->*redoUndo)();
            (lowerGang->*redoUndo)();
            (upperGang->*redoUndo)();
            break;
        case reinhard05:
            (brightnessGang->*redoUndo)();
            (chromaticGang->*redoUndo)();
            (lightGang->*redoUndo)();
            break;
        case ferwerda:
            (ferwerdamultiplierGang->*redoUndo)();
            (adaptationGang->*redoUndo)();
            break;
        case kimkautz:
            (kimkautzc1Gang->*redoUndo)();
            (kimkautzc2Gang->*redoUndo)();
            break;
        case vanhateren:
            (vanhaterenPupilAreaGang->*redoUndo)();
            break;
    }
}

void TonemappingPanel::on_lblOpenQueue_linkActivated(const QString &link) {
    LuminanceOptions options;
    QDesktopServices::openUrl(QUrl::fromLocalFile(options.getExportDir()));
}

void TonemappingPanel::on_loadsettingsbutton_clicked() {
    LuminanceOptions lum_options;

    QString opened = QFileDialog::getOpenFileName(
        this, tr("Load a tonemapping settings text file..."),
        lum_options.getDefaultPathTmoSettings(),
        tr("LuminanceHDR tonemapping settings text file (*.txt)"));
    if (!opened.isEmpty()) {
        QFileInfo qfi(opened);
        if (!qfi.isReadable()) {
            QMessageBox::critical(
                this, tr("Aborting..."),
                tr("File is not readable (check existence, permissions,...)"),
                QMessageBox::Ok, QMessageBox::NoButton);
            return;
        }
        lum_options.setDefaultPathTmoSettings(qfi.path());

        // update filename internal field, used by parsing function
        // fromTxt2Gui()
        tmoSettingsFilename = opened;
        // call parsing function
        fromTxt2Gui();
    }
}

void TonemappingPanel::on_savesettingsbutton_clicked() {
    LuminanceOptions lum_options;

    QString fname = QFileDialog::getSaveFileName(
        this, tr("Save tonemapping settings text file to..."),
        lum_options.getDefaultPathTmoSettings(),
        tr("LuminanceHDR tonemapping settings text file (*.txt)"));
    if (!fname.isEmpty()) {
        QFileInfo qfi(fname);
        if (qfi.suffix().toUpper() != QLatin1String("TXT")) {
            fname += QLatin1String(".txt");
        }

        lum_options.setDefaultPathTmoSettings(qfi.path());

        // write txt file
        fromGui2Txt(fname);
    }
}

void TonemappingPanel::fromGui2Txt(QString destination) {
    QFile file(destination);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(
            this, tr("Aborting..."),
            tr("File is not writable (check permissions, path...)"),
            QMessageBox::Ok, QMessageBox::NoButton);
        return;
    }
    QTextStream out(&file);
    out << "# LuminanceHDR Tonemapping Setting file." << endl;
    out << "# Editing this file by hand is risky, worst case scenario is "
           "Luminance crashing."
        << endl;
    out << "# Please edit this file by hand only if you know what you're "
           "doing, "
           "in any case never change the left hand side text (i.e. the part "
           "before the ``='')."
        << endl;
    out << "TMOSETTINGSVERSION=" << TMOSETTINGSVERSION << endl;
    out << "XSIZE=" << sizes[m_Ui->sizeComboBox->currentIndex()] << endl;

    QWidget *current_page = m_Ui->stackedWidget_operators->currentWidget();
    if (current_page == m_Ui->page_mantiuk06) {
        out << "TMO="
            << "Mantiuk06" << endl;
        out << "CONTRASTFACTOR=" << contrastfactorGang->v() << endl;
        out << "SATURATIONFACTOR=" << saturationfactorGang->v() << endl;
        out << "DETAILFACTOR=" << detailfactorGang->v() << endl;
        out << "CONTRASTEQUALIZATION="
            << (m_Ui->contrastEqualizCheckBox->isChecked() ? "YES" : "NO")
            << endl;
    } else if (current_page == m_Ui->page_mantiuk08) {
        out << "TMO="
            << "Mantiuk08" << endl;
        out << "COLORSATURATION=" << colorSaturationGang->v() << endl;
        out << "CONTRASTENHANCEMENT=" << contrastEnhancementGang->v() << endl;
        out << "LUMINANCELEVEL=" << luminanceLevelGang->v() << endl;
        out << "SETLUMINANCE="
            << (m_Ui->luminanceLevelCheckBox->isChecked() ? "YES" : "NO")
            << endl;
    } else if (current_page == m_Ui->page_fattal) {
        out << "TMO="
            << "Fattal02" << endl;
        out << "ALPHA=" << alphaGang->v() << endl;
        out << "BETA=" << betaGang->v() << endl;
        out << "COLOR=" << saturation2Gang->v() << endl;
        out << "NOISE=" << noiseGang->v() << endl;
        out << "OLDFATTAL="
            << (m_Ui->fftVersionCheckBox->isChecked() ? "NO" : "YES") << endl;
    } else if (current_page == m_Ui->page_ferradans) {
        out << "TMO="
            << "Ferradans11" << endl;
        out << "RHO=" << rhoGang->v() << endl;
        out << "INV_ALPHA=" << inv_alphaGang->v() << endl;
    } else if (current_page == m_Ui->page_ferwerda) {
        out << "TMO="
            << "Ferwerda96" << endl;
        out << "MAX_LUMINANCE=" << ferwerdamultiplierGang->v() << endl;
        out << "ADAPTATION_LUMINANCE=" << adaptationGang->v() << endl;
    } else if (current_page == m_Ui->page_kimkautz) {
        out << "TMO="
            << "KimKautz08" << endl;
        out << "KK_C1=" << kimkautzc1Gang->v() << endl;
        out << "KK_C2=" << kimkautzc2Gang->v() << endl;
    } else if (current_page == m_Ui->page_mai) {
        out << "TMO="
            << "Mai11" << endl;
    } else if (current_page == m_Ui->page_ashikhmin) {
        out << "TMO="
            << "Ashikhmin02" << endl;
        out << "SIMPLE=" << (m_Ui->simpleCheckBox->isChecked() ? "YES" : "NO")
            << endl;
        out << "EQUATION=" << (m_Ui->eq2RadioButton->isChecked() ? "2" : "4")
            << endl;
        out << "CONTRAST=" << contrastGang->v() << endl;
    } else if (current_page == m_Ui->page_durand) {
        out << "TMO="
            << "Durand02" << endl;
        out << "SPATIAL=" << spatialGang->v() << endl;
        out << "RANGE=" << rangeGang->v() << endl;
        out << "BASE=" << baseGang->v() << endl;
    } else if (current_page == m_Ui->page_drago) {
        out << "TMO="
            << "Drago03" << endl;
        out << "BIAS=" << biasGang->v() << endl;
    } else if (current_page == m_Ui->page_pattanaik) {
        out << "TMO="
            << "Pattanaik00" << endl;
        out << "MULTIPLIER=" << multiplierGang->v() << endl;
        out << "LOCAL=" << (m_Ui->pattalocal->isChecked() ? "YES" : "NO")
            << endl;
        out << "AUTOLUMINANCE="
            << (m_Ui->autoYcheckbox->isChecked() ? "YES" : "NO") << endl;
        out << "CONE=" << coneGang->v() << endl;
        out << "ROD=" << rodGang->v() << endl;
    } else if (current_page == m_Ui->page_reinhard02) {
        out << "TMO="
            << "Reinhard02" << endl;
        out << "KEY=" << keyGang->v() << endl;
        out << "PHI=" << phiGang->v() << endl;
        out << "SCALES="
            << (m_Ui->usescalescheckbox->isChecked() ? "YES" : "NO") << endl;
        out << "RANGE=" << range2Gang->v() << endl;
        out << "LOWER=" << lowerGang->v() << endl;
        out << "UPPER=" << upperGang->v() << endl;
    } else if (current_page == m_Ui->page_reinhard05) {
        out << "TMO="
            << "Reinhard05" << endl;
        out << "BRIGHTNESS=" << brightnessGang->v() << endl;
        out << "CHROMATICADAPTATION=" << chromaticGang->v() << endl;
        out << "LIGHTADAPTATION=" << lightGang->v() << endl;
    } else if (current_page == m_Ui->page_vanhateren) {
        out << "TMO="
            << "VanHateren06" << endl;
        out << "PUPIL_AREA=" << vanhaterenPupilAreaGang->v() << endl;
    }
    out << "PREGAMMA=" << pregammaGang->v() << endl;
    out << "POSTSATURATION=" << postsaturationGang->v() << endl;
    out << "POSTGAMMA=" << postgammaGang->v() << endl;
    file.close();
}

void TonemappingPanel::fromTxt2Gui() {
    QFile file(tmoSettingsFilename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text) || file.size() == 0) {
        QMessageBox::critical(
            this, tr("Aborting..."),
            tr("File is not readable (check permissions, path...)"),
            QMessageBox::Ok, QMessageBox::NoButton);
        return;
    }
    QTextStream in(&file);
    QString field, value;

    while (!in.atEnd()) {
        QString line = in.readLine();
        // skip comments
        if (line.startsWith('#')) continue;

        QString tmo;  // Hack, same parameter "RANGE" in durand and reinhard02

        field = line.section('=', 0, 0);  // get the field
        value = line.section('=', 1, 1);  // get the value
        if (field == QLatin1String("TMOSETTINGSVERSION")) {
            if (value != TMOSETTINGSVERSION) {
                QMessageBox::critical(this, tr("Aborting..."),
                                      tr("Error: The tone mapping settings "
                                         "file format has changed. This "
                                         "(old) file cannot be used with this "
                                         "version of LuminanceHDR. "
                                         "Create a new one."),
                                      QMessageBox::Ok, QMessageBox::NoButton);
                return;
            }
        } else if (field == QLatin1String("XSIZE")) {
            int idx;
            for (idx = 0; idx < m_Ui->sizeComboBox->count(); idx++) {
                if (sizes[idx] == value.toInt()) break;
            }
            if (idx == m_Ui->sizeComboBox->count())  // Custom XSIZE
            {
                sizes.push_back(value.toInt());
                fillCustomSizeComboBox();
                m_Ui->sizeComboBox->setCurrentIndex(
                    m_Ui->sizeComboBox->count() - 1);
            } else
                m_Ui->sizeComboBox->setCurrentIndex(idx);
        }
        // else if ( field == "QUALITY" å)
        //{
        //    m_Ui->qualityHS->setValue(value.toInt());
        //    m_Ui->qualitySB->setValue(value.toInt());
        //}
        else if (field == QLatin1String("TMO")) {
            if (value == QLatin1String("Ashikhmin02")) {
                m_Ui->stackedWidget_operators->setCurrentWidget(
                    m_Ui->page_ashikhmin);
                tmo = QStringLiteral("Ashikhmin02");
            } else if (value == QLatin1String("Mantiuk06")) {
                m_Ui->stackedWidget_operators->setCurrentWidget(
                    m_Ui->page_mantiuk06);
                tmo = QStringLiteral("Mantiuk06");
            } else if (value == QLatin1String("Mantiuk08")) {
                m_Ui->stackedWidget_operators->setCurrentWidget(
                    m_Ui->page_mantiuk08);
                tmo = QStringLiteral("Mantiuk08");
            } else if (value == QLatin1String("Drago03")) {
                m_Ui->stackedWidget_operators->setCurrentWidget(
                    m_Ui->page_drago);
                tmo = QStringLiteral("Drago03");
            } else if (value == QLatin1String("Durand02")) {
                m_Ui->stackedWidget_operators->setCurrentWidget(
                    m_Ui->page_durand);
                tmo = QStringLiteral("Durand02");
            } else if (value == QLatin1String("Fattal02")) {
                m_Ui->stackedWidget_operators->setCurrentWidget(
                    m_Ui->page_fattal);
                tmo = QStringLiteral("Fattal02");
            } else if (value == QLatin1String("Ferradans11")) {
                m_Ui->stackedWidget_operators->setCurrentWidget(
                    m_Ui->page_ferradans);
                tmo = QStringLiteral("Ferradans11");
            } else if (value == QLatin1String("Ferwerda96")) {
                m_Ui->stackedWidget_operators->setCurrentWidget(
                    m_Ui->page_ferwerda);
                tmo = QStringLiteral("Ferwerda96");
            } else if (value == QLatin1String("KimKautz08")) {
                m_Ui->stackedWidget_operators->setCurrentWidget(
                    m_Ui->page_kimkautz);
                tmo = QStringLiteral("KimKautz08");
            } else if (value == QLatin1String("Mai11")) {
                m_Ui->stackedWidget_operators->setCurrentWidget(m_Ui->page_mai);
                tmo = QStringLiteral("Mai11");
            } else if (value == QLatin1String("Pattanaik00")) {
                m_Ui->stackedWidget_operators->setCurrentWidget(
                    m_Ui->page_pattanaik);
                tmo = QStringLiteral("Pattanaik00");
            } else if (value == QLatin1String("Reinhard02")) {
                m_Ui->stackedWidget_operators->setCurrentWidget(
                    m_Ui->page_reinhard02);
                tmo = QStringLiteral("Reinhard02");
            } else if (value == QLatin1String("Reinhard05")) {
                m_Ui->stackedWidget_operators->setCurrentWidget(
                    m_Ui->page_reinhard05);
                tmo = QStringLiteral("Reinhard05");
            } else if (value == QLatin1String("VanHateren06")) {
                m_Ui->stackedWidget_operators->setCurrentWidget(
                    m_Ui->page_vanhateren);
                tmo = QStringLiteral("VanHateren06");
            }
        } else if (field == QLatin1String("CONTRASTFACTOR")) {
            m_Ui->contrastFactorSlider->setValue(
                contrastfactorGang->v2p(value.toFloat()));
        } else if (field == QLatin1String("SATURATIONFACTOR")) {
            m_Ui->saturationFactorSlider->setValue(
                saturationfactorGang->v2p(value.toFloat()));
        } else if (field == QLatin1String("DETAILFACTOR")) {
            m_Ui->detailFactorSlider->setValue(
                detailfactorGang->v2p(value.toFloat()));
        } else if (field == QLatin1String("CONTRASTEQUALIZATION")) {
            m_Ui->contrastEqualizCheckBox->setChecked(
                (value == QLatin1String("YES")));
        } else if (field == QLatin1String("COLORSATURATION")) {
            m_Ui->contrastFactorSlider->setValue(
                colorSaturationGang->v2p(value.toFloat()));
        } else if (field == QLatin1String("CONTRASTENHANCEMENT")) {
            m_Ui->saturationFactorSlider->setValue(
                contrastEnhancementGang->v2p(value.toFloat()));
        } else if (field == QLatin1String("LUMINANCELEVEL")) {
            m_Ui->detailFactorSlider->setValue(
                luminanceLevelGang->v2p(value.toFloat()));
        } else if (field == QLatin1String("SIMPLE")) {
            m_Ui->simpleCheckBox->setChecked((value == QLatin1String("YES")));
        } else if (field == QLatin1String("EQUATION")) {
            m_Ui->eq2RadioButton->setChecked((value == QLatin1String("2")));
            m_Ui->eq4RadioButton->setChecked((value == QLatin1String("4")));
        } else if (field == QLatin1String("CONTRAST")) {
            m_Ui->contrastSlider->setValue(contrastGang->v2p(value.toFloat()));
        } else if (field == QLatin1String("BIAS")) {
            m_Ui->biasSlider->setValue(biasGang->v2p(value.toFloat()));
        } else if (field == QLatin1String("SPATIAL")) {
            m_Ui->spatialSlider->setValue(spatialGang->v2p(value.toFloat()));
        } else if (field == QLatin1String("RANGE")) {
            if (tmo == QLatin1String("Durand02"))
                m_Ui->rangeSlider->setValue(rangeGang->v2p(value.toFloat()));
            else
                m_Ui->range2Slider->setValue(range2Gang->v2p(value.toFloat()));
        } else if (field == QLatin1String("BASE")) {
            m_Ui->baseSlider->setValue(baseGang->v2p(value.toFloat()));
        } else if (field == QLatin1String("ALPHA")) {
            m_Ui->alphaSlider->setValue(alphaGang->v2p(value.toFloat()));
        } else if (field == QLatin1String("BETA")) {
            m_Ui->betaSlider->setValue(betaGang->v2p(value.toFloat()));
        } else if (field == QLatin1String("COLOR")) {
            m_Ui->saturation2Slider->setValue(
                saturation2Gang->v2p(value.toFloat()));
        } else if (field == QLatin1String("NOISE")) {
            m_Ui->noiseSlider->setValue(noiseGang->v2p(value.toFloat()));
        } else if (field == QLatin1String("OLDFATTAL")) {
            m_Ui->fftVersionCheckBox->setChecked(value != QLatin1String("YES"));
        } else if (field == QLatin1String("RHO")) {
            m_Ui->rhoSlider->setValue(rhoGang->v2p(value.toFloat()));
        } else if (field == QLatin1String("INV_ALPHA")) {
            m_Ui->inv_alphaSlider->setValue(
                inv_alphaGang->v2p(value.toFloat()));
        } else if (field == QLatin1String("MAX_LUMINANCE")) {
            m_Ui->ferwerdaMultiplierSlider->setValue(ferwerdamultiplierGang->v2p(value.toFloat()));
        } else if (field == QLatin1String("ADAPTATION_LUMINANCE")) {
            m_Ui->adaptationLuminanceSlider->setValue(
                adaptationGang->v2p(value.toFloat()));
        } else if (field == QLatin1String("KK_C1")) {
            m_Ui->kimkautzC1Slider->setValue(kimkautzc1Gang->v2p(value.toFloat()));
        } else if (field == QLatin1String("KK_C2")) {
            m_Ui->kimkautzC2Slider->setValue(kimkautzc2Gang->v2p(value.toFloat()));
        } else if (field == QLatin1String("MULTIPLIER")) {
            m_Ui->multiplierSlider->setValue(
                multiplierGang->v2p(value.toFloat()));
        } else if (field == QLatin1String("LOCAL")) {
            (value == QLatin1String("YES"))
                ? m_Ui->pattalocal->setChecked(value == QLatin1String("YES"))
                : m_Ui->pattalocal->setChecked(value == QLatin1String("NO"));
        } else if (field == QLatin1String("AUTOLUMINANCE")) {
            (value == QLatin1String("YES"))
                ? m_Ui->autoYcheckbox->setChecked(value == QLatin1String("YES"))
                : m_Ui->autoYcheckbox->setChecked(value == QLatin1String("NO"));
        } else if (field == QLatin1String("CONE")) {
            m_Ui->coneSlider->setValue(coneGang->v2p(value.toFloat()));
        } else if (field == QLatin1String("ROD")) {
            m_Ui->rodSlider->setValue(rodGang->v2p(value.toFloat()));
        } else if (field == QLatin1String("KEY")) {
            m_Ui->keySlider->setValue(keyGang->v2p(value.toFloat()));
        } else if (field == QLatin1String("PHI")) {
            m_Ui->phiSlider->setValue(phiGang->v2p(value.toFloat()));
        } else if (field == QLatin1String("SCALES")) {
            (value == QLatin1String("YES"))
                ? m_Ui->usescalescheckbox->setChecked(value ==
                                                      QLatin1String("YES"))
                : m_Ui->usescalescheckbox->setChecked(value ==
                                                      QLatin1String("NO"));
        } else if (field == QLatin1String("LOWER")) {
            m_Ui->lowerSlider->setValue(lowerGang->v2p(value.toFloat()));
        } else if (field == QLatin1String("UPPER")) {
            m_Ui->upperSlider->setValue(upperGang->v2p(value.toFloat()));
        } else if (field == QLatin1String("BRIGHTNESS")) {
            m_Ui->brightnessSlider->setValue(
                brightnessGang->v2p(value.toFloat()));
        } else if (field == QLatin1String("CHROMATICADAPTATION")) {
            m_Ui->chromaticAdaptSlider->setValue(
                chromaticGang->v2p(value.toFloat()));
        } else if (field == QLatin1String("LIGHTADAPTATION")) {
            m_Ui->lightAdaptSlider->setValue(lightGang->v2p(value.toFloat()));
        } else if (field == QLatin1String("PUPIL_AREA")) {
            m_Ui->pupil_areaSlider->setValue(vanhaterenPupilAreaGang->v2p(value.toFloat()));
        } else if (field == QLatin1String("PREGAMMA")) {
            m_Ui->pregammaSlider->setValue(pregammaGang->v2p(value.toFloat()));
        } else if (field == QLatin1String("POSTSATURATION")) {
            m_Ui->postsaturationSlider->setValue(postsaturationGang->v2p(value.toFloat()));
        } else if (field == QLatin1String("POSTGAMMA")) {
            m_Ui->postgammaSlider->setValue(postgammaGang->v2p(value.toFloat()));
        }
    }
}

void TonemappingPanel::on_addCustomSizeButton_clicked() {
    bool ok;
    int i = QInputDialog::getInt(this, tr("Custom LDR size"),
                                 tr("Enter the width of the new size:"), 0, 0,
                                 2147483647, 1, &ok);
    if (ok && i > 0) {
        sizes.push_back(i);
        fillCustomSizeComboBox();
        m_Ui->sizeComboBox->setCurrentIndex(m_Ui->sizeComboBox->count() - 1);
    }
}

void TonemappingPanel::fillCustomSizeComboBox() {
    m_Ui->sizeComboBox->clear();
    for (int i = 0; i < sizes.size(); i++) {
        m_Ui->sizeComboBox->addItem(QStringLiteral("%1x%2").arg(sizes[i]).arg(
            (int)(heightToWidthRatio * sizes[i])));
    }
}

void TonemappingPanel::setEnabled(bool b) {
    if (b) {
        updateUndoState();
    } else {
        m_Ui->undoButton->setEnabled(false);
        m_Ui->redoButton->setEnabled(false);
    }
    m_Ui->tonemapGroupBox->setEnabled(b);
    m_Ui->processingToolBox->setEnabled(b);

    m_Ui->applyButton->setEnabled(b);
    m_Ui->replaceLdrCheckBox->setEnabled(b);
    m_Ui->autoLevelsCheckBox->setEnabled(b);
    m_Ui->toolButtonThreshold->setEnabled(b);
}

void TonemappingPanel::updatedHDR(pfs::Frame *f) {
    setSizes(f->getWidth(), f->getHeight());
    m_currentFrame = f;
}

/*
 * This function should set the entire status.
 * Currently I'm only interested in changing the TM operator
 */
void TonemappingPanel::updateTonemappingParams(TonemappingOptions *opts) {
    qDebug() << "TonemappingPanel::updateTonemappingParams(TonemappingOptions "
                "*opts)";
    //    m_currentTmoOperator = opts->tmoperator;
    //    updateUndoState();
    m_Ui->cmbOperators->setCurrentIndex(opts->tmoperator);
}

void TonemappingPanel::saveParameters() {
    SavingParameters dialog;
    if (dialog.exec()) {
        QString comment = dialog.getComment();

        switch (m_currentTmoOperator) {
            case ashikhmin:
                {
                    bool simple = simpleGang->isCheckBox1Checked();
                    bool eq2 = eq2Gang->isRadioButton1Checked();
                    float lct = contrastGang->v();
                    execAshikhminQuery(simple, eq2, lct, comment);
                }
                break;
            case drago:
                {
                    float bias = biasGang->v();
                    execDragoQuery(bias, comment);
                }
                break;
            case durand:
                {
                    float spatial = spatialGang->v();
                    float range = rangeGang->v();
                    float base = baseGang->v();
                    execDurandQuery(spatial, range, base, comment);
                }
                break;
            case fattal:
                {
                    float alpha = alphaGang->v();
                    float beta = betaGang->v();
                    float colorSat = saturation2Gang->v();
                    bool  noiseReduction = noiseGang->v();
                    bool  oldFattal = !fftSolverGang->isCheckBox1Checked();
                    execFattalQuery(alpha, beta, colorSat, noiseReduction,
                                    oldFattal, comment);
                }
                break;
            case ferradans:
                {
                    float rho = rhoGang->v();
                    float inv_alpha = inv_alphaGang->v();
                    execFerradansQuery(rho, inv_alpha, comment);
                }
                break;
            case mai:  // no options
                break;
            case mantiuk06:
                {
                    float contrastFactor = contrastfactorGang->v();
                    float saturationFactor = saturationfactorGang->v();
                    float detailFactor = detailfactorGang->v();
                    bool  contrastEqualization = contrastfactorGang->isCheckBox1Checked();
                    execMantiuk06Query(contrastEqualization, contrastFactor,
                                       saturationFactor, detailFactor, comment);
                }
                break;
            case mantiuk08:
                {
                    float colorSaturation = colorSaturationGang->v();
                    float contrastEnhancement = contrastEnhancementGang->v();
                    float luminanceLevel = luminanceLevelGang->v();
                    bool  manualLuminanceLevel = luminanceLevelGang->isCheckBox1Checked();
                    execMantiuk08Query(colorSaturation, contrastEnhancement,
                                       luminanceLevel, manualLuminanceLevel,
                                       comment);
                }
                break;
            case pattanaik:
                {
                    bool  autolum = autoYGang->isCheckBox1Checked();
                    bool  local = pattalocalGang->isCheckBox1Checked();
                    float cone = coneGang->v();
                    float rod = rodGang->v();
                    float multiplier = multiplierGang->v();
                    execPattanaikQuery(autolum, local, cone, rod, multiplier,
                                       comment);
                }
                break;
            case reinhard02:
                {
                    bool  scales = usescalesGang->isCheckBox1Checked();
                    float key = keyGang->v();
                    float phi = phiGang->v();
                    int   irange = (int)range2Gang->v();
                    int   lower = (int)lowerGang->v();
                    int   upper = (int)upperGang->v();
                    execReinhard02Query(scales, key, phi, irange, lower, upper,
                                        comment);
                }
                break;
            case reinhard05:
                {
                    float brightness = brightnessGang->v();
                    float chromaticAdaptation = chromaticGang->v();
                    float lightAdaptation = lightGang->v();
                    execReinhard05Query(brightness, chromaticAdaptation,
                                        lightAdaptation, comment);
                }
                break;
            case ferwerda:
                {
                    float maxLuminance = ferwerdamultiplierGang->v();
                    float adaptationLuminance = adaptationGang->v();
                    execFerwerdaQuery(maxLuminance, adaptationLuminance, comment);
                }
                break;
            case kimkautz:
                {
                    float kk_c1 = kimkautzc1Gang->v();
                    float kk_c2 = kimkautzc2Gang->v();
                    execKimKautzQuery(kk_c1, kk_c2, comment);
                }
                break;
            case vanhateren:
                {
                    float pupil_area = vanhaterenPupilAreaGang->v();
                    execVanHaterenQuery(pupil_area, comment);
                }
                break;
        }
    }
}

void TonemappingPanel::loadParameters() {
    TonemappingSettings dialog(this, m_currentFrame, m_databaseconnection);

    if (dialog.exec()) {
        LuminanceOptions *luminance_options = new LuminanceOptions;
        setRealtimePreviews(false);
        TonemappingOptions *tmopts = dialog.getTonemappingOptions();
        // Ashikhmin
        bool simple, eq2;
        float lct;
        // Drago
        float bias;
        // Durand
        float spatial, range, base;
        // Fattal
        float alpha, beta, colorSat, noiseReduction;
        bool fftsolver;
        // Ferradans
        float rho, inv_alpha;
        // Mantiuk 06
        bool contrastEqualization;
        float contrastFactor;
        float saturationFactor;
        float detailFactor;
        // Mantiuk 08
        float colorSaturation, contrastEnhancement, luminanceLevel;
        bool manualLuminanceLevel;
        // Pattanaik
        float multiplier, rod, cone;
        bool autolum, local;
        // Reinhard 02
        bool scales;
        float key, phi;
        int irange, lower, upper;
        // Reinhard 05
        float brightness, chromaticAdaptation, lightAdaptation;
        // Ferwerda 96
        float maxLuminance, adaptationLuminance;
        // KimKrautz 08
        float kk_c1, kk_c2;

        // Pre-gamma
        float pregamma;
        // Post-saturation
        float postsaturation;
        // Post-gamma
        float postgamma;

        switch (tmopts->tmoperator) {
            case ashikhmin:
                m_Ui->stackedWidget_operators->setCurrentIndex(ashikhmin);
                simple = tmopts->operator_options.ashikhminoptions.simple;
                eq2 = tmopts->operator_options.ashikhminoptions.eq2;
                lct = tmopts->operator_options.ashikhminoptions.lct;
                pregamma = tmopts->pregamma;
                postsaturation = tmopts->postsaturation;
                postgamma = tmopts->postgamma;
                m_Ui->simpleCheckBox->setChecked(simple);
                if (eq2)
                    m_Ui->eq2RadioButton->setChecked(true);
                else
                    m_Ui->eq4RadioButton->setChecked(true);
                m_Ui->contrastSlider->setValue(lct);
                m_Ui->contrastdsb->setValue(lct);
                m_Ui->pregammaSlider->setValue(pregamma);
                m_Ui->pregammadsb->setValue(pregamma);
                m_Ui->postsaturationSlider->setValue(postsaturation);
                m_Ui->postsaturationdsb->setValue(postsaturation);
                m_Ui->postgammaSlider->setValue(postgamma);
                m_Ui->postgammadsb->setValue(postgamma);
                break;
            case drago:
                m_Ui->stackedWidget_operators->setCurrentIndex(drago);
                bias = tmopts->operator_options.dragooptions.bias;
                pregamma = tmopts->pregamma;
                postsaturation = tmopts->postsaturation;
                postgamma = tmopts->postgamma;
                m_Ui->biasSlider->setValue(bias);
                m_Ui->biasdsb->setValue(bias);
                m_Ui->pregammaSlider->setValue(pregamma);
                m_Ui->pregammadsb->setValue(pregamma);
                m_Ui->postsaturationSlider->setValue(postsaturation);
                m_Ui->postsaturationdsb->setValue(postsaturation);
                m_Ui->postgammaSlider->setValue(postgamma);
                m_Ui->postgammadsb->setValue(postgamma);
                break;
            case durand:
                m_Ui->stackedWidget_operators->setCurrentIndex(durand);
                spatial = tmopts->operator_options.durandoptions.spatial;
                range = tmopts->operator_options.durandoptions.range;
                base = tmopts->operator_options.durandoptions.base;
                pregamma = tmopts->pregamma;
                postsaturation = tmopts->postsaturation;
                postgamma = tmopts->postgamma;
                m_Ui->spatialSlider->setValue(spatial);
                m_Ui->spatialdsb->setValue(spatial);
                m_Ui->rangeSlider->setValue(range);
                m_Ui->rangedsb->setValue(range);
                m_Ui->baseSlider->setValue(base);
                m_Ui->basedsb->setValue(base);
                m_Ui->pregammaSlider->setValue(pregamma);
                m_Ui->pregammadsb->setValue(pregamma);
                m_Ui->postsaturationSlider->setValue(postsaturation);
                m_Ui->postsaturationdsb->setValue(postsaturation);
                m_Ui->postgammaSlider->setValue(postgamma);
                m_Ui->postgammadsb->setValue(postgamma);
                break;
            case fattal:
                m_Ui->stackedWidget_operators->setCurrentIndex(fattal);
                alpha = tmopts->operator_options.fattaloptions.alpha;
                beta = tmopts->operator_options.fattaloptions.beta;
                colorSat = tmopts->operator_options.fattaloptions.color;
                noiseReduction =
                    tmopts->operator_options.fattaloptions.noiseredux;
                fftsolver = tmopts->operator_options.fattaloptions.fftsolver;
                pregamma = tmopts->pregamma;
                postsaturation = tmopts->postsaturation;
                postgamma = tmopts->postgamma;
                m_Ui->alphaSlider->setValue(alpha);
                m_Ui->alphadsb->setValue(alpha);
                m_Ui->betaSlider->setValue(beta);
                m_Ui->betadsb->setValue(beta);
                m_Ui->saturation2Slider->setValue(colorSat);
                m_Ui->saturation2dsb->setValue(colorSat);
                m_Ui->noiseSlider->setValue(noiseReduction);
                m_Ui->noisedsb->setValue(noiseReduction);
                m_Ui->fftVersionCheckBox->setChecked(fftsolver);
                m_Ui->pregammaSlider->setValue(pregamma);
                m_Ui->pregammadsb->setValue(pregamma);
                m_Ui->postsaturationSlider->setValue(postsaturation);
                m_Ui->postsaturationdsb->setValue(postsaturation);
                m_Ui->postgammaSlider->setValue(postgamma);
                m_Ui->postgammadsb->setValue(postgamma);
                break;
            case ferradans:
                m_Ui->stackedWidget_operators->setCurrentIndex(ferradans);
                rho = tmopts->operator_options.ferradansoptions.rho;
                inv_alpha = tmopts->operator_options.ferradansoptions.inv_alpha;
                pregamma = tmopts->pregamma;
                postsaturation = tmopts->postsaturation;
                postgamma = tmopts->postgamma;
                m_Ui->rhoSlider->setValue(rho);
                m_Ui->rhodsb->setValue(rho);
                m_Ui->inv_alphaSlider->setValue(inv_alpha);
                m_Ui->inv_alphadsb->setValue(inv_alpha);
                m_Ui->pregammaSlider->setValue(pregamma);
                m_Ui->pregammadsb->setValue(pregamma);
                m_Ui->postsaturationSlider->setValue(postsaturation);
                m_Ui->postsaturationdsb->setValue(postsaturation);
                m_Ui->postgammaSlider->setValue(postgamma);
                m_Ui->postgammadsb->setValue(postgamma);
                break;
            case mai:
                m_Ui->stackedWidget_operators->setCurrentIndex(mai);
                pregamma = tmopts->pregamma;
                postsaturation = tmopts->postsaturation;
                postgamma = tmopts->postgamma;
                m_Ui->pregammaSlider->setValue(pregamma);
                m_Ui->pregammadsb->setValue(pregamma);
                m_Ui->postsaturationSlider->setValue(postsaturation);
                m_Ui->postsaturationdsb->setValue(postsaturation);
                m_Ui->postgammaSlider->setValue(postgamma);
                m_Ui->postgammadsb->setValue(postgamma);
                break;
            case mantiuk06:
                m_Ui->stackedWidget_operators->setCurrentIndex(mantiuk06);
                contrastEqualization = tmopts->operator_options.mantiuk06options
                                           .contrastequalization;
                contrastFactor =
                    tmopts->operator_options.mantiuk06options.contrastfactor;
                saturationFactor =
                    tmopts->operator_options.mantiuk06options.saturationfactor;
                detailFactor =
                    tmopts->operator_options.mantiuk06options.detailfactor;
                pregamma = tmopts->pregamma;
                postsaturation = tmopts->postsaturation;
                postgamma = tmopts->postgamma;
                m_Ui->contrastEqualizCheckBox->setChecked(contrastEqualization);
                m_Ui->contrastFactorSlider->setValue(contrastFactor);
                m_Ui->contrastFactordsb->setValue(contrastFactor);
                m_Ui->saturationFactorSlider->setValue(saturationFactor);
                m_Ui->saturationFactordsb->setValue(saturationFactor);
                m_Ui->detailFactorSlider->setValue(detailFactor);
                m_Ui->detailFactordsb->setValue(detailFactor);
                m_Ui->pregammaSlider->setValue(pregamma);
                m_Ui->pregammadsb->setValue(pregamma);
                m_Ui->postsaturationSlider->setValue(postsaturation);
                m_Ui->postsaturationdsb->setValue(postsaturation);
                m_Ui->postgammaSlider->setValue(postgamma);
                m_Ui->postgammadsb->setValue(postgamma);
                break;
            case mantiuk08:
                m_Ui->stackedWidget_operators->setCurrentIndex(mantiuk08);
                colorSaturation =
                    tmopts->operator_options.mantiuk08options.colorsaturation;
                contrastEnhancement = tmopts->operator_options.mantiuk08options
                                          .contrastenhancement;
                luminanceLevel =
                    tmopts->operator_options.mantiuk08options.luminancelevel;
                manualLuminanceLevel =
                    tmopts->operator_options.mantiuk08options.setluminance;
                pregamma = tmopts->pregamma;
                postsaturation = tmopts->postsaturation;
                postgamma = tmopts->postgamma;
                m_Ui->colorSaturationSlider->setValue(colorSaturation);
                m_Ui->colorSaturationDSB->setValue(colorSaturation);
                m_Ui->contrastEnhancementSlider->setValue(contrastEnhancement);
                m_Ui->contrastEnhancementDSB->setValue(contrastEnhancement);
                m_Ui->luminanceLevelSlider->setValue(luminanceLevel);
                m_Ui->luminanceLevelDSB->setValue(luminanceLevel);
                m_Ui->luminanceLevelCheckBox->setChecked(manualLuminanceLevel);
                m_Ui->pregammaSlider->setValue(pregamma);
                m_Ui->pregammadsb->setValue(pregamma);
                m_Ui->postsaturationSlider->setValue(postsaturation);
                m_Ui->postsaturationdsb->setValue(postsaturation);
                m_Ui->postgammaSlider->setValue(postgamma);
                m_Ui->postgammadsb->setValue(postgamma);
                break;
            case pattanaik:
                m_Ui->stackedWidget_operators->setCurrentIndex(pattanaik);
                multiplier =
                    tmopts->operator_options.pattanaikoptions.multiplier;
                rod = tmopts->operator_options.pattanaikoptions.rod;
                cone = tmopts->operator_options.pattanaikoptions.cone;
                autolum = tmopts->operator_options.pattanaikoptions.autolum;
                local = tmopts->operator_options.pattanaikoptions.local;
                pregamma = tmopts->pregamma;
                postsaturation = tmopts->postsaturation;
                postgamma = tmopts->postgamma;
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
                m_Ui->postsaturationSlider->setValue(postsaturation);
                m_Ui->postsaturationdsb->setValue(postsaturation);
                m_Ui->postgammaSlider->setValue(postgamma);
                m_Ui->postgammadsb->setValue(postgamma);
                break;
            case reinhard02:
                m_Ui->stackedWidget_operators->setCurrentIndex(reinhard02);
                scales = tmopts->operator_options.reinhard02options.scales;
                key = tmopts->operator_options.reinhard02options.key;
                phi = tmopts->operator_options.reinhard02options.phi;
                irange = tmopts->operator_options.reinhard02options.range;
                lower = tmopts->operator_options.reinhard02options.lower;
                upper = tmopts->operator_options.reinhard02options.upper;
                pregamma = tmopts->pregamma;
                postsaturation = tmopts->postsaturation;
                postgamma = tmopts->postgamma;
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
                m_Ui->postsaturationSlider->setValue(postsaturation);
                m_Ui->postsaturationdsb->setValue(postsaturation);
                m_Ui->postgammaSlider->setValue(postgamma);
                m_Ui->postgammadsb->setValue(postgamma);
                break;
            case reinhard05:
                m_Ui->stackedWidget_operators->setCurrentIndex(reinhard05);
                brightness =
                    tmopts->operator_options.reinhard05options.brightness;
                chromaticAdaptation = tmopts->operator_options.reinhard05options
                                          .chromaticAdaptation;
                lightAdaptation =
                    tmopts->operator_options.reinhard05options.lightAdaptation;
                pregamma = tmopts->pregamma;
                postsaturation = tmopts->postsaturation;
                postgamma = tmopts->postgamma;
                m_Ui->brightnessSlider->setValue(brightness);
                m_Ui->brightnessdsb->setValue(brightness);
                m_Ui->chromaticAdaptSlider->setValue(chromaticAdaptation);
                m_Ui->chromaticAdaptdsb->setValue(chromaticAdaptation);
                m_Ui->lightAdaptSlider->setValue(lightAdaptation);
                m_Ui->lightAdaptdsb->setValue(lightAdaptation);
                m_Ui->pregammaSlider->setValue(pregamma);
                m_Ui->pregammadsb->setValue(pregamma);
                m_Ui->postsaturationSlider->setValue(postsaturation);
                m_Ui->postsaturationdsb->setValue(postsaturation);
                m_Ui->postgammaSlider->setValue(postgamma);
                m_Ui->postgammadsb->setValue(postgamma);
                break;
            case ferwerda:
                m_Ui->stackedWidget_operators->setCurrentIndex(ferwerda);
                maxLuminance =
                    tmopts->operator_options.ferwerdaoptions.multiplier;
                adaptationLuminance = tmopts->operator_options.ferwerdaoptions
                                          .adaptationluminance;
                pregamma = tmopts->pregamma;
                postsaturation = tmopts->postsaturation;
                postgamma = tmopts->postgamma;
                m_Ui->ferwerdaMultiplierSlider->setValue(maxLuminance);
                m_Ui->ferwerdaMultiplierDsb->setValue(maxLuminance);
                m_Ui->adaptationLuminanceSlider->setValue(adaptationLuminance);
                m_Ui->adaptationLuminanceDsb->setValue(adaptationLuminance);
                m_Ui->pregammaSlider->setValue(pregamma);
                m_Ui->pregammadsb->setValue(pregamma);
                m_Ui->postsaturationSlider->setValue(postsaturation);
                m_Ui->postsaturationdsb->setValue(postsaturation);
                m_Ui->postgammaSlider->setValue(postgamma);
                m_Ui->postgammadsb->setValue(postgamma);
                break;
            case kimkautz:
                m_Ui->stackedWidget_operators->setCurrentIndex(kimkautz);
                kk_c1 =
                    tmopts->operator_options.kimkautzoptions.c1;
                kk_c2 =
                    tmopts->operator_options.kimkautzoptions.c2;

                pregamma = tmopts->pregamma;
                postsaturation = tmopts->postsaturation;
                postgamma = tmopts->postgamma;
                m_Ui->kimkautzC1Slider->setValue(kk_c1);
                m_Ui->kimkautzC1Dsb->setValue(kk_c1);
                m_Ui->kimkautzC2Slider->setValue(kk_c2);
                m_Ui->kimkautzC2Dsb->setValue(kk_c2);
                m_Ui->pregammaSlider->setValue(pregamma);
                m_Ui->pregammadsb->setValue(pregamma);
                m_Ui->postsaturationSlider->setValue(postsaturation);
                m_Ui->postsaturationdsb->setValue(postsaturation);
                m_Ui->postgammaSlider->setValue(postgamma);
                m_Ui->postgammadsb->setValue(postgamma);
                break;
            case vanhateren:
                m_Ui->stackedWidget_operators->setCurrentIndex(vanhateren);
                float pupil_area =
                    tmopts->operator_options.vanhaterenoptions.pupil_area;

                pregamma = tmopts->pregamma;
                postsaturation = tmopts->postsaturation;
                postgamma = tmopts->postgamma;
                m_Ui->pupil_areaSlider->setValue(pupil_area);
                m_Ui->pupil_areaDsb->setValue(pupil_area);
                m_Ui->pregammaSlider->setValue(pregamma);
                m_Ui->pregammadsb->setValue(pregamma);
                m_Ui->postsaturationSlider->setValue(postsaturation);
                m_Ui->postsaturationdsb->setValue(postsaturation);
                m_Ui->postgammaSlider->setValue(postgamma);
                m_Ui->postgammadsb->setValue(postgamma);
                break;
        }
        if (dialog.wantsTonemap()) {
            TonemappingOptions *t = new TonemappingOptions(*tmopts);
            m_toneMappingOptionsToDelete.push_back(t);
            t->origxsize = sizes[0];
            t->xsize = sizes[0];
            emit startTonemapping(t);
        }
        setRealtimePreviews(luminance_options->isRealtimePreviewsActive());
        delete luminance_options;
    }
}

void TonemappingPanel::execMantiuk06Query(bool contrastEqualization,
                                          float contrastFactor,
                                          float saturationFactor,
                                          float detailFactor, QString comment) {
    qDebug() << "TonemappingPanel::execMantiuk06Query";
    QSqlDatabase db = QSqlDatabase::database(m_databaseconnection);
    QSqlQuery query(db);
    float pregamma = m_Ui->pregammadsb->value();
    float postsaturation = m_Ui->postsaturationdsb->value();
    float postgamma = m_Ui->postgammadsb->value();
    query.prepare(
        "INSERT INTO mantiuk06 (contrastEqualization, contrastFactor, \
        saturationFactor, detailFactor, pregamma, comment, postsaturation, postgamma) \
        VALUES (:contrastEqualization, :contrastFactor, :saturationFactor, \
        :detailFactor, :pregamma, :comment, :postsaturation, :postgamma)");
    query.bindValue(QStringLiteral(":contrastEqualization"),
                    contrastEqualization);
    query.bindValue(QStringLiteral(":contrastFactor"), contrastFactor);
    query.bindValue(QStringLiteral(":saturationFactor"), saturationFactor);
    query.bindValue(QStringLiteral(":detailFactor"), detailFactor);
    query.bindValue(QStringLiteral(":pregamma"), pregamma);
    query.bindValue(QStringLiteral(":comment"), comment);
    query.bindValue(QStringLiteral(":postsaturation"), postsaturation);
    query.bindValue(QStringLiteral(":postgamma"), postgamma);
    bool res = query.exec();
    if (res == false) qDebug() << query.lastError();
}

void TonemappingPanel::execMantiuk08Query(float colorSaturation,
                                          float contrastEnhancement,
                                          float luminanceLevel,
                                          bool manualLuminanceLevel,
                                          QString comment) {
    qDebug() << "TonemappingPanel::execMantiuk08Query";
    QSqlDatabase db = QSqlDatabase::database(m_databaseconnection);
    QSqlQuery query(db);
    float pregamma = m_Ui->pregammadsb->value();
    float postsaturation = m_Ui->postsaturationdsb->value();
    float postgamma = m_Ui->postgammadsb->value();
    query.prepare(
        "INSERT INTO mantiuk08 (colorSaturation, contrastEnhancement, \
        luminanceLevel, manualLuminanceLevel, pregamma, comment, postsaturation, postgamma) \
        VALUES (:colorSaturation, :contrastEnhancement, :luminanceLevel, \
        :manualLuminanceLevel, :pregamma, :comment, :postsaturation, :postgamma)");
    query.bindValue(QStringLiteral(":colorSaturation"), colorSaturation);
    query.bindValue(QStringLiteral(":contrastEnhancement"),
                    contrastEnhancement);
    query.bindValue(QStringLiteral(":luminanceLevel"), luminanceLevel);
    query.bindValue(QStringLiteral(":manualLuminanceLevel"),
                    manualLuminanceLevel);
    query.bindValue(QStringLiteral(":pregamma"), pregamma);
    query.bindValue(QStringLiteral(":comment"), comment);
    query.bindValue(QStringLiteral(":postsaturation"), postsaturation);
    query.bindValue(QStringLiteral(":postgamma"), postgamma);
    bool res = query.exec();
    if (res == false) qDebug() << query.lastError();
}

void TonemappingPanel::execAshikhminQuery(bool simple, bool eq2, float lct,
                                          QString comment) {
    qDebug() << "TonemappingPanel::execAshikhminQuery";
    QSqlDatabase db = QSqlDatabase::database(m_databaseconnection);
    QSqlQuery query(db);
    float pregamma = m_Ui->pregammadsb->value();
    float postsaturation = m_Ui->postsaturationdsb->value();
    float postgamma = m_Ui->postgammadsb->value();
    query.prepare(
        "INSERT INTO ashikhmin (simple, eq2, lct, pregamma, comment, postsaturation, postgamma) \
        VALUES (:simple, :eq2, :lct, :pregamma, :comment, :postsaturation, :postgamma)");
    query.bindValue(QStringLiteral(":simple"), simple);
    query.bindValue(QStringLiteral(":eq2"), eq2);
    query.bindValue(QStringLiteral(":lct"), lct);
    query.bindValue(QStringLiteral(":pregamma"), pregamma);
    query.bindValue(QStringLiteral(":comment"), comment);
    query.bindValue(QStringLiteral(":postsaturation"), postsaturation);
    query.bindValue(QStringLiteral(":postgamma"), postgamma);
    bool res = query.exec();
    if (res == false) qDebug() << query.lastError();
}

void TonemappingPanel::execDragoQuery(float bias, QString comment) {
    qDebug() << "TonemappingPanel::execDragoQuery";
    QSqlDatabase db = QSqlDatabase::database(m_databaseconnection);
    QSqlQuery query(db);
    float pregamma = m_Ui->pregammadsb->value();
    float postsaturation = m_Ui->postsaturationdsb->value();
    float postgamma = m_Ui->postgammadsb->value();
    query.prepare(
        "INSERT INTO drago (bias, pregamma, comment, postsaturation, postgamma) \
        VALUES (:bias, :pregamma, :comment, :postsaturation, :postgamma)");
    query.bindValue(QStringLiteral(":bias"), bias);
    query.bindValue(QStringLiteral(":pregamma"), pregamma);
    query.bindValue(QStringLiteral(":comment"), comment);
    query.bindValue(QStringLiteral(":postsaturation"), postsaturation);
    query.bindValue(QStringLiteral(":postgamma"), postgamma);
    bool res = query.exec();
    if (res == false) qDebug() << query.lastError();
}

void TonemappingPanel::execDurandQuery(float spatial, float range, float base,
                                       QString comment) {
    qDebug() << "TonemappingPanel::execDurandQuery";
    QSqlDatabase db = QSqlDatabase::database(m_databaseconnection);
    QSqlQuery query(db);
    float pregamma = m_Ui->pregammadsb->value();
    float postsaturation = m_Ui->postsaturationdsb->value();
    float postgamma = m_Ui->postgammadsb->value();
    query.prepare(
        "INSERT INTO durand (spatial, range, base, pregamma, comment, postsaturation, postgamma) \
        VALUES (:spatial, :range, :base, :pregamma, :comment, :postsaturation, :postgamma)");
    query.bindValue(QStringLiteral(":spatial"), spatial);
    query.bindValue(QStringLiteral(":base"), base);
    query.bindValue(QStringLiteral(":range"), range);
    query.bindValue(QStringLiteral(":pregamma"), pregamma);
    query.bindValue(QStringLiteral(":comment"), comment);
    query.bindValue(QStringLiteral(":postsaturation"), postsaturation);
    query.bindValue(QStringLiteral(":postgamma"), postgamma);
    bool res = query.exec();
    if (res == false) qDebug() << query.lastError();
}

void TonemappingPanel::execFattalQuery(float alpha, float beta,
                                       float colorSaturation,
                                       float noiseReduction, bool oldFattal,
                                       QString comment) {
    qDebug() << "TonemappingPanel::execFattalQuery";
    QSqlDatabase db = QSqlDatabase::database(m_databaseconnection);
    QSqlQuery query(db);
    float pregamma = m_Ui->pregammadsb->value();
    float postsaturation = m_Ui->postsaturationdsb->value();
    float postgamma = m_Ui->postgammadsb->value();
    query.prepare(
        "INSERT INTO fattal (alpha, beta, colorSaturation, noiseReduction, \
        oldFattal, pregamma, comment, postsaturation, postgamma) \
        VALUES (:alpha, :beta, :colorSaturation, :noiseReduction, :oldFattal, \
        :pregamma, :comment, :postsaturation, :postgamma)");
    query.bindValue(QStringLiteral(":alpha"), alpha);
    query.bindValue(QStringLiteral(":beta"), beta);
    query.bindValue(QStringLiteral(":colorSaturation"), colorSaturation);
    query.bindValue(QStringLiteral(":noiseReduction"), noiseReduction);
    query.bindValue(QStringLiteral(":oldFattal"), oldFattal);
    query.bindValue(QStringLiteral(":pregamma"), pregamma);
    query.bindValue(QStringLiteral(":comment"), comment);
    query.bindValue(QStringLiteral(":postsaturation"), postsaturation);
    query.bindValue(QStringLiteral(":postgamma"), postgamma);
    bool res = query.exec();
    if (res == false) qDebug() << query.lastError();
}

void TonemappingPanel::execFerradansQuery(float rho, float inv_alpha,
                                          QString comment) {
    qDebug() << "TonemappingPanel::execFerradansQuery";
    QSqlDatabase db = QSqlDatabase::database(m_databaseconnection);
    QSqlQuery query(db);
    float pregamma = m_Ui->pregammadsb->value();
    float postsaturation = m_Ui->postsaturationdsb->value();
    float postgamma = m_Ui->postgammadsb->value();
    query.prepare(
        "INSERT INTO ferradans (rho, inv_alpha, pregamma, comment, postsaturation, postgamma) \
        VALUES (:rho, :inv_alpha, :pregamma, :comment, :postsaturation, :postgamma)");
    query.bindValue(QStringLiteral(":rho"), rho);
    query.bindValue(QStringLiteral(":inv_alpha"), inv_alpha);
    query.bindValue(QStringLiteral(":pregamma"), pregamma);
    query.bindValue(QStringLiteral(":comment"), comment);
    query.bindValue(QStringLiteral(":postsaturation"), postsaturation);
    query.bindValue(QStringLiteral(":postgamma"), postgamma);
    bool res = query.exec();
    if (res == false) qDebug() << query.lastError();
}

void TonemappingPanel::execFerwerdaQuery(float maxlum, float adaptlum,
                                          QString comment) {
    qDebug() << "TonemappingPanel::execFerwerdaQuery";
    QSqlDatabase db = QSqlDatabase::database(m_databaseconnection);
    QSqlQuery query(db);
    float pregamma = m_Ui->pregammadsb->value();
    float postsaturation = m_Ui->postsaturationdsb->value();
    float postgamma = m_Ui->postgammadsb->value();
    query.prepare(
        "INSERT INTO ferwerda (maxlum, adaptlum, pregamma, comment, postsaturation, postgamma) \
        VALUES (:maxlum, :adaptlum, :pregamma, :comment, :postsaturation, :postgamma)");
    query.bindValue(QStringLiteral(":maxlum"), maxlum);
    query.bindValue(QStringLiteral(":adaptlum"), adaptlum);
    query.bindValue(QStringLiteral(":pregamma"), pregamma);
    query.bindValue(QStringLiteral(":comment"), comment);
    query.bindValue(QStringLiteral(":postsaturation"), postsaturation);
    query.bindValue(QStringLiteral(":postgamma"), postgamma);
    bool res = query.exec();
    if (res == false) qDebug() << query.lastError();
}

void TonemappingPanel::execKimKautzQuery(float kk_c1, float kk_c2,
                                          QString comment) {
    qDebug() << "TonemappingPanel::execKimKautzQuery";
    QSqlDatabase db = QSqlDatabase::database(m_databaseconnection);
    QSqlQuery query(db);
    float pregamma = m_Ui->pregammadsb->value();
    float postsaturation = m_Ui->postsaturationdsb->value();
    float postgamma = m_Ui->postgammadsb->value();
    query.prepare(
        "INSERT INTO kimkautz (kk_c1, kk_c2, pregamma, comment, postsaturation, postgamma) \
        VALUES (:kk_c1, :kk_c2, :pregamma, :comment, :postsaturation, :postgamma)");
    query.bindValue(QStringLiteral(":kk_c1"), kk_c1);
    query.bindValue(QStringLiteral(":kk_c2"), kk_c2);
    query.bindValue(QStringLiteral(":pregamma"), pregamma);
    query.bindValue(QStringLiteral(":comment"), comment);
    query.bindValue(QStringLiteral(":postsaturation"), postsaturation);
    query.bindValue(QStringLiteral(":postgamma"), postgamma);
    bool res = query.exec();
    if (res == false) qDebug() << query.lastError();
}

void TonemappingPanel::execPattanaikQuery(bool autolum, bool local, float cone,
                                          float rod, float multiplier,
                                          QString comment) {
    qDebug() << "TonemappingPanel::execPattanaikQuery";
    QSqlDatabase db = QSqlDatabase::database(m_databaseconnection);
    QSqlQuery query(db);
    float pregamma = m_Ui->pregammadsb->value();
    float postsaturation = m_Ui->postsaturationdsb->value();
    float postgamma = m_Ui->postgammadsb->value();
    query.prepare(
        "INSERT INTO pattanaik (autolum, local, cone, rod, multiplier, \
        pregamma, \
        comment, postsaturation, postgamma) \
        VALUES (:autolum, :local, :cone, :rod, :multiplier, :pregamma, \
        :comment, :postsaturation, :postgamma)");
    query.bindValue(QStringLiteral(":autolum"), autolum);
    query.bindValue(QStringLiteral(":local"), local);
    query.bindValue(QStringLiteral(":cone"), cone);
    query.bindValue(QStringLiteral(":rod"), rod);
    query.bindValue(QStringLiteral(":multiplier"), multiplier);
    query.bindValue(QStringLiteral(":pregamma"), pregamma);
    query.bindValue(QStringLiteral(":comment"), comment);
    query.bindValue(QStringLiteral(":postsaturation"), postsaturation);
    query.bindValue(QStringLiteral(":postgamma"), postgamma);
    bool res = query.exec();
    if (res == false) qDebug() << query.lastError();
}

void TonemappingPanel::execReinhard02Query(bool scales, float key, float phi,
                                           int range, int lower, int upper,
                                           QString comment) {
    qDebug() << "TonemappingPanel::execReinhard02Query";
    QSqlDatabase db = QSqlDatabase::database(m_databaseconnection);
    QSqlQuery query(db);
    float pregamma = m_Ui->pregammadsb->value();
    float postsaturation = m_Ui->postsaturationdsb->value();
    float postgamma = m_Ui->postgammadsb->value();
    query.prepare(
        "INSERT INTO reinhard02 (scales, key, phi, range, lower, upper, \
        pregamma, comment, postsaturation, postgamma) \
        VALUES (:scales, :key, :phi, :range, :lower, :upper, :pregamma, \
        :comment)");
    query.bindValue(QStringLiteral(":scales"), scales);
    query.bindValue(QStringLiteral(":key"), key);
    query.bindValue(QStringLiteral(":phi"), phi);
    query.bindValue(QStringLiteral(":range"), range);
    query.bindValue(QStringLiteral(":lower"), lower);
    query.bindValue(QStringLiteral(":upper"), upper);
    query.bindValue(QStringLiteral(":pregamma"), pregamma);
    query.bindValue(QStringLiteral(":comment"), comment);
    query.bindValue(QStringLiteral(":postsaturation"), postsaturation);
    query.bindValue(QStringLiteral(":postgamma"), postgamma);
    bool res = query.exec();
    if (res == false) qDebug() << query.lastError();
}

void TonemappingPanel::execReinhard05Query(float brightness,
                                           float chromaticAdaptation,
                                           float lightAdaptation,
                                           QString comment) {
    qDebug() << "TonemappingPanel::execReinhard05Query";
    QSqlDatabase db = QSqlDatabase::database(m_databaseconnection);
    QSqlQuery query(db);
    float pregamma = m_Ui->pregammadsb->value();
    float postsaturation = m_Ui->postsaturationdsb->value();
    float postgamma = m_Ui->postgammadsb->value();
    query.prepare(
        "INSERT INTO reinhard05 (brightness, chromaticAdaptation, \
        lightAdaptation, pregamma, comment, postsaturation, postgamma) \
        VALUES (:brightness, :chromaticAdaptation, \
        :lightAdaptation, :pregamma, \
        :comment, :postsaturation, :postgamma)");
    query.bindValue(QStringLiteral(":brightness"), brightness);
    query.bindValue(QStringLiteral(":chromaticAdaptation"),
                    chromaticAdaptation);
    query.bindValue(QStringLiteral(":lightAdaptation"), lightAdaptation);
    query.bindValue(QStringLiteral(":pregamma"), pregamma);
    query.bindValue(QStringLiteral(":comment"), comment);
    query.bindValue(QStringLiteral(":postsaturation"), postsaturation);
    query.bindValue(QStringLiteral(":postgamma"), postgamma);
    bool res = query.exec();
    if (res == false) qDebug() << query.lastError();
}

void TonemappingPanel::execVanHaterenQuery(float pupil_area,
                                          QString comment) {
    qDebug() << "TonemappingPanel::execVanHaterenQuery";
    QSqlDatabase db = QSqlDatabase::database(m_databaseconnection);
    QSqlQuery query(db);
    float pregamma = m_Ui->pregammadsb->value();
    float postsaturation = m_Ui->postsaturationdsb->value();
    float postgamma = m_Ui->postgammadsb->value();
    query.prepare(
        "INSERT INTO vanhateren (pupil_area, pregamma, comment, postsaturation, postgamma) \
        VALUES (:pupil_area, :pregamma, :comment, :postsaturation, :postgamma)");
    query.bindValue(QStringLiteral(":pupil_area"), pupil_area);
    query.bindValue(QStringLiteral(":pregamma"), pregamma);
    query.bindValue(QStringLiteral(":comment"), comment);
    query.bindValue(QStringLiteral(":postsaturation"), postsaturation);
    query.bindValue(QStringLiteral(":postgamma"), postgamma);
    bool res = query.exec();
    if (res == false) qDebug() << query.lastError();
}

bool TonemappingPanel::replaceLdr() {
    return m_Ui->replaceLdrCheckBox->isChecked();
}

bool TonemappingPanel::doAutoLevels() {
    return m_Ui->autoLevelsCheckBox->isChecked();
}

float TonemappingPanel::getAutoLevelsThreshold() {
    return m_autolevelThreshold;
}

void TonemappingPanel::updatePreviews(double v) {
    int index = m_Ui->stackedWidget_operators->currentIndex();
    TonemappingOptions *tmopts =
        new TonemappingOptions(*m_toneMappingOptions);  // make a copy
    fillToneMappingOptions(false);
    QObject *eventSender(sender());
    // Mantiuk06
    if (eventSender == m_Ui->contrastFactordsb)
        tmopts->operator_options.mantiuk06options.contrastfactor = v;
    else if (eventSender == m_Ui->saturationFactordsb)
        tmopts->operator_options.mantiuk06options.saturationfactor = v;
    else if (eventSender == m_Ui->detailFactordsb)
        tmopts->operator_options.mantiuk06options.detailfactor = v;
    // Mantiuk08
    else if (eventSender == m_Ui->colorSaturationDSB)
        tmopts->operator_options.mantiuk08options.colorsaturation = v;
    else if (eventSender == m_Ui->contrastEnhancementDSB)
        tmopts->operator_options.mantiuk08options.contrastenhancement = v;
    else if (eventSender == m_Ui->luminanceLevelDSB)
        tmopts->operator_options.mantiuk08options.luminancelevel = v;
    // Fattal
    else if (eventSender == m_Ui->alphadsb)
        tmopts->operator_options.fattaloptions.alpha = v;
    else if (eventSender == m_Ui->betadsb)
        tmopts->operator_options.fattaloptions.beta = v;
    else if (eventSender == m_Ui->saturation2dsb)
        tmopts->operator_options.fattaloptions.color = v;
    else if (eventSender == m_Ui->noisedsb)
        tmopts->operator_options.fattaloptions.noiseredux = v;
    // Ferradans
    else if (eventSender == m_Ui->rhodsb)
        tmopts->operator_options.ferradansoptions.rho = v;
    else if (eventSender == m_Ui->inv_alphadsb)
        tmopts->operator_options.ferradansoptions.inv_alpha = v;
    // Ferwerda
    else if (eventSender == m_Ui->ferwerdaMultiplierDsb)
        tmopts->operator_options.ferwerdaoptions.multiplier = v;
    else if (eventSender == m_Ui->adaptationLuminanceDsb)
        tmopts->operator_options.ferwerdaoptions.adaptationluminance = v;
    // KimKautz
    else if (eventSender == m_Ui->kimkautzC1Dsb)
        tmopts->operator_options.kimkautzoptions.c1 = v;
    else if (eventSender == m_Ui->kimkautzC2Dsb)
        tmopts->operator_options.kimkautzoptions.c2 = v;
    // Drago
    else if (eventSender == m_Ui->biasdsb)
        tmopts->operator_options.dragooptions.bias = v;
    // Durand
    else if (eventSender == m_Ui->basedsb)
        tmopts->operator_options.durandoptions.base = v;
    else if (eventSender == m_Ui->spatialdsb)
        tmopts->operator_options.durandoptions.spatial = v;
    else if (eventSender == m_Ui->rangedsb)
        tmopts->operator_options.durandoptions.range = v;
    // Reinhard02
    else if (eventSender == m_Ui->keydsb)
        tmopts->operator_options.reinhard02options.key = v;
    else if (eventSender == m_Ui->phidsb)
        tmopts->operator_options.reinhard02options.phi = v;
    else if (eventSender == m_Ui->range2dsb)
        tmopts->operator_options.reinhard02options.range = (int)v;
    else if (eventSender == m_Ui->lowerdsb)
        tmopts->operator_options.reinhard02options.lower = (int)v;
    else if (eventSender == m_Ui->upperdsb)
        tmopts->operator_options.reinhard02options.upper = (int)v;
    // Reinhard05
    else if (eventSender == m_Ui->brightnessdsb)
        tmopts->operator_options.reinhard05options.brightness = v;
    else if (eventSender == m_Ui->chromaticAdaptdsb)
        tmopts->operator_options.reinhard05options.chromaticAdaptation = v;
    else if (eventSender == m_Ui->lightAdaptdsb)
        tmopts->operator_options.reinhard05options.lightAdaptation = v;
    // Ashikhmin
    else if (eventSender == m_Ui->contrastdsb)
        tmopts->operator_options.ashikhminoptions.lct = v;
    // Pattanaik
    else if (eventSender == m_Ui->multiplierdsb)
        tmopts->operator_options.pattanaikoptions.multiplier = v;
    else if (eventSender == m_Ui->conedsb)
        tmopts->operator_options.pattanaikoptions.cone = v;
    else if (eventSender == m_Ui->roddsb)
        tmopts->operator_options.pattanaikoptions.rod = v;
    // VanHateren
    else if (eventSender == m_Ui->pupil_areaDsb)
        tmopts->operator_options.vanhaterenoptions.pupil_area = v;
    // else if(eventSender == m_Ui->pregammadsb)
    //    tmopts->pregamma = v;

    if (index >= 0) {
        if (eventSender == m_Ui->pregammadsb) {
            int maxIndex = m_Ui->stackedWidget_operators->count();
            for (int i = 0; i < maxIndex; i++) {
                updateCurrentTmoOperator(i);
                fillToneMappingOptions(false);
                TonemappingOptions *tmopts =
                    new TonemappingOptions(*m_toneMappingOptions);  // make a copy
                tmopts->pregamma = v;
                m_previewPanel->getLabel(i)->setTonemappingOptions(tmopts);
                m_previewPanel->updatePreviews(m_currentFrame, i);
            }
            updateCurrentTmoOperator(index);
        } else if (eventSender == m_Ui->postgammadsb) {
            int maxIndex = m_Ui->stackedWidget_operators->count();
            for (int i = 0; i < maxIndex; i++) {
                updateCurrentTmoOperator(i);
                fillToneMappingOptions(false);
                TonemappingOptions *tmopts =
                    new TonemappingOptions(*m_toneMappingOptions);  // make a copy
                tmopts->postgamma = v;
                m_previewPanel->getLabel(i)->setTonemappingOptions(tmopts);
                m_previewPanel->updatePreviews(m_currentFrame, i);
            }
            updateCurrentTmoOperator(index);
        } else if (eventSender == m_Ui->postsaturationdsb) {
            int maxIndex = m_Ui->stackedWidget_operators->count();
            for (int i = 0; i < maxIndex; i++) {
                updateCurrentTmoOperator(i);
                fillToneMappingOptions(false);
                TonemappingOptions *tmopts =
                    new TonemappingOptions(*m_toneMappingOptions);  // make a copy
                tmopts->postsaturation = v;
                m_previewPanel->getLabel(i)->setTonemappingOptions(tmopts);
                m_previewPanel->updatePreviews(m_currentFrame, i);
            }
            updateCurrentTmoOperator(index);
        } else {

            m_previewPanel->getLabel(index)->setTonemappingOptions(tmopts);
            m_previewPanel->updatePreviews(m_currentFrame, index);
        }
    }
    delete tmopts;
}

void TonemappingPanel::updatePreviewsCB(bool state) {
    int index = m_Ui->stackedWidget_operators->currentIndex();
    fillToneMappingOptions(false);
    TonemappingOptions *tmopts =
        new TonemappingOptions(*m_toneMappingOptions);  // make a copy
    QObject *eventSender(sender());
    // Mantiuk06
    if (eventSender == m_Ui->contrastEqualizCheckBox)
        tmopts->operator_options.mantiuk06options.contrastequalization = state;
    // Mantiuk08
    else if (eventSender == m_Ui->luminanceLevelCheckBox)
        tmopts->operator_options.mantiuk08options.luminancelevel = state;
    // Fattal
    else if (eventSender == m_Ui->fftVersionCheckBox)
        tmopts->operator_options.fattaloptions.fftsolver = state;
    // Reinhard02
    else if (eventSender == m_Ui->usescalescheckbox)
        tmopts->operator_options.reinhard02options.scales = state;
    // Ashikhmin
    else if (eventSender == m_Ui->simpleCheckBox)
        tmopts->operator_options.ashikhminoptions.simple = state;
    // Pattanaik
    else if (eventSender == m_Ui->pattalocal)
        tmopts->operator_options.pattanaikoptions.local = state;
    else if (eventSender == m_Ui->autoYcheckbox)
        tmopts->operator_options.pattanaikoptions.autolum = state;

    if (index >= 0) {
        m_previewPanel->getLabel(index)->setTonemappingOptions(tmopts);
        m_previewPanel->updatePreviews(m_currentFrame, index);
    }
    delete tmopts;
}

void TonemappingPanel::updatePreviewsRB(bool toggled) {
    int index = m_Ui->stackedWidget_operators->currentIndex();
    TonemappingOptions *tmopts =
        new TonemappingOptions(*m_toneMappingOptions);  // make a copy
    fillToneMappingOptions(false);

    // Only one sender: Ashikhmin
    tmopts->operator_options.ashikhminoptions.eq2 = toggled;

    if (index >= 0) {
        m_previewPanel->getLabel(index)->setTonemappingOptions(tmopts);
        m_previewPanel->updatePreviews(m_currentFrame, index);
    }
    delete tmopts;
}

void TonemappingPanel::setRealtimePreviews(bool toggled) {
    if (toggled) {
        fillToneMappingOptions(false);

        // Mantiuk06
        connect(m_Ui->contrastFactordsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        connect(m_Ui->saturationFactordsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        connect(m_Ui->detailFactordsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        connect(m_Ui->contrastEqualizCheckBox, &QCheckBox::stateChanged, this,
                &TonemappingPanel::updatePreviewsCB);

        // Mantiuk08
        connect(m_Ui->colorSaturationDSB, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        connect(m_Ui->contrastEnhancementDSB, SIGNAL(valueChanged(double)),
                this, SLOT(updatePreviews(double)));
        connect(m_Ui->luminanceLevelDSB, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        connect(m_Ui->luminanceLevelCheckBox, &QCheckBox::stateChanged, this,
                &TonemappingPanel::updatePreviewsCB);

        // Fattal
        connect(m_Ui->alphadsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        connect(m_Ui->betadsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        connect(m_Ui->saturation2dsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        connect(m_Ui->noisedsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        connect(m_Ui->fftVersionCheckBox, &QCheckBox::stateChanged, this,
                &TonemappingPanel::updatePreviewsCB);

        // Ferradans
        connect(m_Ui->rhodsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        connect(m_Ui->inv_alphadsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));

        // Ferwerda
        connect(m_Ui->ferwerdaMultiplierDsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        connect(m_Ui->adaptationLuminanceDsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));

        // KimKautz
        connect(m_Ui->kimkautzC1Dsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        connect(m_Ui->kimkautzC2Dsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));

        // Drago
        connect(m_Ui->biasdsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));

        // Durand
        connect(m_Ui->basedsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        connect(m_Ui->spatialdsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        connect(m_Ui->rangedsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));

        // Reinhard02
        connect(m_Ui->keydsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        connect(m_Ui->phidsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        connect(m_Ui->range2dsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        connect(m_Ui->lowerdsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        connect(m_Ui->upperdsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        connect(m_Ui->usescalescheckbox, &QCheckBox::stateChanged, this,
                &TonemappingPanel::updatePreviewsCB);

        // Reinhard05
        connect(m_Ui->brightnessdsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        connect(m_Ui->chromaticAdaptdsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        connect(m_Ui->lightAdaptdsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));

        // Ashikhmin
        connect(m_Ui->contrastdsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        connect(m_Ui->simpleCheckBox, &QCheckBox::stateChanged, this,
                &TonemappingPanel::updatePreviewsCB);
        connect(m_Ui->eq2RadioButton, &QAbstractButton::toggled, this,
                &TonemappingPanel::updatePreviewsRB);

        // Pattanaik
        connect(m_Ui->multiplierdsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        connect(m_Ui->conedsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        connect(m_Ui->roddsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        connect(m_Ui->pattalocal, &QCheckBox::stateChanged, this,
                &TonemappingPanel::updatePreviewsCB);
        connect(m_Ui->autoYcheckbox, &QCheckBox::stateChanged, this,
                &TonemappingPanel::updatePreviewsCB);

        // VanHateren
        connect(m_Ui->pupil_areaDsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));

        // GLOBAL Pregamma
        connect(m_Ui->pregammadsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        connect(m_Ui->postgammadsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        connect(m_Ui->postsaturationdsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
    } else {
        // Mantiuk06
        disconnect(m_Ui->contrastFactordsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        disconnect(m_Ui->saturationFactordsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        disconnect(m_Ui->detailFactordsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        disconnect(m_Ui->contrastEqualizCheckBox, &QCheckBox::stateChanged, this,
                &TonemappingPanel::updatePreviewsCB);

        // Mantiuk08
        disconnect(m_Ui->colorSaturationDSB, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        disconnect(m_Ui->contrastEnhancementDSB, SIGNAL(valueChanged(double)),
                this, SLOT(updatePreviews(double)));
        disconnect(m_Ui->luminanceLevelDSB, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        disconnect(m_Ui->luminanceLevelCheckBox, &QCheckBox::stateChanged, this,
                &TonemappingPanel::updatePreviewsCB);

        // Fattal
        disconnect(m_Ui->alphadsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        disconnect(m_Ui->betadsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        disconnect(m_Ui->saturation2dsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        disconnect(m_Ui->noisedsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        disconnect(m_Ui->fftVersionCheckBox, &QCheckBox::stateChanged, this,
                &TonemappingPanel::updatePreviewsCB);

        // Ferradans
        disconnect(m_Ui->rhodsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        disconnect(m_Ui->inv_alphadsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));

        // Ferwerda
        disconnect(m_Ui->ferwerdaMultiplierDsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        disconnect(m_Ui->adaptationLuminanceDsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));

        // KimKautz
        disconnect(m_Ui->kimkautzC1Dsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        disconnect(m_Ui->kimkautzC2Dsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));

        // Drago
        disconnect(m_Ui->biasdsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));

        // Durand
        disconnect(m_Ui->basedsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        disconnect(m_Ui->spatialdsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        disconnect(m_Ui->rangedsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));

        // Reinhard02
        disconnect(m_Ui->keydsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        disconnect(m_Ui->phidsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        disconnect(m_Ui->range2dsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        disconnect(m_Ui->lowerdsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        disconnect(m_Ui->upperdsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        disconnect(m_Ui->usescalescheckbox, &QCheckBox::stateChanged, this,
                &TonemappingPanel::updatePreviewsCB);

        // Reinhard05
        disconnect(m_Ui->brightnessdsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        disconnect(m_Ui->chromaticAdaptdsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        disconnect(m_Ui->lightAdaptdsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));

        // Ashikhmin
        disconnect(m_Ui->contrastdsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        disconnect(m_Ui->simpleCheckBox, &QCheckBox::stateChanged, this,
                &TonemappingPanel::updatePreviewsCB);
        disconnect(m_Ui->eq2RadioButton, &QAbstractButton::toggled, this,
                &TonemappingPanel::updatePreviewsRB);

        // Pattanaik
        disconnect(m_Ui->multiplierdsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        disconnect(m_Ui->conedsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        disconnect(m_Ui->roddsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        disconnect(m_Ui->pattalocal, &QCheckBox::stateChanged, this,
                &TonemappingPanel::updatePreviewsCB);
        disconnect(m_Ui->autoYcheckbox, &QCheckBox::stateChanged, this,
                &TonemappingPanel::updatePreviewsCB);

        // VanHateren
        disconnect(m_Ui->pupil_areaDsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));

        // GLOBAL Pregamma
        disconnect(m_Ui->pregammadsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        disconnect(m_Ui->postgammadsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
        disconnect(m_Ui->postsaturationdsb, SIGNAL(valueChanged(double)), this,
                SLOT(updatePreviews(double)));
    }
}

void TonemappingPanel::on_pattalocal_toggled(bool b) {
    bool autoY = m_Ui->autoYcheckbox->isChecked();

    m_Ui->label_cone->setDisabled(b || autoY);
    m_Ui->coneSlider->setDisabled(b || autoY);
    m_Ui->conedsb->setDisabled(b || autoY);
    m_Ui->label_rod->setDisabled(b || autoY);
    m_Ui->rodSlider->setDisabled(b || autoY);
    m_Ui->roddsb->setDisabled(b || autoY);
}

void TonemappingPanel::autoLevels(bool b) {
    emit autoLevels(b, m_autolevelThreshold);
}

void TonemappingPanel::on_toolButtonThreshold_clicked() {
    QPoint pos = mapToGlobal(m_Ui->toolButtonThreshold->pos());
    m_thd->move(pos.x() - 40, pos.y() - 20);
    m_thd->show();
}

void TonemappingPanel::thresholdReady() {
    m_autolevelThreshold = m_thd->threshold();
    m_thd->hide();
    emit autoLevels(doAutoLevels(), m_autolevelThreshold);
}

QString & TonemappingPanel::getDatabaseConnection() {
    return m_databaseconnection;
}
// ------------------------- // END FILE
