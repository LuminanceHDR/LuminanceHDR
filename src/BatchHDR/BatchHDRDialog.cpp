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

#include <QDebug>
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>

#include "arch/math.h"
#include "BatchHDR/BatchHDRDialog.h"
#include "ui_BatchHDRDialog.h"
#include "Libpfs/pfs.h"
#include "Libpfs/domio.h"
#include "Core/IOWorker.h"
#include "HdrCreation/HdrCreationManager.h"
#include "OsIntegration/osintegration.h"

BatchHDRDialog::BatchHDRDialog(QWidget *p):
QDialog(p),
	m_Ui(new Ui::BatchHDRDialog),
	m_numProcessed(0),
	m_processed(0),
	m_errors(false),
	m_loading_error(false),
	m_abort(false),
	m_processing(false)
{
	m_Ui->setupUi(this);

	m_Ui->closeButton->hide();

	m_hdrCreationManager = new HdrCreationManager;
	m_IO_Worker = new IOWorker;

	connect(m_Ui->horizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(num_bracketed_changed(int)));
	connect(m_Ui->spinBox, SIGNAL(valueChanged(int)), this, SLOT(num_bracketed_changed(int)));

    connect(m_Ui->MTBRadioButton, SIGNAL(clicked()), this, SLOT(on_align_selection_clicked()));
    connect(m_Ui->aisRadioButton, SIGNAL(clicked()), this, SLOT(on_align_selection_clicked()));

	connect(m_hdrCreationManager, SIGNAL(finishedLoadingInputFiles(QStringList)), this, SLOT(align(QStringList)));
	connect(m_hdrCreationManager, SIGNAL(finishedAligning(int)), this, SLOT(create_hdr(int)));
	connect(m_hdrCreationManager, SIGNAL(errorWhileLoading(QString)), this, SLOT(error_while_loading(QString)));
	connect(m_hdrCreationManager, SIGNAL(aisDataReady(QByteArray)), this, SLOT(writeAisData(QByteArray)));
	connect(m_hdrCreationManager, SIGNAL(processed()), this, SLOT(processed()));

	m_tempDir = m_luminance_options.getTempDir();
	m_batchHdrInputDir = m_luminance_options.getBatchHdrPathInput("");
	m_batchHdrOutputDir = m_luminance_options.getBatchHdrPathOutput("");

	m_Ui->inputLineEdit->setText(m_batchHdrInputDir);
	m_Ui->outputLineEdit->setText(m_batchHdrOutputDir);
    check_start_button();
}

BatchHDRDialog::~BatchHDRDialog()
{
	qDebug() << "BatchHDRDialog::~BatchHDRDialog()";
	QStringList  fnames = m_hdrCreationManager->getFileList();
	int n = fnames.size();

	for (int i = 0; i < n; i++) {
		QString fname = m_hdrCreationManager->getFileList().at(i);
		QFileInfo qfi(fname);

		QString thumb_name = QString(m_tempDir + "/"+  qfi.completeBaseName() + ".thumb.jpg");
		QFile::remove(thumb_name);

		thumb_name = QString(m_tempDir + "/" + qfi.completeBaseName() + ".thumb.ppm");
		QFile::remove(thumb_name);
	}
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

	if (doStart) {
		m_Ui->horizontalSlider->setEnabled(false);
		m_Ui->spinBox->setEnabled(false);
		m_Ui->profileComboBox->setEnabled(false);
		m_Ui->formatComboBox->setEnabled(false);
		m_Ui->selectInputFolder->setEnabled(false);
		m_Ui->inputLineEdit->setEnabled(false);
		m_Ui->selectOutputFolder->setEnabled(false);
		m_Ui->outputLineEdit->setEnabled(false);
		m_Ui->groupBox->setEnabled(false);
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
		m_hdrCreationManager->reset();
		this->reject();
	}
	if (!m_bracketed.isEmpty())
	{
		m_Ui->textEdit->append(tr("Creating HDR..."));
		m_numProcessed++;
		QStringList toProcess;
		for (int i = 0; i < m_Ui->spinBox->value(); ++i)
		{
			toProcess << m_bracketed.takeFirst();
		}
		qDebug() << "BatchHDRDialog::batch_hdr() Files to process: " << toProcess;
		m_hdrCreationManager->setFileList(toProcess);
		m_hdrCreationManager->loadInputFiles();
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

void BatchHDRDialog::align(QStringList filesLackingExif)
{
	if (!filesLackingExif.isEmpty())
	{
		qDebug() << "BatchHDRDialog::align Error: missing EXIF data";
		m_Ui->textEdit->append(tr("Error: missing EXIF data"));
		foreach (QString fname, filesLackingExif)
			m_Ui->textEdit->append(fname);
		m_errors = true;
		QStringList  fnames = m_hdrCreationManager->getFileList();
		int n = fnames.size();

		for (int i = 0; i < n; i++) {
			QString fname = m_hdrCreationManager->getFileList().at(i);
			QFileInfo qfi(fname);
			QString thumb_name = QString(m_tempDir + "/"+  qfi.completeBaseName() + ".thumb.jpg");
			QFile::remove(thumb_name);
			thumb_name = QString(m_tempDir + "/" + qfi.completeBaseName() + ".thumb.ppm");
			QFile::remove(thumb_name);
		}
		m_hdrCreationManager->reset();
		batch_hdr();
		return;
	}
	if (m_Ui->autoAlignCheckBox->isChecked())
	{
		m_Ui->textEdit->append(tr("Aligning..."));
		if (m_Ui->aisRadioButton->isChecked())
        {
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
	QString suffix = m_Ui->formatComboBox->currentText();
	m_hdrCreationManager->chosen_config = predef_confs[m_Ui->profileComboBox->currentIndex()];
	pfs::Frame* resultHDR = m_hdrCreationManager->createHdr(false, 1);

	int paddingLength = ceil(log10(m_total + 1.0f));
	QString outName = m_Ui->outputLineEdit->text() + "/hdr_" + QString("%1").arg(m_numProcessed, paddingLength, 10, QChar('0')) + "." + suffix;
	m_IO_Worker->write_hdr_frame(resultHDR, outName);
	
    pfs::DOMIO::freeFrame(resultHDR);
	
	QStringList  fnames = m_hdrCreationManager->getFileList();
	int n = fnames.size();

	for (int i = 0; i < n; i++) {
		QString fname = m_hdrCreationManager->getFileList().at(i);
		QFileInfo qfi(fname);
		QString thumb_name = QString(m_tempDir + "/"+  qfi.completeBaseName() + ".thumb.jpg");
		QFile::remove(thumb_name);
		thumb_name = QString(m_tempDir + "/" + qfi.completeBaseName() + ".thumb.ppm");
		QFile::remove(thumb_name);
	}
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
	QStringList  fnames = m_hdrCreationManager->getFileList();
	int n = fnames.size();

	for (int i = 0; i < n; i++) {
		QString fname = m_hdrCreationManager->getFileList().at(i);
		QFileInfo qfi(fname);
		QString thumb_name = QString(m_tempDir + "/"+  qfi.completeBaseName() + ".thumb.jpg");
		QFile::remove(thumb_name);
		thumb_name = QString(m_tempDir + "/" + qfi.completeBaseName() + ".thumb.ppm");
		QFile::remove(thumb_name);
	}
	try_to_continue();
}

void BatchHDRDialog::writeAisData(QByteArray data)
{
	qDebug() << data;
	m_Ui->textEdit->append(data);
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
		m_Ui->cancelButton->setText(tr("Aborting..."));
		m_Ui->cancelButton->setEnabled(false);
	}
	else
		this->reject();
}

void BatchHDRDialog::on_align_selection_clicked()
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
			m_hdrCreationManager->reset();
			batch_hdr(); // try to continue
		}
	}
}
