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

#include "Common/config.h"
#include "Common/GammaAndLevels.h"
#include "Fileformat/pfstiff.h"
#include "EditingTools.h"

EditingTools::EditingTools(HdrCreationManager *hcm, QWidget *parent) : QDialog(parent), additional_shift_value(0)
{
	setupUi(this);

	original_ldrlist=hcm->getLDRList();
	filelist=hcm->getFileList();
	this->hcm=hcm;

	toolOptionsFrame->setVisible(false);
	maskColorButton->setVisible(false);
	QColor maskcolor=QColor(settings.value(KEY_MANUAL_AG_MASK_COLOR,0x00FF0000).toUInt());
	Qt::ToolButtonStyle style = (Qt::ToolButtonStyle) settings.value(KEY_TOOLBAR_MODE,Qt::ToolButtonTextUnderIcon).toInt();
	maskColorButton->setStyleSheet(QString("background: rgb(%1,%2,%3)").arg(maskcolor.red()).arg(maskcolor.green()).arg(maskcolor.blue()));
	assert(original_ldrlist.size()==filelist.size());
	QVBoxLayout *qvl=new QVBoxLayout;
	qvl->setMargin(0);
	qvl->setSpacing(0);

	scrollArea = new QScrollArea(previewImageFrame);
	previewWidget = new PreviewWidget(this,original_ldrlist[1],original_ldrlist[0]);
	previewWidget->setBrushColor(maskcolor);
	previewWidget->adjustSize();
	previewWidget->update();

	cornerButton=new QToolButton(this);
	cornerButton->setToolTip("Pan the image to a region");
	cornerButton->setIcon(QIcon(":/new/prefix1/images/move.png"));
	scrollArea->setCornerWidget(cornerButton);

	scrollArea->setFocusPolicy(Qt::NoFocus);
	scrollArea->setBackgroundRole(QPalette::Window);
	scrollArea->setWidget(previewWidget);

	qvl->addWidget(scrollArea);
	previewImageFrame->setLayout(qvl);

	foreach(QString s,filelist) {
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

	QStringList::ConstIterator it = filelist.begin();
	while( it != filelist.end() ) {
		HV_offsets.append(qMakePair(0,0));
		++it;
	}

	histogram=new HistogramLDR(this);
	histogram->setData(*(original_ldrlist.at(1)));
	histogram->adjustSize();
	((QHBoxLayout*)(visualizationGroupBox->layout()))->insertWidget(0,histogram);
	previewWidget->setFocus();

	selectionTool = new SelectionTool(previewWidget);
	selectionTool->show();
	//selectionTool->hide();
	
	setupConnections();
} //end of constructor

void EditingTools::setupConnections() {
	connect(cornerButton, SIGNAL(pressed()), this, SLOT(slotCornerButtonPressed()));
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
	connect(prevBothButton,SIGNAL(clicked()),this,SLOT(prevBoth()));
	connect(nextBothButton,SIGNAL(clicked()),this,SLOT(nextBoth()));

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
	connect(previewWidget, SIGNAL(moved(QPoint)), this, SLOT(updateScrollBars(QPoint)));
	connect(selectionTool,SIGNAL(selectionReady(bool)),cropButton,SLOT(setEnabled(bool)));
	connect(selectionTool, SIGNAL(moved(QPoint)), this, SLOT(updateScrollBars(QPoint)));
	connect(removeMaskRadioButton,SIGNAL(toggled(bool)),previewWidget,SLOT(setBrushMode(bool)));
}

void EditingTools::slotCornerButtonPressed() {
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
	panIconWidget->popup(QPoint(g.x() - panIconWidget->width()/2,
					g.y() - panIconWidget->height()/2));

	panIconWidget->setCursorToLocalRegionSelectionCenter();
}

void EditingTools::slotPanIconSelectionMoved(QRect gotopos, bool mousereleased) {
	if (mousereleased) {
		scrollArea->horizontalScrollBar()->setValue((int)(gotopos.x()*previewWidget->getScaleFactor()));
		scrollArea->verticalScrollBar()->setValue((int)(gotopos.y()*previewWidget->getScaleFactor()));
		panIconWidget->close();
		slotPanIconHidden();
	}
}

void EditingTools::slotPanIconHidden()
{
    cornerButton->blockSignals(true);
    cornerButton->animateClick();
    cornerButton->blockSignals(false);
}

EditingTools::~EditingTools() {
	delete previewWidget;
	delete histogram;
	delete cornerButton;
}

void EditingTools::keyPressEvent(QKeyEvent *event) {
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

void EditingTools::keyReleaseEvent ( QKeyEvent * event ) {
	additional_shift_value=0;
	event->ignore();
}

void EditingTools::crop_stack() {
	//zoom the image to 1:1, so that the crop area is in a one-to-one relationship with the pixel coordinates.
	origSize();

	hcm->applyShiftsToImageStack(HV_offsets);

	resetAll();
	//QRect ca=previewWidget->getCropArea();
	QRect ca=selectionTool->getSelectionRect();
	if(ca.width()<=0|| ca.height()<=0)
		return;

	hcm->cropLDR(ca);
	selectionTool->removeSelection();

	original_ldrlist=hcm->getLDRList();

	previewWidget->setMovable(original_ldrlist[movableListWidget->currentRow()]);
	previewWidget->setPivot(original_ldrlist[referenceListWidget->currentRow()]);
	//restore fit
	if (fitButton->isChecked())
		fitPreview(true);
	//and start it up
	previewWidget->update();
	previewWidget->hideRubberBand();
}

void EditingTools::nextClicked() {
	hcm->applyShiftsToImageStack(HV_offsets);
	emit accept();
}

void EditingTools::updateMovable(int newidx) {
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

void EditingTools::updatePivot(int newidx) {
	previewWidget->setPivot(original_ldrlist[newidx],HV_offsets[newidx].first, HV_offsets[newidx].second);
	previewWidget->update();
}

void EditingTools::upClicked() {
	vertShiftSB->setValue(vertShiftSB->value()-1-additional_shift_value);
}
void EditingTools::downClicked() {
	vertShiftSB->setValue(vertShiftSB->value()+1+additional_shift_value);
}
void EditingTools::rightClicked() {
	horizShiftSB->setValue(horizShiftSB->value()+1+additional_shift_value);
}
void EditingTools::leftClicked() {
	horizShiftSB->setValue(horizShiftSB->value()-1-additional_shift_value);
}

void EditingTools::vertShiftChanged(int v) {
	HV_offsets[movableListWidget->currentRow()].second=v;
	previewWidget->updateVertShiftMovable(v);
	previewWidget->update();
}
void EditingTools::horizShiftChanged(int v) {
	HV_offsets[movableListWidget->currentRow()].first=v;
	previewWidget->updateHorizShiftMovable(v);
	previewWidget->update();
}

void EditingTools::resetCurrent() {
	horizShiftSB->setValue(0);
	vertShiftSB->setValue(0);
}

void EditingTools::resetAll() {
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

void EditingTools::prevLeft() {
	int prev=(movableListWidget->currentRow()==0) ? movableListWidget->count()-1 : movableListWidget->currentRow()-1;
	movableListWidget->setCurrentRow(prev);
}

void EditingTools::nextLeft() {
	int next=(movableListWidget->currentRow()==movableListWidget->count()-1) ? 0 : movableListWidget->currentRow()+1;
	movableListWidget->setCurrentRow(next);
}

void EditingTools::prevBoth() {
	prevRight();
	prevLeft();
}

void EditingTools::nextBoth() {
	nextRight();
	nextLeft();
}

void EditingTools::prevRight() {
	int prev=(referenceListWidget->currentRow()==0) ? referenceListWidget->count()-1 : referenceListWidget->currentRow()-1;
	referenceListWidget->setCurrentRow(prev);
}

void EditingTools::nextRight() {
	int next=(referenceListWidget->currentRow()==referenceListWidget->count()-1) ? 0 : referenceListWidget->currentRow()+1;
	referenceListWidget->setCurrentRow(next);
}

void EditingTools::enterWhatsThis() {
	QWhatsThis::enterWhatsThisMode();
}

void EditingTools::zoomIn() {
	previewWidget->resize(previewWidget->size()*1.25f);
	zoomOutButton->setEnabled(true);
	zoomInButton->setEnabled(previewWidget->getScaleFactor() < 6.0);
}
void EditingTools::zoomOut() {
	previewWidget->resize(previewWidget->size()*0.8f);
	zoomInButton->setEnabled(true);
	zoomOutButton->setEnabled(previewWidget->getScaleFactor() > 0.166);
}
void EditingTools::fitPreview(bool checked) {
	zoomInButton->setEnabled(!checked);
	zoomOutButton->setEnabled(!checked);
	origSizeButton->setEnabled(!checked);
	if (checked) {
		previousPreviewWidgetSize=previewWidget->size();
		QSize fillWinSize=original_ldrlist.at(0)->size();
		fillWinSize.scale(scrollArea->maximumViewportSize(),Qt::KeepAspectRatio);
		previewWidget->resize(fillWinSize);
	} else {
		previewWidget->resize(previousPreviewWidgetSize);
	}
}
void EditingTools::origSize() {
	zoomInButton->setEnabled(true);
	zoomOutButton->setEnabled(true);
	previewWidget->resize(original_ldrlist.at(0)->size());
}

void EditingTools::antighostToolButtonToggled(bool toggled) {
// 	if (toggled)
// 		blendModeCB->setCurrentIndex(4);
	prevBothButton->setDisabled(toggled);
	nextBothButton->setDisabled(toggled);
	label_reference_list->setDisabled(toggled);
	referenceListWidget->setDisabled(toggled);
	previewWidget->update();
	toggled ? selectionTool->hide() : selectionTool->show();
}

void EditingTools::maskColorButtonClicked() {
	QColor returned=QColorDialog::getColor();
	if (returned.isValid()) {
		previewWidget->setBrushColor(returned);
		maskColorButton->setStyleSheet(QString("background: rgb(%1,%2,%3)").arg(returned.red()).arg(returned.green()).arg(returned.blue()));
		settings.setValue(KEY_MANUAL_AG_MASK_COLOR,returned.rgb());
	}
}

void EditingTools::blendModeCBIndexChanged(int newindex) {
	maskColorButton->setVisible(newindex==4);
}

void EditingTools::saveImagesButtonClicked() {
	QString fnameprefix=QFileDialog::getSaveFileName(
				this,
				tr("Choose a directory and a prefix"),
				settings.value(KEY_RECENT_PATH_LOAD_LDRs_FOR_HDR,QDir::currentPath()).toString());
	if (fnameprefix.isEmpty())
		return;

	QFileInfo qfi(fnameprefix);
	QFileInfo test(qfi.path());

	settings.setValue(KEY_RECENT_PATH_LOAD_LDRs_FOR_HDR, qfi.path());

	if (test.isWritable() && test.exists() && test.isDir()) {
		int counter=0;
		foreach(QImage *p, original_ldrlist) {
			TiffWriter tiffwriter( QFile::encodeName((qfi.path() + "/" + qfi.fileName() + QString("_%1.tiff").arg(counter))).constData(), p);
			tiffwriter.write8bitTiff();
			counter++;
		}
	}
}

void EditingTools::updateScrollBars(QPoint diff) {
        scrollArea->verticalScrollBar()->setValue(scrollArea->verticalScrollBar()->value() + diff.y());
        scrollArea->horizontalScrollBar()->setValue(scrollArea->horizontalScrollBar()->value() + diff.x());
}

