/**
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
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
 *
 */

#include "UI/TiffModeDialog.h"
#include "UI/ui_TiffModeDialog.h"

namespace {
const static QString TIFF_MODE_HDR_KEY =
    QStringLiteral("tiffmodedialog/mode/hdr");
const static int TIFF_MODE_HDR_VALUE = 0;

const static QString TIFF_MODE_LDR_KEY =
    QStringLiteral("tiffmodedialog/mode/ldr");
const static int TIFF_MODE_LDR_VALUE = 0;
}

TiffModeDialog::TiffModeDialog(bool hdrMode, int defaultValue, QWidget *parent)
    : QDialog(parent),
      m_hdrMode(hdrMode),
      m_ui(new Ui::TiffModeDialog),
      m_options(new LuminanceOptions()) {
    m_ui->setupUi(this);

    if (m_hdrMode) {
        m_ui->comboBox->insertItem(
            0, QStringLiteral("TIFF 32 bit/channel floating point"));
        m_ui->comboBox->insertItem(1, QStringLiteral("TIFF LogLuv"));

        if (defaultValue >= 0) {
            m_ui->comboBox->setCurrentIndex(defaultValue - 2);
        } else
            m_ui->comboBox->setCurrentIndex(
                m_options->value(TIFF_MODE_HDR_KEY, TIFF_MODE_HDR_VALUE)
                    .toInt());
    } else {
        m_ui->comboBox->insertItem(0, QStringLiteral("TIFF 8 bit/channel"));
        m_ui->comboBox->insertItem(1, QStringLiteral("TIFF 16 bit/channel"));
        m_ui->comboBox->insertItem(
            2, QStringLiteral("TIFF 32 bit/channel floating point"));

        if (defaultValue >= 0) {
            m_ui->comboBox->setCurrentIndex(defaultValue);
        } else
            m_ui->comboBox->setCurrentIndex(
                m_options->value(TIFF_MODE_LDR_KEY, TIFF_MODE_LDR_VALUE)
                    .toInt());
    }

#ifdef Q_OS_MAC
    this->setWindowModality(
        Qt::WindowModal);  // In OS X, the QMessageBox is modal to the window
#endif
}

TiffModeDialog::~TiffModeDialog() {
    if (m_hdrMode) {
        m_options->setValue(TIFF_MODE_HDR_KEY, m_ui->comboBox->currentIndex());
    } else {
        m_options->setValue(TIFF_MODE_LDR_KEY, m_ui->comboBox->currentIndex());
    }
}

int TiffModeDialog::getTiffWriterMode() {
    if (m_hdrMode) {
        return m_ui->comboBox->currentIndex() + 2;
    } else {
        return m_ui->comboBox->currentIndex();
    }
}
