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

#include "ImageQualityDialog.h"
#include "ui_ImageQualityDialog.h"

#include "Fileformat/jpegwriter.h"

ImageQualityDialog::~ImageQualityDialog() {}

ImageQualityDialog::ImageQualityDialog(const QImage *img, QString fmt, QWidget *parent):
    QDialog(parent),
    image(img),
    m_Ui(new Ui::ImgQualityDialog)
{
    m_Ui->setupUi(this);
    format = fmt;
    connect(m_Ui->spinBox, SIGNAL(valueChanged(int)), this, SLOT(reset(int)));
    connect(m_Ui->horizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(reset(int)));
}

int ImageQualityDialog::getQuality(void)
{
    return m_Ui->spinBox->value();
}

void ImageQualityDialog::on_getSizeButton_clicked()
{
    setCursor(QCursor(Qt::WaitCursor));
    int quality = m_Ui->spinBox->value();
	char *outbuf;
	int outlen;
	int size;
    //QByteArray ba;
    //QBuffer buffer(&ba);
    //buffer.open(QIODevice::WriteOnly);
    //image->save(&buffer, (const char *) format.toLatin1(), quality);
	JpegWriter writer(image, quality);
	writer.writeQImageToJpeg();
	outbuf = writer.getBuffer();
	outlen = writer.getBufferLenght();
	for (size = outlen; size > 0; size--)
		if (*(outbuf + size) != 0)
			break;
    QLocale def;
    QString s = def.toString( size );
    //label_filesize->setText(QString::number( ba.size() )); //the JPG on disk differs by 374 more bytes
    m_Ui->label_filesize->setText( s ); //the JPG on disk differs by 374 more bytes
    setCursor(QCursor(Qt::ArrowCursor));
	delete outbuf;
}

void ImageQualityDialog::reset(int)
{
    m_Ui->label_filesize->setText(tr("Unknown"));
}
