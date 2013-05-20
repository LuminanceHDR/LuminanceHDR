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

#include <cmath>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#include <QDebug>

#include <QStringList>
#include <QFileDialog>
#include <QDir>
#include <QUrl>
#include <QMimeData>
#include <QMessageBox>
#include <QMimeData>
#include <QProcess>
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

#include "HdrWizard.h"
#include "ui_HdrWizard.h"

#include "arch/math.h"
#include "arch/freebsd/math.h"
#include "Common/config.h"
#include "Common/global.h"
#include "HdrWizard/EditingTools.h"
#include "HdrWizard/HdrCreationManager.h"

HdrWizard::HdrWizard(QWidget *p,
                     const QStringList &files,
                     const QStringList &/*inputFilesName*/,
                     const QVector<float> &/*inputExpoTimes*/)
    : QDialog(p)
    , m_ui(new Ui::HdrWizard)
    , m_hdrCreationManager(new HdrCreationManager)
    , loadcurvefilename()
    , savecurvefilename()
//    , m_inputFilesName(inputFilesName)
//    , m_inputExpoTimes(inputExpoTimes)
{
    m_ui->setupUi(this);
    setAcceptDrops(true);
    setupConnections();

    weights_in_gui[0] = TRIANGULAR;
    weights_in_gui[1] = GAUSSIAN;
    weights_in_gui[2] = PLATEAU;
    responses_in_gui[0] = GAMMA;
    responses_in_gui[1] = LINEAR;
    responses_in_gui[2] = LOG10;
    responses_in_gui[3] = FROM_ROBERTSON;
    models_in_gui[0] = DEBEVEC;
    models_in_gui[1] = ROBERTSON;

    m_ui->tableWidget->setHorizontalHeaderLabels(
                QStringList() << tr("Image Filename") << tr("Exposure")
                );
    m_ui->tableWidget->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
    m_ui->tableWidget->verticalHeader()->hide();
    // m_ui->tableWidget->resizeColumnsToContents();
    
    if ( !luminance_options.isShowFirstPageWizard() )
    {
        m_ui->NextFinishButton->setEnabled(false);
        m_ui->pagestack->setCurrentIndex(1);
    }

    m_ui->progressBar->hide();
    m_ui->textEdit->hide();

    if (files.size())
    {
        m_ui->pagestack->setCurrentIndex(1);

        QMetaObject::invokeMethod(this, "loadInputFiles", Qt::QueuedConnection,
                                  Q_ARG(QStringList, files));
    }

    /*
    QSqlQueryModel model;
    model.setQuery("SELECT * FROM parameters"); 
    for (int i = 0; i < model.rowCount(); i++) {
        m_ui->predefConfigsComboBox->addItem(tr("Custom config %1").arg(i+1));
        int weight_ = model.record(i).value("weight").toInt(); 
        int response_ = model.record(i).value("response").toInt(); 
        int model_ = model.record(i).value("model").toInt(); 
        QString filename_ = model.record(i).value("filename").toString(); 
        config_triple ct;
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
/*
    QStringList  fnames = m_hdrCreationManager->getFileList();
    int n = fnames.size();

    for (int i = 0; i < n; i++)
    {
        QString fname = m_hdrCreationManager->getFileList().at(i);
        QFileInfo qfi(fname);
        QString thumb_name = QString(luminance_options.getTempDir() + "/"+  qfi.completeBaseName() + ".thumb.jpg");
        QFile::remove(thumb_name);
        thumb_name = QString(luminance_options.getTempDir() + "/" + qfi.completeBaseName() + ".thumb.ppm");
        QFile::remove(thumb_name);
    }
*/
}

void HdrWizard::setupConnections()
{
    connect(&m_ioFutureWatcher, SIGNAL(finished()), this, SLOT(loadInputFilesDone()));

    // connect(m_hdrCreationManager.data(), SIGNAL(progressStarted()), m_ui->progressBar, SLOT());
    connect(m_hdrCreationManager.data(), SIGNAL(progressStarted()), m_ui->progressBar, SLOT(show()));
    connect(m_hdrCreationManager.data(), SIGNAL(progressFinished()), m_ui->progressBar, SLOT(reset()));
    connect(m_hdrCreationManager.data(), SIGNAL(progressFinished()), m_ui->progressBar, SLOT(hide()));
    connect(m_hdrCreationManager.data(), SIGNAL(progressRangeChanged(int,int)), m_ui->progressBar, SLOT(setRange(int,int)));
    connect(m_hdrCreationManager.data(), SIGNAL(progressValueChanged(int)), m_ui->progressBar, SLOT(setValue(int)));

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

    connect(m_ui->predefConfigsComboBox, SIGNAL(activated(int)), this, SLOT(predefConfigsComboBoxActivated(int)));
    connect(m_ui->antighostRespCurveCombobox, SIGNAL(activated(int)), this, SLOT(antighostRespCurveComboboxActivated(int)));
    connect(m_ui->customConfigCheckBox, SIGNAL(toggled(bool)), this, SLOT(customConfigCheckBoxToggled(bool)));
    connect(m_ui->triGaussPlateauComboBox, SIGNAL(activated(int)), this, SLOT(triGaussPlateauComboBoxActivated(int)));
    connect(m_ui->predefRespCurveRadioButton, SIGNAL(toggled(bool)), this, SLOT(predefRespCurveRadioButtonToggled(bool)));
    connect(m_ui->gammaLinLogComboBox, SIGNAL(activated(int)), this, SLOT(gammaLinLogComboBoxActivated(int)));
    connect(m_ui->loadRespCurveFromFileCheckbox, SIGNAL(toggled(bool)), this, SLOT(loadRespCurveFromFileCheckboxToggled(bool)));
    connect(m_ui->loadRespCurveFileButton, SIGNAL(clicked()), this, SLOT(loadRespCurveFileButtonClicked()));
    connect(m_ui->saveRespCurveToFileCheckbox, SIGNAL(toggled(bool)), this, SLOT(saveRespCurveToFileCheckboxToggled(bool)));
    connect(m_ui->saveRespCurveFileButton, SIGNAL(clicked()), this, SLOT(saveRespCurveFileButtonClicked()));
    connect(m_ui->modelComboBox, SIGNAL(activated(int)), this, SLOT(modelComboBoxActivated(int)));
    connect(m_ui->RespCurveFileLoadedLineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(loadRespCurveFilename(const QString&)));

    connect(m_hdrCreationManager.data(), SIGNAL(fileLoaded(int,QString,float)), this, SLOT(fileLoaded(int,QString,float)));
    connect(m_hdrCreationManager.data(), SIGNAL(finishedLoadingInputFiles(QStringList)), this, SLOT(finishedLoadingInputFiles(QStringList)));
    connect(m_hdrCreationManager.data(), SIGNAL(errorWhileLoading(QString)), this, SLOT(errorWhileLoading(QString)));
    connect(m_hdrCreationManager.data(), SIGNAL(expotimeValueChanged(float,int)), this, SLOT(updateGraphicalEVvalue(float,int)));
    connect(m_hdrCreationManager.data(), SIGNAL(finishedAligning(int)), this, SLOT(finishedAligning(int)));
    connect(m_hdrCreationManager.data(), SIGNAL(ais_failed(QProcess::ProcessError)), this, SLOT(ais_failed(QProcess::ProcessError)));
    connect(m_hdrCreationManager.data(), SIGNAL(aisDataReady(QByteArray)), this, SLOT(writeAisData(QByteArray)));

    connect(this, SIGNAL(rejected()), m_hdrCreationManager.data(), SLOT(removeTempFiles()));
*/
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

    if ( currentRow >= 0 && currentRow < m_hdrCreationManager->availableInputFiles()) {
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
                        m_hdrCreationManager->getFile(currentRow).qimage().scaled(
                            m_ui->previewLabel->size(), Qt::KeepAspectRatio)
                        ));

        m_ui->ImageEVdsb->setFocus();
    }
/*
    if (m_hdrCreationManager->isValidEV(i))
        m_ui->ImageEVdsb->setValue(m_hdrCreationManager->getEV(i));
    if (m_hdrCreationManager->inputImageType() == HdrCreationManager::LDR_INPUT_TYPE) {
        QImage *image = m_hdrCreationManager->getLDRList().at(i);
        m_ui->previewLabel->setPixmap(QPixmap::fromImage(image->scaled(m_ui->previewLabel->size(), Qt::KeepAspectRatio)));
    }
    else { // load preview from thumbnail previously created on disk
        QString fname = m_hdrCreationManager->getFileList().at(i);
        QFileInfo qfi(fname);
        QString thumb_name = QString(luminance_options.getTempDir() + "/" + qfi.completeBaseName() + ".thumb.jpg");

        if (QFile::exists(thumb_name))
        {
            QImage thumb_image(thumb_name);
            m_ui->previewLabel->setPixmap(QPixmap::fromImage(thumb_image.scaled(m_ui->previewLabel->size(), Qt::KeepAspectRatio)));
        }
        else
        {
            QString thumb_name = QString(luminance_options.getTempDir() + "/" + qfi.completeBaseName() + ".thumb.ppm");
            if ( QFile::exists(thumb_name))  {
                QImage thumb_image(thumb_name);
                m_ui->previewLabel->setPixmap(QPixmap::fromImage(thumb_image.scaled(m_ui->previewLabel->size(), Qt::KeepAspectRatio)));
            }
        }
    }
    m_ui->ImageEVdsb->setFocus();
*/
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

        // m_hdrCreationManager->loadFiles(files);
        m_ioFutureWatcher.setFuture(
                    QtConcurrent::run(
                        boost::bind(&HdrCreationManager::loadFiles,
                                    m_hdrCreationManager.data(),
                                    files)
                        )
                    );


        // Query the progress dialog to check if was canceled.
        // qDebug() << "Canceled?" << progressDialog.wasCanceled();
    }

    /*

    int shift = m_ui->tableWidget->rowCount();
    m_ui->tableWidget->setEnabled(false);
    m_ui->tableWidget->setRowCount(shift + count);
    m_ui->progressBar->setMaximum(count);
    m_ui->progressBar->setValue(0);
    //connect(m_hdrCreationManager, SIGNAL(maximumValue(int)), progressBar, SLOT(setMaximum(int)));
    //connect(m_hdrCreationManager, SIGNAL(nextstep(int)), progressBar, SLOT(setValue(int)));
    m_ui->progressBar->show();
    
    // DAVIDE _ HDR CREATION
    // m_hdrCreationManager->setShift(shift);
    // m_hdrCreationManager->setFileList(files);
    // m_hdrCreationManager->loadInputFiles();
    */
}

void HdrWizard::loadInputFilesDone()
{
    m_ioFutureWatcher.waitForFinished();    // should breeze over...
    qDebug() << "HdrWizard::loadInputFilesDone()";

    m_ui->loadImagesButton->setEnabled(true);

    QApplication::restoreOverrideCursor();
    updateTableGrid();
}

/*
void HdrWizard::fileLoaded(int index, const QString& fname, float expotime)
{
    qDebug("WIZ: fileLoaded, expotimes[%d]=%f --- EV=%f",
           index, expotime, log2f(expotime));

    updateGraphicalEVvalue(expotime,index);
    // m_inputFilesName.push_back(fname);
    // m_inputExpoTimes.push_back(expotime);
    //fill graphical list
    QFileInfo qfi(fname);
    m_ui->tableWidget->setItem(index, 0, new QTableWidgetItem(qfi.fileName()));
    // increment progressbar
    m_ui->progressBar->setValue(m_ui->progressBar->value()+1);
}
*/




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

/*
    //do not load any more images
    //loadImagesButton->setEnabled(false);
    //graphical fix
    m_ui->tableWidget->resizeColumnsToContents();
    //enable user EV input
    m_ui->EVgroupBox->setEnabled(true);
    m_ui->tableWidget->selectRow(0);
    m_ui->tableWidget->setEnabled(true);

    //FIXME mtb doesn't work with 16bit data yet (and probably ever)
    if ((m_ui->tableWidget->rowCount() >= 2) && (m_hdrCreationManager->inputImageType() == HdrCreationManager::LDR_INPUT_TYPE)) {
        m_ui->alignCheckBox->setEnabled(true);
        m_ui->alignGroupBox->setEnabled(true);
    }
    else if ((m_ui->tableWidget->rowCount() >= 2) && (m_hdrCreationManager->inputImageType() == HdrCreationManager::MDR_INPUT_TYPE)) {
        m_ui->alignCheckBox->setEnabled(true);
        m_ui->alignGroupBox->setEnabled(true);
        m_ui->mtb_radioButton->setEnabled(false);
    }
    m_ui->removeImageButton->setEnabled(true);
    m_ui->clearListButton->setEnabled(true);
    m_ui->progressBar->hide();
    QApplication::restoreOverrideCursor();
}
*/

/*
void HdrWizard::errorWhileLoading(const QString& error)
{
    // disconnect(m_ui->tableWidget, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(inputHdrFileSelected(int)));

    m_ui->tableWidget->clear();
    m_ui->tableWidget->setRowCount(0);
    m_ui->tableWidget->setEnabled(true);
    m_ui->progressBar->setValue(0);
    m_ui->progressBar->hide();
    m_ui->previewLabel->clear();
    m_ui->removeImageButton->setEnabled(false);
    m_ui->clearListButton->setEnabled(false);
    m_ui->NextFinishButton->setEnabled(false);
    m_ui->EVgroupBox->setEnabled(false);
    QMessageBox::critical(this,tr("Loading Error: "), error);
    // DAVIDE _ HDR CREATION
    // m_hdrCreationManager->clearlists(true);
    QApplication::restoreOverrideCursor();

    m_ui->confirmloadlabel->setText("<center><h3><b>"+
                                    tr("Start loading a set of images with different exposure") +
                                    "</b></h3></center>");

    // connect(m_ui->tableWidget, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(inputHdrFileSelected(int)));
}
*/

/*
void HdrWizard::updateGraphicalEVvalue(float expotime, int index_in_table)
{
    qDebug("WIZ: updateGraphicalEVvalue EV[%d]=%f",
           index_in_table, log2f(expotime));

    if (expotime != -1)
    {
        QString EVdisplay;
        QTextStream ts(&EVdisplay);
        ts.setRealNumberPrecision(2);
        ts << right << forcesign << fixed << log2f(expotime) << " EV";
        QTableWidgetItem *tableitem = new QTableWidgetItem(EVdisplay);
        tableitem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_ui->tableWidget->setItem(index_in_table,1,tableitem);
    }
    else
    {
        //if image doesn't contain (the required) exif tags
        QTableWidgetItem *tableitem = new QTableWidgetItem(QString(tr("Unknown")));
        tableitem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        tableitem->setBackground(QBrush(Qt::yellow));
        tableitem->setForeground(QBrush(Qt::red));
        m_ui->tableWidget->setItem(index_in_table,1,tableitem);
    }
}
*/

void HdrWizard::finishedAligning(int exitcode)
{
    QApplication::restoreOverrideCursor();
    if (exitcode != 0)
    {
        QMessageBox::warning(this, tr("Error..."),
                             tr("align_image_stack failed to align images."));
    }
    m_ui->NextFinishButton->setEnabled(true);
    m_ui->pagestack->setCurrentIndex(2);
    m_ui->progressBar->hide();
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
    if (want_predef_resp_curve) {
        //ENABLE load_curve_button and lineedit when "load from file" is checked.
        if (!m_ui->loadRespCurveFromFileCheckbox->isChecked()) {
            m_ui->loadRespCurveFileButton->setEnabled(false);
            m_ui->RespCurveFileLoadedLineEdit->setEnabled(false);
        }
        loadRespCurveFromFileCheckboxToggled(m_ui->loadRespCurveFromFileCheckbox->isChecked());
    } else { //want to recover response curve via robertson02
        //update m_hdrCreationManager->chosen_config
        m_hdrCreationManager->chosen_config.response_curve = FROM_ROBERTSON;
        //always enable
        m_ui->NextFinishButton->setEnabled(true);
        saveRespCurveToFileCheckboxToggled(m_ui->saveRespCurveToFileCheckbox->isChecked());
    }
}

void HdrWizard::loadRespCurveFromFileCheckboxToggled( bool checkedfile ) {
    //if checkbox is checked AND we have a valid filename
    if (checkedfile && loadcurvefilename != "") {
    //update chosen config
    m_hdrCreationManager->chosen_config.response_curve = FROM_FILE;
    m_hdrCreationManager->chosen_config.LoadCurveFromFilename = strdup(QFile::encodeName(loadcurvefilename).constData());
    //and ENABLE nextbutton
    m_ui->NextFinishButton->setEnabled(true);
    }
    //if checkbox is checked AND no valid filename
    else  if (checkedfile && loadcurvefilename == "") {
    // DISABLE nextbutton until situation is fixed
    m_ui->NextFinishButton->setEnabled(false);
//  qDebug("Load checkbox is checked AND no valid filename");
    }
    //checkbox not checked
    else {
    // update chosen config
    m_hdrCreationManager->chosen_config.response_curve = responses_in_gui[m_ui->gammaLinLogComboBox->currentIndex()];
    m_hdrCreationManager->chosen_config.LoadCurveFromFilename = "";
    //and ENABLE nextbutton
    m_ui->NextFinishButton->setEnabled(true);
    }
}

void HdrWizard::saveRespCurveToFileCheckboxToggled( bool checkedfile ) {
    //if checkbox is checked AND we have a valid filename
    if (checkedfile && savecurvefilename != "") {
        m_hdrCreationManager->chosen_config.SaveCurveToFilename = strdup(QFile::encodeName(savecurvefilename).constData());
        m_ui->NextFinishButton->setEnabled(true);
    }
    //if checkbox is checked AND no valid filename
    else  if (checkedfile && savecurvefilename == "") {
        // DISABLE nextbutton until situation is fixed
        m_ui->NextFinishButton->setEnabled(false);
    }
    //checkbox not checked
    else {
        m_hdrCreationManager->chosen_config.SaveCurveToFilename = "";
        //and ENABLE nextbutton
        m_ui->NextFinishButton->setEnabled(true);
    }
}

void HdrWizard::NextFinishButtonClicked() {
    int currentpage = m_ui->pagestack->currentIndex();
    switch (currentpage) {
    case 0:
        m_ui->pagestack->setCurrentIndex(1);
        m_ui->NextFinishButton->setDisabled(true);
        break;
    case 1:
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
            m_ui->EVgroupBox->setDisabled(true);
            m_ui->tableWidget->setDisabled(true);
            repaint();
            m_ui->progressBar->setMaximum(0);
            m_ui->progressBar->setMinimum(0);
            m_ui->progressBar->show();
            if (m_ui->ais_radioButton->isChecked()) {
                m_ui->textEdit->show();
                m_hdrCreationManager->set_ais_crop_flag(m_ui->autoCropCheckBox->isChecked());
                m_hdrCreationManager->align_with_ais();
            }
            else
                m_hdrCreationManager->align_with_mtb();
            return;
        }
        m_ui->pagestack->setCurrentIndex(2);
        break;
    case 2:
        if(!m_ui->customConfigCheckBox->isChecked()) {
            currentpage = 3;
        } else {
            m_ui->pagestack->setCurrentIndex(3);
            break;
        }
    case 3:
        m_ui->settings_label->setText("<center><h3><b>"+tr("Processing...")+"</b></h3></center>");
        m_ui->customize_label->setText("<center><h3><b>"+tr("Processing...")+"</b></h3></center>");
        repaint();
        QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

        if (m_ui->antighostingCheckBox->isChecked()) {
            m_hdrCreationManager->doAutoAntiGhosting(m_ui->doubleSpinBoxThreshold->value());
        }
        m_pfsFrameHDR = m_hdrCreationManager->createHdr(false, m_ui->spinBoxIterations->value());

        QApplication::restoreOverrideCursor();
        accept();
        return;
    }
}

void HdrWizard::currentPageChangedInto(int newindex)
{
    //predefined configs page
    // m_ui->textEdit->hide();
    if (newindex == 2) {
        m_hdrCreationManager->removeTempFiles();
        m_ui->NextFinishButton->setText(tr("&Finish"));
        //when at least 2 LDR or MDR inputs perform Manual Alignment
        int numldrs;
        if (m_hdrCreationManager->inputImageType() == HdrCreationManager::LDR_INPUT_TYPE)
            numldrs = m_hdrCreationManager->getLDRList().size();
        else
            numldrs = m_hdrCreationManager->getMDRList().size();
        
        qDebug() << "numldrs = " << numldrs;
        //if (m_hdrCreationManager->inputImageType() == HdrCreationManager::LDR_INPUT_TYPE && numldrs >= 2) {
        if (numldrs >= 2) {
            this->setDisabled(true);
            //fix for some platforms/Qt versions: makes sure LDR images have alpha channel
            if (m_hdrCreationManager->inputImageType() == HdrCreationManager::LDR_INPUT_TYPE)
                m_hdrCreationManager->makeSureLDRsHaveAlpha();
            EditingTools *editingtools = new EditingTools(m_hdrCreationManager.data());
            if (editingtools->exec() == QDialog::Accepted) {
                this->setDisabled(false);
            } else {
                emit reject();
            }
            delete editingtools;
        }
    }
    else if (newindex == 3) { //custom config
        predefConfigsComboBoxActivated(1);
        m_ui->NextFinishButton->setText(tr("&Finish"));
        return;
    }
}

void HdrWizard::antighostRespCurveComboboxActivated(int fromgui) {
    gammaLinLogComboBoxActivated(fromgui);
}

void HdrWizard::loadRespCurveFileButtonClicked() {
    loadcurvefilename = QFileDialog::getOpenFileName(
            this,
            tr("Load a camera response curve file"),
            QDir::currentPath(),
            tr("Camera response curve (*.m);;All Files (*)") );
    if (!loadcurvefilename.isEmpty())  {
        m_ui->RespCurveFileLoadedLineEdit->setText(loadcurvefilename);
        loadRespCurveFromFileCheckboxToggled(m_ui->loadRespCurveFromFileCheckbox->isChecked());
    }
}

void HdrWizard::saveRespCurveFileButtonClicked() {
    savecurvefilename = QFileDialog::getSaveFileName(
            this,
            tr("Save a camera response curve file"),
            QDir::currentPath(),
            tr("Camera response curve (*.m);;All Files (*)") );
    if (!savecurvefilename.isEmpty())  {
        m_ui->CurveFileNameSaveLineEdit->setText(savecurvefilename);
        saveRespCurveToFileCheckboxToggled(m_ui->saveRespCurveToFileCheckbox->isChecked());
    }
}

void HdrWizard::predefConfigsComboBoxActivated( int index_from_gui ) {
    if (index_from_gui <= 5) {
        m_hdrCreationManager->chosen_config = predef_confs[index_from_gui];
    }
    else {
        m_hdrCreationManager->chosen_config = m_customConfig[index_from_gui - 6];
    }
    m_ui->lineEdit_showWeight->setText(getQStringFromConfig(1));
    m_ui->lineEdit_show_resp->setText(getQStringFromConfig(2));
    m_ui->lineEdit_showmodel->setText(getQStringFromConfig(3));
}

void HdrWizard::triGaussPlateauComboBoxActivated(int from_gui) {
    m_hdrCreationManager->chosen_config.weights = weights_in_gui[from_gui];
}

void HdrWizard::gammaLinLogComboBoxActivated(int from_gui) {
    m_hdrCreationManager->chosen_config.response_curve = responses_in_gui[from_gui];
}

void HdrWizard::modelComboBoxActivated(int from_gui) {
    m_hdrCreationManager->chosen_config.model = models_in_gui[from_gui];
}

void HdrWizard::loadRespCurveFilename( const QString & filename_from_gui) {
    if (!filename_from_gui.isEmpty()) {
        m_hdrCreationManager->chosen_config.response_curve = FROM_FILE;
        m_hdrCreationManager->chosen_config.LoadCurveFromFilename = strdup(QFile::encodeName(filename_from_gui).constData());
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

QString HdrWizard::getQStringFromConfig( int type )
{
    if (type == 1) {
        // return String for weights
        switch (m_hdrCreationManager->chosen_config.weights) {
        case TRIANGULAR:
            return tr("Triangular");
        case PLATEAU:
            return tr("Plateau");
        case GAUSSIAN:
            return tr("Gaussian");
        }
    } else if (type == 2) {
        // return String for response curve
        switch (m_hdrCreationManager->chosen_config.response_curve) {
        case LINEAR:
            return tr("Linear");
        case GAMMA:
            return tr("Gamma");
        case LOG10:
            return tr("Logarithmic");
        case FROM_ROBERTSON:
            return tr("From Calibration");
        case FROM_FILE:
            return tr("From File: ") + m_hdrCreationManager->chosen_config.LoadCurveFromFilename;
        }
    } else if (type == 3) {
        // return String for model
        switch (m_hdrCreationManager->chosen_config.model) {
        case DEBEVEC:
            return tr("Debevec");
        case ROBERTSON:
            return tr("Robertson");
        }
    } else {
        return "";
    }
    return "";
}

// triggered by user interaction
void HdrWizard::editingEVfinished()
{
    //transform from EV value to expotime value
    m_hdrCreationManager->setEV(m_ui->ImageEVdsb->value(), m_ui->tableWidget->currentRow());
    if (m_hdrCreationManager->getFilesLackingExif().size() == 0) {
        m_ui->NextFinishButton->setEnabled(true);
        //give an offset to the EV values if they are outside of the -10..10 range.
        m_hdrCreationManager->checkEVvalues();
        m_ui->confirmloadlabel->setText(tr("<center><font color=\"#008400\"><h3><b>All the EV values have been set.<br>Now click on Next button.</b></h3></font></center>"));
    } else {
        m_ui->confirmloadlabel->setText( QString(tr("<center><h3><b>To proceed you need to manually set the exposure values.<br><font color=\"#FF0000\">%1</font> values still required.</b></h3></center>")).arg(m_hdrCreationManager->getFilesLackingExif().size()) );
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
                        m_hdrCreationManager->getFile(currentRow).qimage().scaled(
                            m_ui->previewLabel->size(), Qt::KeepAspectRatio)
                        ));
    }
    else {
        m_ui->previewLabel->setText(QString());
    }

/*
    // make sure we ask for a thumbnail only when we need it
    if ((m_ui->pagestack->currentIndex() == 0) && (m_ui->tableWidget->currentRow() != -1) && (m_hdrCreationManager->inputImageType() == HdrCreationManager::LDR_INPUT_TYPE)) {
        QImage *image = m_hdrCreationManager->getLDRList().at(m_ui->tableWidget->currentRow());
        m_ui->previewLabel->setPixmap(QPixmap::fromImage(image->scaled(m_ui->previewLabel->size(), Qt::KeepAspectRatio)));
    }
    else if ((m_ui->pagestack->currentIndex() == 0) && (m_ui->tableWidget->currentRow() != -1) && (m_hdrCreationManager->inputImageType() != HdrCreationManager::LDR_INPUT_TYPE))
    {
        // load preview from thumbnail previously created on disk
        QString fname = m_hdrCreationManager->getFileList().at(m_ui->tableWidget->currentRow());
        QFileInfo qfi(fname);
                QString thumb_name = QString(luminance_options.getTempDir() + "/" + qfi.completeBaseName() + ".thumb.jpg");

        if ( QFile::exists(thumb_name))  {
            QImage thumb_image(thumb_name);
            m_ui->previewLabel->setPixmap(QPixmap::fromImage(thumb_image.scaled(m_ui->previewLabel->size(), Qt::KeepAspectRatio)));
        }
        else
        {
            QString thumb_name = QString(luminance_options.getTempDir() + "/" + qfi.completeBaseName() + ".thumb.ppm");
            if ( QFile::exists(thumb_name))  {
                QImage thumb_image(thumb_name);
                m_ui->previewLabel->setPixmap(QPixmap::fromImage(thumb_image.scaled(m_ui->previewLabel->size(), Qt::KeepAspectRatio)));
            }
        }
    }
*/
}

void HdrWizard::alignSelectionClicked()
{
    m_ui->autoCropCheckBox->setEnabled(m_ui->ais_radioButton->isChecked());
}

void HdrWizard::reject() {
    QApplication::restoreOverrideCursor();
    QDialog::reject();
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
    if (data.contains(QChar(0x01B).toAscii()))
        data.replace(QChar(0x01B).toAscii(), "");
    m_ui->textEdit->append(data);
    if (data.contains(": remapping")) {
        data.replace(0,data.size() - 6, " ");
        emit setValue(QString(data.data()).toInt());
    }
}

void HdrWizard::on_pushButtonSaveSettings_clicked()
{
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
}

void HdrWizard::updateProgressBar(int value)
{
    if (value == 0) m_ui->progressBar->setMaximum(100);
    m_ui->progressBar->setValue(value);
}
