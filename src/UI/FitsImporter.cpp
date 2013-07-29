/**
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2013 Franco Comida
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
 * @author Franco Comida <fcomida@users.sourceforge.net>
 */

#include <QFileDialog>
#include <QDebug>


#include "FitsImporter.h"
#include "ui_FitsImporter.h"
#include "Common/LuminanceOptions.h"


FitsImporter::~FitsImporter() {}

FitsImporter::FitsImporter(QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::FitsImporter)
{
    m_ui->setupUi(this);

#ifdef Q_WS_MAC
    this->setWindowModality(Qt::WindowModal); // In OS X, the QMessageBox is modal to the window
#endif
}

void FitsImporter::on_pushButtonLuminosity_clicked()
{
    QString filetypes = "FITS (*.fit *.FIT *.fits *.FITS);;";
    m_luminosityChannel = QFileDialog::getOpenFileName(this,
                                                      tr("Load one FITS image..."),
                                                      LuminanceOptions().getDefaultPathHdrIn(),
                                                      filetypes );
    m_ui->lineEditLuminosity->setText(m_luminosityChannel);
    checkOKButton();
}

void FitsImporter::on_pushButtonRed_clicked()
{
    QString filetypes = "FITS (*.fit *.FIT *.fits *.FITS);;";
    m_redChannel = QFileDialog::getOpenFileName(this,
                                                      tr("Load one FITS image..."),
                                                      LuminanceOptions().getDefaultPathHdrIn(),
                                                      filetypes );
    m_ui->lineEditRed->setText(m_redChannel);
    checkOKButton();
}

void FitsImporter::on_pushButtonGreen_clicked()
{
    QString filetypes = "FITS (*.fit *.FIT *.fits *.FITS);;";
    m_greenChannel = QFileDialog::getOpenFileName(this,
                                                      tr("Load one FITS image..."),
                                                      LuminanceOptions().getDefaultPathHdrIn(),
                                                      filetypes );
    m_ui->lineEditGreen->setText(m_greenChannel);
    checkOKButton();
}

void FitsImporter::on_pushButtonBlue_clicked()
{
    QString filetypes = "FITS (*.fit *.FIT *.fits *.FITS);;";
    m_blueChannel = QFileDialog::getOpenFileName(this,
                                                      tr("Load one FITS image..."),
                                                      LuminanceOptions().getDefaultPathHdrIn(),
                                                      filetypes );
    m_ui->lineEditBlue->setText(m_blueChannel);
    checkOKButton();
}

void FitsImporter::checkOKButton()
{
    if (m_redChannel != "" && m_greenChannel != "" && m_blueChannel != "")
        m_ui->pushButtonOK->setEnabled(true);
}
