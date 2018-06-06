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

#include "HdrWizard/HdrWizard.h"
#include "HdrWizard/ui_HdrWizard.h"

#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <cmath>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QProcess>
#include <QRegExp>
#include <QStringList>
#include <QTextStream>
#include <QUrl>
// #include <QProgressDialog>
#include <QComboBox>
#include <QLabel>
#include <QThread>
#include <QtConcurrentRun>

// --- SQL handling
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QSqlRecord>
// --- end SQL handling

#include "Common/CommonFunctions.h"
#include "Common/config.h"
#include "Common/global.h"
#include "HdrWizard/EditingTools.h"
#include "HdrWizard/HdrCreationManager.h"
#include "OsIntegration/osintegration.h"
#include "arch/freebsd/math.h"
#include "arch/math.h"

using namespace libhdr::fusion;

static const ResponseCurveType responses_in_gui[] = {
    RESPONSE_LINEAR, RESPONSE_GAMMA,  RESPONSE_LOG10,
    RESPONSE_SRGB,   RESPONSE_CUSTOM,
};

static const FusionOperator models_in_gui[] = {DEBEVEC, ROBERTSON,
                                               ROBERTSON_AUTO};

static const WeightFunctionType weights_in_gui[] = {
    WEIGHT_TRIANGULAR, WEIGHT_GAUSSIAN, WEIGHT_PLATEAU, WEIGHT_FLAT};

namespace {

static QString buildEVString(float newEV) {
    QString EVdisplay;
    QTextStream ts(&EVdisplay);
    ts.setRealNumberPrecision(2);
    ts << right << forcesign << fixed << newEV << " EV";

    return EVdisplay;
}

static void updateTableItem(QTableWidgetItem *tableItem, float newEV) {
    tableItem->setBackground(QBrush(Qt::white));
    tableItem->setForeground(QBrush(Qt::black));
    tableItem->setText(buildEVString(newEV));
}
}

HdrWizard::HdrWizard(QWidget *p, const QStringList &files,
                     const QStringList & /*inputFilesName*/,
                     const QVector<float> & /*inputExpoTimes*/)
    : QDialog(p),
      m_Ui(new Ui::HdrWizard),
      m_hdrCreationManager(new HdrCreationManager),
      m_pfsFrameHDR(nullptr),
      m_doAutoAntighosting(false),
      m_doManualAntighosting(false),
      m_processing(false),
      m_isConfigChanged(false),
      m_hdrPreview(new HdrPreview(0)) {
    m_Ui->setupUi(this);

    if (!QIcon::hasThemeIcon(QStringLiteral("edit-clear-list")))
        m_Ui->clearListButton->setIcon(
            QIcon(":/program-icons/edit-clear-list"));

    setAcceptDrops(true);
    setupConnections();

    m_Ui->tableWidget->setHorizontalHeaderLabels(
        QStringList() << tr("Image Filename") << tr("Exposure"));
    m_Ui->tableWidget->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::Stretch);
    m_Ui->tableWidget->verticalHeader()->hide();
    // m_Ui->tableWidget->resizeColumnsToContents();

    m_Ui->progressBar->hide();
    m_Ui->textEdit->hide();
    m_Ui->HideLogButton->hide();

    if (files.size()) {
        m_Ui->pagestack->setCurrentIndex(0);

        QMetaObject::invokeMethod(this, "loadInputFiles", Qt::QueuedConnection,
                                  Q_ARG(QStringList, files));
    }
}

HdrWizard::~HdrWizard() {
#ifdef QT_DEBUG
    qDebug() << "HdrWizard::~HdrWizard()";
#endif
}

void HdrWizard::setupConnections() {

    connect(m_hdrCreationManager.data(),
            &HdrCreationManager::finishedLoadingFiles, this,
            &HdrWizard::loadInputFilesDone);
    connect(m_hdrCreationManager.data(),
            &HdrCreationManager::progressRangeChanged, this,
            &HdrWizard::setRange, Qt::DirectConnection);
    connect(m_hdrCreationManager.data(),
            &HdrCreationManager::progressValueChanged, this,
            &HdrWizard::setValue, Qt::DirectConnection);
    connect(this, &HdrWizard::setValue, m_Ui->progressBar,
            &QProgressBar::setValue, Qt::DirectConnection);
    connect(this, &HdrWizard::setRange, m_Ui->progressBar,
            &QProgressBar::setRange, Qt::DirectConnection);

    connect(this, &HdrWizard::setValue, OsIntegration::getInstancePtr(),
            &OsIntegration::setProgressValue, Qt::DirectConnection);
    connect(this, &HdrWizard::setRange, OsIntegration::getInstancePtr(),
            &OsIntegration::setProgressRange, Qt::DirectConnection);

    connect(m_Ui->NextFinishButton, &QAbstractButton::clicked, this,
            &HdrWizard::NextFinishButtonClicked);
    connect(m_Ui->cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
    connect(m_Ui->pagestack, &QStackedWidget::currentChanged, this,
            &HdrWizard::currentPageChangedInto);

    connect(m_Ui->loadImagesButton, &QAbstractButton::clicked, this,
            &HdrWizard::loadImagesButtonClicked);
    connect(m_Ui->removeImageButton, &QAbstractButton::clicked, this,
            &HdrWizard::removeImageButtonClicked);
    connect(m_Ui->clearListButton, &QAbstractButton::clicked, this,
            &HdrWizard::clearListButtonClicked);

    connect(m_Ui->tableWidget, &QTableWidget::currentCellChanged, this,
            &HdrWizard::inputHdrFileSelected);
    connect(m_Ui->EVSlider, &QAbstractSlider::valueChanged, this,
            &HdrWizard::updateEVSlider);
    connect(m_Ui->ImageEVdsb, SIGNAL(valueChanged(double)), this,
            SLOT(updateEVSpinBox(double)));
    connect(m_Ui->profileComboBox, SIGNAL(activated(int)), this,
            SLOT(predefConfigsComboBoxActivated(int)));
    connect(m_Ui->customConfigCheckBox, &QAbstractButton::toggled, this,
            &HdrWizard::customConfigCheckBoxToggled);
    connect(m_Ui->weightFunctionComboBox, SIGNAL(activated(int)), this,
            SLOT(weightingFunctionComboBoxActivated(int)));
    connect(m_Ui->responseCurveComboBox, SIGNAL(activated(int)), this,
            SLOT(responseCurveComboBoxActivated(int)));
    connect(m_Ui->saveRespCurveFileButton, &QAbstractButton::clicked, this,
            &HdrWizard::saveRespCurveFileButtonClicked);
    connect(m_Ui->modelComboBox, SIGNAL(activated(int)), this,
            SLOT(modelComboBoxActivated(int)));

    connect(m_hdrCreationManager.data(), &HdrCreationManager::errorWhileLoading,
            this, &HdrWizard::errorWhileLoading);
    connect(m_hdrCreationManager.data(), &HdrCreationManager::finishedAligning,
            this, &HdrWizard::finishedAligning);
    connect(m_hdrCreationManager.data(), &HdrCreationManager::ais_failed, this,
            &HdrWizard::ais_failed);
    connect(m_hdrCreationManager.data(), &HdrCreationManager::aisDataReady,
            this, &HdrWizard::writeAisData);

    connect(m_Ui->threshold_horizontalSlider, &QAbstractSlider::valueChanged,
            this, &HdrWizard::updateThresholdSlider);
    connect(m_Ui->threshold_doubleSpinBox, SIGNAL(valueChanged(double)), this,
            SLOT(updateThresholdSpinBox(double)));
    connect(m_Ui->HideLogButton, SIGNAL(clicked(bool)),
            this, SLOT(updateHideLogButtonText(bool)));
}

void HdrWizard::loadImagesButtonClicked() {
    QString filetypes;
    // when changing these filetypes, also change in DnDOption - for Drag and
    // Drop
    filetypes += tr(
        "All formats (*.jpeg *.jpg *.tiff *.tif *.crw *.cr2 *.nef *.dng *.mrw "
        "*.orf *.kdc *.dcr *.arw *.raf *.ptx *.pef *.x3f *.raw *.sr2 *.rw2 "
        "*.3fr *.mef *.mos *.erf *.nrw *.srw");
    filetypes += tr(
        "*.JPEG *.JPG *.TIFF *.TIF *.CRW *.CR2 *.NEF *.DNG *.MRW *.ORF *.KDC "
        "*.DCR *.ARW *.RAF *.PTX *.PEF *.X3F *.RAW *.SR2 *.RW2 *.3FR *.MEF "
        "*.MOS *.ERF *.NRW *.SRW);;");
    filetypes += tr("JPEG (*.jpeg *.jpg *.JPEG *.JPG);;");
    filetypes += tr("TIFF Images (*.tiff *.tif *.TIFF *.TIF);;");
    filetypes +=
        tr("RAW Images (*.crw *.cr2 *.nef *.dng *.mrw *.orf *.kdc *.dcr *.arw "
           "*.raf *.ptx *.pef *.x3f *.raw *.sr2 *.rw2 *.3fr *.mef *.mos *.erf "
           "*.nrw *.srw");
    filetypes += tr(
        "*.CRW *.CR2 *.NEF *.DNG *.MRW *.ORF *.KDC *.DCR *.ARW *.RAF *.PTX "
        "*.PEF *.X3F *.RAW *.SR2 *.RW2 *.3FR *.MEF *.MOS *.ERF *.NRW *.SRW)");

    QStringList files = QFileDialog::getOpenFileNames(
        this, tr("Select the input images"),
        luminance_options.getDefaultPathLdrIn(), filetypes);

    loadInputFiles(files);
}

void HdrWizard::updateTableGrid() {
    qDebug() << "HdrWizard::updateTableGrid(): Fill grid with values in the "
                "m_data structure";

    int currentRow = m_Ui->tableWidget->currentRow();

    // empty grid...
    m_Ui->tableWidget->clear();
    m_Ui->tableWidget->setRowCount(0);

    // insert the row at the bottom of the table widget
    int counter = 0;
    QStringList filesWithoutExif;
    BOOST_FOREACH (const HdrCreationItem &item, *m_hdrCreationManager) {
        float normalizedEV = item.getEV() - m_hdrCreationManager->getEVOffset();

        qDebug() << QStringLiteral(
                        "HdrWizard::updateTableGrid(): Fill row %1: \
                        %2 %3 EV (%4 EV)")
                        .arg(counter)
                        .arg(item.filename())
                        .arg(item.getEV())
                        .arg(normalizedEV);

        // fill graphical list
        m_Ui->tableWidget->insertRow(counter);
        m_Ui->tableWidget->setItem(
            counter, 0,
            new QTableWidgetItem(QFileInfo(item.filename()).fileName()));
        if (item.hasEV()) {
            QTableWidgetItem *tableitem =
                new QTableWidgetItem(buildEVString(normalizedEV));
            tableitem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_Ui->tableWidget->setItem(counter, 1, tableitem);
        } else {
            // if image doesn't contain (the required) exif tags
            // I keep the name of all the files without exif data...
            filesWithoutExif.push_back(item.filename());

            QTableWidgetItem *tableitem =
                new QTableWidgetItem(QString(tr("Unknown")));
            tableitem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            tableitem->setBackground(QBrush(Qt::yellow));
            tableitem->setForeground(QBrush(Qt::red));
            m_Ui->tableWidget->setItem(counter, 1, tableitem);
        }

        ++counter;
    }

    // highlight current row (possibly remain on the previously selected one!)
    if (currentRow < 0) {
        if (m_hdrCreationManager->availableInputFiles() > 0) {
            currentRow = 0;
        } else {
            currentRow = -1;
        }
    } else {
        if (currentRow > (int)m_hdrCreationManager->availableInputFiles()) {
            currentRow = 0;
        }
        // else, don't change the value!
    }
    m_Ui->tableWidget->selectRow(currentRow);

    if (counter) {
        m_Ui->clearListButton->setEnabled(true);

        enableNextOrWarning(filesWithoutExif);
    } else {
        m_Ui->clearListButton->setEnabled(false);
        m_Ui->removeImageButton->setEnabled(false);
        m_Ui->NextFinishButton->setEnabled(false);

        m_Ui->confirmloadlabel->setText(QString());
    }
}

void HdrWizard::removeImageButtonClicked() {
    qDebug() << "HdrWizard::removeImageButtonClicked()";

    int index = m_Ui->tableWidget->currentRow();

    Q_ASSERT(index >= 0);
    Q_ASSERT(index < m_hdrCreationManager->availableInputFiles());

    m_hdrCreationManager->removeFile(index);

    updateTableGrid();
}

void HdrWizard::clearListButtonClicked() {
    qDebug() << "HdrWizard::clearListButtonClicked()";

    m_hdrCreationManager->clearFiles();

    updateTableGrid();
}

void HdrWizard::updateEVSlider(int newValue) {
    int currentRow = m_Ui->tableWidget->currentRow();
    float newEV = ((float)newValue) / 100.f;
    bool oldState = m_Ui->ImageEVdsb->blockSignals(true);
    m_Ui->ImageEVdsb->setValue(newEV);
    m_Ui->ImageEVdsb->blockSignals(oldState);

    qDebug() << QStringLiteral("HdrWizard::updateEVSlider(): %1 EV (%2) for %3")
                    .arg(newEV)
                    .arg(newEV + m_hdrCreationManager->getEVOffset())
                    .arg(currentRow);

    QTableWidgetItem *tableitem = m_Ui->tableWidget->item(currentRow, 1);
    if (tableitem) {
        updateTableItem(tableitem, newEV);
    }

    m_hdrCreationManager->getFile(currentRow)
        .setEV(newEV + m_hdrCreationManager->getEVOffset());
    updateLabelMaybeNext(m_hdrCreationManager->numFilesWithoutExif());
}

void HdrWizard::updateEVSpinBox(double newEV) {
    int currentRow = m_Ui->tableWidget->currentRow();

    bool oldState = m_Ui->EVSlider->blockSignals(true);
    m_Ui->EVSlider->setValue((int)(newEV * 100));
    m_Ui->EVSlider->blockSignals(oldState);

    qDebug() << QStringLiteral(
                    "HdrWizard::updateEVSpinBox(): %1 EV (%2) for %3")
                    .arg(newEV)
                    .arg(newEV + m_hdrCreationManager->getEVOffset())
                    .arg(currentRow);

    QTableWidgetItem *tableitem = m_Ui->tableWidget->item(currentRow, 1);
    if (tableitem) {
        updateTableItem(tableitem, newEV);
    }

    m_hdrCreationManager->getFile(currentRow)
        .setEV(newEV + m_hdrCreationManager->getEVOffset());
    updateLabelMaybeNext(m_hdrCreationManager->numFilesWithoutExif());
}

void HdrWizard::inputHdrFileSelected(int currentRow) {
    qDebug() << QStringLiteral("HdrWizard::inputHdrFileSelected(%1)")
                    .arg(currentRow);

    if ((currentRow < 0) || (m_Ui->tableWidget->rowCount() < 0)) {
        // no selection...
        m_Ui->EVgroupBox->setEnabled(false);
        m_Ui->removeImageButton->setEnabled(false);
        m_Ui->clearListButton->setEnabled(false);
        m_Ui->NextFinishButton->setEnabled(false);
        m_Ui->previewLabel->clear();
    } else  // if ( m_Ui->tableWidget->rowCount() > 0 )
    {
        // enable remove button
        m_Ui->removeImageButton->setEnabled(true);
        m_Ui->clearListButton->setEnabled(true);

        // update ev slider and spinbox
        m_Ui->ImageEVdsb->blockSignals(true);
        m_Ui->EVSlider->blockSignals(true);

        m_Ui->EVgroupBox->setEnabled(true);
        if (m_hdrCreationManager->getFile(currentRow).hasEV()) {
            float normalizedEV =
                m_hdrCreationManager->getFile(currentRow).getEV() -
                m_hdrCreationManager->getEVOffset();

            m_Ui->ImageEVdsb->setValue(normalizedEV);
            m_Ui->EVSlider->setValue(
                static_cast<int>(normalizedEV * 100.f + 0.5f));
        } else {
            m_Ui->ImageEVdsb->setValue(0.0);
            m_Ui->EVSlider->setValue(0);
        }
        m_Ui->ImageEVdsb->blockSignals(false);
        m_Ui->EVSlider->blockSignals(false);

        // load QImage...
        m_Ui->previewLabel->setPixmap(QPixmap::fromImage(
            m_hdrCreationManager->getFile(currentRow)
                .qimage()
                .scaled(m_Ui->previewLabel->size(), Qt::KeepAspectRatio)));

        m_Ui->ImageEVdsb->setFocus();
    }
}

void HdrWizard::dragEnterEvent(QDragEnterEvent *event) {
    if (m_Ui->loadImagesButton->isEnabled()) {
        event->acceptProposedAction();
    }
}

void HdrWizard::dropEvent(QDropEvent *event) {
    if (event->mimeData()->hasUrls()) {
        QStringList files =
            convertUrlListToFilenameList(event->mimeData()->urls());
        loadInputFiles(files);
    }
    event->acceptProposedAction();
}

void HdrWizard::loadInputFiles(const QStringList &files) {
    if (!files.isEmpty()) {
        // update the luminance_options
        luminance_options.setDefaultPathLdrIn(QFileInfo(files.at(0)).path());

        m_Ui->loadImagesButton->setEnabled(false);
        m_Ui->confirmloadlabel->setText("<center><h3><b>" + tr("Loading...") +
                                        "</b></h3></center>");

        QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
        m_Ui->progressBar->show();

        m_inputFilesName = files;

        m_futureWatcher.setFuture(
            QtConcurrent::run(boost::bind(&HdrCreationManager::loadFiles,
                                          m_hdrCreationManager.data(), files)));
    }
}

void HdrWizard::loadInputFilesDone() {
    m_futureWatcher.waitForFinished();  // should breeze over...
    qDebug() << "HdrWizard::loadInputFilesDone()";

    m_Ui->progressBar->hide();
    m_Ui->loadImagesButton->setEnabled(true);

    updateTableGrid();

    m_Ui->alignGroupBox->setEnabled(true);
    m_Ui->agGroupBox->setEnabled(true);
    if (m_Ui->tableWidget->rowCount() > 1) {
        m_Ui->alignCheckBox->setEnabled(true);
    }

    QApplication::restoreOverrideCursor();
}

// this function should be called if we have at least a file currently in
// memory, otherwise it will give a misleading information
void HdrWizard::enableNextOrWarning(const QStringList &filesWithoutExif) {
    // If I have at least one file with empty EXIF, I raise an error...
    if (!filesWithoutExif.empty()) {
        QString warningMessage =
            tr("<font color=\"#FF0000\"><h3><b>WARNING:</b></h3></font> "
               "Luminance HDR was not able to find the relevant <b>EXIF</b> "
               "tags for the following images:"
               "<ul>");

        foreach (const QString &filename, filesWithoutExif) {
            QFileInfo qfi(filename);
            warningMessage += "<li>" + qfi.baseName() + "</li>";
        }

        warningMessage += tr(
            "</ul>"
            "<hr>Luminance HDR has inserted those values for you, two stops "
            "apart.<br> "
            "If the guess is correct you can proceed creating the HDR, "
            "otherwise you have to <b>manually</b> correct the EVs (exposure "
            "values) or stop difference values. "
            "<hr>To avoid this warning in the future you must load images that "
            "have at least the following exif data: "
            "<ul><li>Shutter Speed (seconds)</li></ul>"
            "<ul><li>Aperture (F-Number)</li></ul>"
            "<ul><li>Exposure Bias</li></ul>"
            "<hr><b>HINT:</b> Losing EXIF data usually happens when you "
            "preprocess your pictures.<br>"
            "You can perform a <b>one-to-one copy of the exif data</b> between "
            "two sets of images via the "
            "<b>Tools->Copy Exif Data...</b> menu item.");

        QApplication::restoreOverrideCursor();
        if (luminance_options.isShowMissingEVsWarning()) {
            QMessageBox msgBox;
            QCheckBox cb(tr("Do not show this message again"));
            msgBox.setText(tr("EXIF data not found"));
            msgBox.setInformativeText(warningMessage);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setCheckBox(&cb);
            msgBox.exec();
            if (msgBox.checkBox()->isChecked())
                luminance_options.setShowMissingEVsWarning(false);
        }
        setEVsValues();
        QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
    }
    updateLabelMaybeNext(filesWithoutExif.size());
}

void HdrWizard::updateLabelMaybeNext(size_t numFilesWithoutExif) {
    if (numFilesWithoutExif == 0) {
        m_Ui->NextFinishButton->setEnabled(true);
        m_Ui->confirmloadlabel->setText(
            tr("<center><font color=\"#008400\"><h3><b>Images Loaded.</b>"
               "</h3></font></center>"));
    } else {
        m_Ui->NextFinishButton->setEnabled(true);
        m_Ui->confirmloadlabel->setText(
            tr("<center><font color=\"#ffaa00\"><h3><b>Please check that "
               "all exposure values are correct before "
               "proceedings.</h3></font></center>"));
    }
}

void HdrWizard::errorWhileLoading(const QString &error) {
    m_Ui->tableWidget->clear();
    m_Ui->tableWidget->setRowCount(0);
    m_Ui->tableWidget->setEnabled(true);
    m_Ui->progressBar->setValue(0);
    m_Ui->progressBar->hide();
    m_Ui->previewLabel->clear();
    m_Ui->loadImagesButton->setEnabled(true);
    m_Ui->removeImageButton->setEnabled(false);
    m_Ui->clearListButton->setEnabled(false);
    m_Ui->NextFinishButton->setEnabled(false);
    m_Ui->EVgroupBox->setEnabled(false);
    QMessageBox::critical(this, tr("Loading Error: "), error);
    // DAVIDE _ HDR CREATION
    m_hdrCreationManager->clearFiles();

    QApplication::restoreOverrideCursor();

    m_Ui->confirmloadlabel->setText(
        "<center><h3><b>" +
        tr("Start loading a set of images with different exposure") +
        "</b></h3></center>");
}

void HdrWizard::finishedAligning(int exitcode) {
    emit setValue(-1);
    m_Ui->progressBar->hide();
    m_Ui->textEdit->hide();
    QApplication::restoreOverrideCursor();
    if (exitcode != 0) {
        ais_failed(QProcess::UnknownError);
        return;
    }
    m_Ui->NextFinishButton->setEnabled(true);
    m_Ui->pagestack->setCurrentIndex(1);
    m_Ui->progressBar->hide();
}

void HdrWizard::ais_failed(QProcess::ProcessError e) {
    emit setValue(-1);
    m_Ui->progressBar->hide();
    m_Ui->textEdit->hide();
    QApplication::restoreOverrideCursor();

    switch (e) {
        case QProcess::FailedToStart:
            QMessageBox::warning(
                this, tr("Error..."),
                tr("Failed to start external application "
                   "\"<em>align_image_stack</em>\".<br>Please read "
                   "\"Help -> Contents... -> Setting up -> External "
                   "Tools\" for more information."));
            break;
        case QProcess::Crashed:
            QMessageBox::warning(
                this, tr("Error..."),
                tr("The external application "
                   "\"<em>align_image_stack</em>\" crashed..."));
            break;
        case QProcess::Timedout:
        case QProcess::ReadError:
        case QProcess::WriteError:
        case QProcess::UnknownError:
            QMessageBox::warning(
                this, tr("Error..."),
                tr("An unknown error occurred while executing the "
                   "\"<em>align_image_stack</em>\" application..."));
            break;
    }
    m_Ui->tableWidget->setEnabled(true);
    m_Ui->previewLabel->setEnabled(true);
    m_Ui->EVgroupBox->setEnabled(true);
    m_Ui->agGroupBox->setEnabled(true);
    m_Ui->loadImagesButton->setEnabled(true);
    m_Ui->removeImageButton->setEnabled(true);
    m_Ui->clearListButton->setEnabled(true);
    m_Ui->NextFinishButton->setEnabled(true);
    m_Ui->alignGroupBox->setEnabled(true);
    m_Ui->alignCheckBox->setChecked(false);
    m_Ui->HideLogButton->hide();
    m_Ui->NextFinishButton->setEnabled(true);
    m_Ui->confirmloadlabel->setText("<center><h3><b>" +
                                    tr("Now click on next button") +
                                    "</b></h3></center>");
}

void HdrWizard::customConfigCheckBoxToggled(bool wantCustom) {
    if (wantCustom == false) {
        predefConfigsComboBoxActivated(m_Ui->profileComboBox->currentIndex());
    }
}

void HdrWizard::on_hdrPreviewButton_clicked() {
    showHDR();
}

void HdrWizard::on_hdrPreviewCheckBox_stateChanged(int state) {
    if (state == Qt::Checked) {
        m_Ui->NextFinishButton->setText(tr("Compute"));
    }
    else {
        m_Ui->NextFinishButton->setText(tr("Finish"));
    }
}

void HdrWizard::showHDR() {
    m_hdrPreview->exec();
}

void HdrWizard::NextFinishButtonClicked() {
    int currentpage = m_Ui->pagestack->currentIndex();
    switch (currentpage) {
        case 0: {
            // now align, if requested
            if (m_Ui->alignCheckBox->isChecked()) {
                QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
                m_Ui->HideLogButton->show();
                m_Ui->confirmloadlabel->setText("<center><h3><b>" +
                                                tr("Aligning...") +
                                                "</b></h3></center>");
                m_Ui->loadImagesButton->setDisabled(true);
                m_Ui->removeImageButton->setDisabled(true);
                m_Ui->clearListButton->setDisabled(true);
                m_Ui->previewLabel->setDisabled(true);
                m_Ui->NextFinishButton->setDisabled(true);
                m_Ui->alignGroupBox->setDisabled(true);
                m_Ui->agGroupBox->setDisabled(true);
                m_Ui->EVgroupBox->setDisabled(true);
                m_Ui->tableWidget->setDisabled(true);
                repaint();
                m_Ui->progressBar->setMaximum(0);
                m_Ui->progressBar->setMinimum(0);
                m_Ui->progressBar->show();
                if (m_Ui->ais_radioButton->isChecked()) {
                    m_Ui->textEdit->show();
                    m_hdrCreationManager->set_ais_crop_flag(
                        m_Ui->autoCropCheckBox->isChecked());
                    m_hdrCreationManager->align_with_ais();
                } else {
                    m_hdrCreationManager->align_with_mtb();
                }
                return;
            }
            m_Ui->pagestack->setCurrentIndex(1);
        } break;
        case 1: {
            if (m_pfsFrameHDR == nullptr || m_isConfigChanged) {
                startComputation();
            }
            else if (!m_Ui->hdrPreviewCheckBox->isChecked()) {
                accept();
            }
            else {
                showHDR();
            }
        }
    }
}

void HdrWizard::startComputation() {
    m_processing = true;
    repaint();
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
    m_Ui->NextFinishButton->setEnabled(false);
    m_Ui->cancelButton->setEnabled(false);

    if (m_Ui->autoAG_checkBox->isChecked() &&
        !m_Ui->checkBoxEditingTools->isChecked()) {
        int num_images = m_hdrCreationManager->getData().size();
        QList<QPair<int, int>> HV_offsets;
        for (int i = 0; i < num_images; i++) {
            HV_offsets.append(qMakePair(0, 0));
        }
        float patchesPercent;
        m_agGoodImageIndex = m_hdrCreationManager->computePatches(
            m_Ui->threshold_doubleSpinBox->value(), m_patches,
            patchesPercent, HV_offsets);
        m_doAutoAntighosting = true;
    }
    if (m_doAutoAntighosting) {
        int h0;
        m_hdrCreationManager->getAgData(m_patches, h0);
        m_future = QtConcurrent::run(boost::bind(
            &HdrCreationManager::doAntiGhosting,
            m_hdrCreationManager.data(), m_patches, h0, false,
            &m_ph));  // false means auto anti-ghosting
        if (m_pfsFrameHDR == nullptr) {
            connect(&m_futureWatcher, &QFutureWatcherBase::finished, this,
                    &HdrWizard::autoAntighostingFinished,
                    Qt::DirectConnection);
        }
        m_Ui->progressBar->show();
        m_futureWatcher.setFuture(m_future);
    } else if (m_doManualAntighosting) {
        m_future = QtConcurrent::run(
            boost::bind(&HdrCreationManager::doAntiGhosting,
                        m_hdrCreationManager.data(), m_patches,
                        m_agGoodImageIndex, true,
                        &m_ph));  // true means manual anti-ghosting
        if (m_pfsFrameHDR == nullptr) {
            connect(&m_futureWatcher, &QFutureWatcherBase::finished, this,
                    &HdrWizard::autoAntighostingFinished,
                    Qt::DirectConnection);
        }
        m_Ui->progressBar->show();
        m_futureWatcher.setFuture(m_future);
    } else {
        createHdr();
    }
}

void HdrWizard::createHdr() {
    m_Ui->NextFinishButton->setEnabled(false);
    m_Ui->cancelButton->setEnabled(false);

    m_future = QtConcurrent::run(boost::bind(&HdrCreationManager::createHdr,
                                             m_hdrCreationManager.data()));

    if (m_pfsFrameHDR == nullptr) {
        connect(&m_futureWatcher, &QFutureWatcherBase::finished, this,
                &HdrWizard::createHdrFinished, Qt::DirectConnection);
    }
    m_futureWatcher.setFuture(m_future);
}

void HdrWizard::createHdrFinished() {
    m_isConfigChanged = false;
    m_processing = false;
    m_Ui->NextFinishButton->setEnabled(true);
    m_Ui->cancelButton->setEnabled(true);
    m_Ui->hdrPreviewButton->setEnabled(true);

    m_pfsFrameHDR.reset(m_future.result());
    OsIntegration::getInstance().setProgress(-1);
    QApplication::restoreOverrideCursor();

    if (m_Ui->hdrPreviewCheckBox->isChecked() ) {
        m_hdrPreview->setFrame(m_pfsFrameHDR);
        m_hdrPreview->exec();
    }
    else {
        accept();
    }
}

void HdrWizard::autoAntighostingFinished() {
    m_isConfigChanged = false;
    m_processing = false;
    m_Ui->NextFinishButton->setEnabled(true);
    m_Ui->cancelButton->setEnabled(true);
    m_Ui->hdrPreviewButton->setEnabled(true);

    m_pfsFrameHDR.reset(m_future.result());
    m_Ui->progressBar->hide();
    OsIntegration::getInstance().setProgress(-1);
    QApplication::restoreOverrideCursor();
    /*
    if (m_pfsFrameHDR == nullptr)
        QDialog::reject();
    else
        accept();
    */
    if (m_Ui->hdrPreviewCheckBox->isChecked() ) {
        m_hdrPreview->setFrame(m_pfsFrameHDR);
        m_hdrPreview->exec();
    }
    else {
        accept();
    }
}

void HdrWizard::currentPageChangedInto(int newindex) {
    // predefined configs page
    if (newindex == 1) {
        m_Ui->NextFinishButton->setText(tr("&Finish"));
        // when at least 2 LDR or MDR inputs perform Manual Alignment
        int num_images = m_hdrCreationManager->getData().size();
        if (m_Ui->checkBoxEditingTools->isChecked() && num_images >= 2) {
            this->setDisabled(true);
            EditingTools *editingtools =
                new EditingTools(m_hdrCreationManager.data(),
                                 m_Ui->autoAG_checkBox->isChecked());
            if (editingtools->exec() == QDialog::Accepted) {
                m_doAutoAntighosting =
                    editingtools->isAutoAntighostingEnabled();
                m_doManualAntighosting =
                    editingtools->isManualAntighostingEnabled();
                m_agGoodImageIndex = editingtools->getAgGoodImageIndex();
                this->setDisabled(false);
            } else {
                emit reject();
            }
            delete editingtools;
        }
    } else if (newindex == 2) {  // custom config
        predefConfigsComboBoxActivated(1);
        m_Ui->NextFinishButton->setText(tr("&Finish"));
        return;
    }
}

bool HdrWizard::loadRespCurve() {
    QString loadcurvefilename = QFileDialog::getOpenFileName(
        this, tr("Load camera response curve file"), QDir::currentPath(),
        tr("Camera response curve (*.m);;All Files (*)"));

    if (loadcurvefilename.isEmpty()) {
        return false;
    }

    try {
        HdrCreationItemContainer c = m_hdrCreationManager->getData();
        m_hdrCreationManager->getResponseCurve().readFromFile(
            QFile::encodeName(loadcurvefilename).constData());

        Q_ASSERT(m_hdrCreationManager->getResponseCurve().getType() ==
                 RESPONSE_CUSTOM);

        m_Ui->RespCurveFileLoadedLineEdit->setText(loadcurvefilename);
        return true;
    } catch (const std::runtime_error & /*err*/) {
        QMessageBox::warning(
            this, tr("Invalid Response Curve File"),
            tr("Invalid Response Curve File: please try a different file"));

        return false;
    }
}

void HdrWizard::saveRespCurveFileButtonClicked() {
    QString savecurvefilename = QFileDialog::getSaveFileName(
        this, tr("Save a camera response curve file"), QDir::currentPath(),
        tr("Camera response curve (*.m)"));
    if (!savecurvefilename.isEmpty()) {
        m_Ui->CurveFileNameSaveLineEdit->setText(savecurvefilename);
        m_hdrCreationManager->setResponseCurveOutputFile(savecurvefilename);
    }
}

namespace {
void updateHdrCreationManagerModel(HdrCreationManager &manager,
                                   FusionOperator fusionOperator) {
    qDebug() << "Change model to " << (int)fusionOperator;

    manager.setFusionOperator(fusionOperator);

    if (fusionOperator != ROBERTSON_AUTO) {
    }
}

void updateHdrCreationManagerResponse(HdrCreationManager &manager,
                                      ResponseCurveType responseType) {
    qDebug() << "Change response to " << (int)responseType;

    manager.getResponseCurve().setType(responseType);
}

void updateHdrCreationManagerWeight(HdrCreationManager &manager,
                                    WeightFunctionType weight) {
    qDebug() << "Change weights to " << (int)weight;

    manager.getWeightFunction().setType(weight);
}
}

void HdrWizard::predefConfigsComboBoxActivated(int index_from_gui) {
    m_isConfigChanged = true;
    const FusionOperatorConfig *cfg = nullptr;

    if (index_from_gui <= 5) {
        cfg = &predef_confs[index_from_gui];
    } else {
        cfg = &m_customConfig[index_from_gui - 6];
    }

    updateHdrCreationManagerModel(*m_hdrCreationManager, cfg->fusionOperator);
    updateHdrCreationManagerResponse(*m_hdrCreationManager, cfg->responseCurve);
    updateHdrCreationManagerWeight(*m_hdrCreationManager, cfg->weightFunction);

    m_Ui->modelComboBox->setCurrentIndex(cfg->fusionOperator);
    m_Ui->responseCurveComboBox->setCurrentIndex(cfg->responseCurve);
    m_Ui->weightFunctionComboBox->setCurrentIndex(cfg->weightFunction);

    m_Ui->responseCurveOutputFileLabel->setEnabled(false);
    m_Ui->CurveFileNameSaveLineEdit->setEnabled(false);
    m_Ui->CurveFileNameSaveLineEdit->clear();
    m_hdrCreationManager->setResponseCurveOutputFile(QString());
    m_Ui->saveRespCurveFileButton->setEnabled(false);
}

void HdrWizard::weightingFunctionComboBoxActivated(int from_gui) {
    m_isConfigChanged = true;
    updateHdrCreationManagerWeight(*m_hdrCreationManager,
                                   weights_in_gui[from_gui]);
}

void HdrWizard::responseCurveComboBoxActivated(int from_gui) {
    m_isConfigChanged = true;
    ResponseCurveType rc = responses_in_gui[from_gui];

    if (rc == RESPONSE_CUSTOM) {
        if (loadRespCurve()) {
            m_Ui->responseCurveInputFileLabel->setEnabled(true);
            m_Ui->RespCurveFileLoadedLineEdit->setEnabled(true);
        } else if (m_hdrCreationManager->getResponseCurve().getType() !=
                   RESPONSE_CUSTOM) {
            m_Ui->responseCurveInputFileLabel->setEnabled(false);
            m_Ui->RespCurveFileLoadedLineEdit->setEnabled(false);

            // I didn't load, so I select the previous value (which is stored
            // in the current ResponseCurve object...
            updateHdrCreationManagerResponse(
                *m_hdrCreationManager,
                m_hdrCreationManager->getResponseCurve().getType());
            switch (m_hdrCreationManager->getResponseCurve().getType()) {
                case RESPONSE_LINEAR:
                    m_Ui->responseCurveComboBox->setCurrentIndex(0);
                    break;
                case RESPONSE_GAMMA:
                    m_Ui->responseCurveComboBox->setCurrentIndex(1);
                    break;
                case RESPONSE_LOG10:
                    m_Ui->responseCurveComboBox->setCurrentIndex(2);
                    break;
                case RESPONSE_SRGB:
                    m_Ui->responseCurveComboBox->setCurrentIndex(3);
                    break;
                default:
                    break;
            }
        } else {
            updateHdrCreationManagerResponse(*m_hdrCreationManager,
                                             RESPONSE_LINEAR);
            m_Ui->responseCurveComboBox->setCurrentIndex(0);
        }
    } else {
        m_Ui->responseCurveInputFileLabel->setEnabled(false);
        m_Ui->RespCurveFileLoadedLineEdit->setEnabled(false);
        m_Ui->RespCurveFileLoadedLineEdit->clear();

        updateHdrCreationManagerResponse(*m_hdrCreationManager, rc);
    }
}

void HdrWizard::modelComboBoxActivated(int from_gui) {
    qDebug() << "void HdrWizard::modelComboBoxActivated(int from_gui)";
    m_isConfigChanged = true;
    FusionOperator fo = models_in_gui[from_gui];

    updateHdrCreationManagerModel(*m_hdrCreationManager, fo);
    if (fo == ROBERTSON_AUTO) {
        m_Ui->responseCurveOutputFileLabel->setEnabled(true);
        m_Ui->CurveFileNameSaveLineEdit->setEnabled(true);
        m_Ui->saveRespCurveFileButton->setEnabled(true);
    } else {
        m_Ui->responseCurveOutputFileLabel->setEnabled(false);
        m_Ui->CurveFileNameSaveLineEdit->setEnabled(false);
        m_Ui->saveRespCurveFileButton->setEnabled(false);
    }
}

QString HdrWizard::getCaptionTEXT() {
    return QString(
        QObject::tr("Weights= ") +
        getQString(m_hdrCreationManager->getWeightFunction().getType()) +
        QObject::tr(" - Response curve= ") +
        getQString(m_hdrCreationManager->getResponseCurve().getType()) +
        QObject::tr(" - Model= ") +
        getQString(m_hdrCreationManager->getFusionOperator()));
}

QStringList HdrWizard::getInputFilesNames() {
    // return QStringList();
    return m_inputFilesName;
}

// triggered by user interaction
void HdrWizard::editingEVfinished() {
    // transform from EV value to expotime value
    if (m_hdrCreationManager->getFilesWithoutExif().empty()) {
        m_Ui->NextFinishButton->setEnabled(true);
        // give an offset to the EV values if they are outside of the -10..10
        // range.
        // m_hdrCreationManager->checkEVvalues();
        m_Ui->confirmloadlabel->setText(
            tr("<center><font color=\"#008400\"><h3><b>All the EV values have "
               "been "
               "set.<br>Now click on Next button.</b></h3></font></center>"));
    } else {
        m_Ui->confirmloadlabel->setText(
            QString(tr("<center><h3><b>To proceed you need to manually set the "
                       "exposure values.<br><font color=\"#FF0000\">%1</font> "
                       "values still required.</b></h3></center>"))
                .arg(m_hdrCreationManager->getFilesWithoutExif().size()));
    }
}

void HdrWizard::resizeEvent(QResizeEvent *) {
    qDebug() << "void HdrWizard::resizeEvent(QResizeEvent*)";

    int currentRow = m_Ui->tableWidget->currentRow();
    int numberOfRow = m_Ui->tableWidget->rowCount();

    if (currentRow >= 0 && currentRow < numberOfRow) {
        m_Ui->previewLabel->setPixmap(QPixmap::fromImage(
            m_hdrCreationManager->getFile(currentRow)
                .qimage()
                .scaled(m_Ui->previewLabel->size(), Qt::KeepAspectRatio)));
    } else {
        m_Ui->previewLabel->setText(QString());
    }
}

void HdrWizard::alignSelectionClicked() {
    m_Ui->autoCropCheckBox->setEnabled(m_Ui->ais_radioButton->isChecked());
}

void HdrWizard::reject() {
    QApplication::restoreOverrideCursor();
    if (m_processing) {
        m_ph.qtCancel();
    } else {
        m_hdrCreationManager->reset();
        QDialog::reject();
    }
}

void HdrWizard::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        m_Ui->tableWidget->selectRow((m_Ui->tableWidget->currentRow() ==
                                      m_Ui->tableWidget->rowCount() - 1)
                                         ? 0
                                         : m_Ui->tableWidget->currentRow() + 1);
    } else if (event->key() == Qt::Key_Escape) {
        emit reject();
    }
}

void HdrWizard::writeAisData(QByteArray data) {
    qDebug() << data;
    if (data.contains("[1A")) data.replace("[1A", "");
    if (data.contains("[2A")) data.replace("[2A", "");
    if (data.contains(QChar(0x01B).toLatin1()))
        data.replace(QChar(0x01B).toLatin1(), "");

    m_Ui->textEdit->append(data);
    if (data.contains(": remapping")) {
        QRegExp exp("\\:\\s*(\\d+)\\s*");
        exp.indexIn(QString(data.data()));
        emit setRange(0, 100);
        emit setValue(exp.cap(1).toInt());
    }
}

void HdrWizard::on_pushButtonSaveSettings_clicked() {
    /*
    QSqlQuery query;
    QString response_filename;
    int weight = m_Ui->triGaussPlateauComboBox->currentIndex();
    int response;
    if (m_Ui->predefRespCurveRadioButton->isChecked()) {
        response = m_Ui->gammaLinLogComboBox->currentIndex();
    }
    else if (m_Ui->loadRespCurveFromFileCheckbox->isChecked()) {
        response = FROM_FILE;
        response_filename = m_Ui->RespCurveFileLoadedLineEdit->text();
    }
    else
        response = FROM_ROBERTSON;

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
    */
}

void HdrWizard::updateProgressBar(int value) {
    if (value == 0) m_Ui->progressBar->setMaximum(100);
    m_Ui->progressBar->setValue(value);
}

void HdrWizard::updateThresholdSlider(int newValue) {
    float newThreshold = ((float)newValue) / 10000.f;
    bool oldState = m_Ui->threshold_doubleSpinBox->blockSignals(true);
    m_Ui->threshold_doubleSpinBox->setValue(newThreshold);
    m_Ui->threshold_doubleSpinBox->blockSignals(oldState);
}

void HdrWizard::updateThresholdSpinBox(double newThreshold) {
    bool oldState = m_Ui->threshold_horizontalSlider->blockSignals(true);
    m_Ui->threshold_horizontalSlider->setValue((int)(newThreshold * 10000));
    m_Ui->threshold_horizontalSlider->blockSignals(oldState);
}

void HdrWizard::setEVsValues() {
    int tot_images = m_Ui->tableWidget->rowCount();
    float minEV = -2.0f * (float)(tot_images / 2);
    float EV = minEV;
    for (int i = 0; i < tot_images; i++, EV += 2.0f) {
        QTableWidgetItem *tableitem = m_Ui->tableWidget->item(i, 1);
        updateTableItem(tableitem, EV);
        m_hdrCreationManager->getFile(i).setEV(
            EV + m_hdrCreationManager->getEVOffset());
    }
    bool oldState = m_Ui->ImageEVdsb->blockSignals(true);
    m_Ui->ImageEVdsb->setValue(minEV);
    m_Ui->EVSlider->setValue((int)100.f * minEV);
    m_Ui->ImageEVdsb->blockSignals(oldState);
    updateLabelMaybeNext(1);
}

void HdrWizard::updateHideLogButtonText(bool b) {
    b ? m_Ui->HideLogButton->setText(tr("Show Log")) : m_Ui->HideLogButton->setText(tr("Hide Log"));
}
