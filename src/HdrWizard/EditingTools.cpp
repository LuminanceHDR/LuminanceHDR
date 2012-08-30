/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2006,2007 Giuseppe Rota
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ----------------------------------------------------------------------
 *
 * Original Work
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * Improvements, bugfixing, anti ghosting
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#include <QFileInfo>
#include <cassert>
#include <QWhatsThis>
#include <QColorDialog>
#include <QFileDialog>
#include <QDebug>

#include "Common/config.h"
#include "UI/GammaAndLevels.h"
#include "Viewers/PanIconWidget.h"
#include "Fileformat/pfstiff.h"
#include "HdrWizard/EditingTools.h"
#include "Exif/ExifOperations.h"
#include "HdrCreation/mtb_alignment.h"

EditingTools::EditingTools(HdrCreationManager *hcm, QWidget *parent) :
    QDialog(parent),
    m_hcm(hcm),
    m_additionalShiftValue(0),
    m_MdrSaved(false),
    m_goodImageIndex(-1),
    m_antiGhosting(false)
{
	setupUi(this);

	if (m_hcm->inputImageType() == HdrCreationManager::LDR_INPUT_TYPE) {
		m_originalImagesList=m_hcm->getLDRList();
		m_expotimes = m_hcm->getExpotimes();
	}
	else {
		m_originalImagesList=m_hcm->getMDRList();
	}

	m_fileList=m_hcm->getFileList();
	m_antiGhostingMasksList = m_hcm->getAntiGhostingMasksList();

	toolOptionsFrame->setVisible(false);
	maskColorButton->setVisible(false);
	QColor maskcolor = QColor(m_luminanceOptions.value(KEY_MANUAL_AG_MASK_COLOR,0x00FF0000).toUInt());
	Qt::ToolButtonStyle style = (Qt::ToolButtonStyle) m_luminanceOptions.value(KEY_TOOLBAR_MODE,Qt::ToolButtonTextUnderIcon).toInt();
	maskColorButton->setStyleSheet(QString("background: rgb("+QString(maskcolor.red())+","+QString(maskcolor.green())+","+QString(maskcolor.blue())+")"));
	assert(m_originalImagesList.size()==m_fileList.size());
	QVBoxLayout *qvl = new QVBoxLayout;
	qvl->setMargin(0);
	qvl->setSpacing(0);

	m_scrollArea = new QScrollArea(previewImageFrame);
	m_previewWidget = new PreviewWidget(this,m_originalImagesList[1],m_originalImagesList[0]);
	m_previewWidget->adjustSize();
	m_previewWidget->update();
	m_agWidget = new AntiGhostingWidget(m_previewWidget, m_antiGhostingMasksList[1]);
	m_agWidget->setBrushColor(maskcolor);
	m_agWidget->adjustSize();
	m_agWidget->update();

	m_cornerButton = new QToolButton(this);
	m_cornerButton->setToolTip(tr("Pan the image to a region"));
	m_cornerButton->setIcon(QIcon(":/new/prefix1/images/move.png"));
	m_scrollArea->setCornerWidget(m_cornerButton);

	m_scrollArea->setFocusPolicy(Qt::NoFocus);
	m_scrollArea->setBackgroundRole(QPalette::Window);
	m_scrollArea->setWidget(m_previewWidget);

	qvl->addWidget(m_scrollArea);
	previewImageFrame->setLayout(qvl);

    int idx = 0;
	foreach(QString s,m_fileList) {
        m_filesMap[QFileInfo(s).fileName()] = idx++;
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

	QStringList::ConstIterator it = m_fileList.begin();
	while( it != m_fileList.end() ) {
		m_HV_offsets.append(qMakePair(0,0));
		++it;
	}

	m_histogram=new HistogramLDR(this);
	m_histogram->setData( m_originalImagesList.at(1) );
	m_histogram->adjustSize();

	((QGridLayout*)(groupBoxHistogram->layout()))->addWidget(m_histogram);
	m_previewWidget->setFocus();

	m_selectionTool = new SelectionTool(m_previewWidget);
	m_selectionTool->show();
	
	setupConnections();
} //end of constructor

void EditingTools::setupConnections() {
	connect(m_cornerButton, SIGNAL(pressed()), this, SLOT(slotCornerButtonPressed()));
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
	connect(cropButton,SIGNAL(clicked()),this,SLOT(cropStack()));
	connect(saveImagesButton,SIGNAL(clicked()),this,SLOT(saveImagesButtonClicked()));
	connect(blendModeCB,SIGNAL(currentIndexChanged(int)),m_previewWidget,SLOT(requestedBlendMode(int)));
	connect(blendModeCB,SIGNAL(currentIndexChanged(int)),this,SLOT(blendModeCBIndexChanged(int)));
	connect(antighostToolButton,SIGNAL(toggled(bool)),toolOptionsFrame,SLOT(setVisible(bool)));
	connect(antighostToolButton,SIGNAL(toggled(bool)),m_agWidget,SLOT(switchAntighostingMode(bool)));
	connect(antighostToolButton,SIGNAL(toggled(bool)),this,SLOT(antighostToolButtonToggled(bool)));
	connect(agBrushSizeQSpinbox,SIGNAL(valueChanged(int)),m_agWidget,SLOT(setBrushSize(int)));
	connect(agBrushStrengthQSpinbox,SIGNAL(valueChanged(int)),m_agWidget,SLOT(setBrushStrength(int)));
	connect(maskColorButton,SIGNAL(clicked()),this,SLOT(maskColorButtonClicked()));

	connect(Next_Finishbutton,SIGNAL(clicked()),this,SLOT(nextClicked()));
	connect(m_previewWidget, SIGNAL(moved(QPoint)), this, SLOT(updateScrollBars(QPoint)));
	connect(m_agWidget, SIGNAL(moved(QPoint)), this, SLOT(updateScrollBars(QPoint)));
	connect(m_selectionTool,SIGNAL(selectionReady(bool)),cropButton,SLOT(setEnabled(bool)));
	connect(m_selectionTool, SIGNAL(moved(QPoint)), this, SLOT(updateScrollBars(QPoint)));
	connect(removeMaskRadioButton,SIGNAL(toggled(bool)),m_agWidget,SLOT(setBrushMode(bool)));

	connect(m_hcm, SIGNAL(mdrSaved()), this, SLOT(restoreSaveImagesButtonState()));
}

void EditingTools::slotCornerButtonPressed() {
	m_panIconWidget=new PanIconWidget;
	m_panIconWidget->setImage(m_previewWidget->getPreviewImage());
	float zf=m_previewWidget->getScaleFactor();
	float leftviewpos=(float)(m_scrollArea->horizontalScrollBar()->value());
	float topviewpos=(float)(m_scrollArea->verticalScrollBar()->value());
	float wps_w=(float)(m_scrollArea->maximumViewportSize().width());
	float wps_h=(float)(m_scrollArea->maximumViewportSize().height());
	QRect r((int)(leftviewpos/zf), (int)(topviewpos/zf), (int)(wps_w/zf), (int)(wps_h/zf));
	m_panIconWidget->setRegionSelection(r);
	m_panIconWidget->setMouseFocus();
	connect(m_panIconWidget, SIGNAL(selectionMoved(QRect)), this, SLOT(slotPanIconSelectionMoved(QRect)));
	connect(m_panIconWidget, SIGNAL(finished()), this, SLOT(slotPanIconHidden()));
	QPoint g = m_scrollArea->mapToGlobal(m_scrollArea->viewport()->pos());
	g.setX(g.x()+ m_scrollArea->viewport()->size().width());
	g.setY(g.y()+ m_scrollArea->viewport()->size().height());
	m_panIconWidget->popup(QPoint(g.x() - m_panIconWidget->width()/2,
					g.y() - m_panIconWidget->height()/2));

	m_panIconWidget->setCursorToLocalRegionSelectionCenter();
}

void EditingTools::slotPanIconSelectionMoved(QRect gotopos) {
	m_scrollArea->horizontalScrollBar()->setValue((int)(gotopos.x()*m_previewWidget->getScaleFactor()));
	m_scrollArea->verticalScrollBar()->setValue((int)(gotopos.y()*m_previewWidget->getScaleFactor()));
}

void EditingTools::slotPanIconHidden()
{
	m_panIconWidget->close();
	m_cornerButton->blockSignals(true);
	m_cornerButton->animateClick();
	m_cornerButton->blockSignals(false);
}

EditingTools::~EditingTools() {
	delete m_previewWidget;
	delete m_histogram;
	delete m_cornerButton;
}

void EditingTools::keyPressEvent(QKeyEvent *event) {
	int key=event->key();
	Qt::KeyboardModifiers mods=event->modifiers();
	if ((mods & Qt::ShiftModifier)!=0 && (mods & Qt::ControlModifier)!=0)
		m_additionalShiftValue=99;
	else if (mods & Qt::ControlModifier)
		m_additionalShiftValue=49;
	else if (mods & Qt::ShiftModifier)
		m_additionalShiftValue=9;

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
	m_additionalShiftValue=0;
	event->ignore();
}

void EditingTools::cropStack() {
	//zoom the image to 1:1, so that the crop area is in a one-to-one relationship with the pixel coordinates.
	origSize();

	if (m_hcm->inputImageType() == HdrCreationManager::LDR_INPUT_TYPE) 
		m_hcm->applyShiftsToImageStack(m_HV_offsets);
	else
		m_hcm->applyShiftsToMdrImageStack(m_HV_offsets);

	resetAll();
	QRect ca=m_selectionTool->getSelectionRect();
	if(ca.width()<=0 || ca.height()<=0)
		return;

	if (m_hcm->inputImageType() == HdrCreationManager::LDR_INPUT_TYPE) {
		m_hcm->cropLDR(ca);
		m_originalImagesList=m_hcm->getLDRList();
	}
	else {
		m_hcm->cropMDR(ca);
		m_originalImagesList=m_hcm->getMDRList();
	}
	
	m_antiGhostingMasksList = m_hcm->getAntiGhostingMasksList();
		
	m_selectionTool->removeSelection();

	m_previewWidget->setMovable(m_originalImagesList[movableListWidget->currentRow()]);
	m_previewWidget->setPivot(m_originalImagesList[referenceListWidget->currentRow()]);
	m_agWidget->setMask(m_antiGhostingMasksList[movableListWidget->currentRow()]);
	//restore fit
	if (fitButton->isChecked())
		fitPreview(true);
	//and start it up
	m_previewWidget->update();
}

void EditingTools::nextClicked() {
	Next_Finishbutton->setEnabled(false);
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	if (m_hcm->inputImageType() == HdrCreationManager::LDR_INPUT_TYPE) { 
		m_hcm->applyShiftsToImageStack(m_HV_offsets);
        if (m_goodImageIndex != -1)
		    m_hcm->doAntiGhosting(m_goodImageIndex);
	}
	else {
		if (!m_MdrSaved)
			m_hcm->applyShiftsToMdrImageStack(m_HV_offsets);
        if (m_goodImageIndex != -1)
    		m_hcm->doAntiGhosting(m_goodImageIndex);
	}
	QApplication::restoreOverrideCursor();
	emit accept();
}

void EditingTools::updateMovable(int newidx) {
	//inform display_widget of the change
	m_previewWidget->setMovable(m_originalImagesList[newidx], m_HV_offsets[newidx].first, m_HV_offsets[newidx].second);
	m_agWidget->setMask(m_antiGhostingMasksList[newidx]);
	m_agWidget->setHV_offset(m_HV_offsets[newidx]);
	//prevent a change in the spinboxes to start a useless calculation
	horizShiftSB->blockSignals(true);
	horizShiftSB->setValue(m_HV_offsets[newidx].first);
	horizShiftSB->blockSignals(false);
	vertShiftSB->blockSignals(true);
	vertShiftSB->setValue(m_HV_offsets[newidx].second);
	vertShiftSB->blockSignals(false);
	m_previewWidget->update();
	m_histogram->setData(m_originalImagesList[newidx]);
	m_histogram->update();
}

void EditingTools::updatePivot(int newidx) {
	m_previewWidget->setPivot(m_originalImagesList[newidx],m_HV_offsets[newidx].first, m_HV_offsets[newidx].second);
	m_previewWidget->update();
}

void EditingTools::upClicked() {
	vertShiftSB->setValue(vertShiftSB->value()-1-m_additionalShiftValue);
}
void EditingTools::downClicked() {
	vertShiftSB->setValue(vertShiftSB->value()+1+m_additionalShiftValue);
}
void EditingTools::rightClicked() {
	horizShiftSB->setValue(horizShiftSB->value()+1+m_additionalShiftValue);
}
void EditingTools::leftClicked() {
	horizShiftSB->setValue(horizShiftSB->value()-1-m_additionalShiftValue);
}

void EditingTools::vertShiftChanged(int v) {
	m_HV_offsets[movableListWidget->currentRow()].second=v;
	m_previewWidget->updateVertShiftMovable(v);
	m_agWidget->updateVertShift(v);
	m_previewWidget->update();
	m_agWidget->update();
}
void EditingTools::horizShiftChanged(int v) {
	m_HV_offsets[movableListWidget->currentRow()].first=v;
	m_previewWidget->updateHorizShiftMovable(v);
	m_agWidget->updateHorizShift(v);
	m_previewWidget->update();
	m_agWidget->update();
}

void EditingTools::resetCurrent() {
	horizShiftSB->setValue(0);
	vertShiftSB->setValue(0);
}

void EditingTools::resetAll() {
	for (int i = 0; i < m_HV_offsets.size(); ++i) {
		m_HV_offsets[i].first=0;
		m_HV_offsets[i].second=0;
	}
	//prevent a change in the spinboxes to start a useless calculation
	horizShiftSB->blockSignals(true);
	vertShiftSB->blockSignals(true);
	resetCurrent(); //graphical update
	horizShiftSB->blockSignals(false);
	vertShiftSB->blockSignals(false);
	m_previewWidget->updateHorizShiftMovable(0);
	m_previewWidget->updateVertShiftMovable(0);
	m_previewWidget->updateHorizShiftPivot(0);
	m_previewWidget->updateVertShiftPivot(0);
	m_previewWidget->update();
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
	m_previewWidget->resize(m_previewWidget->size()*1.25f);
	m_agWidget->resize(m_previewWidget->size()*1.25f);
	zoomOutButton->setEnabled(true);
	zoomInButton->setEnabled(m_previewWidget->getScaleFactor() < 6.0);
}

void EditingTools::zoomOut() {
	m_previewWidget->resize(m_previewWidget->size()*0.8f);
	m_agWidget->resize(m_previewWidget->size()*0.8f);
	zoomInButton->setEnabled(true);
	zoomOutButton->setEnabled(m_previewWidget->getScaleFactor() > 0.166);
}

void EditingTools::fitPreview(bool checked) {
    if (!m_antiGhosting) {
    	zoomInButton->setEnabled(!checked);
	    zoomOutButton->setEnabled(!checked);
    }
	origSizeButton->setEnabled(!checked);
	if (checked) {
		m_previousPreviewWidgetSize=m_previewWidget->size();
		QSize fillWinSize=m_originalImagesList.at(0)->size();
		fillWinSize.scale(m_scrollArea->maximumViewportSize(),Qt::KeepAspectRatio);
		m_previewWidget->resize(fillWinSize);
		m_agWidget->resize(fillWinSize);
	} else {
		m_previewWidget->resize(m_previousPreviewWidgetSize);
		m_agWidget->resize(m_previousPreviewWidgetSize);
	}
}

void EditingTools::origSize() {
	zoomInButton->setEnabled(true);
	zoomOutButton->setEnabled(true);
	m_previewWidget->resize(m_originalImagesList.at(0)->size());
	m_agWidget->resize(m_originalImagesList.at(0)->size());
}

void EditingTools::antighostToolButtonToggled(bool toggled) {
	m_previewWidget->update();
	toggled ? m_selectionTool->disable() : m_selectionTool->enable();
	if (toggled) {
        m_antiGhosting = true;
        m_previewWidget->hide();
	    m_agWidget->show();
        zoomInButton->setDisabled(true);
        zoomOutButton->setDisabled(true);
        saveImagesButton->setDisabled(true);
        prevBothButton->setIcon(QIcon(":new/prefix1/images/forward.png"));	
        nextBothButton->setIcon(QIcon(":new/prefix1/images/back.png"));	
	    nextBothButton->setDisabled(true);
	    disconnect(prevBothButton,SIGNAL(clicked()),this,SLOT(prevBoth()));
	    disconnect(nextBothButton,SIGNAL(clicked()),this,SLOT(nextBoth()));
	    disconnect(movableListWidget,SIGNAL(currentRowChanged(int)),this,SLOT(updateMovable(int)));
	    disconnect(referenceListWidget,SIGNAL(currentRowChanged(int)),this,SLOT(updatePivot(int)));
	    connect(prevBothButton,SIGNAL(clicked()),this,SLOT(addGoodImage()));
	    connect(nextBothButton,SIGNAL(clicked()),this,SLOT(removeGoodImage()));
        referenceListWidget->clear();
        if (m_goodImageIndex != -1) {
            prevBothButton->setDisabled(true);
            nextBothButton->setDisabled(false);
	        referenceListWidget->addItem(QFileInfo(m_fileList[m_goodImageIndex]).fileName());
	        referenceListWidget->setCurrentRow(0);
            movableListWidget->takeItem(m_goodImageIndex);
            updatePivot(m_goodImageIndex);
            m_previewWidget->show();
        }
	    connect(movableListWidget,SIGNAL(currentRowChanged(int)),this,SLOT(updateAgMask(int)));
    }
	else {
        m_antiGhosting = false;
        m_previewWidget->show();
	    m_agWidget->hide();
	    disconnect(movableListWidget,SIGNAL(currentRowChanged(int)),this,SLOT(updateAgMask(int)));
        prevBothButton->setIcon(QIcon(":new/prefix1/images/uparrow.png"));	
        nextBothButton->setIcon(QIcon(":new/prefix1/images/downarrow.png"));	
	    prevBothButton->setDisabled(false);
	    nextBothButton->setDisabled(false);
        zoomInButton->setDisabled(false);
        zoomOutButton->setDisabled(false);
        saveImagesButton->setDisabled(false);
        movableListWidget->clear();
        referenceListWidget->clear();
	    disconnect(prevBothButton,SIGNAL(clicked()),this,SLOT(addGoodImage()));
	    disconnect(nextBothButton,SIGNAL(clicked()),this,SLOT(removeGoodImage()));
	    connect(prevBothButton,SIGNAL(clicked()),this,SLOT(prevBoth()));
	    connect(nextBothButton,SIGNAL(clicked()),this,SLOT(nextBoth()));
	    connect(movableListWidget,SIGNAL(currentRowChanged(int)),this,SLOT(updateMovable(int)));
	    connect(referenceListWidget,SIGNAL(currentRowChanged(int)),this,SLOT(updatePivot(int)));
	    connect(blendModeCB,SIGNAL(currentIndexChanged(int)),m_previewWidget,SLOT(requestedBlendMode(int)));
	    foreach(QString s,m_fileList) {
		    movableListWidget->addItem(QFileInfo(s).fileName());
		    referenceListWidget->addItem(QFileInfo(s).fileName());
	    }
	    movableListWidget->setCurrentRow(1);
	    referenceListWidget->setCurrentRow(0);
    }
}

void EditingTools::maskColorButtonClicked() {
	QColor returned = QColorDialog::getColor();
	if (returned.isValid()) {
		m_agWidget->setBrushColor(returned);
		maskColorButton->setStyleSheet(QString("background: rgb(%1,%2,%3)").arg(returned.red()).arg(returned.green()).arg(returned.blue()));
		m_luminanceOptions.setValue(KEY_MANUAL_AG_MASK_COLOR,returned.rgb());
	}
}

void EditingTools::blendModeCBIndexChanged(int newindex) {
	maskColorButton->setVisible(newindex == 4);
}

void EditingTools::saveImagesButtonClicked() {
	saveImagesButton->setEnabled(false);
	Next_Finishbutton->setEnabled(false);
	QString fnameprefix=QFileDialog::getSaveFileName(
				this,
				tr("Choose a directory and a prefix"),
								m_luminanceOptions.value(KEY_RECENT_PATH_LOAD_LDRs_FOR_HDR,QDir::currentPath()).toString());
	if (fnameprefix.isEmpty())
		return;

	QFileInfo qfi(fnameprefix);
	QFileInfo test(qfi.path());

	m_luminanceOptions.setValue(KEY_RECENT_PATH_LOAD_LDRs_FOR_HDR, qfi.path());

	if (test.isWritable() && test.exists() && test.isDir()) {
		QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
		if (m_hcm->inputImageType() == HdrCreationManager::LDR_INPUT_TYPE) {
			int counter=0;
			foreach(QImage *p, m_originalImagesList) {
				TiffWriter tiffwriter( QFile::encodeName((qfi.path() + "/" + qfi.fileName() + QString("_%1.tiff").arg(counter))), p);
				tiffwriter.write8bitTiff();
				ExifOperations::writeExifData(QFile::encodeName((qfi.path() + "/" + qfi.fileName() + QString("_%1.tiff").arg(counter))).constData(), "Edited Images", m_expotimes[counter]);	
				counter++;
			}
			saveImagesButton->setEnabled(true);
			Next_Finishbutton->setEnabled(true);
			QApplication::restoreOverrideCursor();
		}
		else {
			m_MdrSaved = true;
	
			m_hcm->applyShiftsToMdrImageStack(m_HV_offsets);
			m_hcm->saveMDRs(QFile::encodeName((qfi.path() + "/" + qfi.fileName())));
		}
	}
}

void EditingTools::updateScrollBars(QPoint diff) {
	m_scrollArea->verticalScrollBar()->setValue(m_scrollArea->verticalScrollBar()->value() + diff.y());
	m_scrollArea->horizontalScrollBar()->setValue(m_scrollArea->horizontalScrollBar()->value() + diff.x());
}

void EditingTools::restoreSaveImagesButtonState()
{
	saveImagesButton->setEnabled(true);
	Next_Finishbutton->setEnabled(true);
	QApplication::restoreOverrideCursor();
}

void EditingTools::setAntiGhostingWidget(QImage *mask, QPair<int, int> HV_offset)
{
	m_agWidget->setMask(mask);
	m_agWidget->setHV_offset(HV_offset);
	m_agWidget->show();
}

void EditingTools::addGoodImage()
{
    QString filename = movableListWidget->currentItem()->text();
    int idx = m_filesMap[filename];
    int idxToRemove = movableListWidget->currentRow();
	referenceListWidget->addItem(QFileInfo(m_fileList[idx]).fileName());
	referenceListWidget->setCurrentRow(0);
    m_goodImageIndex = idx;
    m_antiGhostingMasksList[idx]->fill(0x00000000);
    movableListWidget->takeItem(idxToRemove);
    prevBothButton->setDisabled(true);
    nextBothButton->setDisabled(false);
    updatePivot(idx);
    m_previewWidget->show();
}

void EditingTools::removeGoodImage()
{
    QString filename = referenceListWidget->currentItem()->text();
    int idx = m_filesMap[filename];
	movableListWidget->addItem(QFileInfo(m_fileList[idx]).fileName());
    referenceListWidget->takeItem(0);
    prevBothButton->setDisabled(false);
    nextBothButton->setDisabled(true);
    m_goodImageIndex = -1;
	m_agWidget->hide();
    m_previewWidget->hide();
}

void EditingTools::updateAgMask(int)
{
    QString filename = movableListWidget->currentItem()->text();
    int idx = m_filesMap[filename];
	setAntiGhostingWidget(m_antiGhostingMasksList[idx], m_HV_offsets[idx]);
    updateMovable(idx);
}

