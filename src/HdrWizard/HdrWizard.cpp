/**
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
 * Improvements, bugfixing
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */
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
#include <QSqlRecord>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QSqlError>

#include "HdrWizard.h"
#include "ui_HdrWizard.h"

#include "arch/math.h"
#include "arch/freebsd/math.h"
#include "Common/config.h"
#include "HdrWizard/EditingTools.h"
#include "UI/Gang.h"
#include "HdrCreation/HdrCreationManager.h"

HdrWizard::HdrWizard(QWidget *p,
                     const QStringList &files,
                     const QStringList &inputFilesName,
                     const QVector<float> &inputExpoTimes) :
    QDialog(p),
    hdrCreationManager(new HdrCreationManager),
    loadcurvefilename(),
    savecurvefilename(),
    m_inputFilesName(inputFilesName),
    m_inputExpoTimes(inputExpoTimes),
    m_Ui(new Ui::HdrWizard)
{
    m_Ui->setupUi(this);
    setAcceptDrops(true);

    weights_in_gui[0] = TRIANGULAR;
    weights_in_gui[1] = GAUSSIAN;
    weights_in_gui[2] = PLATEAU;
    responses_in_gui[0] = GAMMA;
    responses_in_gui[1] = LINEAR;
    responses_in_gui[2] = LOG10;
    responses_in_gui[3] = FROM_ROBERTSON;
    models_in_gui[0] = DEBEVEC;
    models_in_gui[1] = ROBERTSON;

    m_Ui->tableWidget->setHorizontalHeaderLabels(
                QStringList() << tr("Image Filename") << tr("Exposure"));
    m_Ui->tableWidget->resizeColumnsToContents();
    
    EVgang = new Gang(m_Ui->EVSlider, m_Ui->ImageEVdsb, NULL, NULL, NULL,NULL, -10,10,0);

    if ( !luminance_options.isShowFirstPageWizard() )
    {
        m_Ui->NextFinishButton->setEnabled(false);
        m_Ui->pagestack->setCurrentIndex(1);
    }

    m_Ui->progressBar->hide();
    m_Ui->textEdit->hide();

    setupConnections();

    if (files.size())
    {
        m_Ui->pagestack->setCurrentIndex(1);

        QMetaObject::invokeMethod(this, "loadInputFiles", Qt::QueuedConnection,
                                  Q_ARG(QStringList, files), Q_ARG(int, files.size()));
    }

    QSqlQueryModel model;
    model.setQuery("SELECT * FROM parameters"); 
    for (int i = 0; i < model.rowCount(); i++) {
        m_Ui->predefConfigsComboBox->addItem(tr("Custom config %1").arg(i+1));
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
}

HdrWizard::~HdrWizard()
{
#ifdef QT_DEBUG
    qDebug() << "HdrWizard::~HdrWizard()";
#endif
    
    QStringList  fnames = hdrCreationManager->getFileList();
    int n = fnames.size();

    for (int i = 0; i < n; i++)
    {
        QString fname = hdrCreationManager->getFileList().at(i);
        QFileInfo qfi(fname);
        QString thumb_name = QString(luminance_options.getTempDir() + "/"+  qfi.completeBaseName() + ".thumb.jpg");
        QFile::remove(thumb_name);
        thumb_name = QString(luminance_options.getTempDir() + "/" + qfi.completeBaseName() + ".thumb.ppm");
        QFile::remove(thumb_name);
    }

    delete EVgang;
    delete hdrCreationManager;
}

void HdrWizard::setupConnections()
{
    connect(EVgang, SIGNAL(finished()), this, SLOT(editingEVfinished()));
    connect(m_Ui->tableWidget, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(inputHdrFileSelected(int)));

    connect(m_Ui->NextFinishButton,SIGNAL(clicked()),this,SLOT(NextFinishButtonClicked()));
    connect(m_Ui->cancelButton,SIGNAL(clicked()),this,SLOT(reject()));
    connect(m_Ui->pagestack,SIGNAL(currentChanged(int)),this,SLOT(currentPageChangedInto(int)));

    connect(m_Ui->ais_radioButton, SIGNAL(clicked()), this, SLOT(alignSelectionClicked()));
    connect(m_Ui->mtb_radioButton, SIGNAL(clicked()), this, SLOT(alignSelectionClicked()));

    connect(m_Ui->predefConfigsComboBox,SIGNAL(activated(int)),this,
    SLOT(predefConfigsComboBoxActivated(int)));
    connect(m_Ui->antighostRespCurveCombobox,SIGNAL(activated(int)),this,
    SLOT(antighostRespCurveComboboxActivated(int)));
    connect(m_Ui->customConfigCheckBox,SIGNAL(toggled(bool)),this,
    SLOT(customConfigCheckBoxToggled(bool)));
    connect(m_Ui->triGaussPlateauComboBox,SIGNAL(activated(int)),this,
    SLOT(triGaussPlateauComboBoxActivated(int)));
    connect(m_Ui->predefRespCurveRadioButton,SIGNAL(toggled(bool)),this,
    SLOT(predefRespCurveRadioButtonToggled(bool)));
    connect(m_Ui->gammaLinLogComboBox,SIGNAL(activated(int)),this,
    SLOT(gammaLinLogComboBoxActivated(int)));
    connect(m_Ui->loadRespCurveFromFileCheckbox,SIGNAL(toggled(bool)),this,
    SLOT(loadRespCurveFromFileCheckboxToggled(bool)));
    connect(m_Ui->loadRespCurveFileButton,SIGNAL(clicked()),this,
    SLOT(loadRespCurveFileButtonClicked()));
    connect(m_Ui->saveRespCurveToFileCheckbox,SIGNAL(toggled(bool)),this,
    SLOT(saveRespCurveToFileCheckboxToggled(bool)));
    connect(m_Ui->saveRespCurveFileButton,SIGNAL(clicked()),this,
    SLOT(saveRespCurveFileButtonClicked()));
    connect(m_Ui->modelComboBox,SIGNAL(activated(int)),this,
    SLOT(modelComboBoxActivated(int)));
    connect(m_Ui->RespCurveFileLoadedLineEdit,SIGNAL(textChanged(const QString&)),this,
    SLOT(loadRespCurveFilename(const QString&)));
    connect(m_Ui->loadImagesButton,SIGNAL(clicked()),this,SLOT(loadImagesButtonClicked()));
    connect(m_Ui->removeImageButton,SIGNAL(clicked()),this,SLOT(removeImageButtonClicked()));
    connect(m_Ui->clearListButton,SIGNAL(clicked()),this,SLOT(clearListButtonClicked()));
    connect(hdrCreationManager, SIGNAL(fileLoaded(int,QString,float)), this, SLOT(fileLoaded(int,QString,float)));
    connect(hdrCreationManager,SIGNAL(finishedLoadingInputFiles(QStringList)),this, SLOT(finishedLoadingInputFiles(QStringList)));
    connect(hdrCreationManager,SIGNAL(errorWhileLoading(QString)),this, SLOT(errorWhileLoading(QString)));
    connect(hdrCreationManager,SIGNAL(expotimeValueChanged(float,int)),this, SLOT(updateGraphicalEVvalue(float,int)));
    connect(hdrCreationManager, SIGNAL(finishedAligning(int)), this, SLOT(finishedAligning(int)));
    connect(hdrCreationManager, SIGNAL(ais_failed(QProcess::ProcessError)), this, SLOT(ais_failed(QProcess::ProcessError)));
    connect(hdrCreationManager, SIGNAL(aisDataReady(QByteArray)), this, SLOT(writeAisData(QByteArray)));

    connect(this,SIGNAL(rejected()),hdrCreationManager,SLOT(removeTempFiles()));

}

void HdrWizard::loadImagesButtonClicked() {
    QString filetypes;
    // when changing these filetypes, also change in DnDOption - for Drag and Drop
    filetypes += tr("All formats (*.jpeg *.jpg *.tiff *.tif *.crw *.cr2 *.nef *.dng *.mrw *.orf *.kdc *.dcr *.arw *.raf *.ptx *.pef *.x3f *.raw *.sr2 *.rw2 *.3fr *.mef *.mos *.erf *.nrw *.srw");
    filetypes += tr("*.JPEG *.JPG *.TIFF *.TIF *.CRW *.CR2 *.NEF *.DNG *.MRW *.ORF *.KDC *.DCR *.ARW *.RAF *.PTX *.PEF *.X3F *.RAW *.SR2 *.RW2 *.3FR *.MEF *.MOS *.ERF *.NRW *.SRW);;");
    filetypes += tr("JPEG (*.jpeg *.jpg *.JPEG *.JPG);;");
    filetypes += tr("TIFF Images (*.tiff *.tif *.TIFF *.TIF);;");
    filetypes += tr("RAW Images (*.crw *.cr2 *.nef *.dng *.mrw *.orf *.kdc *.dcr *.arw *.raf *.ptx *.pef *.x3f *.raw *.sr2 *.rw2 *.3fr *.mef *.mos *.erf *.nrw *.srw");
    filetypes += tr("*.CRW *.CR2 *.NEF *.DNG *.MRW *.ORF *.KDC *.DCR *.ARW *.RAF *.PTX *.PEF *.X3F *.RAW *.SR2 *.RW2 *.3FR *.MEF *.MOS *.ERF *.NRW *.SRW)");

    QString RecentDirInputLDRs = luminance_options.getDefaultPathLdrIn();

    QStringList files = QFileDialog::getOpenFileNames(this, tr("Select the input images"), RecentDirInputLDRs, filetypes );

    if (!files.isEmpty() ) {
        QFileInfo qfi(files.at(0));
        // if the new dir, the one just chosen by the user, is different from the one stored in the settings, update the luminance_options.
        if (RecentDirInputLDRs != qfi.path()) {
            // update internal field variable
            RecentDirInputLDRs = qfi.path();
            luminance_options.setDefaultPathLdrIn(RecentDirInputLDRs);
        }
        //loadImagesButton->setEnabled(false);
        m_Ui->confirmloadlabel->setText("<center><h3><b>"+tr("Loading...")+"</b></h3></center>");
        loadInputFiles(files, files.count());
        QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
    } //if (!files.isEmpty())
}

void HdrWizard::removeImageButtonClicked()
{
    disconnect(m_Ui->tableWidget, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(inputHdrFileSelected(int)));
    int index = m_Ui->tableWidget->currentRow();

    if (m_Ui->tableWidget->rowCount() == 1)
    {
        clearListButtonClicked();
    }
    else 
    {
        QString fname = hdrCreationManager->getFileList().at(index);
        QFileInfo qfi(fname);
                QString thumb_name = QString(luminance_options.getTempDir() + "/"+  qfi.completeBaseName() + ".thumb.jpg");
        QFile::remove(thumb_name);
                thumb_name = QString(luminance_options.getTempDir() + "/" + qfi.completeBaseName() + ".thumb.ppm");
        QFile::remove(thumb_name);

        hdrCreationManager->remove(index);
        m_Ui->tableWidget->removeRow(index);
        inputHdrFileSelected(m_Ui->tableWidget->currentRow());
    }
    connect(m_Ui->tableWidget, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(inputHdrFileSelected(int)));
}

void HdrWizard::clearListButtonClicked()
{
    disconnect(m_Ui->tableWidget, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(inputHdrFileSelected(int)));
    m_Ui->previewLabel->clear();
    for (int i = m_Ui->tableWidget->rowCount()-1; i >= 0; --i)
        m_Ui->tableWidget->removeRow(i);

    QStringList  fnames = hdrCreationManager->getFileList();
    int n = fnames.size();
    
    for (int i = 0; i < n; i++) {
        QString fname = hdrCreationManager->getFileList().at(i);
        QFileInfo qfi(fname);
                QString thumb_name = QString(luminance_options.getTempDir() + "/"+  qfi.completeBaseName() + ".thumb.jpg");
        QFile::remove(thumb_name);
                thumb_name = QString(luminance_options.getTempDir() + "/" + qfi.completeBaseName() + ".thumb.ppm");
        QFile::remove(thumb_name);
    }

    hdrCreationManager->reset();
    m_Ui->removeImageButton->setEnabled(false);
    m_Ui->clearListButton->setEnabled(false);
    m_Ui->EVgroupBox->setEnabled(false);
    m_Ui->alignGroupBox->setEnabled(false);
    //EVSlider->setValue(0);
    m_Ui->NextFinishButton->setEnabled(false);
    m_Ui->progressBar->setValue(0);
    m_Ui->progressBar->hide();
    m_Ui->confirmloadlabel->setText("<center><h3><b>"+tr("Start loading a set of images with different exposure")+"</b></h3></center>");
    connect(m_Ui->tableWidget, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(inputHdrFileSelected(int)));
}

void HdrWizard::dragEnterEvent(QDragEnterEvent *event) {
    if (m_Ui->loadImagesButton->isEnabled())
        event->acceptProposedAction();
}

void HdrWizard::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        QStringList files = convertUrlListToFilenameList(event->mimeData()->urls());
        if (files.size() > 0)
            loadInputFiles(files, files.size());
    }
    event->acceptProposedAction();
}

void HdrWizard::loadInputFiles(const QStringList& files, int count)
{
    int shift = m_Ui->tableWidget->rowCount();
    m_Ui->tableWidget->setEnabled(false);
    m_Ui->tableWidget->setRowCount(shift + count);
    m_Ui->progressBar->setMaximum(count);
    m_Ui->progressBar->setValue(0);
    //connect(hdrCreationManager, SIGNAL(maximumValue(int)), progressBar, SLOT(setMaximum(int)));
    //connect(hdrCreationManager, SIGNAL(nextstep(int)), progressBar, SLOT(setValue(int)));
    m_Ui->progressBar->show();
    
    hdrCreationManager->setShift(shift);
    hdrCreationManager->setFileList(files);
    hdrCreationManager->loadInputFiles();
}

void HdrWizard::fileLoaded(int index, const QString& fname, float expotime)
{
    qDebug("WIZ: fileLoaded, expotimes[%d]=%f --- EV=%f",
           index, expotime, log2f(expotime));

    updateGraphicalEVvalue(expotime,index);
    m_inputFilesName.push_back(fname);
    m_inputExpoTimes.push_back(expotime);
    //fill graphical list
    QFileInfo qfi(fname);
    m_Ui->tableWidget->setItem(index, 0, new QTableWidgetItem(qfi.fileName()));
    // increment progressbar
    m_Ui->progressBar->setValue(m_Ui->progressBar->value()+1);
}

void HdrWizard::finishedLoadingInputFiles(const QStringList& filesLackingExif)
{
    if (filesLackingExif.size() == 0)
    {
        m_Ui->NextFinishButton->setEnabled(true);
        m_Ui->confirmloadlabel->setText(tr("<center><font color=\"#008400\"><h3><b>Images Loaded.</b></h3></font></center>"));
    }
    else
    {
        QString warning_message = (QString(tr("<font color=\"#FF0000\"><h3><b>WARNING:</b></h3></font>\
        Luminance HDR was not able to find the relevant <i>EXIF</i> tags\nfor the following images:\n <ul>\
        %1</ul>\
        <hr>You can still proceed creating an Hdr. To do so you have to insert <b>manually</b> the EV (exposure values) or stop difference values.\
        <hr>If you want Luminance HDR to do this <b>automatically</b>, you have to load images that have at least\nthe following exif data: \
        <ul><li>Shutter Speed (seconds)</li>\
        <li>Aperture (f-number)</li></ul>\
        <hr><b>HINT:</b> Losing EXIF data usually happens when you preprocess your pictures.<br>\
        You can perform a <b>one-to-one copy of the exif data</b> between two sets of images via the <i><b>\"Tools->Copy Exif Data...\"</b></i> menu item."))).arg(filesLackingExif.join(""));
        QMessageBox::warning(this,tr("EXIF data not found"),warning_message);
        m_Ui->confirmloadlabel->setText(QString(tr("<center><h3><b>To proceed you need to manually set the exposure values.<br><font color=\"#FF0000\">%1</font> values still required.</b></h3></center>")).arg(filesLackingExif.size()));
    }
    //do not load any more images
    //loadImagesButton->setEnabled(false);
    //graphical fix
    m_Ui->tableWidget->resizeColumnsToContents();
    //enable user EV input
    m_Ui->EVgroupBox->setEnabled(true);
    m_Ui->tableWidget->selectRow(0);
    m_Ui->tableWidget->setEnabled(true);

    //FIXME mtb doesn't work with 16bit data yet (and probably ever)
    if ((m_Ui->tableWidget->rowCount() >= 2) && (hdrCreationManager->inputImageType() == HdrCreationManager::LDR_INPUT_TYPE)) {
        m_Ui->alignCheckBox->setEnabled(true);
        m_Ui->alignGroupBox->setEnabled(true);
    }
    else if ((m_Ui->tableWidget->rowCount() >= 2) && (hdrCreationManager->inputImageType() == HdrCreationManager::MDR_INPUT_TYPE)) {
        m_Ui->alignCheckBox->setEnabled(true);
        m_Ui->alignGroupBox->setEnabled(true);
        m_Ui->mtb_radioButton->setEnabled(false);
    }
    m_Ui->removeImageButton->setEnabled(true);
    m_Ui->clearListButton->setEnabled(true);
    m_Ui->progressBar->hide();
    QApplication::restoreOverrideCursor();
}

void HdrWizard::errorWhileLoading(const QString& error)
{
    disconnect(m_Ui->tableWidget, SIGNAL(currentCellChanged(int,int,int,int)),
               this, SLOT(inputHdrFileSelected(int)));

    m_Ui->tableWidget->clear();
    m_Ui->tableWidget->setRowCount(0);
    m_Ui->tableWidget->setEnabled(true);
    m_Ui->progressBar->setValue(0);
    m_Ui->progressBar->hide();
    m_Ui->previewLabel->clear();
    m_Ui->removeImageButton->setEnabled(false);
    m_Ui->clearListButton->setEnabled(false);
    m_Ui->NextFinishButton->setEnabled(false);
    m_Ui->EVgroupBox->setEnabled(false);
    QMessageBox::critical(this,tr("Loading Error: "), error);
    hdrCreationManager->clearlists(true);
    QApplication::restoreOverrideCursor();

    m_Ui->confirmloadlabel->setText("<center><h3><b>"+
                                    tr("Start loading a set of images with different exposure") +
                                    "</b></h3></center>");

    connect(m_Ui->tableWidget, SIGNAL(currentCellChanged(int,int,int,int)),
            this, SLOT(inputHdrFileSelected(int)));
}

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
        m_Ui->tableWidget->setItem(index_in_table,1,tableitem);
    }
    else
    {
        //if image doesn't contain (the required) exif tags
        QTableWidgetItem *tableitem = new QTableWidgetItem(QString(tr("Unknown")));
        tableitem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        tableitem->setBackground(QBrush(Qt::yellow));
        tableitem->setForeground(QBrush(Qt::red));
        m_Ui->tableWidget->setItem(index_in_table,1,tableitem);
    }
}

void HdrWizard::finishedAligning(int exitcode)
{
    QApplication::restoreOverrideCursor();
    if (exitcode != 0)
    {
        QMessageBox::warning(this, tr("Error..."),
                             tr("align_image_stack failed to align images."));
    }
    m_Ui->NextFinishButton->setEnabled(true);
    m_Ui->pagestack->setCurrentIndex(2);
    m_Ui->progressBar->hide();
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
    m_Ui->progressBar->hide();
    m_Ui->textEdit->hide();
    QApplication::restoreOverrideCursor();
    m_Ui->alignGroupBox->setEnabled(true);
    m_Ui->alignCheckBox->setChecked(false);
    m_Ui->NextFinishButton->setEnabled(true);
    m_Ui->confirmloadlabel->setText("<center><h3><b>" +
                                    tr("Now click on next button") +
                                    "</b></h3></center>");
}

void HdrWizard::customConfigCheckBoxToggled(bool want_custom)
{
    if (!want_custom)
    {
        if (!m_Ui->antighostingCheckBox->isChecked())
        {
            m_Ui->label_RespCurve_Antighost->setDisabled(true);
            m_Ui->antighostRespCurveCombobox->setDisabled(true);
            m_Ui->label_Iterations->setDisabled(true);
            m_Ui->spinBoxIterations->setDisabled(true);
            //temporary disable anti-ghosting until it's fixed
            m_Ui->antighostingCheckBox->setDisabled(true);
        }
        else
        {
            m_Ui->label_predef_configs->setDisabled(true);
            m_Ui->predefConfigsComboBox->setDisabled(true);
            m_Ui->label_weights->setDisabled(true);
            m_Ui->lineEdit_showWeight->setDisabled(true);
            m_Ui->label_resp->setDisabled(true);
            m_Ui->lineEdit_show_resp->setDisabled(true);
            m_Ui->label_model->setDisabled(true);
            m_Ui->lineEdit_showmodel->setDisabled(true);
        }
        predefConfigsComboBoxActivated(m_Ui->predefConfigsComboBox->currentIndex());
        m_Ui->NextFinishButton->setText(tr("&Finish"));
    }
    else
    {
        m_Ui->NextFinishButton->setText(tr("&Next >"));
    }
}

void HdrWizard::predefRespCurveRadioButtonToggled(bool want_predef_resp_curve)
{
    if (want_predef_resp_curve) {
        //ENABLE load_curve_button and lineedit when "load from file" is checked.
        if (!m_Ui->loadRespCurveFromFileCheckbox->isChecked()) {
            m_Ui->loadRespCurveFileButton->setEnabled(false);
            m_Ui->RespCurveFileLoadedLineEdit->setEnabled(false);
        }
        loadRespCurveFromFileCheckboxToggled(m_Ui->loadRespCurveFromFileCheckbox->isChecked());
    } else { //want to recover response curve via robertson02
        //update hdrCreationManager->chosen_config
        hdrCreationManager->chosen_config.response_curve = FROM_ROBERTSON;
        //always enable
        m_Ui->NextFinishButton->setEnabled(true);
        saveRespCurveToFileCheckboxToggled(m_Ui->saveRespCurveToFileCheckbox->isChecked());
    }
}

void HdrWizard::loadRespCurveFromFileCheckboxToggled( bool checkedfile ) {
    //if checkbox is checked AND we have a valid filename
    if (checkedfile && loadcurvefilename != "") {
    //update chosen config
    hdrCreationManager->chosen_config.response_curve = FROM_FILE;
    hdrCreationManager->chosen_config.LoadCurveFromFilename = strdup(QFile::encodeName(loadcurvefilename).constData());
    //and ENABLE nextbutton
    m_Ui->NextFinishButton->setEnabled(true);
    }
    //if checkbox is checked AND no valid filename
    else  if (checkedfile && loadcurvefilename == "") {
    // DISABLE nextbutton until situation is fixed
    m_Ui->NextFinishButton->setEnabled(false);
//  qDebug("Load checkbox is checked AND no valid filename");
    }
    //checkbox not checked
    else {
    // update chosen config
    hdrCreationManager->chosen_config.response_curve = responses_in_gui[m_Ui->gammaLinLogComboBox->currentIndex()];
    hdrCreationManager->chosen_config.LoadCurveFromFilename = "";
    //and ENABLE nextbutton
    m_Ui->NextFinishButton->setEnabled(true);
    }
}

void HdrWizard::saveRespCurveToFileCheckboxToggled( bool checkedfile ) {
    //if checkbox is checked AND we have a valid filename
    if (checkedfile && savecurvefilename != "") {
        hdrCreationManager->chosen_config.SaveCurveToFilename = strdup(QFile::encodeName(savecurvefilename).constData());
        m_Ui->NextFinishButton->setEnabled(true);
    }
    //if checkbox is checked AND no valid filename
    else  if (checkedfile && savecurvefilename == "") {
        // DISABLE nextbutton until situation is fixed
        m_Ui->NextFinishButton->setEnabled(false);
    }
    //checkbox not checked
    else {
        hdrCreationManager->chosen_config.SaveCurveToFilename = "";
        //and ENABLE nextbutton
        m_Ui->NextFinishButton->setEnabled(true);
    }
}

void HdrWizard::NextFinishButtonClicked() {
    int currentpage = m_Ui->pagestack->currentIndex();
    switch (currentpage) {
    case 0:
        m_Ui->pagestack->setCurrentIndex(1);
        m_Ui->NextFinishButton->setDisabled(true);
        break;
    case 1:
        //now align, if requested
        if (m_Ui->alignCheckBox->isChecked()) {
            QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
            m_Ui->confirmloadlabel->setText("<center><h3><b>"+tr("Aligning...")+"</b></h3></center>");
            m_Ui->loadImagesButton->setDisabled(true);
            m_Ui->removeImageButton->setDisabled(true);
            m_Ui->clearListButton->setDisabled(true);
            m_Ui->previewLabel->setDisabled(true);
            m_Ui->NextFinishButton->setDisabled(true);
            m_Ui->alignGroupBox->setDisabled(true);
            m_Ui->EVgroupBox->setDisabled(true);
            m_Ui->tableWidget->setDisabled(true);
            repaint();
            m_Ui->progressBar->setMaximum(0);
            m_Ui->progressBar->setMinimum(0);
            m_Ui->progressBar->show();
            if (m_Ui->ais_radioButton->isChecked()) {
                m_Ui->textEdit->show();
                hdrCreationManager->set_ais_crop_flag(m_Ui->autoCropCheckBox->isChecked());
                hdrCreationManager->align_with_ais();
            }
            else
                hdrCreationManager->align_with_mtb();
            return;
        }
        m_Ui->pagestack->setCurrentIndex(2);
        break;
    case 2:
        if(!m_Ui->customConfigCheckBox->isChecked()) {
            currentpage = 3;
        } else {
            m_Ui->pagestack->setCurrentIndex(3);
            break;
        }
    case 3:
        m_Ui->settings_label->setText("<center><h3><b>"+tr("Processing...")+"</b></h3></center>");
        m_Ui->customize_label->setText("<center><h3><b>"+tr("Processing...")+"</b></h3></center>");
        repaint();
        QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
        PfsFrameHDR = hdrCreationManager->createHdr(m_Ui->antighostingCheckBox->isChecked(),m_Ui->spinBoxIterations->value());
        QApplication::restoreOverrideCursor();
        accept();
        return;
    }
}

void HdrWizard::currentPageChangedInto(int newindex) {
    //predefined configs page
    // m_Ui->textEdit->hide();
    if (newindex == 2) {
        hdrCreationManager->removeTempFiles();
        m_Ui->NextFinishButton->setText(tr("&Finish"));
        //when at least 2 LDR or MDR inputs perform Manual Alignment
        int numldrs;
        if (hdrCreationManager->inputImageType() == HdrCreationManager::LDR_INPUT_TYPE)
            numldrs = hdrCreationManager->getLDRList().size();
        else
            numldrs = hdrCreationManager->getMDRList().size();
        
        qDebug() << "numldrs = " << numldrs;
        //if (hdrCreationManager->inputImageType() == HdrCreationManager::LDR_INPUT_TYPE && numldrs >= 2) {
        if (numldrs >= 2) {
            this->setDisabled(true);
            //fix for some platforms/Qt versions: makes sure LDR images have alpha channel
            if (hdrCreationManager->inputImageType() == HdrCreationManager::LDR_INPUT_TYPE)
                hdrCreationManager->makeSureLDRsHaveAlpha();
            EditingTools *editingtools = new EditingTools(hdrCreationManager);
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
        m_Ui->NextFinishButton->setText(tr("&Finish"));
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
        m_Ui->RespCurveFileLoadedLineEdit->setText(loadcurvefilename);
        loadRespCurveFromFileCheckboxToggled(m_Ui->loadRespCurveFromFileCheckbox->isChecked());
    }
}

void HdrWizard::saveRespCurveFileButtonClicked() {
    savecurvefilename = QFileDialog::getSaveFileName(
            this,
            tr("Save a camera response curve file"),
            QDir::currentPath(),
            tr("Camera response curve (*.m);;All Files (*)") );
    if (!savecurvefilename.isEmpty())  {
        m_Ui->CurveFileNameSaveLineEdit->setText(savecurvefilename);
        saveRespCurveToFileCheckboxToggled(m_Ui->saveRespCurveToFileCheckbox->isChecked());
    }
}

void HdrWizard::predefConfigsComboBoxActivated( int index_from_gui ) {
    if (index_from_gui <= 5) {
        hdrCreationManager->chosen_config = predef_confs[index_from_gui];
    }
    else {
        hdrCreationManager->chosen_config = m_customConfig[index_from_gui - 6];
    }
    m_Ui->lineEdit_showWeight->setText(getQStringFromConfig(1));
    m_Ui->lineEdit_show_resp->setText(getQStringFromConfig(2));
    m_Ui->lineEdit_showmodel->setText(getQStringFromConfig(3));
}

void HdrWizard::triGaussPlateauComboBoxActivated(int from_gui) {
    hdrCreationManager->chosen_config.weights = weights_in_gui[from_gui];
}

void HdrWizard::gammaLinLogComboBoxActivated(int from_gui) {
    hdrCreationManager->chosen_config.response_curve = responses_in_gui[from_gui];
}

void HdrWizard::modelComboBoxActivated(int from_gui) {
    hdrCreationManager->chosen_config.model = models_in_gui[from_gui];
}

void HdrWizard::loadRespCurveFilename( const QString & filename_from_gui) {
    if (!filename_from_gui.isEmpty()) {
        hdrCreationManager->chosen_config.response_curve = FROM_FILE;
        hdrCreationManager->chosen_config.LoadCurveFromFilename = strdup(QFile::encodeName(filename_from_gui).constData());
    }
}

QString HdrWizard::getCaptionTEXT()
{
    return tr("Weights: ")+getQStringFromConfig(1) + tr(" - Response curve: ") + getQStringFromConfig(2) + tr(" - Model: ") + getQStringFromConfig(3);
}

QStringList HdrWizard::getInputFilesNames()
{
    return m_inputFilesName;
}

QString HdrWizard::getQStringFromConfig( int type ) {
    if (type == 1) { //return String for weights
    switch (hdrCreationManager->chosen_config.weights) {
    case TRIANGULAR:
        return tr("Triangular");
    case PLATEAU:
        return tr("Plateau");
    case GAUSSIAN:
        return tr("Gaussian");
    }
    } else if (type == 2) {   //return String for response curve
    switch (hdrCreationManager->chosen_config.response_curve) {
    case LINEAR:
        return tr("Linear");
    case GAMMA:
        return tr("Gamma");
    case LOG10:
        return tr("Logarithmic");
    case FROM_ROBERTSON:
        return tr("From Calibration");
    case FROM_FILE:
        return tr("From File: ") + hdrCreationManager->chosen_config.LoadCurveFromFilename;
    }
    } else if (type == 3) {   //return String for model
    switch (hdrCreationManager->chosen_config.model) {
    case DEBEVEC:
        return tr("Debevec");
    case ROBERTSON:
        return tr("Robertson");
    }
    } else return "";
return "";
}

//triggered by user interaction
void HdrWizard::editingEVfinished() {
    //transform from EV value to expotime value
    hdrCreationManager->setEV(m_Ui->ImageEVdsb->value(), m_Ui->tableWidget->currentRow());
    if (hdrCreationManager->getFilesLackingExif().size() == 0) {
        m_Ui->NextFinishButton->setEnabled(true);
        //give an offset to the EV values if they are outside of the -10..10 range.
        hdrCreationManager->checkEVvalues();
        m_Ui->confirmloadlabel->setText(tr("<center><font color=\"#008400\"><h3><b>All the EV values have been set.<br>Now click on Next button.</b></h3></font></center>"));
    } else {
        m_Ui->confirmloadlabel->setText( QString(tr("<center><h3><b>To proceed you need to manually set the exposure values.<br><font color=\"#FF0000\">%1</font> values still required.</b></h3></center>")).arg(hdrCreationManager->getFilesLackingExif().size()) );
    }
}

void HdrWizard::inputHdrFileSelected(int i) {
    if (hdrCreationManager->isValidEV(i))
        m_Ui->ImageEVdsb->setValue(hdrCreationManager->getEV(i));
    if (hdrCreationManager->inputImageType() == HdrCreationManager::LDR_INPUT_TYPE) {
        QImage *image = hdrCreationManager->getLDRList().at(i);
        m_Ui->previewLabel->setPixmap(QPixmap::fromImage(image->scaled(m_Ui->previewLabel->size(), Qt::KeepAspectRatio)));
    }
    else { // load preview from thumbnail previously created on disk
        QString fname = hdrCreationManager->getFileList().at(i);
        QFileInfo qfi(fname);
        QString thumb_name = QString(luminance_options.getTempDir() + "/" + qfi.completeBaseName() + ".thumb.jpg");

        if (QFile::exists(thumb_name))
        {
            QImage thumb_image(thumb_name);
            m_Ui->previewLabel->setPixmap(QPixmap::fromImage(thumb_image.scaled(m_Ui->previewLabel->size(), Qt::KeepAspectRatio)));
        }
        else
        {
            QString thumb_name = QString(luminance_options.getTempDir() + "/" + qfi.completeBaseName() + ".thumb.ppm");
            if ( QFile::exists(thumb_name))  {
                QImage thumb_image(thumb_name);
                m_Ui->previewLabel->setPixmap(QPixmap::fromImage(thumb_image.scaled(m_Ui->previewLabel->size(), Qt::KeepAspectRatio)));
            }
        }
    }
    m_Ui->ImageEVdsb->setFocus();
}

void HdrWizard::resizeEvent ( QResizeEvent * )
{
    //qDebug() << "void HdrWizard::resizeEvent ( QResizeEvent * )";
    //make sure we ask for a thumbnail only when we need it
    if ((m_Ui->pagestack->currentIndex() == 0) && (m_Ui->tableWidget->currentRow() != -1) && (hdrCreationManager->inputImageType() == HdrCreationManager::LDR_INPUT_TYPE)) {
        QImage *image = hdrCreationManager->getLDRList().at(m_Ui->tableWidget->currentRow());
        m_Ui->previewLabel->setPixmap(QPixmap::fromImage(image->scaled(m_Ui->previewLabel->size(), Qt::KeepAspectRatio)));
    }
    else if ((m_Ui->pagestack->currentIndex() == 0) && (m_Ui->tableWidget->currentRow() != -1) && (hdrCreationManager->inputImageType() != HdrCreationManager::LDR_INPUT_TYPE))
    { // load preview from thumbnail previously created on disk
        QString fname = hdrCreationManager->getFileList().at(m_Ui->tableWidget->currentRow());
        QFileInfo qfi(fname);
                QString thumb_name = QString(luminance_options.getTempDir() + "/" + qfi.completeBaseName() + ".thumb.jpg");

        if ( QFile::exists(thumb_name))  {
            QImage thumb_image(thumb_name);
            m_Ui->previewLabel->setPixmap(QPixmap::fromImage(thumb_image.scaled(m_Ui->previewLabel->size(), Qt::KeepAspectRatio)));
        }
        else
        {
            QString thumb_name = QString(luminance_options.getTempDir() + "/" + qfi.completeBaseName() + ".thumb.ppm");
            if ( QFile::exists(thumb_name))  {
                QImage thumb_image(thumb_name);
                m_Ui->previewLabel->setPixmap(QPixmap::fromImage(thumb_image.scaled(m_Ui->previewLabel->size(), Qt::KeepAspectRatio)));
            }
        }
    }
}

void HdrWizard::alignSelectionClicked()
{
    m_Ui->autoCropCheckBox->setEnabled(m_Ui->ais_radioButton->isChecked());
}

void HdrWizard::reject() {
    QApplication::restoreOverrideCursor();
    QDialog::reject();
}

void HdrWizard::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        m_Ui->tableWidget->selectRow((m_Ui->tableWidget->currentRow() == m_Ui->tableWidget->rowCount()-1) ? 0 : m_Ui->tableWidget->currentRow()+1);
    } else if (event->key() == Qt::Key_Escape) {
        emit reject();
    }
}

void HdrWizard::writeAisData(QByteArray data)
{
    qDebug() << data;
    m_Ui->textEdit->append(data);
}

void HdrWizard::on_pushButtonSaveSettings_clicked()
{
    QSqlQuery query;
    QString response_filename;
    int weight = m_Ui->triGaussPlateauComboBox->currentIndex();
    int response;
    if (m_Ui->predefRespCurveRadioButton->isChecked() && !m_Ui->loadRespCurveFromFileCheckbox->isChecked()) {
        response = m_Ui->gammaLinLogComboBox->currentIndex();
    }
    else if (m_Ui->recoverRespCurveRadio->isChecked()) {
        response = FROM_ROBERTSON;
    }
    else if (m_Ui->loadRespCurveFromFileCheckbox->isChecked()) {
        response = FROM_FILE;
        response_filename = m_Ui->RespCurveFileLoadedLineEdit->text();
    }
    int model = m_Ui->modelComboBox->currentIndex();

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
    m_Ui->pushButtonSaveSettings->setEnabled(false);
}
