/**
 * This file is a part of Qtpfsgui package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2006,2007,2008 Giuseppe Rota
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
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 */
#include <QImage>
#include <QString>
#include <QByteArray>
#include <QBuffer>

#include "imageQualityDialog.h"

ImageQualityDialog::~ImageQualityDialog() {}

ImageQualityDialog::ImageQualityDialog(const QImage *img, QString fmt, QWidget *parent) : QDialog(parent) {
        setupUi(this);
	image = img;
	format = fmt;
	connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(reset(int)));
	connect(horizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(reset(int)));
}

int ImageQualityDialog::getQuality(void) {
	return spinBox->value();
}

void ImageQualityDialog::on_getSizeButton_clicked() {
	setCursor(QCursor(Qt::WaitCursor));
	int quality = spinBox->value();
        QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        image->save(&buffer, (const char *) format.toLatin1(), quality);
	label_filesize->setText(QString::number( ba.size() )); //the JPG on disk differs by 374 more bytes
	setCursor(QCursor(Qt::ArrowCursor));
}

void ImageQualityDialog::reset(int) {
	label_filesize->setText(tr("Unknown"));
}
