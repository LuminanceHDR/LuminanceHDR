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
#include <QVBoxLayout>
#include "ldrviewer.h"
#include "../Common/gamma_and_levels.h"
#include "../Common/config.h"

LdrViewer::LdrViewer(QWidget *parent, const QImage& o, tonemapping_options *opts) : QWidget(parent),origimage(o) {
	currentimage=&origimage;
	setAttribute(Qt::WA_DeleteOnClose);

	QVBoxLayout *VBL_L = new QVBoxLayout(this);
	VBL_L->setSpacing(0);
	VBL_L->setMargin(0);

	imageLabel = new SelectableLabel;
	imageLabel->setPixmap(QPixmap::fromImage(origimage));
	scrollArea = new SmartScrollArea(this,imageLabel);
	VBL_L->addWidget(scrollArea);

	parseOptions(opts);
	setWindowTitle(caption);
	setToolTip(caption);
	cornerButton=new QToolButton(this);
	cornerButton->setToolTip("Pan the image to a region");
	cornerButton->setIcon(QIcon(":/new/prefix1/images/move.png"));
	scrollArea->setCornerWidget(cornerButton);
	connect(cornerButton, SIGNAL(pressed()), this, SLOT(slotCornerButtonPressed()));
}

LdrViewer::~LdrViewer() {
	delete imageLabel;
	delete cornerButton;
	delete scrollArea;
	delete [] origimage.bits();
}


void LdrViewer::slotCornerButtonPressed() {
	panIconWidget=new PanIconWidget;
	panIconWidget->setImage(currentimage);
	float zf=scrollArea->getScaleFactor();
	float leftviewpos=(float)(scrollArea->horizontalScrollBar()->value());
	float topviewpos=(float)(scrollArea->verticalScrollBar()->value());
	float wps_w=(float)(scrollArea->maximumViewportSize().width());
	float wps_h=(float)(scrollArea->maximumViewportSize().height());
	QRect r((int)(leftviewpos/zf), (int)(topviewpos/zf), (int)(wps_w/zf), (int)(wps_h/zf));
	panIconWidget->setRegionSelection(r);
	panIconWidget->setMouseFocus();
	connect(panIconWidget, SIGNAL(signalSelectionMoved(QRect, bool)), this, SLOT(slotPanIconSelectionMoved(QRect, bool)));
	QPoint g = scrollArea->mapToGlobal(scrollArea->viewport()->pos());
	g.setX(g.x()+ scrollArea->viewport()->size().width());
	g.setY(g.y()+ scrollArea->viewport()->size().height());
	panIconWidget->popup(QPoint(g.x() - panIconWidget->width()/2,
					g.y() - panIconWidget->height()/2));

	panIconWidget->setCursorToLocalRegionSelectionCenter();
}

void LdrViewer::slotPanIconSelectionMoved(QRect gotopos, bool mousereleased) {
	if (mousereleased) {
		scrollArea->horizontalScrollBar()->setValue((int)(gotopos.x()*scrollArea->getScaleFactor()));
		scrollArea->verticalScrollBar()->setValue((int)(gotopos.y()*scrollArea->getScaleFactor()));
		panIconWidget->close();
		slotPanIconHidden();
	}
}

void LdrViewer::slotPanIconHidden()
{
    cornerButton->blockSignals(true);
    cornerButton->animateClick();
    cornerButton->blockSignals(false);
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
void LdrViewer::fitToWindow(bool checked) {
	scrollArea->fitToWindow(checked);
}
bool LdrViewer::getFittingWin() {
	return scrollArea->isFitting();
}
const QImage* LdrViewer::getQImage() {
	return currentimage;
}
QString LdrViewer::getExifComment() {
	return exif_comment;
}

void LdrViewer::LevelsRequested(bool a) {
	assert(a); //a is always true
	//copy original data
	previewimage=currentimage->copy();
	GammaAndLevels *levels=new GammaAndLevels(0,origimage);
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
	for (int x=0; x<origimage.width(); x++) {
		for (int y=0; y<origimage.height(); y++) {
			QRgb rgb=origimage.pixel(x,y);
			QRgb withgamma = qRgb(LUT[qRed(rgb)],LUT[qGreen(rgb)],LUT[qBlue(rgb)]);
			previewimage.setPixel(x,y,withgamma);
		}
	}
	imageLabel->setPixmap(QPixmap::fromImage(previewimage));
	currentimage=&previewimage;
}

void LdrViewer::restoreoriginal() {
	imageLabel->setPixmap(QPixmap::fromImage(origimage));
	currentimage=&origimage;
}
