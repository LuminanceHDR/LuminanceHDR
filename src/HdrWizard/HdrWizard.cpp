/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2006,2007 Giuseppe Rota
 * Copyrighr (C) 2010,2011,2012 Franco Comida
 * Copyright (C) 2013 Davide Anastasia
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
 */

#include "HdrWizard.h"
#include "ui_HdrWizard.h"

#include <cmath>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#include <QDebug>

#include <QStringList>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QUrl>
#include <QMimeData>
#include <QMessageBox>
#include <QMimeData>
#include <QProcess>
#include <QRegExp>
#include <QTextStream>
#include <QProgressDialog>
#include <QThread>
#include <QtConcurrentRun>

// --- SQL handling
#include <QSqlRecord>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QSqlError>
// --- end SQL handling

#include "arch/math.h"
#include "arch/freebsd/math.h"
#include "Common/config.h"
#include "Common/global.h"
#include "OsIntegration/osintegration.h"
#include "HdrWizard/EditingTools.h"
#include "HdrWizard/HdrCreationManager.h"

using namespace libhdr::fusion;


const ResponseFunction responses_in_gui[4] = { RESPONSE_GAMMA, RESPONSE_LINEAR, RESPONSE_LOG10, RESPONSE_CUSTOM };
const FusionOperator models_in_gui[2] = { DEBEVEC, ROBERTSON };
const WeightFunction weights_in_gui[3] = { WEIGHT_TRIANGULAR, WEIGHT_GAUSSIAN, WEIGHT_PLATEAU };

HdrWizard::HdrWizard(QWidget *p,
                     const QStringList &files,
                     const QStringList &/*inputFilesName*/,
                     const QVector<float> &/*inputExpoTimes*/)
    : QDialog(p)
    , m_ui(new Ui::HdrWizard)
    , m_hdrCreationManager(new HdrCreationManager)
    , m_doAutoAntighosting(false)
    , m_doManualAntighosting(false)
    , m_processing(false)
{
    m_ui->setupUi(this);
    setAcceptDrops(true);
    setupConnections();

    m_ui->tableWidget->setHorizontalHeaderLabels(
                QStringList() << tr("Image Filename") << tr("Exposure")
                );
    m_ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_ui->tableWidget->verticalHeader()->hide();
    // m_ui->tableWidget->resizeColumnsToContents();
    
    m_ui->progressBar->hide();
    m_ui->textEdit->hide();

    if (files.size())
    {
        m_ui->pagestack->setCurrentIndex(0);

        QMetaObject::invokeMethod(this, "loadInputFiles", Qt::QueuedConnection,
                                  Q_ARG(QStringList, files));
    }

    // Same code as the one in BatchHDRDialog!!! TODO: put everything in the same
    // place :(
    /*
    QSqlQueryModel model;
    model.setQuery("SELECT * FROM parameters"); 
    for (int i = 0; i < model.rowCount(); i++) {
        m_ui->predefConfigsComboBox->addItem(tr("Custom config %1").arg(i+1));
        int weight_ = model.record(i).value("weight").toInt(); 
        int response_ = model.record(i).value("response").toInt(); 
        int model_ = model.record(i).value("model").toInt(); 
        QString filename_ = model.record(i).value("filename").toString(); 
        FusionOperatorConfig ct;
        switch (weight_) {
            case 0:
                ct.weights = TRIANGULAR;
                break;
            case 1:
                ct.weights = GAUSSIAN;
                break;
            case 2:
                ct.weights = PLATEAU;
                break;
        }
        switch (response_) {
            case 0:
                ct.response_curve = FROM_FILE;
                ct.LoadCurveFromFilename = filename_;
                ct.SaveCurveToFilename = "";    
                break;
            case 1:
                ct.response_curve = LINEAR;
                break; 
            case 2:
                ct.response_curve = GAMMA;
                break; 
            case 3:
                ct.response_curve = LOG10;
                break; 
            case 4:
                ct.response_curve = FROM_ROBERTSON;
                break; 
        }
        switch (model_) {
            case 0:
                ct.model = DEBEVEC;
                break; 
            case 1:
                ct.model = ROBERTSON;
                break; 
        }
        m_customConfig.push_back(ct);   
    }
    */
}

HdrWizard::~HdrWizard()
{
#ifdef QT_DEBUG
    qDebug() << "HdrWizard::~HdrWizard()";
#endif
}

void HdrWizard::setupConnections()
{
    //connect(&m_ioFutureWatcher, SIGNAL(finished()), this, SLOT(loadInputFilesDone()));

    connect(m_hdrCreationManager.data(), SIGNAL(finishedLoadingFiles()), this, SLOT(loadInputFilesDone()));
    //connect(m_hdrCreationManager.data(), SIGNAL(progressStarted()), m_ui->progressBar, SLOT(show()), Qt::DirectConnection);
    //connect(m_hdrCreationManager.data(), SIGNAL(progressFinished()), m_ui->progressBar, SLOT(reset()));
    //connect(m_hdrCreationManager.data(), SIGNAL(progressFinished()), m_ui->progressBar, SLOT(hide()), Qt::DirectConnection);
    
    connect(m_hdrCreationManager.data(), SIGNAL(progressRangeChanged(int,int)), this, SIGNAL(setRange(int,int)), Qt::DirectConnection);
    connect(m_hdrCreationManager.data(), SIGNAL(progressValueChanged(int)), this, SIGNAL(setValue(int)), Qt::DirectConnection);
    
    connect(this, SIGNAL(setValue(int)), m_ui->progressBar, SLOT(setValue(int)), Qt::DirectConnection);
    connect(this, SIGNAL(setRange(int,int)), m_ui->progressBar, SLOT(setRange(int,int)), Qt::DirectConnection);

    connect(this, SIGNAL(setValue(int)), OsIntegration::getInstancePtr(), SLOT(setProgressValue(int)), Qt::DirectConnection);
    connect(this, SIGNAL(setRange(int,int)), OsIntegration::getInstancePtr(), SLOT(setProgressRange(int,int)), Qt::DirectConnection);

    connect(m_ui->NextFinishButton, SIGNAL(clicked()), this, SLOT(NextFinishButtonClicked()));
    connect(m_ui->cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
    connect(m_ui->pagestack, SIGNAL(currentChanged(int)), this, SLOT(currentPageChangedInto(int)));

    connect(m_ui->loadImagesButton, SIGNAL(clicked()), this, SLOT(loadImagesButtonClicked()));
    connect(m_ui->removeImageButton, SIGNAL(clicked()), this, SLOT(removeImageButtonClicked()));
    connect(m_ui->clearListButton, SIGNAL(clicked()), this, SLOT(clearListButtonClicked()));

    connect(m_ui->tableWidget, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(inputHdrFileSelected(int)));
    connect(m_ui->EVSlider, SIGNAL(valueChanged(int)), this, SLOT(updateEVSlider(int)));
    connect(m_ui->ImageEVdsb, SIGNAL(valueChanged(double)), this, SLOT(updateEVSpinBox(double)));
    /*
    connect(m_ui->ais_radioButton, SIGNAL(clicked()), this, SLOT(alignSelectionClicked()));
    connect(m_ui->mtb_radioButton, SIGNAL(clicked()), this, SLOT(alignSelectionClicked()));
*/
    connect(m_ui->predefConfigsComboBox, SIGNAL(activated(int)), this, SLOT(predefConfigsComboBoxActivated(int)));
    /*
    connect(m_ui->antighostRespCurveCombobox, SIGNAL(activated(int)), this, SLOT(antighostRespCurveComboboxActivated(int)));
    */
    connect(m_ui->customConfigCheckBox, SIGNAL(toggled(bool)), this, SLOT(customConfigCheckBoxToggled(bool)));
    connect(m_ui->triGaussPlateauComboBox, SIGNAL(activated(int)), this, SLOT(triGaussPlateauComboBoxActivated(int)));
    /*
    connect(m_ui->predefRespCurveRadioButton, SIGNAL(toggled(bool)), this, SLOT(predefRespCurveRadioButtonToggled(bool)));
    */

    connect(m_ui->gammaLinLogComboBox, SIGNAL(activated(int)), this, SLOT(gammaLinLogComboBoxActivated(int)));
    connect(m_ui->loadRespCurveFromFileCheckbox, SIGNAL(toggled(bool)), this, SLOT(loadRespCurveFromFileCheckboxToggled(bool)));
    connect(m_ui->loadRespCurveFileButton, SIGNAL(clicked()), this, SLOT(loadRespCurveFileButtonClicked()));
    connect(m_ui->saveRespCurveToFileCheckbox, SIGNAL(toggled(bool)), this, SLOT(saveRespCurveToFileCheckboxToggled(bool)));
    connect(m_ui->saveRespCurveFileButton, SIGNAL(clicked()), this, SLOT(saveRespCurveFileButtonClicked()));
    connect(m_ui->modelComboBox, SIGNAL(activated(int)), this, SLOT(modelComboBoxActivated(int)));

    connect(m_ui->RespCurveFileLoadedLineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(loadRespCurveFilename(const QString&)));
    /*
    connect(m_hdrCreationManager.data(), SIGNAL(fileLoaded(int,QString,float)), this, SLOT(fileLoaded(int,QString,float)));
    connect(m_hdrCreationManager.data(), SIGNAL(finishedLoadingInputFiles(QStringList)), this, SLOT(finishedLoadingInputFiles(QStringList)));
    */

    connect(m_hdrCreationManager.data(), SIGNAL(errorWhileLoading(QString)), this, SLOT(errorWhileLoading(QString)));
    //connect(m_hdrCreationManager.data(), SIGNAL(expotimeValueChanged(float,int)), this, SLOT(updateGraphicalEVvalue(float,int)));
    connect(m_hdrCreationManager.data(), SIGNAL(finishedAligning(int)), this, SLOT(finishedAligning(int)));
    connect(m_hdrCreationManager.data(), SIGNAL(ais_failed(QProcess::ProcessError)), this, SLOT(ais_failed(QProcess::ProcessError)));
    connect(m_hdrCreationManager.data(), SIGNAL(aisDataReady(QByteArray)), this, SLOT(writeAisData(QByteArray)));

    connect(this, SIGNAL(rejected()), m_hdrCreationManager.data(), SLOT(removeTempFiles()));
    //connect(this, SIGNAL(rejected()), OsIntegration::getInstancePtr(), SLOT(setProgressValue(-1)), Qt::DirectConnection);
    connect(m_ui->threshold_horizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(updateThresholdSlider(int)));
    connect(m_ui->threshold_doubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateThresholdSpinBox(double)));
}

void HdrWizard::loadImagesButtonClicked()
{
    QString filetypes;
    // when changing these filetypes, also change in DnDOption - for Drag and Drop
    filetypes += tr("All formats (*.jpeg *.jpg *.tiff *.tif *.crw *.cr2 *.nef *.dng *.mrw *.orf *.kdc *.dcr *.arw *.raf *.ptx *.pef *.x3f *.raw *.sr2 *.rw2 *.3fr *.mef *.mos *.erf *.nrw *.srw");
    filetypes += tr("*.JPEG *.JPG *.TIFF *.TIF *.CRW *.CR2 *.NEF *.DNG *.MRW *.ORF *.KDC *.DCR *.ARW *.RAF *.PTX *.PEF *.X3F *.RAW *.SR2 *.RW2 *.3FR *.MEF *.MOS *.ERF *.NRW *.SRW);;");
    filetypes += tr("JPEG (*.jpeg *.jpg *.JPEG *.JPG);;");
    filetypes += tr("TIFF Images (*.tiff *.tif *.TIFF *.TIF);;");
    filetypes += tr("RAW Images (*.crw *.cr2 *.nef *.dng *.mrw *.orf *.kdc *.dcr *.arw *.raf *.ptx *.pef *.x3f *.raw *.sr2 *.rw2 *.3fr *.mef *.mos *.erf *.nrw *.srw");
    filetypes += tr("*.CRW *.CR2 *.NEF *.DNG *.MRW *.ORF *.KDC *.DCR *.ARW *.RAF *.PTX *.PEF *.X3F *.RAW *.SR2 *.RW2 *.3FR *.MEF *.MOS *.ERF *.NRW *.SRW)");

    QStringList files = QFileDialog::getOpenFileNames(this, tr("Select the input images"), luminance_options.getDefaultPathLdrIn(), filetypes );

    loadInputFiles(files);
}

void HdrWizard::updateTableGrid()
{
    qDebug() << "Fill grid with values in the m_data structure";

    int currentRow = m_ui->tableWidget->currentRow();

    // empty grid...
    m_ui->tableWidget->setRowCount(0);

    // insert the row at the bottom of the table widget
    int counter = 0;
    QStringList filesWithoutExif;
    BOOST_FOREACH(const HdrCreationItem& item, *m_hdrCreationManager)
    {
        qDebug() << QString("Fill row %1: %2 %3 EV")
                    .arg(counter)
                    .arg(item.filename())
                    .arg(item.getEV());

        // fill graphical list
        m_ui->tableWidget->insertRow(counter);
        m_ui->tableWidget->setItem(counter, 0, new QTableWidgetItem(QFileInfo(item.filename()).fileName()));
        if (item.hasEV())
        {
            QString EVdisplay;
            QTextStream ts(&EVdisplay);
            ts.setRealNumberPrecision(2);
            ts << right << forcesign << fixed << item.getEV() << " EV";
            QTableWidgetItem *tableitem = new QTableWidgetItem(EVdisplay);
            tableitem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_ui->tableWidget->setItem(counter, 1, tableitem);
        }
        else
        {
            // if image doesn't contain (the required) exif tags
            // I keep the name of all the files without exif data...
            filesWithoutExif.push_back(item.filename());


            QTableWidgetItem *tableitem = new QTableWidgetItem(QString(tr("Unknown")));
            tableitem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            tableitem->setBackground(QBrush(Qt::yellow));
            tableitem->setForeground(QBrush(Qt::red));
            m_ui->tableWidget->setItem(counter, 1, tableitem);
        }

        ++counter;
    }

    if ( currentRow >= 0 && currentRow < (int)m_hdrCreationManager->availableInputFiles()) {
        m_ui->tableWidget->selectRow(currentRow);
    } else if (currentRow >= 0) {
        m_ui->tableWidget->selectRow(0);
    } else {
        m_ui->tableWidget->selectRow(-1);
    }

    if ( counter ) {
        m_ui->clearListButton->setEnabled(true);

        enableNextOrWarning(filesWithoutExif);
    } else {
        m_ui->clearListButton->setEnabled(false);
        m_ui->removeImageButton->setEnabled(false);
        m_ui->NextFinishButton->setEnabled(false);

        m_ui->confirmloadlabel->setText(QString());
    }
}

void HdrWizard::removeImageButtonClicked()
{
    qDebug() << "HdrWizard::removeImageButtonClicked()";

    int index = m_ui->tableWidget->currentRow();

    Q_ASSERT(index >= 0);
    Q_ASSERT(index < m_hdrCreationManager->availableInputFiles());

    m_hdrCreationManager->removeFile(index);

    updateTableGrid();
}

void HdrWizard::clearListButtonClicked()
{
    qDebug() << "HdrWizard::clearListButtonClicked()";

    m_hdrCreationManager->clearFiles();

    updateTableGrid();
}

void HdrWizard::updateEVSlider(int newValue)
{
    int currentRow = m_ui->tableWidget->currentRow();
    float newEV = ((float)newValue)/100.f;
    bool oldState = m_ui->ImageEVdsb->blockSignals(true);
    m_ui->ImageEVdsb->setValue( newEV );
    m_ui->ImageEVdsb->blockSignals(oldState);

    qDebug() << QString("HdrWizard::updateEVSlider(%1) for %2")
                .arg(newEV).arg(currentRow);

    QTableWidgetItem *tableitem = m_ui->tableWidget->item(currentRow, 1);
    if ( tableitem )
    {
        QString EVdisplay;
        QTextStream ts(&EVdisplay);
        ts.setRealNumberPrecision(2);
        ts << right << forcesign << fixed << newEV << " EV";
        tableitem->setBackground(QBrush(Qt::white));
        tableitem->setForeground(QBrush(Qt::black));
        tableitem->setText(EVdisplay);
    }

    m_hdrCreationManager->getFile(currentRow).setEV(newEV);
    updateLabelMaybeNext( m_hdrCreationManager->numFilesWithoutExif() );
}

void HdrWizard::updateEVSpinBox(double newEV)
{
    int currentRow = m_ui->tableWidget->currentRow();

    bool oldState = m_ui->EVSlider->blockSignals(true);
    m_ui->EVSlider->setValue( (int)(newEV*100) );
    m_ui->EVSlider->blockSignals(oldState);

    qDebug() << QString("HdrWizard::updateEVSpinBox(%1) for %2")
                .arg(newEV).arg(currentRow);

    QTableWidgetItem *tableitem = m_ui->tableWidget->item(currentRow, 1);
    if ( tableitem )
    {
        QString EVdisplay;
        QTextStream ts(&EVdisplay);
        ts.setRealNumberPrecision(2);
        ts << right << forcesign << fixed << newEV << " EV";
        tableitem->setBackground(QBrush(Qt::white));
        tableitem->setForeground(QBrush(Qt::black));
        tableitem->setText(EVdisplay);
    }

    m_hdrCreationManager->getFile(currentRow).setEV(newEV);
    updateLabelMaybeNext( m_hdrCreationManager->numFilesWithoutExif() );
}

void HdrWizard::inputHdrFileSelected(int currentRow)
{
    qDebug() << QString("HdrWizard::inputHdrFileSelected(%1)").arg(currentRow);

    if ( currentRow < 0 || m_ui->tableWidget->rowCount() < 0 ) {
        // no selection...
        m_ui->EVgroupBox->setEnabled(false);
        m_ui->removeImageButton->setEnabled(false);
        m_ui->clearListButton->setEnabled(false);
        m_ui->NextFinishButton->setEnabled(false);
        m_ui->previewLabel->clear();
    }
    else // if ( m_ui->tableWidget->rowCount() > 0 )
    {
        // enable remove button
        m_ui->removeImageButton->setEnabled(true);
        m_ui->clearListButton->setEnabled(true);

        // update ev slider and spinbox
        m_ui->ImageEVdsb->blockSignals(true);
        m_ui->EVSlider->blockSignals(true);

        m_ui->EVgroupBox->setEnabled(true);
        if ( m_hdrCreationManager->getFile(currentRow).hasEV() )
        {
            m_ui->ImageEVdsb->setValue( m_hdrCreationManager->getFile(currentRow).getEV() );
            m_ui->EVSlider->setValue( (int)(m_hdrCreationManager->getFile(currentRow).getEV()*100.f + 0.5f) );
        }
        else
        {
            m_ui->ImageEVdsb->setValue( 0.0 );
            m_ui->EVSlider->setValue( 0 );
        }
        m_ui->ImageEVdsb->blockSignals(false);
        m_ui->EVSlider->blockSignals(false);

        // load QImage...
        m_ui->previewLabel->setPixmap(
                    QPixmap::fromImage(
                        m_hdrCreationManager->getFile(currentRow).qimage()->scaled(
                            m_ui->previewLabel->size(), Qt::KeepAspectRatio)
                        ));

        m_ui->ImageEVdsb->setFocus();
    }
}


void HdrWizard::dragEnterEvent(QDragEnterEvent *event)
{
    if (m_ui->loadImagesButton->isEnabled()) {
        event->acceptProposedAction();
    }
}

void HdrWizard::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        QStringList files = convertUrlListToFilenameList(event->mimeData()->urls());
        loadInputFiles(files);
    }
    event->acceptProposedAction();
}

void HdrWizard::loadInputFiles(const QStringList& files)
{
    if ( !files.isEmpty() )
    {
        // update the luminance_options
        luminance_options.setDefaultPathLdrIn(QFileInfo(files.at(0)).path());

        m_ui->loadImagesButton->setEnabled(false);
        m_ui->confirmloadlabel->setText("<center><h3><b>" + tr("Loading...") + "</b></h3></center>");
/*
        QProgressDialog progressDialog(this);
        progressDialog.setWindowModality(Qt::WindowModal);
        progressDialog.setLabelText(QString("Loading %1 file(s) using %2 thread(s)...")
                                    .arg(files.size())
                                    .arg(QThread::idealThreadCount())
                                    );

        connect(m_hdrCreationManager.data(), SIGNAL(progressStarted()), &progressDialog, SLOT(exec()));
        connect(m_hdrCreationManager.data(), SIGNAL(progressStarted()), &progressDialog, SLOT(show()));
        connect(m_hdrCreationManager.data(), SIGNAL(progressFinished()), &progressDialog, SLOT(reset()));
        connect(m_hdrCreationManager.data(), SIGNAL(progressFinished()), &progressDialog, SLOT(hide()));
        connect(m_hdrCreationManager.data(), SIGNAL(progressRangeChanged(int,int)), &progressDialog, SLOT(setRange(int,int)));
        connect(m_hdrCreationManager.data(), SIGNAL(progressValueChanged(int)), &progressDialog, SLOT(setValue(int)));
        connect(&progressDialog, SIGNAL(canceled()), m_hdrCreationManager.data(), SIGNAL(progressCancel()));
*/
        QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
        m_ui->progressBar->show();

        // m_hdrCreationManager->loadFiles(files);
        //connect(&m_futureWatcher, SIGNAL(started()), m_ui->progressBar, SLOT(show()));
        //connect(&m_futureWatcher, SIGNAL(finished()), m_ui->progressBar, SLOT(hide()));

        m_futureWatcher.setFuture(
                    QtConcurrent::run(
                        boost::bind(&HdrCreationManager::loadFiles,
                                    m_hdrCreationManager.data(),
                                    files)
                        )
                    );


        // Query the progress dialog to check if was canceled.
        // qDebug() << "Canceled?" << progressDialog.wasCanceled();
    }
}

void HdrWizard::loadInputFilesDone()
{
    m_futureWatcher.waitForFinished();    // should breeze over...
    qDebug() << "HdrWizard::loadInputFilesDone()";

    m_ui->progressBar->hide();
    m_ui->loadImagesButton->setEnabled(true);

    QApplication::restoreOverrideCursor();
    updateTableGrid();
    inputHdrFileSelected(0);
    m_ui->tableWidget->selectRow(0);
    m_ui->alignGroupBox->setEnabled(true);
    m_ui->agGroupBox->setEnabled(true);
    if (m_ui->tableWidget->rowCount() > 1) {
        m_ui->alignCheckBox->setEnabled(true);
    }
}

// this function should be called if we have at least a file currently in
// memory, otherwise it will give a misleading information
void HdrWizard::enableNextOrWarning(const QStringList& filesWithoutExif)
{
    // If I have at least one file with empty EXIF, I raise an error...
    if ( !filesWithoutExif.empty() )
    {
        QString warningMessage =
                tr("<font color=\"#FF0000\"><h3><b>WARNING:</b></h3></font> "\
                   "Luminance HDR was not able to find the relevant <b>EXIF</b> " \
                   "tags for the following images:" \
                   "<ul>");

        foreach(const QString& filename, filesWithoutExif) {
            QFileInfo qfi(filename);
            warningMessage += "<li>" + qfi.baseName() + "</li>";
        }

        warningMessage +=
                tr("</ul>"\
                   "<hr>You can still proceed creating an Hdr. To do so you have to insert <b>manually</b> the EV (exposure values) or stop difference values." \
                   "<hr>If you want Luminance HDR to do this <b>automatically</b>, you have to load images that have at least the following exif data: " \
                   "<ul><li>Exposure Bias</li></ul>"
                   "<hr><b>HINT:</b> Losing EXIF data usually happens when you preprocess your pictures.<br>" \
                   "You can perform a <b>one-to-one copy of the exif data</b> between two sets of images via the " \
                   "<b>Tools->Copy Exif Data...</b> menu item.");

        // "<ul><li>Shutter Speed (seconds)</li>"
        // "<li>Aperture (f-number)</li></ul>"

        QMessageBox::warning(this, tr("EXIF data not found"), warningMessage);
    }
    updateLabelMaybeNext(filesWithoutExif.size());
}

void HdrWizard::updateLabelMaybeNext(size_t numFilesWithoutExif)
{
    if ( numFilesWithoutExif == 0 ) {
        m_ui->NextFinishButton->setEnabled(true);
        m_ui->confirmloadlabel->setText(
                    tr("<center><font color=\"#008400\"><h3><b>Images Loaded.</b>" \
                       "</h3></font></center>"));
    } else {
        m_ui->NextFinishButton->setEnabled(false);
        m_ui->confirmloadlabel->setText(
                    tr("<center><h3><b>To proceed you need to manually " \
                       "set the exposure values.<br><font color=\"#FF0000\">%1</font> " \
                       "values still required.</b></h3></center>")
                    .arg(numFilesWithoutExif));
    }
}

void HdrWizard::errorWhileLoading(const QString& error)
{
    // disconnect(m_ui->tableWidget, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(inputHdrFileSelected(int)));

    m_ui->tableWidget->clear();
    m_ui->tableWidget->setRowCount(0);
    m_ui->tableWidget->setEnabled(true);
    m_ui->progressBar->setValue(0);
    m_ui->progressBar->hide();
    m_ui->previewLabel->clear();
    m_ui->loadImagesButton->setEnabled(true);
    m_ui->removeImageButton->setEnabled(false);
    m_ui->clearListButton->setEnabled(false);
    m_ui->NextFinishButton->setEnabled(false);
    m_ui->EVgroupBox->setEnabled(false);
    QMessageBox::critical(this,tr("Loading Error: "), error);
    // DAVIDE _ HDR CREATION
    m_hdrCreationManager->clearFiles();
    QApplication::restoreOverrideCursor();

    m_ui->confirmloadlabel->setText("<center><h3><b>"+
                                    tr("Start loading a set of images with different exposure") +
                                    "</b></h3></center>");

    // connect(m_ui->tableWidget, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(inputHdrFileSelected(int)));
}

void HdrWizard::finishedAligning(int exitcode)
{
    QApplication::restoreOverrideCursor();
    if (exitcode != 0)
    {
        QMessageBox::warning(this, tr("Error..."),
                             tr("align_image_stack failed to align images."));
    }
    m_ui->NextFinishButton->setEnabled(true);
    m_ui->pagestack->setCurrentIndex(1);
    m_ui->progressBar->hide();
    m_hdrCreationManager->removeTempFiles();
}

void HdrWizard::ais_failed(QProcess::ProcessError e)
{
    switch (e) {
    case QProcess::FailedToStart:
        QMessageBox::warning(this, tr("Error..."), tr("Failed to start external application \"<em>align_image_stack</em>\".<br>Please read \"Help -> Contents... -> Setting up -> External Tools\" for more information."));
    break;
    case QProcess::Crashed:
        QMessageBox::warning(this, tr("Error..."), tr("The external application \"<em>align_image_stack</em>\" crashed..."));
    break;
    case QProcess::Timedout:
    case QProcess::ReadError:
    case QProcess::WriteError:
    case QProcess::UnknownError:
        QMessageBox::warning(this, tr("Error..."), tr("An unknown error occurred while executing the \"<em>align_image_stack</em>\" application..."));
    break;
    }
    m_ui->progressBar->hide();
    m_ui->textEdit->hide();
    QApplication::restoreOverrideCursor();
    m_ui->alignGroupBox->setEnabled(true);
    m_ui->alignCheckBox->setChecked(false);
    m_ui->NextFinishButton->setEnabled(true);
    m_ui->confirmloadlabel->setText("<center><h3><b>" +
                                    tr("Now click on next button") +
                                    "</b></h3></center>");
}

void HdrWizard::customConfigCheckBoxToggled(bool want_custom)
{
    if (!want_custom)
    {
        /*
        if (!m_ui->antighostingCheckBox->isChecked())
        {
            m_ui->label_RespCurve_Antighost->setDisabled(true);
            m_ui->antighostRespCurveCombobox->setDisabled(true);
            m_ui->label_Iterations->setDisabled(true);
            m_ui->spinBoxIterations->setDisabled(true);
            //temporary disable anti-ghosting until it's fixed
            m_ui->antighostingCheckBox->setDisabled(true);

        }
        else
        {
            m_ui->label_predef_configs->setDisabled(true);
            m_ui->predefConfigsComboBox->setDisabled(true);
            m_ui->label_weights->setDisabled(true);
            m_ui->lineEdit_showWeight->setDisabled(true);
            m_ui->label_resp->setDisabled(true);
            m_ui->lineEdit_show_resp->setDisabled(true);
            m_ui->label_model->setDisabled(true);
            m_ui->lineEdit_showmodel->setDisabled(true);
        }
        */
        m_ui->label_Iterations->setDisabled(true);
        m_ui->spinBoxIterations->setDisabled(true);
        m_ui->label_predef_configs->setDisabled(true);
        m_ui->predefConfigsComboBox->setDisabled(true);
        m_ui->label_weights->setDisabled(true);
        m_ui->lineEdit_showWeight->setDisabled(true);
        m_ui->label_resp->setDisabled(true);
        m_ui->lineEdit_show_resp->setDisabled(true);
        m_ui->label_model->setDisabled(true);
        m_ui->lineEdit_showmodel->setDisabled(true);
        predefConfigsComboBoxActivated(m_ui->predefConfigsComboBox->currentIndex());
        m_ui->NextFinishButton->setText(tr("&Finish"));

    }
    else
    {
        m_ui->NextFinishButton->setText(tr("&Next >"));
    }
}

void HdrWizard::predefRespCurveRadioButtonToggled(bool want_predef_resp_curve)
{
    if (want_predef_resp_curve)
    {
        //ENABLE load_curve_button and lineedit when "load from file" is checked.
        if (!m_ui->loadRespCurveFromFileCheckbox->isChecked())
        {
            m_ui->loadRespCurveFileButton->setEnabled(false);
            m_ui->RespCurveFileLoadedLineEdit->setEnabled(false);
        }
        loadRespCurveFromFileCheckboxToggled(m_ui->loadRespCurveFromFileCheckbox->isChecked());
    }
    else
    {
        // want to recover response curve via robertson02
        // TODO : m_hdrCreationManager->chosen_config.resresponse_curve = FROM_ROBERTSON;
        m_ui->NextFinishButton->setEnabled(true);
        saveRespCurveToFileCheckboxToggled(m_ui->saveRespCurveToFileCheckbox->isChecked());
    }
}

void HdrWizard::loadRespCurveFromFileCheckboxToggled(bool checkedfile)
{
    //if checkbox is checked AND we have a valid filename
    if (checkedfile)
    {
        if (!m_ui->RespCurveFileLoadedLineEdit->text().isEmpty()) {
            //update chosen config
            m_hdrCreationManager->fusionOperatorConfig.responseFunction = RESPONSE_CUSTOM;
            m_hdrCreationManager->fusionOperatorConfig.inputResponseFunctionFilename =
                    m_ui->RespCurveFileLoadedLineEdit->text();

            //and ENABLE nextbutton
            m_ui->NextFinishButton->setEnabled(true);
        }
        else    // if checkbox is checked AND no valid filename
        {
            // DISABLE nextbutton until situation is fixed
            m_ui->NextFinishButton->setEnabled(false);
            qDebug("Load checkbox is checked AND no valid filename");
        }

    }
    else    // checkbox not checked
    {
        // update chosen config
        m_hdrCreationManager->fusionOperatorConfig.responseFunction = responses_in_gui[m_ui->gammaLinLogComboBox->currentIndex()];
        m_hdrCreationManager->fusionOperatorConfig.inputResponseFunctionFilename.clear();
        //and ENABLE nextbutton
        m_ui->NextFinishButton->setEnabled(true);
    }
}

void HdrWizard::saveRespCurveToFileCheckboxToggled(bool checkedfile)
{
    // if checkbox is checked AND we have a valid filename
    if (checkedfile)
    {
        if (!m_ui->CurveFileNameSaveLineEdit->text().isEmpty())
        {
            m_hdrCreationManager->fusionOperatorConfig.outputResponseFunctionFilename =
                    m_ui->CurveFileNameSaveLineEdit->text();
            m_ui->NextFinishButton->setEnabled(true);
        }

        else  // if checkbox is checked AND no valid filename
        {
            // DISABLE nextbutton until situation is fixed
            m_ui->NextFinishButton->setEnabled(false);
        }
    }
    else    // checkbox not checked
    {
        m_hdrCreationManager->fusionOperatorConfig.outputResponseFunctionFilename.clear();
        //and ENABLE nextbutton
        m_ui->NextFinishButton->setEnabled(true);
    }
}

void HdrWizard::NextFinishButtonClicked() {
    int currentpage = m_ui->pagestack->currentIndex();
    switch (currentpage) {
    case 0:
        //now align, if requested
        if (m_ui->alignCheckBox->isChecked()) {
            QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
            m_ui->confirmloadlabel->setText("<center><h3><b>"+tr("Aligning...")+"</b></h3></center>");
            m_ui->loadImagesButton->setDisabled(true);
            m_ui->removeImageButton->setDisabled(true);
            m_ui->clearListButton->setDisabled(true);
            m_ui->previewLabel->setDisabled(true);
            m_ui->NextFinishButton->setDisabled(true);
            m_ui->alignGroupBox->setDisabled(true);
            m_ui->agGroupBox->setDisabled(true);
            m_ui->EVgroupBox->setDisabled(true);
            m_ui->tableWidget->setDisabled(true);
            repaint();
            m_ui->progressBar->setMaximum(0);
            m_ui->progressBar->setMinimum(0);
            m_ui->progressBar->show();
            if (m_ui->ais_radioButton->isChecked()) {
                m_ui->progressBar->setRange(0,100);
                m_ui->progressBar->setValue(0);
                m_ui->textEdit->show();
                m_hdrCreationManager->set_ais_crop_flag(m_ui->autoCropCheckBox->isChecked());
                m_hdrCreationManager->align_with_ais();
            }
            else
                m_hdrCreationManager->align_with_mtb();
            return;
        }
        m_ui->pagestack->setCurrentIndex(1);
        break;
    case 1:
        if(!m_ui->customConfigCheckBox->isChecked()) {
            currentpage = 3;
        } else {
            m_ui->pagestack->setCurrentIndex(2);
            break;
        }
    case 2:
        m_processing = true;
        m_ui->settings_label->setText("<center><h3><b>"+tr("Processing...")+"</b></h3></center>");
        m_ui->customize_label->setText("<center><h3><b>"+tr("Processing...")+"</b></h3></center>");
        repaint();
        QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
        m_ui->NextFinishButton->setEnabled(false);
        m_ui->cancelButton->setEnabled(false);

        if (m_ui->autoAG_checkBox->isChecked() && !m_ui->checkBoxEditingTools->isChecked()) {
            int num_images = m_hdrCreationManager->getData().size();
            QList< QPair<int,int> > HV_offsets;
            for (int i = 0; i < num_images; i++) {
                HV_offsets.append(qMakePair(0,0));
            }
            float patchesPercent;
            m_agGoodImageIndex = m_hdrCreationManager->computePatches(m_ui->threshold_doubleSpinBox->value(), m_patches, patchesPercent, HV_offsets);
            m_doAutoAntighosting = true; 
        }
        if (m_doAutoAntighosting) {
            int h0;
            m_hdrCreationManager->getAgData(m_patches, h0);
            m_future = QtConcurrent::run( boost::bind(&HdrCreationManager::doAntiGhosting,
                                                       m_hdrCreationManager.data(),
                                                       m_patches, h0, false, &m_ph)); // false means auto anti-ghosting
            connect(&m_futureWatcher, SIGNAL(finished()), this, SLOT(autoAntighostingFinished()), Qt::DirectConnection);
            m_ui->progressBar->show();
            m_futureWatcher.setFuture(m_future);
        }
        else if (m_doManualAntighosting) {
            m_future = QtConcurrent::run( boost::bind(&HdrCreationManager::doAntiGhosting,
                                                       m_hdrCreationManager.data(),
                                                       m_patches, m_agGoodImageIndex, true, &m_ph)); // true means manual anti-ghosting
            connect(&m_futureWatcher, SIGNAL(finished()), this, SLOT(autoAntighostingFinished()), Qt::DirectConnection);
            m_ui->progressBar->show();
            m_futureWatcher.setFuture(m_future);
        }
        else
            createHdr();
    }
}

void HdrWizard::createHdr()
{
    m_future = QtConcurrent::run( boost::bind(&HdrCreationManager::createHdr, 
                                               m_hdrCreationManager.data(),
                                               false, 
                                               m_ui->spinBoxIterations->value()));

    connect(&m_futureWatcher, SIGNAL(finished()), this, SLOT(createHdrFinished()), Qt::DirectConnection);
    m_futureWatcher.setFuture(m_future);
}

void HdrWizard::createHdrFinished()
{
    m_pfsFrameHDR = m_future.result();
    OsIntegration::getInstance().setProgress(-1);
    QApplication::restoreOverrideCursor();
    accept();
}

void HdrWizard::autoAntighostingFinished()
{
    m_pfsFrameHDR = m_future.result();
    m_ui->progressBar->hide();
    OsIntegration::getInstance().setProgress(-1);
    QApplication::restoreOverrideCursor();
    if (m_pfsFrameHDR == NULL)
        QDialog::reject();
    else
        accept();
}

void HdrWizard::currentPageChangedInto(int newindex)
{
    //predefined configs page
    // m_ui->textEdit->hide();
    if (newindex == 1) {
        //m_hdrCreationManager->removeTempFiles();
        m_ui->NextFinishButton->setText(tr("&Finish"));
        //when at least 2 LDR or MDR inputs perform Manual Alignment
        int num_images = m_hdrCreationManager->getData().size();
/*
        if (m_hdrCreationManager->inputImageType() == HdrCreationManager::LDR_INPUT_TYPE)
            numldrs = m_hdrCreationManager->getLDRList().size();
        else
            numldrs = m_hdrCreationManager->getMDRList().size();
      
        qDebug() << "numldrs = " << numldrs;
*/
        //if (m_hdrCreationManager->inputImageType() == HdrCreationManager::LDR_INPUT_TYPE && numldrs >= 2) {
        if (m_ui->checkBoxEditingTools->isChecked() && num_images >= 2) {
            this->setDisabled(true);
            EditingTools *editingtools = new EditingTools(m_hdrCreationManager.data(), m_ui->autoAG_checkBox->isChecked());
            if (editingtools->exec() == QDialog::Accepted) {
                m_doAutoAntighosting = editingtools->isAutoAntighostingEnabled();
                m_doManualAntighosting = editingtools->isManualAntighostingEnabled();
                m_agGoodImageIndex = editingtools->getAgGoodImageIndex();
                this->setDisabled(false);
            } else {
                emit reject();
            }
            delete editingtools;
        }
    }
    else if (newindex == 2) { //custom config
        predefConfigsComboBoxActivated(1);
        m_ui->NextFinishButton->setText(tr("&Finish"));
        return;
    }
}

void HdrWizard::antighostRespCurveComboboxActivated(int fromgui) {
    gammaLinLogComboBoxActivated(fromgui);
}

void HdrWizard::loadRespCurveFileButtonClicked()
{
    QString loadcurvefilename = QFileDialog::getOpenFileName(
            this,
            tr("Load a camera response curve file"),
            QDir::currentPath(),
            tr("Camera response curve (*.m);;All Files (*)") );
    if (!loadcurvefilename.isEmpty())
    {
        m_ui->RespCurveFileLoadedLineEdit->setText(loadcurvefilename);
        loadRespCurveFromFileCheckboxToggled(m_ui->loadRespCurveFromFileCheckbox->isChecked());
    }
}

void HdrWizard::saveRespCurveFileButtonClicked()
{
    QString savecurvefilename = QFileDialog::getSaveFileName(
            this,
            tr("Save a camera response curve file"),
            QDir::currentPath(),
            tr("Camera response curve (*.m)") );
    if (!savecurvefilename.isEmpty())
    {
        m_ui->CurveFileNameSaveLineEdit->setText(savecurvefilename);
        saveRespCurveToFileCheckboxToggled(m_ui->saveRespCurveToFileCheckbox->isChecked());
    }
}

namespace
{

void updateHdrCreationManagerModel(HdrCreationManager& manager, FusionOperator fusionOperator)
{
    qDebug() << "Change model to " << (int)fusionOperator;

    manager.setFusionOperator(fusionOperator);
}

void updateHdrCreationManagerResponse(HdrCreationManager& manager, ResponseFunction response)
{
    qDebug() << "Change response to " << (int)response;

    manager.setResponseFunction(response);
}

void updateHdrCreationManagerWeight(HdrCreationManager& manager, WeightFunction weight)
{
    qDebug() << "Change weights to " << (int)weight;

    manager.setWeightFunction(weight);
}

}

void HdrWizard::predefConfigsComboBoxActivated(int index_from_gui)
{
    if (index_from_gui <= 5)
    {
        m_hdrCreationManager->fusionOperatorConfig = predef_confs[index_from_gui];
    }
    else
    {
        m_hdrCreationManager->fusionOperatorConfig = m_customConfig[index_from_gui - 6];
    }
    m_ui->lineEdit_showWeight->setText(getQStringFromConfig(1));
    m_ui->lineEdit_show_resp->setText(getQStringFromConfig(2));
    m_ui->lineEdit_showmodel->setText(getQStringFromConfig(3));


    // update HdrCreationManager (new code)
    updateHdrCreationManagerModel(*m_hdrCreationManager,
                                  m_hdrCreationManager->fusionOperatorConfig.fusionOperator);
    updateHdrCreationManagerResponse(*m_hdrCreationManager,
                                     m_hdrCreationManager->fusionOperatorConfig.responseFunction);
    updateHdrCreationManagerWeight(*m_hdrCreationManager,
                                   m_hdrCreationManager->fusionOperatorConfig.weightFunction);
}

void HdrWizard::triGaussPlateauComboBoxActivated(int from_gui)
{
    m_hdrCreationManager->fusionOperatorConfig.weightFunction = weights_in_gui[from_gui];

    updateHdrCreationManagerWeight(*m_hdrCreationManager,
                                   m_hdrCreationManager->fusionOperatorConfig.weightFunction);
}

void HdrWizard::gammaLinLogComboBoxActivated(int from_gui)
{
    m_hdrCreationManager->fusionOperatorConfig.responseFunction = responses_in_gui[from_gui];

    updateHdrCreationManagerResponse(*m_hdrCreationManager,
                                     m_hdrCreationManager->fusionOperatorConfig.responseFunction);
}

void HdrWizard::modelComboBoxActivated(int from_gui)
{
    m_hdrCreationManager->fusionOperatorConfig.fusionOperator = models_in_gui[from_gui];

    updateHdrCreationManagerModel(*m_hdrCreationManager,
                                  m_hdrCreationManager->fusionOperatorConfig.fusionOperator);
}

void HdrWizard::loadRespCurveFilename(const QString& filename_from_gui)
{
    if (!filename_from_gui.isEmpty())
    {
        m_hdrCreationManager->fusionOperatorConfig.responseFunction = RESPONSE_CUSTOM;
        m_hdrCreationManager->fusionOperatorConfig.inputResponseFunctionFilename =
                filename_from_gui;
    }
}

QString HdrWizard::getCaptionTEXT()
{
    return QString(tr("Weights: ") + getQStringFromConfig(1) +
                   tr(" - Response curve: ") + getQStringFromConfig(2) +
                   tr(" - Model: ") + getQStringFromConfig(3));
}

QStringList HdrWizard::getInputFilesNames()
{
    return QStringList();
    // return m_inputFilesName;
}

QString HdrWizard::getQStringFromConfig(int type)
{
    if (type == 1) {
        // return String for weights
        switch (m_hdrCreationManager->fusionOperatorConfig.weightFunction) {
        case WEIGHT_TRIANGULAR:
            return tr("Triangular");
        case WEIGHT_PLATEAU:
            return tr("Plateau");
        case WEIGHT_GAUSSIAN:
            return tr("Gaussian");
        }
    } else if (type == 2) {
        // return String for response curve
        switch (m_hdrCreationManager->fusionOperatorConfig.responseFunction) {
        case RESPONSE_LINEAR:
            return tr("Linear");
        case RESPONSE_GAMMA:
            return tr("Gamma");
        case RESPONSE_LOG10:
            return tr("Logarithmic");
        case RESPONSE_SRGB:
            return tr("sRGB");
        case RESPONSE_CUSTOM:
         //   return tr("From Calibration");
        //case FROM_FILE:
            return tr("From File: ") + m_hdrCreationManager->fusionOperatorConfig.inputResponseFunctionFilename;
        }
    } else if (type == 3) {
        // return String for model
        switch (m_hdrCreationManager->fusionOperatorConfig.fusionOperator) {
        case DEBEVEC:
            return tr("Debevec");
        case ROBERTSON:
            return tr("Robertson");
        case ROBERTSON_AUTO:
            return tr("Robertson Response Calculation");
        }
    } else {
        return QString();
    }
    return QString();
}

// triggered by user interaction
void HdrWizard::editingEVfinished()
{
    // transform from EV value to expotime value
    if (m_hdrCreationManager->getFilesWithoutExif().empty())
    {
        m_ui->NextFinishButton->setEnabled(true);
        //give an offset to the EV values if they are outside of the -10..10 range.
        //m_hdrCreationManager->checkEVvalues();
        m_ui->confirmloadlabel->setText(tr("<center><font color=\"#008400\"><h3><b>All the EV values have been set.<br>Now click on Next button.</b></h3></font></center>"));
    } else {
        m_ui->confirmloadlabel->setText( QString(tr("<center><h3><b>To proceed you need to manually set the exposure values.<br><font color=\"#FF0000\">%1</font> values still required.</b></h3></center>")).arg(m_hdrCreationManager->getFilesWithoutExif().size()) );
    }
}

void HdrWizard::resizeEvent( QResizeEvent * )
{
    qDebug() << "void HdrWizard::resizeEvent(QResizeEvent*)";

    int currentRow = m_ui->tableWidget->currentRow();
    int numberOfRow = m_ui->tableWidget->rowCount();

    if ( currentRow >= 0 && currentRow < numberOfRow )
    {
        m_ui->previewLabel->setPixmap(
                    QPixmap::fromImage(
                        m_hdrCreationManager->getFile(currentRow).qimage()->scaled(
                            m_ui->previewLabel->size(), Qt::KeepAspectRatio)
                        ));
    }
    else {
        m_ui->previewLabel->setText(QString());
    }
}

void HdrWizard::alignSelectionClicked()
{
    m_ui->autoCropCheckBox->setEnabled(m_ui->ais_radioButton->isChecked());
}

void HdrWizard::reject()
{
    QApplication::restoreOverrideCursor();
    if (m_processing)
    {
        m_ph.qtCancel();
    }
    else
    {
        m_hdrCreationManager->reset();
        QDialog::reject();
    }
}

void HdrWizard::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        m_ui->tableWidget->selectRow((m_ui->tableWidget->currentRow() == m_ui->tableWidget->rowCount()-1) ? 0 : m_ui->tableWidget->currentRow()+1);
    } else if (event->key() == Qt::Key_Escape) {
        emit reject();
    }
}

void HdrWizard::writeAisData(QByteArray data)
{
    qDebug() << data;
    if (data.contains("[1A"))
        data.replace("[1A", "");
    if (data.contains("[2A"))
        data.replace("[2A", "");
    if (data.contains(QChar(0x01B).toLatin1()))
        data.replace(QChar(0x01B).toLatin1(), "");

    m_ui->textEdit->append(data);
    if (data.contains(": remapping")) {
        QRegExp exp("\\:\\s*(\\d+)\\s*");
        exp.indexIn(QString(data.data()));
        emit setRange(0, 100);
        emit setValue(exp.cap(1).toInt());
    }
}

void HdrWizard::on_pushButtonSaveSettings_clicked()
{
    /*
    QSqlQuery query;
    QString response_filename;
    int weight = m_ui->triGaussPlateauComboBox->currentIndex();
    int response;
    if (m_ui->predefRespCurveRadioButton->isChecked()) {
        response = m_ui->gammaLinLogComboBox->currentIndex();
    }
    else if (m_ui->loadRespCurveFromFileCheckbox->isChecked()) {
        response = FROM_FILE;
        response_filename = m_ui->RespCurveFileLoadedLineEdit->text();
    }
    else
        response = FROM_ROBERTSON;

    int model = m_ui->modelComboBox->currentIndex();

    query.prepare("INSERT INTO parameters (weight, response, model, filename) "
                "VALUES (:weight, :response, :model, :filename)");
    qDebug() << "Prepare: " << query.lastError();
    query.bindValue(":weight", weight);
    query.bindValue(":response", response);
    query.bindValue(":model", model);
    query.bindValue(":filename", response_filename);
    bool res = query.exec();
    if (res == false)
        qDebug() << "Insert: " << query.lastError();
    m_ui->pushButtonSaveSettings->setEnabled(false);
    */
}

void HdrWizard::updateProgressBar(int value)
{
    if (value == 0) m_ui->progressBar->setMaximum(100);
    m_ui->progressBar->setValue(value);
}

void HdrWizard::updateThresholdSlider(int newValue)
{
    float newThreshold = ((float)newValue)/10000.f;
    bool oldState = m_ui->threshold_doubleSpinBox->blockSignals(true);
    m_ui->threshold_doubleSpinBox->setValue( newThreshold );
    m_ui->threshold_doubleSpinBox->blockSignals(oldState);
}

void HdrWizard::updateThresholdSpinBox(double newThreshold)
{
    bool oldState = m_ui->threshold_horizontalSlider->blockSignals(true);
    m_ui->threshold_horizontalSlider->setValue( (int)(newThreshold*10000) );
    m_ui->threshold_horizontalSlider->blockSignals(oldState);
}

