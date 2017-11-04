/**
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2009 Franco Comida
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

#include "UI/ImageQualityDialog.h"
#include "UI/ui_ImageQualityDialog.h"

#include <QBuffer>
#include <QByteArray>
#include <QDebug>
#include <QImage>
#include <QString>

#include <Libpfs/io/jpegwriter.h>
#include <Libpfs/io/pngwriter.h>

namespace {
const static QString IMAGE_QUALITY_KEY =
    QStringLiteral("imagequalitydialog/quality");
const static int IMAGE_QUALITY_DEFAULT = 98;
}

ImageQualityDialog::ImageQualityDialog(const pfs::Frame *frame,
                                       const QString &fmt, int defaultValue,
                                       QWidget *parent)
    : QDialog(parent),
      m_frame(frame),
      m_format(fmt),
      m_ui(new Ui::ImgQualityDialog),
      m_options(new LuminanceOptions()) {
    m_ui->setupUi(this);
    if (defaultValue >= 0) {
        m_ui->spinBox->setValue(defaultValue);
    } else if (frame) {
        m_ui->spinBox->setValue(
            m_options->value(IMAGE_QUALITY_KEY, IMAGE_QUALITY_DEFAULT).toInt());
    } else {
        m_ui->spinBox->setValue(100);
    }

    if (frame) {
        connect(m_ui->spinBox, SIGNAL(valueChanged(int)), this,
                SLOT(reset(int)));
        connect(m_ui->horizontalSlider, &QAbstractSlider::valueChanged, this,
                &ImageQualityDialog::reset);
    } else {
        m_ui->fileSizePanel->setVisible(false);
    }

#ifdef Q_OS_MAC
    this->setWindowModality(
        Qt::WindowModal);  // In OS X, the QMessageBox is modal to the window
#endif
}

ImageQualityDialog::~ImageQualityDialog() {
    if (m_frame) {
        m_options->setValue(IMAGE_QUALITY_KEY, getQuality());
    }
}

int ImageQualityDialog::getQuality(void) const {
    return m_ui->spinBox->value();
}

void ImageQualityDialog::on_getSizeButton_clicked() {
    pfs::Params params("quality", (size_t)getQuality());

    setCursor(QCursor(Qt::WaitCursor));
    int size = 0;
    if (m_format.startsWith(QLatin1String("jp"))) {
        pfs::io::JpegWriter writer;
        writer.write(*m_frame, params);
        size = writer.getFileSize();
    } else if (m_format.startsWith(QLatin1String("png"))) {
        pfs::io::PngWriter writer;
        writer.write(*m_frame, params);
        size = writer.getFileSize();
    } else {
        return;
    }
    //    else
    //    {
    //        QByteArray ba;
    //        QBuffer buffer(&ba);
    //        buffer.open(QIODevice::WriteOnly);
    //        m_image->save(&buffer, m_format.toLatin1().constData(), quality);
    //        size = ba.size();
    //    }

    QLocale def;
    QString s = def.toString(size);
    // the JPG on disk differs by 374 more bytes
    // label_filesize->setText(QString::number( ba.size() ));
    // the JPG on disk differs by 374 more bytes
    m_ui->label_filesize->setText(s);
    setCursor(QCursor(Qt::ArrowCursor));
}

void ImageQualityDialog::reset(int) {
    m_ui->label_filesize->setText(tr("Unknown"));
}
