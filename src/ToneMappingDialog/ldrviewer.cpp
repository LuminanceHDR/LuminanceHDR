/**
 * This file is a part of Qtpfsgui package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2006,2007 Giuseppe Rota
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
 *
 */

#include "ldrviewer.h"
#include "../Common/gamma_and_levels.h"
#include "../Common/config.h"

LdrViewer::LdrViewer(const QImage &i, QWidget *parent, bool ns, bool ncf, tonemapping_options *opts) : GenericViewer(parent, ns, ncf), origimage(i) {
	image = origimage;
	setAttribute(Qt::WA_DeleteOnClose);

	QLabel *informativeLabel = new QLabel( "LDR Image", toolBar );
	toolBar->addWidget(informativeLabel);
	toolBar->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
	toolBar->resize(toolBar->size().width(), 30); // same as HdrViewer

	imageLabel.setPixmap(QPixmap::fromImage(origimage));

	parseOptions(opts);
	setWindowTitle(caption);
	setToolTip(caption);
}

LdrViewer::~LdrViewer() {
	//TODO:  shared copy issue?
	delete [] origimage.bits();
}

void LdrViewer::parseOptions(tonemapping_options *opts) {
	TMOptionsOperations tmopts(opts);
	postfix=tmopts.getPostfix();
	caption=tmopts.getCaption();
	exif_comment=tmopts.getExifComment();
}

QString LdrViewer::getFilenamePostFix() {
	return postfix;
}

const QImage LdrViewer::getQImage() {
	return image;
}

QString LdrViewer::getExifComment() {
	return exif_comment;
}

void LdrViewer::levelsRequested(bool a) {
	assert(a); //a is always true
	//copy original data
	previewimage=image.copy();
	GammaAndLevels *levels=new GammaAndLevels(0, origimage);
	levels->setAttribute(Qt::WA_DeleteOnClose);
	//when closing levels, inform the Tone Mapping dialog.
	connect(levels,SIGNAL(closing()),this,SIGNAL(levels_closed()));
	//refresh preview when a values changes
	connect(levels,SIGNAL(LUTrefreshed(unsigned char *)),this,SLOT(updatePreview(unsigned char *)));
	//restore original on "cancel"
	connect(levels,SIGNAL(rejected()),this,SLOT(restoreoriginal()));
	levels->exec();
}

void LdrViewer::updatePreview(unsigned char *LUT) {
	qDebug("LdrViewer::updatePreview\n");
	for (int x=0; x < origimage.width(); x++) {
		for (int y=0; y < origimage.height(); y++) {
			QRgb rgb = origimage.pixel(x,y);
			QRgb withgamma = qRgb(LUT[qRed(rgb)],LUT[qGreen(rgb)],LUT[qBlue(rgb)]);
			previewimage.setPixel(x,y,withgamma);
		}
	}
	imageLabel.setPixmap(QPixmap::fromImage(previewimage));
	image=previewimage;
}

void LdrViewer::restoreoriginal() {
	imageLabel.setPixmap(QPixmap::fromImage(origimage));
	image = origimage;
}

