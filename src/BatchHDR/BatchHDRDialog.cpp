/**
* This file is a part of Luminance HDR package.
* ----------------------------------------------------------------------
* Copyright (C) 2011 Franco Comida
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
* @author Franco Comida <fcomida@users.sourceforge.net>
*
*/

#include "BatchHDR/BatchHDRDialog.h"
#include "ui_BatchHDRDialog.h"

#include <QDebug>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QRegExp>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QSqlError>
#include <QtConcurrentRun>

#include <boost/scoped_ptr.hpp>
#include <boost/bind.hpp>

#include <Libpfs/frame.h>

#include "arch/math.h"
#include "Libpfs/pfs.h"
#include "Core/IOWorker.h"
#include "OsIntegration/osintegration.h"

using namespace libhdr::fusion;

BatchHDRDialog::BatchHDRDialog(QWidget *p):
    QDialog(p),
    m_Ui(new Ui::BatchHDRDialog),
    m_numProcessed(0),
    m_processed(0),
    m_total(0),
    m_errors(false),
    m_loading_error(false),
    m_abort(false),
    m_processing(false)
{
    m_Ui->setupUi(this);

    m_Ui->closeButton->hide();
    m_Ui->progressBar->hide();
    m_Ui->progressBar_2->hide();

    m_hdrCreationManager = new HdrCreationManager;
    m_IO_Worker = new IOWorker;

    connect(m_Ui->horizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(num_bracketed_changed(int)));
    connect(m_Ui->spinBox, SIGNAL(valueChanged(int)), this, SLOT(num_bracketed_changed(int)));

    connect(m_Ui->MTBRadioButton, SIGNAL(clicked()), this, SLOT(align_selection_clicked()));
    connect(m_Ui->aisRadioButton, SIGNAL(clicked()), this, SLOT(align_selection_clicked()));
    connect(m_Ui->threshold_horizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(updateThresholdSlider(int)));
    connect(m_Ui->threshold_doubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateThresholdSpinBox(double)));

    //connect(m_hdrCreationManager, SIGNAL(finishedLoadingInputFiles(QStringList)), this, SLOT(align(QStringList)));
    connect(m_hdrCreationManager, SIGNAL(finishedLoadingFiles()), this, SLOT(align()));
    connect(m_hdrCreationManager, SIGNAL(finishedAligning(int)), this, SLOT(create_hdr(int)));
    connect(m_hdrCreationManager, SIGNAL(errorWhileLoading(QString)), this, SLOT(error_while_loading(QString)));
    connect(m_hdrCreationManager, SIGNAL(aisDataReady(QByteArray)), this, SLOT(writeAisData(QByteArray)));
    connect(m_hdrCreationManager, SIGNAL(ais_failed(QProcess::ProcessError)), this, SLOT(ais_failed(QProcess::ProcessError)));
    connect(m_hdrCreationManager, SIGNAL(processed()), this, SLOT(processed()));

    connect(m_hdrCreationManager, SIGNAL(progressStarted()), m_Ui->progressBar, SLOT(show()));
    connect(m_hdrCreationManager, SIGNAL(progressStarted()), m_Ui->progressBar_2, SLOT(show()));
    connect(m_hdrCreationManager, SIGNAL(progressFinished()), m_Ui->progressBar_2, SLOT(reset()));
    connect(m_hdrCreationManager, SIGNAL(progressFinished()), m_Ui->progressBar_2, SLOT(hide()));
    connect(m_hdrCreationManager, SIGNAL(progressRangeChanged(int,int)), m_Ui->progressBar_2, SLOT(setRange(int,int)));
    connect(m_hdrCreationManager, SIGNAL(progressValueChanged(int)), m_Ui->progressBar_2, SLOT(setValue(int)));
    connect(m_hdrCreationManager, SIGNAL(loadFilesAborted()), this, SLOT(loadFilesAborted()));
    //connect(this, SIGNAL(setRange(int,int)), m_Ui->progressBar_2, SLOT(setRange(int,int)));
    connect(this, SIGNAL(setValue(int)), m_Ui->progressBar_2, SLOT(setValue(int)));

    connect(&m_futureWatcher, SIGNAL(finished()), this, SLOT(createHdrFinished()), Qt::DirectConnection);

    m_formatHelper.initConnection(m_Ui->formatComboBox, m_Ui->formatSettingsButton, true);

    m_tempDir = m_luminance_options.getTempDir();
    m_batchHdrInputDir = m_luminance_options.getBatchHdrPathInput("");
    m_batchHdrOutputDir = m_luminance_options.getBatchHdrPathOutput("");

    m_Ui->inputLineEdit->setText(m_batchHdrInputDir);
    m_Ui->outputLineEdit->setText(m_batchHdrOutputDir);

    QSqlQueryModel model;
    model.setQuery("SELECT * FROM parameters"); 
    for (int i = 0; i < model.rowCount(); i++)
    {
        m_Ui->profileComboBox->addItem(tr("Custom config %1").arg(i+1));
        int weight_ = model.record(i).value("weight").toInt(); 
        int response_ = model.record(i).value("response").toInt(); 
        int model_ = model.record(i).value("model").toInt(); 
        QString filename_ = model.record(i).value("filename").toString(); 
        FusionOperatorConfig ct;

        ct.weightFunction = static_cast<WeightFunctionType>(weight_);
        ct.fusionOperator = static_cast<FusionOperator>(model_);

        switch (response_)
        {
        case 0:
            ct.responseCurve = RESPONSE_CUSTOM;
            ct.inputResponseCurveFilename = QFile::encodeName(filename_).constData();
            ct.outputResponseCurveFilename.clear();
            break;
        case 2:
            ct.responseCurve = RESPONSE_GAMMA;
            break;
        case 3:
            ct.responseCurve = RESPONSE_LOG10;
            break;
        case 4:
            ct.responseCurve = RESPONSE_CUSTOM;
            ct.fusionOperator = ROBERTSON_AUTO;
            break;
        case 1:
        default:
            ct.responseCurve = RESPONSE_LINEAR;
            break;
        }

        m_customConfig.push_back(ct);   
    }
    check_start_button();
}

BatchHDRDialog::~BatchHDRDialog()
{
    qDebug() << "BatchHDRDialog::~BatchHDRDialog()";
    // DAVIDE _ HDR WIZARD
    m_hdrCreationManager->reset();
    delete m_hdrCreationManager;
    delete m_IO_Worker;
}

void BatchHDRDialog::num_bracketed_changed(int value)
{
    qDebug() << "BatchHDRDialog::num_bracketed_changed " << value;
    m_Ui->autoAlignCheckBox->setEnabled(value > 1);
    m_Ui->autoCropCheckBox->setEnabled(value > 1);
    m_Ui->aisRadioButton->setEnabled(value > 1);
    m_Ui->MTBRadioButton->setEnabled(value > 1);
}

void BatchHDRDialog::on_selectInputFolder_clicked()
{
    QString inputDir = QFileDialog::getExistingDirectory(this, tr("Choose a source directory"), m_batchHdrInputDir);
    if (!inputDir.isEmpty())
    {
        m_Ui->inputLineEdit->setText(inputDir);
        if (!m_Ui->inputLineEdit->text().isEmpty())
        {
            // if the new dir, the one just chosen by the user, is different from the one stored in the settings,
            // update the settings
            if (m_batchHdrInputDir != m_Ui->inputLineEdit->text())
            {
                m_batchHdrInputDir = m_Ui->inputLineEdit->text();
                m_luminance_options.setBatchHdrPathInput(m_batchHdrInputDir);
            }

            // defaulting the same output folder as the input folder
            if (m_Ui->outputLineEdit->text().isEmpty())
                add_output_directory(m_Ui->inputLineEdit->text());
        }
        check_start_button();
    }
}

void BatchHDRDialog::on_selectOutputFolder_clicked()
{
    add_output_directory();
}

void BatchHDRDialog::add_output_directory(QString dir)
{
    QString outputDir = !dir.isEmpty() ? dir : QFileDialog::getExistingDirectory(this, tr("Choose a output directory"), m_batchHdrOutputDir);
    if (!outputDir.isEmpty())
    {
        m_Ui->outputLineEdit->setText(outputDir);
        // if the new dir, the one just chosen by the user, is different from the one stored in the settings,
        // update the settings
        if (m_batchHdrOutputDir != m_Ui->outputLineEdit->text() && dir.isEmpty())
        {
            m_batchHdrOutputDir = m_Ui->outputLineEdit->text();
            m_luminance_options.setBatchHdrPathOutput(m_batchHdrOutputDir);
        }
        check_start_button();
    }
}

void BatchHDRDialog::on_startButton_clicked()
{
    if (m_Ui->inputLineEdit->text().isEmpty() || m_Ui->outputLineEdit->text().isEmpty())
        return;

    // check for empty output-folder
    bool foundHDR = false;
    QDir chosendir(m_batchHdrOutputDir);
    chosendir.setFilter(QDir::Files);
    QStringList files = chosendir.entryList();

    bool doStart = true;
    if (!files.empty()) {
        foreach(QString file, files) {
            if (file.startsWith("hdr_"))
                foundHDR = true;
        }
        if (foundHDR)
            doStart = QMessageBox::Yes == QMessageBox::warning(0,tr("Warning"), tr("The chosen output directory contains HDR files. Those files might be overwritten. \n\nContinue?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    }

    // process input images
    QStringList filters;
    filters << "*.jpg" << "*.jpeg" << "*.tiff" << "*.tif" << "*.crw" << "*.cr2" << "*.nef" << "*.dng" << "*.mrw" << "*.orf" << "*.kdc" << "*.dcr" << "*.arw" << "*.raf" << "*.ptx" << "*.pef" << "*.x3f" << "*.raw" << "*.rw2" << "*.sr2" << "*.3fr" << "*.mef" << "*.mos" << "*.erf" << "*.nrw" << "*.srw";
    filters << "*.JPG" << "*.JPEG" << "*.TIFF" << "*.TIF" << "*.CRW" << "*.CR2" << "*.NEF" << "*.DNG" << "*.MRW" << "*.ORF" << "*.KDC" << "*.DCR" << "*.ARW" << "*.RAF" << "*.PTX" << "*.PEF" << "*.X3F" << "*.RAW" << "*.RW2" << "*.SR2" << "*.3FR" << "*.MEF" << "*.MOS" << "*.ERF" << "*.NRW" << "*.SRW";

    QDir chosenInputDir(m_batchHdrInputDir);
    chosenInputDir.setFilter(QDir::Files);
    chosenInputDir.setSorting(QDir::Name);
    chosenInputDir.setNameFilters(filters);
    m_bracketed = chosenInputDir.entryList();
    //hack to prepend to this list the path as prefix.
    m_bracketed.replaceInStrings(QRegExp("(.+)"), chosenInputDir.path()+"/\\1");
    qDebug() << m_bracketed;

    if (m_bracketed.count() < m_Ui->spinBox->value()) {
        qDebug() << "Total number of pictures must be a multiple of number of bracketed images";
        QMessageBox::warning(0,tr("Warning"), tr("Total number of pictures must be a multiple of number of bracketed images."), QMessageBox::Ok, QMessageBox::NoButton);
        return;
    }

    if (m_bracketed.count() % m_Ui->spinBox->value() != 0) {
        qDebug() << "Total number of pictures must be a multiple of number of bracketed images";
        QMessageBox::warning(0,tr("Warning"), tr("Total number of pictures must be a multiple of number of bracketed images."), QMessageBox::Ok, QMessageBox::NoButton);
        return;
    }

    if (doStart) {
        m_Ui->horizontalSlider->setEnabled(false);
        m_Ui->spinBox->setEnabled(false);
/*
        m_Ui->profileComboBox->setEnabled(false);
        m_Ui->formatComboBox->setEnabled(false);
        m_Ui->selectInputFolder->setEnabled(false);
        m_Ui->inputLineEdit->setEnabled(false);
        m_Ui->selectOutputFolder->setEnabled(false);
        m_Ui->outputLineEdit->setEnabled(false);
*/
        m_Ui->groupBoxOutput->setEnabled(false);
        m_Ui->groupBoxAlignment->setEnabled(false);
        m_Ui->groupBoxAg->setEnabled(false);
        m_Ui->groupBoxIO->setEnabled(false);
        m_Ui->startButton->setEnabled(false);
        m_total = m_bracketed.count() / m_Ui->spinBox->value();
        m_Ui->progressBar->setMaximum(m_total);
        m_Ui->textEdit->append(tr("Started processing..."));
        // mouse pointer to busy
        QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
        batch_hdr();
    }
}

void BatchHDRDialog::batch_hdr()
{
    m_processing = true;

    if (m_abort) {
        qDebug() << "Aborted";
        QApplication::restoreOverrideCursor();
        // DAVIDE _ HDR WIZARD
        //m_hdrCreationManager->reset();
        this->reject();
    }
    if (!m_bracketed.isEmpty())
    {
        m_Ui->textEdit->append(tr("Loading files..."));
        m_numProcessed++;
        QStringList toProcess;
        for (int i = 0; i < m_Ui->spinBox->value(); ++i)
        {
            toProcess << m_bracketed.takeFirst();
        }
        qDebug() << "BatchHDRDialog::batch_hdr() Files to process: " << toProcess;
        // DAVIDE _ HDR CREATION
        QtConcurrent::run(
                boost::bind(&HdrCreationManager::loadFiles,
                            m_hdrCreationManager,
                            toProcess)
                );
    }   
    else
    {
        m_Ui->closeButton->show();
        m_Ui->cancelButton->hide();
        m_Ui->startButton->hide();
        m_Ui->progressBar->hide();
        OsIntegration::getInstance().setProgress(-1);
        QApplication::restoreOverrideCursor();
        if (m_errors)
            m_Ui->textEdit->append(tr("Completed with errors"));
        else
            m_Ui->textEdit->append(tr("Completed without errors"));
    }
}

void BatchHDRDialog::align()
{
    QStringList filesLackingExif = m_hdrCreationManager->getFilesWithoutExif();
    if (!filesLackingExif.isEmpty())
    {
        qDebug() << "BatchHDRDialog::align Error: missing EXIF data";
        m_Ui->textEdit->append(tr("Error: missing EXIF data"));
        foreach (QString fname, filesLackingExif)
            m_Ui->textEdit->append(fname);
        m_errors = true;
        // DAVIDE _ HDR WIZARD
        m_hdrCreationManager->reset();
        batch_hdr();
        return;
    }
    if (m_Ui->autoAlignCheckBox->isChecked())
    {
        m_Ui->textEdit->append(tr("Aligning..."));
        if (m_Ui->aisRadioButton->isChecked())
        {
            m_Ui->progressBar_2->show();
            m_Ui->progressBar_2->setRange(0,100);
            m_Ui->progressBar_2->setValue(0);
            m_hdrCreationManager->set_ais_crop_flag(m_Ui->autoCropCheckBox->isChecked());
            m_hdrCreationManager->align_with_ais();
        }
        else
            m_hdrCreationManager->align_with_mtb();
    }
    else
        create_hdr(0);
}

void BatchHDRDialog::create_hdr(int)
{
    qDebug() << "BatchHDRDialog::create_hdr()";

    m_Ui->progressBar_2->hide();
    m_Ui->textEdit->append(tr("Creating HDR..."));
    int idx = m_Ui->profileComboBox->currentIndex();

    const FusionOperatorConfig* cfg = NULL;
    if (idx <= 5)
    {
        cfg = &predef_confs[idx];
    }
    else
    {
        cfg = &m_customConfig[idx - 6];
    }

    m_hdrCreationManager->setFusionOperator(cfg->fusionOperator);
    m_hdrCreationManager->getWeightFunction().setType(cfg->weightFunction);
    m_hdrCreationManager->getResponseCurve().setType(cfg->responseCurve);

    if (m_Ui->autoAG_checkBox->isChecked())
    {
        m_Ui->textEdit->append(tr("Doing auto anti-ghosting..."));
        QList<QPair<int, int> > HV_offsets;
        for (int i = 0; i < m_Ui->spinBox->value(); i++ ) {
            HV_offsets.append(qMakePair(0,0));
        
        }
        float patchesPercent;
        int h0 = m_hdrCreationManager->computePatches(m_Ui->threshold_doubleSpinBox->value(), m_patches, patchesPercent, HV_offsets);
        m_future = QtConcurrent::run( boost::bind(&HdrCreationManager::doAntiGhosting,
                                                   m_hdrCreationManager,
                                                   m_patches, h0, false, &m_ph)); // false means auto anti-ghosting

        m_futureWatcher.setFuture(m_future);
        
    }
    else {
        m_future = QtConcurrent::run(
                       boost::bind(&HdrCreationManager::createHdr, m_hdrCreationManager));

        m_futureWatcher.setFuture(m_future);
    }
}

void BatchHDRDialog::createHdrFinished()
{
    boost::scoped_ptr<pfs::Frame> resultHDR(m_future.result());
    if (resultHDR.get() == NULL) {
        qDebug() << "Aborted";
        QApplication::restoreOverrideCursor();
        this->reject();
        return;
    }
    QString suffix = m_Ui->formatComboBox->currentText();
    int paddingLength = ceil(log10(m_total + 1.0f));
    QString outName = m_Ui->outputLineEdit->text() + "/hdr_" + QString("%1").arg(m_numProcessed, paddingLength, 10, QChar('0')) + "." + suffix;
    m_IO_Worker->write_hdr_frame(resultHDR.get(), outName, m_formatHelper.getParams());
    resultHDR.reset();
    
    // DAVIDE _ HDR WIZARD
    m_hdrCreationManager->reset();
    int progressValue = m_Ui->progressBar->value() + 1;
    m_Ui->progressBar->setValue(progressValue);
    OsIntegration::getInstance().setProgress(progressValue, m_Ui->progressBar->maximum() - m_Ui->progressBar->minimum());
    m_Ui->textEdit->append(tr("Written ") + outName );
    batch_hdr();
}

void BatchHDRDialog::error_while_loading(QString message)
{
    qDebug() << message;
    m_Ui->textEdit->append(tr("Error: ") + message);
    m_errors = true;
    m_loading_error = true;
    m_processed++;
    try_to_continue();
}

void BatchHDRDialog::writeAisData(QByteArray data)
{
    qDebug() << data;
    if (data.contains("[1A"))
        data.replace("[1A", "");
    if (data.contains("[2A"))
        data.replace("[2A", "");
    if (data.contains(QChar(0x01B).toLatin1()))
        data.replace(QChar(0x01B).toLatin1(), "");
    m_Ui->textEdit->append(data);
    if (data.contains(": remapping")) {
        QRegExp exp("\\:\\s*(\\d+)\\s*");
        exp.indexIn(QString(data.data()));
        emit setValue(exp.cap(1).toInt());
    }
}

void BatchHDRDialog::check_start_button()
{
    if (m_Ui->inputLineEdit->text() != "" && m_Ui->outputLineEdit->text() != "")
        m_Ui->startButton->setEnabled(true);
}

void BatchHDRDialog::on_cancelButton_clicked()
{
    if (m_processing) {
        m_abort = true;
        m_ph.qtCancel();
        m_hdrCreationManager->reset();
        m_Ui->cancelButton->setText(tr("Aborting..."));
        m_Ui->cancelButton->setEnabled(false);
    }
    else
        this->reject();
}

void BatchHDRDialog::align_selection_clicked()
{
    m_Ui->autoCropCheckBox->setEnabled(m_Ui->aisRadioButton->isChecked());
}

void BatchHDRDialog::processed()
{
    m_processed++;
    qDebug() << "BatchHDRDialog::processed() : " << m_processed;
    try_to_continue();
}

void BatchHDRDialog::try_to_continue()
{
    if (m_processed == m_Ui->spinBox->value()) {
        m_processed = 0;
        if (m_loading_error) {
            m_loading_error = false;
            // DAVIDE _ HDR WIZARD
            m_hdrCreationManager->reset();
            batch_hdr(); // try to continue
        }
    }
}
void BatchHDRDialog::ais_failed(QProcess::ProcessError error)
{
    qDebug() << "Aborted";
    QApplication::restoreOverrideCursor();
    this->reject();
}

void BatchHDRDialog::loadFilesAborted()
{
    qDebug() << "Aborted";
    QApplication::restoreOverrideCursor();
    this->reject();
}

void BatchHDRDialog::updateThresholdSlider(int newValue)
{
    float newThreshold = ((float)newValue)/10000.f;
    bool oldState = m_Ui->threshold_doubleSpinBox->blockSignals(true);
    m_Ui->threshold_doubleSpinBox->setValue( newThreshold );
    m_Ui->threshold_doubleSpinBox->blockSignals(oldState);
}

void BatchHDRDialog::updateThresholdSpinBox(double newThreshold)
{
    bool oldState = m_Ui->threshold_horizontalSlider->blockSignals(true);
    m_Ui->threshold_horizontalSlider->setValue( (int)(newThreshold*10000) );
    m_Ui->threshold_horizontalSlider->blockSignals(oldState);
}

