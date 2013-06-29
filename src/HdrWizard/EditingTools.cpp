/**
 * This file is a part of Luminance HDR package.
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
 * Original Work
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 *
 * Improvements, bugfixing, anti ghosting, new preview widget based on QGraphicsView
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
#include "HdrWizard/EditingTools.h"
#include "Exif/ExifOperations.h"
#include "HdrCreation/mtb_alignment.h"

EditingTools::EditingTools(HdrCreationManager *hcm, QWidget *parent) :
    QDialog(parent),
    m_currentAgMaskIndex(0),
    m_hcm(hcm),
    m_additionalShiftValue(0),
    m_imagesSaved(false),
    m_agGoodImageIndex(-1),
    m_antiGhosting(false),
    m_doAutoAntighosting(false),
    m_doManualAntighosting(false)
{
    setupUi(this);
   
    for (int i = 0; i < agGridSize; i++)
        for (int j = 0; j < agGridSize; j++)
            m_patches[i][j] = false;

    HdrCreationItemContainer data = m_hcm->getData();
    for ( HdrCreationItemContainer::iterator it = data.begin(), 
          itEnd = data.end(); it != itEnd; ++it) {
        m_originalImagesList.push_back(it->qimage());
        m_fileList.push_back(it->filename());
    }

    int width = m_originalImagesList.at(0)->width();
    int height = m_originalImagesList.at(0)->height();
    m_gridX = width/agGridSize;
    m_gridY = height/agGridSize;

    int size = m_originalImagesList.size();
    for ( int h = 0; h < size; h++) { 
        QImage *img = new QImage(width, height, QImage::Format_ARGB32);
        img->fill(qRgba(0,0,0,0));
        m_antiGhostingMasksList.append(img);
    }
    m_antiGhostingMask = new QImage(width, height, QImage::Format_ARGB32);
    m_antiGhostingMask->fill(qRgba(0,0,0,0));

    m_expotimes = m_hcm->getExpotimes();

    toolOptionsFrame->setVisible(false);
    maskColorButton->setVisible(false);
    lassoColorButton->setVisible(false);
    QColor maskcolor = QColor(m_luminanceOptions.value(KEY_MANUAL_AG_MASK_COLOR,0x00FF0000).toUInt());
    QColor lassocolor = QColor(m_luminanceOptions.value(KEY_MANUAL_AG_LASSO_COLOR,0x000000FF).toUInt());
    Qt::ToolButtonStyle style = (Qt::ToolButtonStyle) m_luminanceOptions.value(KEY_TOOLBAR_MODE,Qt::ToolButtonTextUnderIcon).toInt();
    maskColorButton->setStyleSheet("background: rgb("+QString("%1").arg(maskcolor.red())+","+QString("%1").arg(maskcolor.green())+","+QString("%1").arg(maskcolor.blue())+")");
    lassoColorButton->setStyleSheet("background: rgb("+QString("%1").arg(lassocolor.red())+","+QString("%1").arg(lassocolor.green())+","+QString("%1").arg(lassocolor.blue())+")");
    assert(m_originalImagesList.size()==m_fileList.size());
    QVBoxLayout *qvl = new QVBoxLayout;
    qvl->setMargin(0);
    qvl->setSpacing(0);

    m_previewWidget = new PreviewWidget(this, m_originalImagesList[1],m_originalImagesList[0]);
    m_previewWidget->setMask(m_antiGhostingMasksList[0]);
    m_previewWidget->setBrushColor(maskcolor);
    m_previewWidget->setLassoColor(lassocolor);
    m_previewWidget->adjustSize();
    m_previewWidget->update();
    m_patchesMask = new QImage(width, height, QImage::Format_ARGB32);
    m_previewWidget->setPatchesMask(m_patchesMask);
    
    qvl->addWidget(m_previewWidget);
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
    fillButton->setToolButtonStyle(style);
    whatsThisButton->setToolButtonStyle(style);
    cropButton->setToolButtonStyle(style);
    saveImagesButton->setToolButtonStyle(style);
    antighostToolButton->setToolButtonStyle(style);
    toolButtonPaint->setToolButtonStyle(style);
    toolButtonPath->setToolButtonStyle(style);

    drawingModeFrame->hide();

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

    setupConnections();
} //end of constructor

void EditingTools::setupConnections() {
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
    connect(fitButton,SIGNAL(clicked()),this,SLOT(fitPreview()));
    connect(origSizeButton,SIGNAL(clicked()),this,SLOT(origSize()));
    connect(fillButton,SIGNAL(clicked()),this,SLOT(fillPreview()));
    connect(cropButton,SIGNAL(clicked()),this,SLOT(cropStack()));
    connect(m_previewWidget,SIGNAL(selectionReady(bool)),cropButton,SLOT(setEnabled(bool))); 
    connect(saveImagesButton,SIGNAL(clicked()),this,SLOT(saveImagesButtonClicked()));
    connect(blendModeCB,SIGNAL(currentIndexChanged(int)),m_previewWidget,SLOT(requestedBlendMode(int)));
    connect(blendModeCB,SIGNAL(currentIndexChanged(int)),this,SLOT(blendModeCBIndexChanged(int)));
    //connect(antighostToolButton,SIGNAL(toggled(bool)),toolOptionsFrame,SLOT(setVisible(bool)));
    connect(antighostToolButton,SIGNAL(toggled(bool)),drawingModeFrame,SLOT(setVisible(bool)));
    connect(antighostToolButton,SIGNAL(toggled(bool)),m_previewWidget,SLOT(switchAntighostingMode(bool)));
    connect(antighostToolButton,SIGNAL(toggled(bool)),this,SLOT(antighostToolButtonToggled(bool)));
    connect(toolButtonPaint,SIGNAL(toggled(bool)),this,SLOT(antighostToolButtonPaintToggled(bool)));
    connect(agBrushSizeQSpinbox,SIGNAL(valueChanged(int)),m_previewWidget,SLOT(setBrushSize(int)));
    connect(agBrushStrengthQSpinbox,SIGNAL(valueChanged(int)),m_previewWidget,SLOT(setBrushStrength(int)));
    connect(maskColorButton,SIGNAL(clicked()),this,SLOT(maskColorButtonClicked()));
    connect(lassoColorButton,SIGNAL(clicked()),this,SLOT(lassoColorButtonClicked()));
    connect(toolButtonSaveMask,SIGNAL(clicked()),m_previewWidget,SLOT(saveAgMask()));
    connect(toolButtonSaveMask,SIGNAL(clicked()),this,SLOT(saveAgMask()));
    connect(toolButtonApplyMask,SIGNAL(clicked()),this,SLOT(applySavedAgMask()));
    connect(threshold_horizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(updateThresholdSlider(int)));
    connect(threshold_doubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateThresholdSpinBox(double)));

    connect(Next_Finishbutton,SIGNAL(clicked()),this,SLOT(nextClicked()));
    //connect(m_previewWidget, SIGNAL(moved(QPoint)), this, SLOT(updateScrollBars(QPoint)));
    connect(removeMaskRadioButton,SIGNAL(toggled(bool)),m_previewWidget,SLOT(setBrushMode(bool)));

    connect(m_hcm, SIGNAL(imagesSaved()), this, SLOT(restoreSaveImagesButtonState()));
}

EditingTools::~EditingTools()
{
    delete m_previewWidget;
    delete m_histogram;
    delete m_patchesMask;
    qDeleteAll(m_antiGhostingMasksList);
    delete m_antiGhostingMask;
}

void EditingTools::keyPressEvent(QKeyEvent *event)
{
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

void EditingTools::keyReleaseEvent ( QKeyEvent * event )
{
    m_additionalShiftValue=0;
    event->ignore();
}

void EditingTools::cropStack()
{
    //zoom the image to 1:1, so that the crop area is in a one-to-one relationship with the pixel coordinates.
    origSize();

    m_hcm->applyShiftsToItems(m_HV_offsets);
    resetAll();
    QRect ca = m_previewWidget->getSelectionRect();
    if(ca.width()<=0 || ca.height()<=0)
        return;

    QImage* tmp = m_previewWidget->getMask();
    delete m_antiGhostingMasksList[m_currentAgMaskIndex];
    m_antiGhostingMasksList.replace(m_currentAgMaskIndex, tmp);
    cropAgMasks(ca);
    m_hcm->cropItems(ca);
    m_originalImagesList.clear();
    HdrCreationItemContainer data = m_hcm->getData();
    for ( HdrCreationItemContainer::iterator it = data.begin(), 
          itEnd = data.end(); it != itEnd; ++it) {
        m_originalImagesList.push_back(it->qimage());
    }
    
    m_previewWidget->removeSelection();
    m_previewWidget->setMovable(m_originalImagesList[movableListWidget->currentRow()]);
    m_previewWidget->setPivot(m_originalImagesList[referenceListWidget->currentRow()]);
    m_currentAgMaskIndex = movableListWidget->currentRow();
    m_previewWidget->setMask(m_antiGhostingMasksList[m_currentAgMaskIndex]);
    //restore fit
    if (fitButton->isChecked())
        fitPreview();
    //and start it up
    m_previewWidget->updatePreviewImage();
}

void EditingTools::cropAgMasks(const QRect& ca) {
    int origlistsize = m_antiGhostingMasksList.size();
    for (int image_idx = 0; image_idx < origlistsize; image_idx++) {
        QImage *newimage = new QImage(m_antiGhostingMasksList.at(0)->copy(ca));
        if (newimage == NULL)
            exit(1); // TODO: exit gracefully
        m_antiGhostingMasksList.append(newimage);
        delete m_antiGhostingMasksList.takeAt(0);
    }
}

void EditingTools::computeAgMask()
{
    const int width = m_antiGhostingMasksList.at(0)->width();
    const int height = m_antiGhostingMasksList.at(0)->height();
    const int size = m_antiGhostingMasksList.size();
    QImage* tmp = m_previewWidget->getMask();
    delete m_antiGhostingMasksList[m_currentAgMaskIndex];
    m_antiGhostingMasksList.replace(m_currentAgMaskIndex, tmp);
    for (int h = 0; h < size; h++) {
        for (int j = 0; j < height; j++) {
            for (int i = 0; i < width; i++) {
                if (qAlpha(m_antiGhostingMasksList.at(h)->pixel(i,j)) != 0) 
                    m_antiGhostingMask->setPixel(i, j, m_antiGhostingMasksList.at(h)->pixel(i, j));
            }
        }   
    }
}

void EditingTools::nextClicked()
{
    Next_Finishbutton->setEnabled(false);
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    
    if (!m_imagesSaved)
        m_hcm->applyShiftsToItems(m_HV_offsets);
    if (m_doAutoAntighosting) {
        QStringList::ConstIterator it = m_fileList.begin();
        while( it != m_fileList.end() ) {
            m_HV_offsets.append(qMakePair(0,0));
            ++it;
        }
        float patchesPercent;
        m_agGoodImageIndex = m_hcm->computePatches(threshold_doubleSpinBox->value(), m_patches, patchesPercent, m_HV_offsets);
    }
    else if (m_agGoodImageIndex != -1) {
        computeAgMask();
        m_hcm->setAntiGhostingMask(m_antiGhostingMask);
        m_doManualAntighosting = true;
    }

    QApplication::restoreOverrideCursor();
    emit accept();
}

void EditingTools::updateMovable(int newidx)
{
    //inform display_widget of the change
    m_previewWidget->setMovable(m_originalImagesList[newidx], m_HV_offsets[newidx].first, m_HV_offsets[newidx].second);
    //prevent a change in the spinboxes to start a useless calculation
    horizShiftSB->blockSignals(true);
    horizShiftSB->setValue(m_HV_offsets[newidx].first);
    horizShiftSB->blockSignals(false);
    vertShiftSB->blockSignals(true);
    vertShiftSB->setValue(m_HV_offsets[newidx].second);
    vertShiftSB->blockSignals(false);
    m_previewWidget->updatePreviewImage();
    m_histogram->setData(m_originalImagesList[newidx]);
    m_histogram->update();
}

void EditingTools::updatePivot(int newidx) {
    m_previewWidget->setPivot(m_originalImagesList[newidx],m_HV_offsets[newidx].first, m_HV_offsets[newidx].second);
    m_previewWidget->updatePreviewImage();
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
    m_previewWidget->updatePreviewImage();
}
void EditingTools::horizShiftChanged(int v) {
    m_HV_offsets[movableListWidget->currentRow()].first=v;
    m_previewWidget->updateHorizShiftMovable(v);
    m_previewWidget->updatePreviewImage();
}

void EditingTools::resetCurrent() {
    horizShiftSB->setValue(0);
    vertShiftSB->setValue(0);
//    m_previewWidget->updateVertShiftMovable(0);
//    m_previewWidget->updateHorizShiftMovable(0);
}

void EditingTools::resetAll() {
    for (int i = 0; i < m_HV_offsets.size(); ++i) {
        m_HV_offsets[i].first=0;
        m_HV_offsets[i].second=0;
    }
    //prevent a change in the spinboxes to start a useless calculation
    //horizShiftSB->blockSignals(true);
    //vertShiftSB->blockSignals(true);
    resetCurrent(); //graphical update
    //horizShiftSB->blockSignals(false);
    //vertShiftSB->blockSignals(false);
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
    m_previewWidget->zoomIn();
}

void EditingTools::zoomOut() {
    m_previewWidget->zoomOut();
}

void EditingTools::fitPreview() {
    m_previewWidget->fitToWindow();
}

void EditingTools::fillPreview() {
    m_previewWidget->fillToWindow();
}
void EditingTools::origSize() {
    m_previewWidget->normalSize();
}

void EditingTools::antighostToolButtonToggled(bool toggled) {
    m_previewWidget->update();
    toggled ? m_previewWidget->setSelectionTool(false) : m_previewWidget->setSelectionTool(true);
    if (toggled) {
        m_antiGhosting = true;
        QImage* tmp = m_previewWidget->getMask();
        delete m_antiGhostingMasksList[m_currentAgMaskIndex];
        m_antiGhostingMasksList.replace(m_currentAgMaskIndex, tmp);
        m_currentAgMaskIndex = movableListWidget->currentRow();
        m_previewWidget->setMask(m_antiGhostingMasksList[m_currentAgMaskIndex]);
        m_previewWidget->hide();
        label_editable_list->setText(tr("Maskable"));
        label_reference_list->setText(tr("Good image"));
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
        if (m_agGoodImageIndex != -1) {
            m_previewWidget->show();
            prevBothButton->setDisabled(true);
            nextBothButton->setDisabled(false);
            referenceListWidget->addItem(QFileInfo(m_fileList[m_agGoodImageIndex]).fileName());
            referenceListWidget->setCurrentRow(0);
            //movableListWidget->setCurrentRow(0);
            updatePivot(m_agGoodImageIndex);
            QString filename = movableListWidget->currentItem()->text();
            int idx = m_filesMap[filename];
            updateMovable(idx);
            m_previewWidget->updatePreviewImage();
        }
        connect(movableListWidget,SIGNAL(currentRowChanged(int)),this,SLOT(updateAgMask(int)));
    }
    else {
        m_antiGhosting = false;
        m_previewWidget->show();
        disconnect(movableListWidget,SIGNAL(currentRowChanged(int)),this,SLOT(updateAgMask(int)));
        label_editable_list->setText(tr("Ed&itable"));
        label_reference_list->setText(tr("R&eference"));
        prevBothButton->setIcon(QIcon(":new/prefix1/images/uparrow.png"));  
        nextBothButton->setIcon(QIcon(":new/prefix1/images/downarrow.png"));    
        prevBothButton->setDisabled(false);
        nextBothButton->setDisabled(false);
        saveImagesButton->setDisabled(false);
        movableListWidget->clear();
        referenceListWidget->clear();
        disconnect(prevBothButton,SIGNAL(clicked()),this,SLOT(addGoodImage()));
        disconnect(nextBothButton,SIGNAL(clicked()),this,SLOT(removeGoodImage()));
        connect(prevBothButton,SIGNAL(clicked()),this,SLOT(prevBoth()));
        connect(nextBothButton,SIGNAL(clicked()),this,SLOT(nextBoth()));
        connect(movableListWidget,SIGNAL(currentRowChanged(int)),this,SLOT(updateMovable(int)));
        connect(referenceListWidget,SIGNAL(currentRowChanged(int)),this,SLOT(updatePivot(int)));
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
        m_previewWidget->setBrushColor(returned);
        maskColorButton->setStyleSheet(QString("background: rgb(%1,%2,%3)").arg(returned.red()).arg(returned.green()).arg(returned.blue()));
        m_luminanceOptions.setValue(KEY_MANUAL_AG_MASK_COLOR,returned.rgb());
    }
}

void EditingTools::lassoColorButtonClicked() {
    QColor returned = QColorDialog::getColor();
    if (returned.isValid()) {
        m_previewWidget->setLassoColor(returned);
        lassoColorButton->setStyleSheet(QString("background: rgb(%1,%2,%3)").arg(returned.red()).arg(returned.green()).arg(returned.blue()));
        m_luminanceOptions.setValue(KEY_MANUAL_AG_LASSO_COLOR,returned.rgb());
    }
}

void EditingTools::blendModeCBIndexChanged(int newindex) {
    maskColorButton->setVisible(newindex == 4);
    lassoColorButton->setVisible(newindex == 4);
    if (newindex == 4 && !m_antiGhosting)
        ;//m_agWidget->show();
    else if (newindex != 4 && !m_antiGhosting)
        ;//m_agWidget->hide();
}

void EditingTools::saveImagesButtonClicked() {
    saveImagesButton->setEnabled(false);
    Next_Finishbutton->setEnabled(false);
    QString fnameprefix=QFileDialog::getSaveFileName(
                this,
                tr("Choose a directory and a prefix"),
                m_luminanceOptions.getDefaultPathLdrIn());
    if (fnameprefix.isEmpty())
        return;

    QFileInfo qfi(fnameprefix);
    QFileInfo test(qfi.path());

    m_luminanceOptions.setDefaultPathLdrIn(qfi.path());

    if (test.isWritable() && test.exists() && test.isDir()) {
        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        m_hcm->applyShiftsToItems(m_HV_offsets);
        m_hcm->saveImages(QFile::encodeName((qfi.path() + "/" + qfi.fileName())));
    }
}

void EditingTools::restoreSaveImagesButtonState()
{
    m_imagesSaved = true;
    saveImagesButton->setEnabled(true);
    Next_Finishbutton->setEnabled(true);
    QApplication::restoreOverrideCursor();

    m_originalImagesList.clear();
    HdrCreationItemContainer data = m_hcm->getData();
    for ( HdrCreationItemContainer::iterator it = data.begin(), 
          itEnd = data.end(); it != itEnd; ++it) {
        m_originalImagesList.push_back(it->qimage());
    }
    
    m_previewWidget->setMovable(m_originalImagesList[movableListWidget->currentRow()]);
    m_previewWidget->setPivot(m_originalImagesList[referenceListWidget->currentRow()]);
}

void EditingTools::setAntiGhostingWidget(QImage *mask, QPair<int, int> HV_offset)
{
    m_previewWidget->setMask(mask);
    m_previewWidget->setHV_offset(HV_offset);
}

void EditingTools::addGoodImage()
{
    QString filename = movableListWidget->currentItem()->text();
    int idx = m_filesMap[filename];
    int idxGoodImage = movableListWidget->currentRow();
    referenceListWidget->addItem(QFileInfo(m_fileList[idx]).fileName());
    referenceListWidget->setCurrentRow(0);
    m_agGoodImageIndex = idx;
    m_antiGhostingMasksList[idx]->fill(qRgba(0,0,0,0));
    movableListWidget->item(idxGoodImage)->setBackground(QColor(Qt::yellow));

    prevBothButton->setDisabled(true);
    nextBothButton->setDisabled(false);
    updatePivot(idx);
    m_previewWidget->show();
}

void EditingTools::removeGoodImage()
{
    QString filename = referenceListWidget->currentItem()->text();
    int idx = m_filesMap[filename];
    movableListWidget->item(idx)->setBackground(QColor(Qt::white));
    referenceListWidget->takeItem(0);
    prevBothButton->setDisabled(false);
    nextBothButton->setDisabled(true);
    m_agGoodImageIndex = -1;
    m_previewWidget->hide();
}

void EditingTools::updateAgMask(int)
{
    QString filename = movableListWidget->currentItem()->text();
    int idx = m_filesMap[filename];
    QImage* tmp = m_previewWidget->getMask();
    delete m_antiGhostingMasksList[m_currentAgMaskIndex];
    m_antiGhostingMasksList.replace(m_currentAgMaskIndex, tmp);
    m_currentAgMaskIndex = idx;
    setAntiGhostingWidget(m_antiGhostingMasksList[idx], m_HV_offsets[idx]);
    updateMovable(idx);
}

void EditingTools::saveAgMask()
{
    toolButtonApplyMask->setEnabled(true);
}

void EditingTools::applySavedAgMask()
{
    QString filename = movableListWidget->currentItem()->text();
    int idx = m_filesMap[filename];
    delete m_antiGhostingMasksList[idx];
    m_antiGhostingMasksList[idx] = new QImage(*m_previewWidget->getSavedAgMask());    
    m_previewWidget->setMask(m_antiGhostingMasksList[idx]);
    m_previewWidget->updatePreviewImage();
}

void EditingTools::antighostToolButtonPaintToggled(bool toggled)
{
    (toggled) ? m_previewWidget->setDrawWithBrush() :  m_previewWidget->setDrawPath();
}

void EditingTools::on_recomputePatches_pushButton_clicked()
{
    float patchesPercent;
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    recomputePatches_pushButton->setEnabled(false);
    m_agGoodImageIndex = m_hcm->computePatches(threshold_doubleSpinBox->value(), m_patches, patchesPercent, m_HV_offsets);
    m_previewWidget->switchViewPatchesMode(true);
    m_previewWidget->renderPatchesMask(m_patches, m_gridX, m_gridY);
    totalPatches_lineEdit->setText(QString::number(patchesPercent,'g', 2)+"%");
    recomputePatches_pushButton->setEnabled(true);
    QApplication::restoreOverrideCursor();
}

void EditingTools::on_autoAG_checkBox_toggled(bool toggled)
{
    if (toggled) {
        m_doAutoAntighosting = true;
        antighostToolButton->setEnabled(false);
        m_previewWidget->switchViewPatchesMode(true);
    }
    else {
        m_doAutoAntighosting = false;
        antighostToolButton->setEnabled(true);
        m_previewWidget->switchViewPatchesMode(false);
    }
}

void EditingTools::updateThresholdSlider(int newValue)
{
    float newThreshold = ((float)newValue)/10000.f;
    bool oldState = threshold_doubleSpinBox->blockSignals(true);
    threshold_doubleSpinBox->setValue( newThreshold );
    threshold_doubleSpinBox->blockSignals(oldState);
}

void EditingTools::updateThresholdSpinBox(double newThreshold)
{
    bool oldState = threshold_horizontalSlider->blockSignals(true);
    threshold_horizontalSlider->setValue( (int)(newThreshold*10000) );
    threshold_horizontalSlider->blockSignals(oldState);
}

