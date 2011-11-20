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

#include "BatchHDR/BatchHDRDialog.h"
#include "ui_BatchHDRDialog.h"
#include "Libpfs/pfs.h"
#include "Core/IOWorker.h"
#include "HdrCreation/HdrCreationManager.h"

BatchHDRDialog::BatchHDRDialog(QWidget *p):
    QDialog(p),
    m_Ui(new Ui::BatchHDRDialog),
    m_numProcessed(0),
    m_errors(false)
{
    m_Ui->setupUi(this);

    setWindowModality(Qt::WindowModal);   // Mac Mode
	
    m_Ui->closePushButton->hide();

	m_hdrCreationManager = new HdrCreationManager;
	m_IO_Worker = new IOWorker;
	
        connect(m_Ui->horizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(num_bracketed_changed(int)));
        connect(m_Ui->spinBox, SIGNAL(valueChanged(int)), this, SLOT(num_bracketed_changed(int)));

        connect(m_Ui->inputPushButton, SIGNAL(clicked()), this, SLOT(add_files()));
        connect(m_Ui->outputPushButton, SIGNAL(clicked()), this, SLOT(add_out_dir()));
        connect(m_Ui->startPushButton, SIGNAL(clicked()), this, SLOT(init_batch_hdr()));
        connect(m_Ui->closePushButton, SIGNAL(clicked()), this, SLOT(accept()));

	connect(m_hdrCreationManager, SIGNAL(finishedLoadingInputFiles(QStringList)), this, SLOT(align(QStringList)));
	connect(m_hdrCreationManager, SIGNAL(finishedAligning()), this, SLOT(create_hdr()));
	connect(m_hdrCreationManager, SIGNAL(errorWhileLoading(QString)), this, SLOT(error_while_loading(QString)));
	connect(m_hdrCreationManager, SIGNAL(aisDataReady(QByteArray)), this, SLOT(writeAisData(QByteArray)));

    m_tempDir = m_luminance_options.getTempDir();
    m_batchHdrInputDir = m_luminance_options.getBatchHdrPathInput();
    m_batchHdrOutputDir = m_luminance_options.getBatchHdrPathOutput();
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
	if (value == 1)
	{
                m_Ui->autoAlignCheckBox->setEnabled(false);
                m_Ui->aisRadioButton->setEnabled(false);
                m_Ui->MTBRadioButton->setEnabled(false);
	}
	else
	{
                m_Ui->autoAlignCheckBox->setEnabled(true);
                m_Ui->aisRadioButton->setEnabled(true);
                m_Ui->MTBRadioButton->setEnabled(true);
	}
}

void BatchHDRDialog::add_files()
{
        m_Ui->inputLineEdit->setText(QFileDialog::getExistingDirectory(this, tr("Choose a directory"), m_batchHdrInputDir));
        if (!m_Ui->inputLineEdit->text().isEmpty())
	{
                QDir chosendir(m_Ui->inputLineEdit->text());
		chosendir.setFilter(QDir::Files);
		m_bracketed = chosendir.entryList();
		//hack to prepend to this list the path as prefix.
		m_bracketed.replaceInStrings(QRegExp("(.+)"), chosendir.path()+"/\\1");
		qDebug() << m_bracketed;

                // if the new dir, the one just chosen by the user, is different from the one stored in the settings,
                // update the settings
                if (m_batchHdrInputDir != m_Ui->inputLineEdit->text())
                {
                    m_batchHdrInputDir = m_Ui->inputLineEdit->text();
                    m_luminance_options.setBatchHdrPathInput(m_batchHdrInputDir);
                }
	}
	check_start_button();
}

void BatchHDRDialog::add_out_dir()
{
        m_Ui->outputLineEdit->setText(QFileDialog::getExistingDirectory(this, tr("Choose a directory"), m_batchHdrOutputDir));
        // if the new dir, the one just chosen by the user, is different from the one stored in the settings,
        // update the settings
        if (m_batchHdrOutputDir != m_Ui->outputLineEdit->text())
	{
            m_batchHdrOutputDir = m_Ui->outputLineEdit->text();
            m_luminance_options.setBatchHdrPathOutput(m_batchHdrOutputDir);
	}
	check_start_button();
}

void BatchHDRDialog::init_batch_hdr()
{
        if (m_Ui->inputLineEdit->text().isEmpty() || m_Ui->outputLineEdit->text().isEmpty())
		return;

        m_Ui->startPushButton->setEnabled(false);
        m_Ui->progressBar->setMaximum(m_bracketed.count() / m_Ui->spinBox->value());
        m_Ui->textEdit->append(tr("Started processing..."));
    // mouse pointer to busy
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
	this->batch_hdr();
}

void BatchHDRDialog::batch_hdr()
{
	if (!m_bracketed.isEmpty())
	{
		m_numProcessed++;
		QStringList toProcess;
                for (int i = 0; i < m_Ui->spinBox->value(); ++i)
		{
			toProcess << m_bracketed.takeFirst();
		}
		qDebug() << toProcess;
		m_hdrCreationManager->setFileList(toProcess);
		m_hdrCreationManager->loadInputFiles();
	}	
	else
	{
                m_Ui->closePushButton->show();
                m_Ui->cancelPushButton->hide();
                m_Ui->startPushButton->hide();
                m_Ui->progressBar->hide();
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
		this->batch_hdr(); // try to continue
		return;
	}
        if (m_Ui->autoAlignCheckBox->isChecked())
	{
                m_Ui->textEdit->append(tr("Aligning..."));
                if (m_Ui->aisRadioButton->isChecked())
			m_hdrCreationManager->align_with_ais();
		else
			m_hdrCreationManager->align_with_mtb();
	}
	else
		this->create_hdr();
}

void BatchHDRDialog::create_hdr()
{
        m_Ui->textEdit->append(tr("Creating HDR..."));
        QString suffix = m_Ui->formatComboBox->currentText();
        m_hdrCreationManager->chosen_config = predef_confs[m_Ui->profileComboBox->currentIndex()];
	pfs::Frame* resultHDR = m_hdrCreationManager->createHdr(false, 1);
        m_IO_Worker->write_hdr_frame(resultHDR, m_Ui->outputLineEdit->text() + "/hdr_" + QString::number(m_numProcessed) + "." + suffix);
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
        m_Ui->progressBar->setValue(m_Ui->progressBar->value() + 1);
        m_Ui->textEdit->append(tr("Written ") + m_Ui->outputLineEdit->text() + "/hdr_" + QString::number(m_numProcessed) + "." + suffix );
	this->batch_hdr();
}

void BatchHDRDialog::error_while_loading(QString message)
{
	qDebug() << message;
        m_Ui->textEdit->append(tr("Error: ") + message);
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
	this->batch_hdr(); // try to continue
}

void BatchHDRDialog::writeAisData(QByteArray data)
{
	qDebug() << data;
        m_Ui->textEdit->append(data);
}

void BatchHDRDialog::check_start_button()
{
        if (m_Ui->inputLineEdit->text() != "" && m_Ui->outputLineEdit->text() != "")
                m_Ui->startPushButton->setEnabled(true);
}
