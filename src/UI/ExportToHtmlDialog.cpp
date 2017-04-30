/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2016 Franco Comida
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
 * @author Franco Comida <francocomida@users.sourceforge.net>
 */

#include "ExportToHtmlDialog.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>

#include "ui_ExportToHtmlDialog.h"
#include "HdrHTML/pfsouthdrhtml.h"
#include "Libpfs/frame.h"
#include "Libpfs/exception.h"
#include "Libpfs/manip/resize.h"
#include "Libpfs/manip/copy.h"


ExportToHtmlDialog::ExportToHtmlDialog(QWidget* parent, pfs::Frame *frame)
    : QDialog(parent)
    , m_frame(frame)
    , m_pageName()
    , m_outputFolder()
    , m_imagesFolder()
    , m_Ui(new Ui::ExportToHtmlDialog)
{
    m_Ui->setupUi(this);

    connect(m_Ui->ExportButton, SIGNAL(clicked()), this, SLOT(onExportButtonClicked()));
    connect(m_Ui->OutputFolderButton, SIGNAL(clicked()), this, SLOT(onOutputFolderButtonClicked()));
    connect(m_Ui->lineEditPageName, SIGNAL(editingFinished()), this, SLOT(onEditPageNameFinished()));
    connect(m_Ui->lineEditOutputFolder, SIGNAL(editingFinished()), this, SLOT(onEditOutputFolderFinished()));
    connect(m_Ui->lineEditImagesFolder, SIGNAL(editingFinished()), this, SLOT(onEditImagesFolderFinished()));
}

void ExportToHtmlDialog::onOutputFolderButtonClicked()
{
    QString dirname=QFileDialog::getExistingDirectory(this, tr("Choose a directory"), QDir::homePath() );

    QFileInfo test(dirname);
    if (test.isWritable() && test.exists() && test.isDir() && !dirname.isEmpty())
    {
        m_outputFolder = dirname;

        m_Ui->lineEditOutputFolder->setText(dirname);
        check_enable_export();
    }
}

void ExportToHtmlDialog::onExportButtonClicked()
{
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    
    bool quit = false;
    pfs::Frame *resized;
    int size_percent = m_Ui->spinBoxSize->value();
    if (size_percent == 100) {
        resized = pfs::copy(m_frame);
    }
    else {
        int resized_width = (int)((float)(size_percent*m_frame->getWidth()) / 100.f);
        resized = pfs::resize(m_frame, resized_width);
    }
    try {
        generate_hdrhtml(resized, 
                         m_pageName.toStdString(), m_outputFolder.toStdString(), m_imagesFolder.toStdString(), 
                         "", "", m_Ui->spinBoxQuality->value(), false);
    }
    catch( pfs::Exception &e) {
        delete resized;
        QApplication::restoreOverrideCursor();
        QMessageBox::critical(this, tr("Error: "), e.what() ,
                              QMessageBox::Ok, QMessageBox::NoButton);
        quit = true;
    }
    if (!quit) { 
        QApplication::restoreOverrideCursor();
        if (m_Ui->checkBoxOpenOnBrowser->isChecked()) {
            QString url = "file:///" + m_outputFolder + "/" + m_pageName + ".html";
            QDesktopServices::openUrl(QUrl(url));
        }
    }
    delete resized;
    accept();
}

void ExportToHtmlDialog::onEditPageNameFinished()
{
    m_pageName = m_Ui->lineEditPageName->text();
    QFileInfo qfi = QFileInfo(m_pageName);
    m_pageName = qfi.baseName();
    m_Ui->lineEditPageName->setText(m_pageName);

    check_enable_export();
}

void ExportToHtmlDialog::onEditOutputFolderFinished()
{
    m_outputFolder = m_Ui->lineEditOutputFolder->text();
    QFileInfo qfi = QFileInfo(m_outputFolder);
    if (!m_outputFolder.isEmpty() && !qfi.isDir())
        QMessageBox::critical(this, tr("Error: "), m_outputFolder + tr(" must be a directory.") ,
                              QMessageBox::Ok, QMessageBox::NoButton);
    else
        check_enable_export();
}

void ExportToHtmlDialog::onEditImagesFolderFinished()
{
    m_imagesFolder = m_Ui->lineEditImagesFolder->text();
    check_enable_export();
}

void ExportToHtmlDialog::check_enable_export()
{
    m_Ui->ExportButton->setEnabled(
            (!m_Ui->lineEditPageName->text().isEmpty()) &&
            (!m_Ui->lineEditOutputFolder->text().isEmpty()));
}

ExportToHtmlDialog::~ExportToHtmlDialog()
{
}

