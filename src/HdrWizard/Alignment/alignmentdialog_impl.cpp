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

#include <QPainter>
#include <QFileInfo>
#include <cassert>
#include <QWhatsThis>
#include "alignmentdialog_impl.h"
#include "../../ToneMappingDialog/gamma_and_levels.h"

AlignmentDialog::AlignmentDialog(QWidget *parent, QList<QImage*>&originalldrlist, QList<bool> &ldr_tiff_input, QStringList fileStringList, Qt::ToolButtonStyle style) : QDialog(parent), original_ldrlist(originalldrlist), ldr_tiff_input(ldr_tiff_input), dont_change_shift(false), dont_recompute(false) 
{
	setupUi(this);
	assert(original_ldrlist.size()==fileStringList.size());
	QVBoxLayout *qvl=new QVBoxLayout;
	qvl->setMargin(0);
	qvl->setSpacing(0);
	imageLabel = new QLabel;
	diffImage = new QImage(original_ldrlist[0]->size(),QImage::Format_ARGB32);

	scrollArea = new SmartScrollArea(diffImageFrame,imageLabel);
	qvl->addWidget(scrollArea);

	pivotImage=original_ldrlist[0]; //pivot=first image
	movableImage=original_ldrlist[1]; //movable=second image
	scrollArea->setBackgroundRole(QPalette::Window);

	diffImageFrame->setLayout(qvl);
	scrollArea->adjustSize();

	foreach(QString s,fileStringList) {
		movableListWidget->addItem(QFileInfo(s).fileName());
		referenceListWidget->addItem(QFileInfo(s).fileName());
	}
	movableListWidget->setCurrentRow(1);
	referenceListWidget->setCurrentRow(0);
	
	fitButton->setToolButtonStyle(style);
	origSizeButton->setToolButtonStyle(style);
	zoomOutButton->setToolButtonStyle(style);
	zoomInButton->setToolButtonStyle(style);
	whatsThisButton->setToolButtonStyle(style);
	connect(upToolButton,SIGNAL(clicked()),this,SLOT(upClicked()));
	connect(rightToolButton,SIGNAL(clicked()),this,SLOT(rightClicked()));
	connect(downToolButton,SIGNAL(clicked()),this,SLOT(downClicked()));
	connect(leftToolButton,SIGNAL(clicked()),this,SLOT(leftClicked()));
	connect(horizShiftSB,SIGNAL(valueChanged(int)),this,SLOT(horizShiftChanged(int)));
	connect(vertShiftSB,SIGNAL(valueChanged(int)),this,SLOT(vertShiftChanged(int)));
	connect(resetButton,SIGNAL(clicked()),this,SLOT(resetCurrent()));
	connect(resetAllButton,SIGNAL(clicked()),this,SLOT(resetAll()));

	connect(movableListWidget,SIGNAL(currentRowChanged(int)),this,SLOT(updateMovable(int)));
	connect(referenceListWidget,SIGNAL(currentRowChanged(int)),this,SLOT(updatePivot(int)));
	connect(prevLeftButton,SIGNAL(clicked()),this,SLOT(prevLeft()));
	connect(nextLeftButton,SIGNAL(clicked()),this,SLOT(nextLeft()));
	connect(prevBothButton,SIGNAL(clicked()),this,SLOT(prevBoth()));
	connect(nextBothButton,SIGNAL(clicked()),this,SLOT(nextBoth()));
	connect(prevRightButton,SIGNAL(clicked()),this,SLOT(prevRight()));
	connect(nextRightButton,SIGNAL(clicked()),this,SLOT(nextRight()));
	
	connect(whatsThisButton,SIGNAL(clicked()),this,SLOT(enterWhatsThis()));
	connect(fitButton,SIGNAL(toggled(bool)),this,SLOT(fitDiff(bool)));
	connect(origSizeButton,SIGNAL(clicked()),this,SLOT(origSize()));
	connect(zoomOutButton,SIGNAL(clicked()),this,SLOT(zoomOut()));
	connect(zoomInButton,SIGNAL(clicked()),this,SLOT(zoomIn()));

	connect(Next_Finishbutton,SIGNAL(clicked()),this,SLOT(nextClicked()));
	
	QStringList::ConstIterator it = fileStringList.begin();
	while( it != fileStringList.end() ) {
		HV_offsets.append(qMakePair(0,0));
		++it;
	}
	recomputeDiffImage();
	imageLabel->adjustSize();
	
	histogram=new HistogramLDR(this);
	QVBoxLayout *qvl2=new QVBoxLayout;
	histogramGroupBox->setLayout(qvl2);
	histogram->adjustSize();
	qvl2->addWidget(histogram);
	this->showMaximized();
}

AlignmentDialog::~AlignmentDialog() {
	delete imageLabel;
	delete scrollArea;
	delete diffImage;
	delete histogram;
}

void AlignmentDialog::nextClicked() {
	int originalsize=original_ldrlist.size();
	//shift the images
	for (int i=0; i<originalsize; i++) {
		if (HV_offsets[i].first==HV_offsets[i].second && HV_offsets[i].first==0)
			continue;
		qDebug("shifting image %d of (%d,%d)",i, HV_offsets[i].first, HV_offsets[i].second);
		QImage *shifted=shiftQImage(original_ldrlist[i], HV_offsets[i].first, HV_offsets[i].second);
		if (ldr_tiff_input[i]) {
			qDebug("deleting tiff payload");
			delete [] original_ldrlist[i]->bits();
		}
		delete original_ldrlist[i];
		original_ldrlist.removeAt(i);
		original_ldrlist.insert(i,shifted);
		ldr_tiff_input.removeAt(i);
		ldr_tiff_input.insert(i,false);
	}
	emit accept();
}

void AlignmentDialog::recomputeDiffImage() {
	qDebug("AlignmentDialog::recomputeDiffImage()");
	QApplication::setOverrideCursor( QCursor(Qt::BusyCursor) );
	QRgb outofbounds=qRgba(0,0,0,255);
	int mx=HV_offsets[movableListWidget->currentRow()].first;
	int my=HV_offsets[movableListWidget->currentRow()].second;
	int px=HV_offsets[referenceListWidget->currentRow()].first;
	int py=HV_offsets[referenceListWidget->currentRow()].second;
	int H=diffImage->height();
	int W=diffImage->width();
	QRgb *MovVal=NULL;
	QRgb *PivVal=NULL;
	QRgb* mov_line=NULL;
	QRgb* piv_line=NULL;
	
	for(int i = 0; i < H; i++) {
		QRgb* out = (QRgb*)diffImage->scanLine(i);
		//se con offset verticale vado in linea <0
		//(i-MPy)<0
		//se con offset verticale vado >= height()
		//(i-MPy)>=height()

		//if within bounds with vertical offset
		if ( !( (i-my)<0 || (i-my)>=H) )
			mov_line = (QRgb*)movableImage->scanLine(i-my);
		else
			mov_line = NULL;
			
		if ( !( (i-py)<0 || (i-py)>=H) )
			piv_line = (QRgb*)pivotImage->scanLine(i-py);
		else
			piv_line = NULL;
			

		for(int j = 0; j < W; j++) {
			//okkio a offset horizontal a colonna < 0
			//(j-MPx)<0
			//okkio a offset horizontal a colonna >= width()
			//(j-MPx)>=width()

			if (mov_line==NULL || (j-mx)<0 || (j-mx)>W)
				MovVal=&outofbounds;
			else
				MovVal=&mov_line[j-mx];

			if (piv_line==NULL || (j-px)<0 || (j-px)>W)
				PivVal=&outofbounds;
			else
				PivVal=&piv_line[j-px];

			if (pivotImage==movableImage)
				out[j]=*MovVal;
			else
				out[j]=computeDiffRgba(MovVal,PivVal);
		}
	}
	imageLabel->setPixmap(QPixmap::fromImage(*diffImage));
	imageLabel->update();
	QApplication::restoreOverrideCursor();
}

QRgb AlignmentDialog::computeDiffRgba(QRgb *Mrgba, QRgb *Prgba) {
	int ro,go,bo;
	int Mred=  qRed(*Mrgba);
	int Mgreen=qGreen(*Mrgba);
	int Mblue= qBlue(*Mrgba);
	int Malphaweight=(int)(qAlpha(*Mrgba)/255.0f);
	int Pred  =qRed(*Prgba);
	int Pgreen=qGreen(*Prgba);
	int Pblue =qBlue(*Prgba);
	int Palphaweight=(int)(qAlpha(*Prgba)/255.0f);

	//when both images have alpha==0 we return an opaque black
	if (Malphaweight==0 && Palphaweight==0)
		return qRgba(0,0,0,255);
	else {
		//blend samples using alphas as weights
		ro=qAbs(Pred*Palphaweight - Mred*Malphaweight);
		go=qAbs(Pgreen*Palphaweight - Mgreen*Malphaweight);
		bo=qAbs(Pblue*Palphaweight - Mblue*Malphaweight);
		//the output image still has alpha=255 (opaque)
		return qRgba(ro,go,bo,255);
	}
}

void AlignmentDialog::updateMovable(int newidx) {
	movableImage=original_ldrlist[newidx];
	dont_change_shift=true;
	horizShiftSB->setValue(HV_offsets[newidx].first);
	vertShiftSB->setValue(HV_offsets[newidx].second);
	dont_change_shift=false;
	if (!dont_recompute) {
		recomputeDiffImage();
		if (referenceListWidget->currentRow()==newidx) {
			histogram->setData(*original_ldrlist[newidx]);
			histogram->update();
		}
		else
			histogram->setData(QImage());
	}
}

void AlignmentDialog::updatePivot(int newidx) {
	pivotImage=original_ldrlist[newidx];
	if (!dont_recompute)
		recomputeDiffImage();
}

void AlignmentDialog::upClicked() {
	vertShiftSB->setValue(vertShiftSB->value()-1);
}
void AlignmentDialog::downClicked() {
	vertShiftSB->setValue(vertShiftSB->value()+1);
}
void AlignmentDialog::rightClicked() {
	horizShiftSB->setValue(horizShiftSB->value()+1);
}
void AlignmentDialog::leftClicked() {
	horizShiftSB->setValue(horizShiftSB->value()-1);
}

void AlignmentDialog::vertShiftChanged(int v) {
	if (dont_change_shift)
		return;
	HV_offsets[movableListWidget->currentRow()].second=v;
	recomputeDiffImage();
}
void AlignmentDialog::horizShiftChanged(int v) {
	if (dont_change_shift)
		return;
	HV_offsets[movableListWidget->currentRow()].first=v;
	recomputeDiffImage();
}

void AlignmentDialog::resetCurrent() {
	horizShiftSB->setValue(0);
	vertShiftSB->setValue(0);
}

void AlignmentDialog::resetAll() {
	for (int i = 0; i < HV_offsets.size(); ++i) {
		HV_offsets[i].first=0;
		HV_offsets[i].second=0;
	}
	dont_change_shift=true;
	resetCurrent(); //graphical update
	dont_change_shift=false;
	recomputeDiffImage();
}

void AlignmentDialog::prevLeft() {
	int prev=(movableListWidget->currentRow()==0) ? movableListWidget->count()-1 : movableListWidget->currentRow()-1;
	movableListWidget->setCurrentRow(prev);
}

void AlignmentDialog::nextLeft() {
	int next=(movableListWidget->currentRow()==movableListWidget->count()-1) ? 0 : movableListWidget->currentRow()+1;
	movableListWidget->setCurrentRow(next);
}

void AlignmentDialog::prevBoth() {
	dont_recompute=true;
	prevRight();
	dont_recompute=false;
	prevLeft();
}

void AlignmentDialog::nextBoth() {
	dont_recompute=true;
	nextRight();
	dont_recompute=false;
	nextLeft();
}

void AlignmentDialog::prevRight() {
	int prev=(referenceListWidget->currentRow()==0) ? referenceListWidget->count()-1 : referenceListWidget->currentRow()-1;
	referenceListWidget->setCurrentRow(prev);
}

void AlignmentDialog::nextRight() {
	int next=(referenceListWidget->currentRow()==referenceListWidget->count()-1) ? 0 : referenceListWidget->currentRow()+1;
	referenceListWidget->setCurrentRow(next);
}

void AlignmentDialog::enterWhatsThis() {
	QWhatsThis::enterWhatsThisMode();
}

void AlignmentDialog::zoomIn() {
	scrollArea->zoomIn();
	zoomOutButton->setEnabled(true);
	zoomInButton->setEnabled(scrollArea->getScaleFactor() < 3.0);
}
void AlignmentDialog::zoomOut() {
	scrollArea->zoomOut();
	zoomInButton->setEnabled(true);
	zoomOutButton->setEnabled(scrollArea->getScaleFactor() > 0.222);
}
void AlignmentDialog::fitDiff(bool checked) {
	scrollArea->fitToWindow(checked);
	zoomInButton->setEnabled(!checked);
	zoomOutButton->setEnabled(!checked);
	origSizeButton->setEnabled(!checked);
}
void AlignmentDialog::origSize() {
	scrollArea->normalSize();
	zoomInButton->setEnabled(true);
	zoomOutButton->setEnabled(true);
}
