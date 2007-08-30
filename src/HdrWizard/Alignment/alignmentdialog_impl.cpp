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

#include <QFileInfo>
#include <cassert>
#include <QWhatsThis>
#include "alignmentdialog_impl.h"
#include "../../ToneMappingDialog/gamma_and_levels.h"


////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
AlignmentDialog::AlignmentDialog(QWidget *parent, QList<QImage*>&originalldrlist, QList<bool> &ldr_tiff_input, QStringList fileStringList, Qt::ToolButtonStyle style) : QDialog(parent), additional_shift_value(0), original_ldrlist(originalldrlist), ldr_tiff_input(ldr_tiff_input)
{
	setupUi(this);

	assert(original_ldrlist.size()==fileStringList.size());
	QVBoxLayout *qvl=new QVBoxLayout;
	qvl->setMargin(0);
	qvl->setSpacing(0);
	
	scrollArea = new QScrollArea(previewImageFrame);
	previewWidget = new PreviewWidget(scrollArea,original_ldrlist[1],original_ldrlist[0]);
	previewWidget->adjustSize();
	previewWidget->update();

	panIconWidget=NULL;
	cornerButton=new QToolButton(this);
	cornerButton->setToolTip("Pan the image to a region");
	cornerButton->setIcon(QIcon(":/new/prefix1/images/move.png"));
#if QT_VERSION >= 0x040200
	scrollArea->setCornerWidget(cornerButton);
#else
	QHBoxLayout *p=(QHBoxLayout*)(visualizationGroupBox->layout()->itemAt(1));
	connect(fitButton,SIGNAL(toggled(bool)),cornerButton,SLOT(setDisabled(bool)));
	cornerButton->setIconSize(QSize(22,22));
	cornerButton->setText("Pan");
	cornerButton->setToolButtonStyle(style);
	p->insertWidget(5,cornerButton);
#endif
	connect(cornerButton, SIGNAL(pressed()), this, SLOT(slotCornerButtonPressed()));

	scrollArea->setFocusPolicy(Qt::NoFocus);
	scrollArea->setBackgroundRole(QPalette::Window);
	scrollArea->setWidget(previewWidget);

	qvl->addWidget(scrollArea);
	previewImageFrame->setLayout(qvl);

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
	cropButton->setToolButtonStyle(style);
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
	connect(fitButton,SIGNAL(toggled(bool)),this,SLOT(fitPreview(bool)));
	connect(origSizeButton,SIGNAL(clicked()),this,SLOT(origSize()));
	connect(zoomOutButton,SIGNAL(clicked()),this,SLOT(zoomOut()));
	connect(zoomInButton,SIGNAL(clicked()),this,SLOT(zoomIn()));
	connect(cropButton,SIGNAL(clicked()),this,SLOT(crop_stack()));
	connect(blendModeCB,SIGNAL(currentIndexChanged(int)),previewWidget,SLOT(requestedBlendMode(int)));

	connect(Next_Finishbutton,SIGNAL(clicked()),this,SLOT(nextClicked()));
	connect(previewWidget,SIGNAL(validCropArea(bool)),cropButton,SLOT(setEnabled(bool)));
	
	QStringList::ConstIterator it = fileStringList.begin();
	while( it != fileStringList.end() ) {
		HV_offsets.append(qMakePair(0,0));
		++it;
	}
	
	histogram=new HistogramLDR(this);
	histogram->setData(*original_ldrlist[1]);
	histogram->adjustSize();
	((QHBoxLayout*)(visualizationGroupBox->layout()))->insertWidget(0,histogram);
}

void AlignmentDialog::slotCornerButtonPressed() {
	panIconWidget=new PanIconWidget;
	panIconWidget->setImage(previewWidget->getPreviewImage());
	float zf=previewWidget->getScaleFactor();
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
	panIconWidget->popup(QPoint(g.x() - panIconWidget->width(), 
					g.y() - panIconWidget->height()));
	
	panIconWidget->setCursorToLocalRegionSelectionCenter();
}

void AlignmentDialog::slotPanIconSelectionMoved(QRect gotopos, bool mousereleased) {
	if (mousereleased) {
		scrollArea->horizontalScrollBar()->setValue((int)(gotopos.x()*previewWidget->getScaleFactor()));
		scrollArea->verticalScrollBar()->setValue((int)(gotopos.y()*previewWidget->getScaleFactor()));
		panIconWidget->close();
		slotPanIconHidden();
	}
}

void AlignmentDialog::slotPanIconHidden()
{
    cornerButton->blockSignals(true);
    cornerButton->animateClick();
    cornerButton->blockSignals(false);
}

AlignmentDialog::~AlignmentDialog() {
	delete previewWidget;
	delete histogram;
}

void AlignmentDialog::keyPressEvent(QKeyEvent *event) {
	int key=event->key();
	qDebug("AlignmentDialog::keyPressEvent");
	Qt::KeyboardModifiers mods=event->modifiers();
	if ((mods & Qt::ShiftModifier)!=0 && (mods & Qt::ControlModifier)!=0)
		additional_shift_value=99;
	else if (mods & Qt::ControlModifier)
		additional_shift_value=49;
	else if (mods & Qt::ShiftModifier)
		additional_shift_value=9;

	if (key==Qt::Key_Up)
		upClicked();
	else if (key==Qt::Key_Down)
		downClicked();
	else if (key==Qt::Key_Right)
		rightClicked();
	else if (key==Qt::Key_Left)
		leftClicked();
	if (key==Qt::Key_Escape)
		reject();
}

void AlignmentDialog::keyReleaseEvent ( QKeyEvent * event ) {
	additional_shift_value=0;
	event->ignore();
}

void AlignmentDialog::crop_stack() {

	if (fitButton->isChecked()) {
// 		fitButton->blockSignals(true);
		fitButton->setChecked(false);
// 		fitButton->blockSignals(false);
	}
	//zoom the image to 1:1, so that the crop area is in a one-to-one relationship with the pixel coordinates.
	origSize();

	QRect ca=previewWidget->getCropArea();
	if(ca.width()<=0|| ca.height()<=0)
		return;

	qDebug("cropping left,top=(%d,%d) %dx%d",ca.left(),ca.top(),ca.width(),ca.height());
	//crop all the images
	//If I do this "properly" (i.e. allocating only one QImage) I end up having odd
	//gcc runtime errors (like a double free)...
	//It seems like it allocates the memory that was just freed, anyone knows how to get around this?
	int origlistsize=original_ldrlist.size();
	for (int image_idx=0; image_idx<origlistsize; image_idx++) {
// 		QImage *newimage=new QImage(ca.size(), QImage::Format_ARGB32);
// 		for(int row=0; row<newimage->height(); row++) {
// 			QRgb *inp = (QRgb*)original_ldrlist.at(image_idx)->scanLine(row+ca.top());
// 			QRgb *outp = (QRgb*)newimage->scanLine(row);
// 			for(int col=0; col<newimage->width(); col++) {
// 				outp[col] = *(inp+col+ca.left());
// 			}
// 		}
		QImage *newimage=new QImage(original_ldrlist.at(image_idx)->copy(ca));
		original_ldrlist.append(newimage);
	}
	for (int image_idx=0; image_idx<origlistsize; image_idx++) {
		if (ldr_tiff_input[0]) {
			qDebug("::crop_stack: deleting tiff payload");
			delete [] original_ldrlist[0]->bits();
		}
		delete original_ldrlist.takeAt(0);
		ldr_tiff_input.removeAt(0);
		ldr_tiff_input.append(false);
	}
	//the display widget has to be recreated with the new cropped data.
	delete previewWidget;
	//new display widget uses current indices.
	previewWidget=new PreviewWidget(scrollArea, original_ldrlist[movableListWidget->currentRow()], original_ldrlist[referenceListWidget->currentRow()]);
	connect(blendModeCB,SIGNAL(currentIndexChanged(int)),previewWidget,SLOT(requestedBlendMode(int)));
	connect(previewWidget,SIGNAL(validCropArea(bool)),cropButton,SLOT(setEnabled(bool)));
	previousPreviewWidgetSize=original_ldrlist[0]->size();
	//inform scrollArea of the change
	scrollArea->setWidget(previewWidget);
	//and start it up
	previewWidget->update();
	previewWidget->hideRubberBand();
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



void AlignmentDialog::updateMovable(int newidx) {
	//inform display_widget of the change
	previewWidget->setMovable(original_ldrlist[newidx], HV_offsets[newidx].first, HV_offsets[newidx].second);
	//prevent a change in the spinboxes to start a useless calculation
	horizShiftSB->blockSignals(true);
	horizShiftSB->setValue(HV_offsets[newidx].first);
	horizShiftSB->blockSignals(false);
	vertShiftSB->blockSignals(true);
	vertShiftSB->setValue(HV_offsets[newidx].second);
	vertShiftSB->blockSignals(false);
	previewWidget->update();
	histogram->setData(*original_ldrlist[newidx]);
	histogram->update();
}

void AlignmentDialog::updatePivot(int newidx) {
	previewWidget->setPivot(original_ldrlist[newidx],HV_offsets[newidx].first, HV_offsets[newidx].second);
	previewWidget->update();
}

void AlignmentDialog::upClicked() {
	vertShiftSB->setValue(vertShiftSB->value()-1-additional_shift_value);
}
void AlignmentDialog::downClicked() {
	vertShiftSB->setValue(vertShiftSB->value()+1+additional_shift_value);
}
void AlignmentDialog::rightClicked() {
	horizShiftSB->setValue(horizShiftSB->value()+1+additional_shift_value);
}
void AlignmentDialog::leftClicked() {
	horizShiftSB->setValue(horizShiftSB->value()-1-additional_shift_value);
}

void AlignmentDialog::vertShiftChanged(int v) {
	HV_offsets[movableListWidget->currentRow()].second=v;
	previewWidget->updateVertShiftMovable(v);
	previewWidget->update();
}
void AlignmentDialog::horizShiftChanged(int v) {
	HV_offsets[movableListWidget->currentRow()].first=v;
	previewWidget->updateHorizShiftMovable(v);
	previewWidget->update();
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
	//prevent a change in the spinboxes to start a useless calculation
	horizShiftSB->blockSignals(true);
	vertShiftSB->blockSignals(true);
	resetCurrent(); //graphical update
	horizShiftSB->blockSignals(false);
	vertShiftSB->blockSignals(false);
	previewWidget->updateHorizShiftMovable(0);
	previewWidget->updateVertShiftMovable(0);
	previewWidget->updateHorizShiftPivot(0);
	previewWidget->updateVertShiftPivot(0);
	previewWidget->update();
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
	prevRight();
	prevLeft();
}

void AlignmentDialog::nextBoth() {
	nextRight();
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
	previewWidget->resize(previewWidget->size()*1.25f);
	zoomOutButton->setEnabled(true);
	zoomInButton->setEnabled(previewWidget->getScaleFactor() < 3.0);
}
void AlignmentDialog::zoomOut() {
	previewWidget->resize(previewWidget->size()*0.8f);
	zoomInButton->setEnabled(true);
	zoomOutButton->setEnabled(previewWidget->getScaleFactor() > 0.222);
}
void AlignmentDialog::fitPreview(bool checked) {
// 	qDebug("fitPreview");
	zoomInButton->setEnabled(!checked);
	zoomOutButton->setEnabled(!checked);
	origSizeButton->setEnabled(!checked);
	if (checked) {
		previousPreviewWidgetSize=previewWidget->size();
		QSize fillWinSize=original_ldrlist[0]->size();
		fillWinSize.scale(scrollArea->maximumViewportSize(),Qt::KeepAspectRatio);
		previewWidget->resize(fillWinSize);
	} else {
		previewWidget->resize(previousPreviewWidgetSize);
	}
// 	qDebug("end fitPreview");
}
void AlignmentDialog::origSize() {
	zoomInButton->setEnabled(true);
	zoomOutButton->setEnabled(true);
	previewWidget->resize(original_ldrlist[0]->size());
}
