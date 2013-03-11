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

#include <QImage>
#include <QString>
#include <QByteArray>
#include <QBuffer>
#include <QDebug>

#include "ImageQualityDialog.h"
#include "ui_ImageQualityDialog.h"

#include <Fileformat/jpegwriter.h>
#include <Fileformat/pngwriter.h>

ImageQualityDialog::~ImageQualityDialog() {}

ImageQualityDialog::ImageQualityDialog(const pfs::Frame* frame,
                                       const QString& fmt, QWidget *parent)
    : QDialog(parent)
    , m_frame(frame)
    , m_format(fmt)
    , m_ui(new Ui::ImgQualityDialog)
{
    m_ui->setupUi(this);

    connect(m_ui->spinBox, SIGNAL(valueChanged(int)),
            this, SLOT(reset(int)));
    connect(m_ui->horizontalSlider, SIGNAL(valueChanged(int)),
            this, SLOT(reset(int)));
}

int ImageQualityDialog::getQuality(void)
{
    return m_ui->spinBox->value();
}

void ImageQualityDialog::on_getSizeButton_clicked()
{
    pfs::Params params( "quality", (size_t)getQuality() );

    setCursor(QCursor(Qt::WaitCursor));
    int size = 0;
    if (m_format.startsWith("jp"))
    {
        JpegWriter writer;
        writer.write(*m_frame, params);
        size = writer.getFileSize();
	}
    else if (m_format.startsWith("png"))
    {
        PngWriter writer;
        writer.write(*m_frame, params);
        size = writer.getFileSize();
	}
    else { return; }
//    else
//    {
//    	QByteArray ba;
//    	QBuffer buffer(&ba);
//	    buffer.open(QIODevice::WriteOnly);
//        m_image->save(&buffer, m_format.toLatin1().constData(), quality);
//		size = ba.size();
//	}

    QLocale def;
    QString s = def.toString(size);
    // the JPG on disk differs by 374 more bytes
    // label_filesize->setText(QString::number( ba.size() ));
    // the JPG on disk differs by 374 more bytes
    m_ui->label_filesize->setText(s);
    setCursor(QCursor(Qt::ArrowCursor));
}

void ImageQualityDialog::reset(int)
{
    m_ui->label_filesize->setText(tr("Unknown"));
}
