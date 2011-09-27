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

#include "BatchHDRDialog.h"
#include "Common/config.h"
#include "Common/global.h"
#include "Libpfs/pfs.h"

BatchHDRDialog::BatchHDRDialog(QWidget *p) : QDialog(p), m_numProcessed(0), m_errors(false)
{
    setupUi(this);

    setWindowModality(Qt::WindowModal);   // Mac Mode
	
	closePushButton->hide();

	luminance_options = LuminanceOptions::getInstance();

	RecentBatchHdrInputDir = settings->value(KEY_RECENT_PATH_BATCH_HDR_INPUT, QDir::currentPath()).toString();
	RecentBatchHdrOutputDir = settings->value(KEY_RECENT_PATH_BATCH_HDR_OUTPUT, QDir::currentPath()).toString();

	m_hdrCreationManager = new HdrCreationManager;
	m_IO_Worker = new IOWorker;
	
	connect(horizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(num_bracketed_changed(int)));
	connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(num_bracketed_changed(int)));

	connect(inputPushButton, SIGNAL(clicked()), this, SLOT(add_files()));
	connect(outputPushButton, SIGNAL(clicked()), this, SLOT(add_out_dir()));
	connect(startPushButton, SIGNAL(clicked()), this, SLOT(init_batch_hdr()));
	connect(closePushButton, SIGNAL(clicked()), this, SLOT(accept()));

	connect(m_hdrCreationManager, SIGNAL(finishedLoadingInputFiles(QStringList)), this, SLOT(align(QStringList)));
	connect(m_hdrCreationManager, SIGNAL(finishedAligning()), this, SLOT(create_hdr()));
	connect(m_hdrCreationManager, SIGNAL(errorWhileLoading(QString)), this, SLOT(error_while_loading(QString)));
	connect(m_hdrCreationManager, SIGNAL(aisDataReady(QByteArray)), this, SLOT(writeAisData(QByteArray)));
}

BatchHDRDialog::~BatchHDRDialog()
{
	qDebug() << "BatchHDRDialog::~BatchHDRDialog()";
	QStringList  fnames = m_hdrCreationManager->getFileList();
	int n = fnames.size();
	
	for (int i = 0; i < n; i++) {
		QString fname = m_hdrCreationManager->getFileList().at(i);
		QFileInfo qfi(fname);
		QString thumb_name = QString(luminance_options->tempfilespath + "/"+  qfi.completeBaseName() + ".thumb.jpg");
		QFile::remove(thumb_name);
		thumb_name = QString(luminance_options->tempfilespath + "/" + qfi.completeBaseName() + ".thumb.ppm");
		QFile::remove(thumb_name);
	}
	m_hdrCreationManager->reset();
	delete m_hdrCreationManager;
	delete m_IO_Worker;
}

void BatchHDRDialog::num_bracketed_changed(int value)
{
	qDebug() << "BatchHDRDialog::num_bracketed_changed " << value;
	if (value == 1)
	{
		autoAlignCheckBox->setEnabled(false);
		aisRadioButton->setEnabled(false);
		MTBRadioButton->setEnabled(false);
	}
	else
	{
		autoAlignCheckBox->setEnabled(true);
		aisRadioButton->setEnabled(true);
		MTBRadioButton->setEnabled(true);
	}
}

void BatchHDRDialog::add_files()
{
	inputLineEdit->setText(QFileDialog::getExistingDirectory(this, tr("Choose a directory"), RecentBatchHdrInputDir));
	if (!inputLineEdit->text().isEmpty())
	{
		QDir chosendir(inputLineEdit->text());
		chosendir.setFilter(QDir::Files);
		m_bracketed = chosendir.entryList();
		//hack to prepend to this list the path as prefix.
		m_bracketed.replaceInStrings(QRegExp("(.+)"), chosendir.path()+"/\\1");
		qDebug() << m_bracketed;
		// if the new dir, the one just chosen by the user, is different from the one stored in the settings, update the settings
		if (RecentBatchHdrInputDir != inputLineEdit->text())
		{
			RecentBatchHdrInputDir = inputLineEdit->text();
			settings->setValue(KEY_RECENT_PATH_BATCH_HDR_INPUT, RecentBatchHdrInputDir);
		}
	}
	check_start_button();
}

void BatchHDRDialog::add_out_dir()
{
	outputLineEdit->setText(QFileDialog::getExistingDirectory(this, tr("Choose a directory"), RecentBatchHdrOutputDir));
	// if the new dir, the one just chosen by the user, is different from the one stored in the settings, update the settings
	if (RecentBatchHdrOutputDir != outputLineEdit->text())
	{
		RecentBatchHdrOutputDir = outputLineEdit->text();
		settings->setValue(KEY_RECENT_PATH_BATCH_HDR_OUTPUT, RecentBatchHdrOutputDir);
	}
	check_start_button();
}

void BatchHDRDialog::init_batch_hdr()
{
	if (inputLineEdit->text().isEmpty() || outputLineEdit->text().isEmpty())
		return;

	startPushButton->setEnabled(false);
	progressBar->setMaximum(m_bracketed.count() / spinBox->value());	
	textEdit->append(tr("Started processing..."));
	this->batch_hdr();
}

void BatchHDRDialog::batch_hdr()
{
	if (!m_bracketed.isEmpty())
	{
		m_numProcessed++;
		QStringList toProcess;
		for (int i = 0; i < spinBox->value(); i++)
		{
			toProcess << m_bracketed.takeFirst();
		}
		qDebug() << toProcess;
		m_hdrCreationManager->setFileList(toProcess);
		m_hdrCreationManager->loadInputFiles();
	}	
	else
	{
		closePushButton->show();
		cancelPushButton->hide();
		startPushButton->hide();
		progressBar->hide();
		if (m_errors)
			textEdit->append(tr("Completed with errors"));
		else
			textEdit->append(tr("Completed without errors"));
	}
}

void BatchHDRDialog::align(QStringList filesLackingExif)
{
	if (!filesLackingExif.isEmpty())
	{
		qDebug() << "BatchHDRDialog::align Error: missing EXIF data";
		textEdit->append(tr("Error: missing EXIF data"));
		foreach (QString fname, filesLackingExif)
			textEdit->append(fname);
		m_errors = true;
		QStringList  fnames = m_hdrCreationManager->getFileList();
		int n = fnames.size();
	
		for (int i = 0; i < n; i++) {
			QString fname = m_hdrCreationManager->getFileList().at(i);
			QFileInfo qfi(fname);
			QString thumb_name = QString(luminance_options->tempfilespath + "/"+  qfi.completeBaseName() + ".thumb.jpg");
			QFile::remove(thumb_name);
			thumb_name = QString(luminance_options->tempfilespath + "/" + qfi.completeBaseName() + ".thumb.ppm");
			QFile::remove(thumb_name);
		}
		m_hdrCreationManager->reset();
		this->batch_hdr(); // try to continue
		return;
	}
	if (autoAlignCheckBox->isChecked())
	{
		textEdit->append(tr("Aligning..."));
		if (aisRadioButton->isChecked())
			m_hdrCreationManager->align_with_ais();
		else
			m_hdrCreationManager->align_with_mtb();
	}
	else
		this->create_hdr();
}

void BatchHDRDialog::create_hdr()
{
	textEdit->append(tr("Creating HDR..."));
	QString suffix = formatComboBox->currentText();
	m_hdrCreationManager->chosen_config = predef_confs[profileComboBox->currentIndex()];
	pfs::Frame* resultHDR = m_hdrCreationManager->createHdr(false, 1);
	m_IO_Worker->write_hdr_frame(resultHDR, outputLineEdit->text() + "/hdr_" + QString::number(m_numProcessed) + "." + suffix);
	QStringList  fnames = m_hdrCreationManager->getFileList();
	int n = fnames.size();
	
	for (int i = 0; i < n; i++) {
		QString fname = m_hdrCreationManager->getFileList().at(i);
		QFileInfo qfi(fname);
		QString thumb_name = QString(luminance_options->tempfilespath + "/"+  qfi.completeBaseName() + ".thumb.jpg");
		QFile::remove(thumb_name);
		thumb_name = QString(luminance_options->tempfilespath + "/" + qfi.completeBaseName() + ".thumb.ppm");
		QFile::remove(thumb_name);
	}
	m_hdrCreationManager->reset();
	progressBar->setValue(progressBar->value() + 1);
	textEdit->append(tr("Written ") + outputLineEdit->text() + "/hdr_" + QString::number(m_numProcessed) + "." + suffix );
	this->batch_hdr();
}

void BatchHDRDialog::error_while_loading(QString message)
{
	qDebug() << message;
	textEdit->append(tr("Error: ") + message);
	m_errors = true;
	QStringList  fnames = m_hdrCreationManager->getFileList();
	int n = fnames.size();
	
	for (int i = 0; i < n; i++) {
		QString fname = m_hdrCreationManager->getFileList().at(i);
		QFileInfo qfi(fname);
		QString thumb_name = QString(luminance_options->tempfilespath + "/"+  qfi.completeBaseName() + ".thumb.jpg");
		QFile::remove(thumb_name);
		thumb_name = QString(luminance_options->tempfilespath + "/" + qfi.completeBaseName() + ".thumb.ppm");
		QFile::remove(thumb_name);
	}
	m_hdrCreationManager->reset();
	this->batch_hdr(); // try to continue
}

void BatchHDRDialog::writeAisData(QByteArray data)
{
	qDebug() << data;
	textEdit->append(data);
}

void BatchHDRDialog::check_start_button()
{
	if (inputLineEdit->text() != "" && outputLineEdit->text() != "")
		startPushButton->setEnabled(true);
}
