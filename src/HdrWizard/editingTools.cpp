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
#include <QColorDialog>
#include <QFileDialog>
#include "alignmentdialog_impl.h"
#include "../../ToneMappingDialog/gamma_and_levels.h"
#include "../../Fileformat/pfstiff.h"
#include "../../config.h"

AlignmentDialog::AlignmentDialog(QWidget *parent, QList<QImage*>&originalldrlist, QList<bool> &ldr_tiff_input, QStringList fileStringList, Qt::ToolButtonStyle style) : QDialog(parent), additional_shift_value(0), original_ldrlist(originalldrlist), ldr_tiff_input(ldr_tiff_input), settings("Qtpfsgui", "Qtpfsgui")
{
	setupUi(this);

	if (original_ldrlist.at(0)->format()==QImage::Format_RGB32) {
		int origlistsize=original_ldrlist.size();
		for (int image_idx=0; image_idx<origlistsize; image_idx++) {
			QImage *newimage=new QImage(original_ldrlist.at(0)->convertToFormat(QImage::Format_ARGB32));
			original_ldrlist.append(newimage);
			if (ldr_tiff_input[0]) {
				qDebug("AlignmentDialog::crop_stack(), deleting tiff payload");
				delete [] original_ldrlist[0]->bits();
			}
			delete original_ldrlist.takeAt(0);
			ldr_tiff_input.removeAt(0);
			ldr_tiff_input.append(false);
		}
	}

	toolOptionsFrame->setVisible(false);
	maskColorButton->setVisible(false);
	QColor maskcolor=QColor(settings.value(KEY_MANUAL_AG_MASK_COLOR,0x00FF0000).toUInt());
#if QT_VERSION <= 0x040200
	QPalette modified_palette(maskColorButton->palette());
	modified_palette.setColor(QPalette::Active,QPalette::Window,maskcolor);
	maskColorButton->setPalette( modified_palette );
#else
	maskColorButton->setStyleSheet(QString("background: rgb(%1,%2,%3)").arg(maskcolor.red()).arg(maskcolor.green()).arg(maskcolor.blue()));
#endif
	assert(original_ldrlist.size()==fileStringList.size());
	QVBoxLayout *qvl=new QVBoxLayout;
	qvl->setMargin(0);
	qvl->setSpacing(0);
	
	scrollArea = new QScrollArea(previewImageFrame);
	previewWidget = new PreviewWidget(scrollArea,original_ldrlist.at(1),original_ldrlist.at(0));
	previewWidget->setBrushColor(maskcolor);
	previewWidget->adjustSize();
	previewWidget->update();

	cornerButton=new QToolButton(this);
	cornerButton->setToolTip("Pan the image to a region");
	cornerButton->setIcon(QIcon(":/new/prefix1/images/move.png"));
#if QT_VERSION >= 0x040200
	scrollArea->setCornerWidget(cornerButton);
#else
	QHBoxLayout *p=(QHBoxLayout*)(visualizationGroupBox->layout()->itemAt(0));
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
	saveImagesButton->setToolButtonStyle(style);
	antighostToolButton->setToolButtonStyle(style);
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
	connect(saveImagesButton,SIGNAL(clicked()),this,SLOT(saveImagesButtonClicked()));
	connect(blendModeCB,SIGNAL(currentIndexChanged(int)),previewWidget,SLOT(requestedBlendMode(int)));
	connect(blendModeCB,SIGNAL(currentIndexChanged(int)),this,SLOT(blendModeCBIndexChanged(int)));
	connect(antighostToolButton,SIGNAL(toggled(bool)),toolOptionsFrame,SLOT(setVisible(bool)));
	connect(antighostToolButton,SIGNAL(toggled(bool)),previewWidget,SLOT(switchAntighostingMode(bool)));
	connect(antighostToolButton,SIGNAL(toggled(bool)),this,SLOT(antighostToolButtonToggled(bool)));
	connect(agBrushSizeQSpinbox,SIGNAL(valueChanged(int)),previewWidget,SLOT(setBrushSize(int)));
	connect(agBrushStrengthQSpinbox,SIGNAL(valueChanged(int)),previewWidget,SLOT(setBrushStrength(int)));
	connect(maskColorButton,SIGNAL(clicked()),this,SLOT(maskColorButtonClicked()));

	connect(Next_Finishbutton,SIGNAL(clicked()),this,SLOT(nextClicked()));
	connect(previewWidget,SIGNAL(validCropArea(bool)),cropButton,SLOT(setEnabled(bool)));
	connect(removeMaskRadioButton,SIGNAL(toggled(bool)),previewWidget,SLOT(setBrushMode(bool)));
	
	QStringList::ConstIterator it = fileStringList.begin();
	while( it != fileStringList.end() ) {
		HV_offsets.append(qMakePair(0,0));
		++it;
	}
	
	histogram=new HistogramLDR(this);
	histogram->setData(*original_ldrlist[1]);
	histogram->adjustSize();
	((QHBoxLayout*)(visualizationGroupBox->layout()))->insertWidget(0,histogram);
	previewWidget->setFocus();
	recentPathInputLDR=settings.value(KEY_RECENT_PATH_LOAD_LDRs_FOR_HDR,QDir::currentPath()).toString();
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
#if QT_VERSION >= 0x040200
	panIconWidget->popup(QPoint(g.x() - panIconWidget->width()/2, 
					g.y() - panIconWidget->height()/2));
#else
	panIconWidget->popup(cornerButton->mapToGlobal(QPoint(cornerButton->width()/2-panIconWidget->width()/2,cornerButton->height()/2-panIconWidget->height()/2)));
#endif
	
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
	delete cornerButton;
}

void AlignmentDialog::keyPressEvent(QKeyEvent *event) {
	int key=event->key();
	Qt::KeyboardModifiers mods=event->modifiers();
	if ((mods & Qt::ShiftModifier)!=0 && (mods & Qt::ControlModifier)!=0)
		additional_shift_value=99;
	else if (mods & Qt::ControlModifier)
		additional_shift_value=49;
	else if (mods & Qt::ShiftModifier)
		additional_shift_value=9;

	if (key==Qt::Key_W||key==Qt::Key_I)
		upClicked();
	else if (key==Qt::Key_S||key==Qt::Key_K)
		downClicked();
	else if (key==Qt::Key_D||key==Qt::Key_L)
		rightClicked();
	else if (key==Qt::Key_A||key==Qt::Key_J)
		leftClicked();
	if (key==Qt::Key_Escape)
		reject();
}

void AlignmentDialog::keyReleaseEvent ( QKeyEvent * event ) {
	additional_shift_value=0;
	event->ignore();
}

void AlignmentDialog::crop_stack() {

	//zoom the image to 1:1, so that the crop area is in a one-to-one relationship with the pixel coordinates.
	origSize();

	applyShiftsToImageStack();
	resetAll();
	QRect ca=previewWidget->getCropArea();
	if(ca.width()<=0|| ca.height()<=0)
		return;

	qDebug("cropping left,top=(%d,%d) %dx%d",ca.left(),ca.top(),ca.width(),ca.height());
	//crop all the images
	int origlistsize=original_ldrlist.size();
	for (int image_idx=0; image_idx<origlistsize; image_idx++) {
		QImage *newimage=new QImage(original_ldrlist.at(0)->copy(ca));
		original_ldrlist.append(newimage);



		if (ldr_tiff_input[0]) {
			qDebug("AlignmentDialog::crop_stack(), deleting tiff payload");
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
	QColor maskcolor=QColor(settings.value(KEY_MANUAL_AG_MASK_COLOR,0x00FF0000).toUInt());
	previewWidget->setBrushColor(maskcolor);
	connect(blendModeCB,SIGNAL(currentIndexChanged(int)),previewWidget,SLOT(requestedBlendMode(int)));
	connect(previewWidget,SIGNAL(validCropArea(bool)),cropButton,SLOT(setEnabled(bool)));
	connect(antighostToolButton,SIGNAL(toggled(bool)),previewWidget,SLOT(switchAntighostingMode(bool)));
	connect(agBrushSizeQSpinbox,SIGNAL(valueChanged(int)),previewWidget,SLOT(setBrushSize(int)));
	connect(agBrushStrengthQSpinbox,SIGNAL(valueChanged(int)),previewWidget,SLOT(setBrushStrength(int)));
	connect(removeMaskRadioButton,SIGNAL(toggled(bool)),previewWidget,SLOT(setBrushMode(bool)));
	previousPreviewWidgetSize=original_ldrlist[0]->size();
	//inform scrollArea of the change
	scrollArea->setWidget(previewWidget);

	//restore fit
	if (fitButton->isChecked())
		fitPreview(true);
	//and start it up
	previewWidget->update();
	previewWidget->hideRubberBand();
}

void AlignmentDialog::applyShiftsToImageStack() {
	int originalsize=original_ldrlist.size();
	//shift the images
	for (int i=0; i<originalsize; i++) {
		if (HV_offsets[i].first==HV_offsets[i].second && HV_offsets[i].first==0)
			continue;
// 		qDebug("shifting image %d of (%d,%d)",i, HV_offsets[i].first, HV_offsets[i].second);
		QImage *shifted=shiftQImage(original_ldrlist[i], HV_offsets[i].first, HV_offsets[i].second);
		if (ldr_tiff_input[i]) {
			qDebug("AlignmentDialog::nextClicked(), deleting tiff payload");
			delete [] original_ldrlist[i]->bits();
		}
		delete original_ldrlist.takeAt(i);
		original_ldrlist.insert(i,shifted);
		ldr_tiff_input.removeAt(i);
		ldr_tiff_input.insert(i,false);
	}
}

void AlignmentDialog::nextClicked() {
	applyShiftsToImageStack();
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
	zoomInButton->setEnabled(previewWidget->getScaleFactor() < 6.0);
}
void AlignmentDialog::zoomOut() {
	previewWidget->resize(previewWidget->size()*0.8f);
	zoomInButton->setEnabled(true);
	zoomOutButton->setEnabled(previewWidget->getScaleFactor() > 0.166);
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

void AlignmentDialog::antighostToolButtonToggled(bool toggled) {
// 	if (toggled)
// 		blendModeCB->setCurrentIndex(4);
	prevBothButton->setDisabled(toggled);
	nextBothButton->setDisabled(toggled);
	label_reference_list->setDisabled(toggled);
	referenceListWidget->setDisabled(toggled);
	prevRightButton->setDisabled(toggled);
	nextRightButton->setDisabled(toggled);
	previewWidget->update();
}

void AlignmentDialog::maskColorButtonClicked() {
	QColor returned=QColorDialog::getColor();
	if (returned.isValid()) {
		previewWidget->setBrushColor(returned);
#if QT_VERSION <= 0x040200
		QPalette modified_palette(maskColorButton->palette());
		modified_palette.setColor(QPalette::Active,QPalette::Window,returned);
		maskColorButton->setPalette( modified_palette );
#else
		maskColorButton->setStyleSheet(QString("background: rgb(%1,%2,%3)").arg(returned.red()).arg(returned.green()).arg(returned.blue()));
#endif
		settings.setValue(KEY_MANUAL_AG_MASK_COLOR,returned.rgb());
	}
}

void AlignmentDialog::blendModeCBIndexChanged(int newindex) {
	maskColorButton->setVisible(newindex==4);
}

void AlignmentDialog::saveImagesButtonClicked() {
	QString fnameprefix=QFileDialog::getSaveFileName(
				this,
				tr("Choose a directory and a prefix"),
				recentPathInputLDR);
	if (fnameprefix.isEmpty())
		return;

	QFileInfo qfi(fnameprefix);
	QFileInfo test(qfi.path());

	settings.setValue(KEY_RECENT_PATH_LOAD_LDRs_FOR_HDR, qfi.path());
	recentPathInputLDR=qfi.path();

	if (test.isWritable() && test.exists() && test.isDir()) {
		int counter=0;
		foreach(QImage *p, original_ldrlist) {
			TiffWriter tiffwriter( (qfi.path() + "/" + qfi.fileName() + QString("_%1.tiff").arg(counter)).toUtf8().constData(), p);
			tiffwriter.write8bitTiff();
			counter++;
		}
	}
}
