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
 * Improvements, bugfixing, anti-ghosting, new preview widget based on QGraphicsView
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#include <QFileInfo>
#include <cassert>
#include <QWhatsThis>
#include <QColorDialog>
#include <QFileDialog>
#include <QDebug>

#include "HdrWizard/ui_EditingTools.h"
#include "Common/config.h"
#include "UI/GammaAndLevels.h"
#include "Viewers/PanIconWidget.h"
#include "HdrWizard/EditingTools.h"
#include "Exif/ExifOperations.h"
#include "HdrCreation/mtb_alignment.h"

EditingTools::EditingTools(HdrCreationManager *hcm, bool autoAg, QWidget *parent) :
    QDialog(parent),
    m_Ui(new Ui::EditingToolsDialog),
    m_currentAgMaskIndex(0),
    m_hcm(hcm),
    m_additionalShiftValue(0),
    m_imagesSaved(false),
    m_agGoodImageIndex(-1),
    m_antiGhosting(false),
    m_doAutoAntighosting(autoAg),
    m_doManualAntighosting(false),
    m_patchesEdited(false)
{
    m_Ui->setupUi(this);

    if ( !QIcon::hasThemeIcon(QStringLiteral("edit-select-lasso")) )
    {
        m_Ui->lassoColorButton->setIcon(QIcon(":/program-icons/edit-select-lasso"));
        m_Ui->toolButtonPath->setIcon(QIcon(":/program-icons/edit-select-lasso"));
    }
    if ( !QIcon::hasThemeIcon(QStringLiteral("draw-brush")) )
    {
        m_Ui->maskColorButton->setIcon(QIcon(":/program-icons/draw-brush"));
        m_Ui->toolButtonPaint->setIcon(QIcon(":/program-icons/draw-brush"));
    }
    if ( !QIcon::hasThemeIcon(QStringLiteral("zoom")) )
        m_Ui->fillButton->setIcon(QIcon(":/program-icons/zoom"));
    if ( !QIcon::hasThemeIcon(QStringLiteral("help-whatsthis")) )
        m_Ui->whatsThisButton->setIcon(QIcon(":/program-icons/help-whatsthis"));
    if ( !QIcon::hasThemeIcon(QStringLiteral("transform-crop-and-resize")) )
        m_Ui->cropButton->setIcon(QIcon(":/program-icons/transform-crop-and-resize"));
    if ( !QIcon::hasThemeIcon(QStringLiteral("dialog-ok-apply")) )
        m_Ui->toolButtonApplyMask->setIcon(QIcon(":/program-icons/dialog-ok-apply"));
    if ( !QIcon::hasThemeIcon(QStringLiteral("arrow-up")) )
        m_Ui->prevBothButton->setIcon(QIcon(":/program-icons/uparrow"));
    if ( !QIcon::hasThemeIcon(QStringLiteral("arrow-down")) )
        m_Ui->nextBothButton->setIcon(QIcon(":/program-icons/downarrow"));

    for (int i = 0; i < agGridSize; i++)
        for (int j = 0; j < agGridSize; j++)
            m_patches[i][j] = false;

    HdrCreationItemContainer data = m_hcm->getData();
    for ( HdrCreationItemContainer::iterator it = data.begin(),
          itEnd = data.end(); it != itEnd; ++it) {
        m_originalImagesList.push_back(&it->qimage());
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

    //m_Ui->toolOptionsFrame->setVisible(false);
    m_Ui->maskColorButton->setVisible(false);
    m_Ui->lassoColorButton->setVisible(false);

    QColor maskcolor = QColor(m_luminanceOptions.value(KEY_MANUAL_AG_MASK_COLOR,0x00FF0000).toUInt());
    QColor lassocolor = QColor(m_luminanceOptions.value(KEY_MANUAL_AG_LASSO_COLOR,0x000000FF).toUInt());
    Qt::ToolButtonStyle style = (Qt::ToolButtonStyle) m_luminanceOptions.value(KEY_TOOLBAR_MODE,Qt::ToolButtonTextUnderIcon).toInt();
    m_Ui->maskColorButton->setStyleSheet("background: rgb("+QStringLiteral("%1").arg(maskcolor.red())+","+QStringLiteral("%1").arg(maskcolor.green())+","+QStringLiteral("%1").arg(maskcolor.blue())+")");
    m_Ui->lassoColorButton->setStyleSheet("background: rgb("+QStringLiteral("%1").arg(lassocolor.red())+","+QStringLiteral("%1").arg(lassocolor.green())+","+QStringLiteral("%1").arg(lassocolor.blue())+")");
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
    m_Ui->previewImageFrame->setLayout(qvl);

    int idx = 0;
    foreach(QString s,m_fileList) {
        m_filesMap[QFileInfo(s).fileName()] = idx++;
        m_Ui->movableListWidget->addItem(QFileInfo(s).fileName());
        m_Ui->referenceListWidget->addItem(QFileInfo(s).fileName());
    }
    m_Ui->movableListWidget->setCurrentRow(1);
    m_Ui->referenceListWidget->setCurrentRow(0);

    m_Ui->fitButton->setToolButtonStyle(style);
    m_Ui->origSizeButton->setToolButtonStyle(style);
    m_Ui->fillButton->setToolButtonStyle(style);
    m_Ui->whatsThisButton->setToolButtonStyle(style);
    m_Ui->cropButton->setToolButtonStyle(style);
    m_Ui->saveImagesButton->setToolButtonStyle(style);
    m_Ui->antighostToolButton->setToolButtonStyle(style);
    m_Ui->toolButtonPaint->setToolButtonStyle(style);
    m_Ui->toolButtonPath->setToolButtonStyle(style);

    //m_Ui->drawingModeFrame->hide();

    QStringList::ConstIterator it = m_fileList.begin();
    while( it != m_fileList.constEnd() ) {
        m_HV_offsets.append(qMakePair(0,0));
        ++it;
    }

    m_histogram=new HistogramLDR(this);
    m_histogram->setData( m_originalImagesList.at(1) );
    m_histogram->adjustSize();

    ((QGridLayout*)(m_Ui->groupBoxHistogram->layout()))->addWidget(m_histogram);
    m_previewWidget->setFocus();

    m_Ui->autoAG_checkBox->setChecked(m_doAutoAntighosting);

    setupConnections();
} //end of constructor

void EditingTools::setupConnections() {
    connect(m_Ui->upToolButton,&QAbstractButton::clicked,this,&EditingTools::upClicked);
    connect(m_Ui->rightToolButton,&QAbstractButton::clicked,this,&EditingTools::rightClicked);
    connect(m_Ui->downToolButton,&QAbstractButton::clicked,this,&EditingTools::downClicked);
    connect(m_Ui->leftToolButton,&QAbstractButton::clicked,this,&EditingTools::leftClicked);
    connect(m_Ui->horizShiftSB,SIGNAL(valueChanged(int)),this,SLOT(horizShiftChanged(int)));
    connect(m_Ui->vertShiftSB,SIGNAL(valueChanged(int)),this,SLOT(vertShiftChanged(int)));
    connect(m_Ui->resetButton,&QAbstractButton::clicked,this,&EditingTools::resetCurrent);
    connect(m_Ui->resetAllButton,&QAbstractButton::clicked,this,&EditingTools::resetAll);

    connect(m_Ui->movableListWidget,&QListWidget::currentRowChanged,this,&EditingTools::updateMovable);
    connect(m_Ui->referenceListWidget,&QListWidget::currentRowChanged,this,&EditingTools::updatePivot);
    connect(m_Ui->prevBothButton,&QAbstractButton::clicked,this,&EditingTools::prevBoth);
    connect(m_Ui->nextBothButton,&QAbstractButton::clicked,this,&EditingTools::nextBoth);

    connect(m_Ui->whatsThisButton,&QAbstractButton::clicked,this,&EditingTools::enterWhatsThis);
    connect(m_Ui->fitButton,&QAbstractButton::clicked,this,&EditingTools::fitPreview);
    connect(m_Ui->origSizeButton,&QAbstractButton::clicked,this,&EditingTools::origSize);
    connect(m_Ui->fillButton,&QAbstractButton::clicked,this,&EditingTools::fillPreview);
    connect(m_Ui->cropButton,&QAbstractButton::clicked,this,&EditingTools::cropStack);
    connect(m_previewWidget,&PreviewWidget::selectionReady,m_Ui->cropButton,&QWidget::setEnabled);
    connect(m_previewWidget,&PreviewWidget::patchesEdited,this,&EditingTools::setPatchesEdited);
    connect(m_Ui->saveImagesButton,&QAbstractButton::clicked,this,&EditingTools::saveImagesButtonClicked);
    connect(m_Ui->blendModeCB,SIGNAL(currentIndexChanged(int)),m_previewWidget,SLOT(requestedBlendMode(int)));
    connect(m_Ui->blendModeCB,SIGNAL(currentIndexChanged(int)),this,SLOT(blendModeCBIndexChanged(int)));
    //connect(antighostToolButton,SIGNAL(toggled(bool)),toolOptionsFrame,SLOT(setVisible(bool)));
    //connect(m_Ui->antighostToolButton,SIGNAL(toggled(bool)),m_Ui->drawingModeFrame,SLOT(setVisible(bool)));
    connect(m_Ui->antighostToolButton,&QAbstractButton::toggled,m_previewWidget,&PreviewWidget::switchAntighostingMode);
    connect(m_Ui->antighostToolButton,&QAbstractButton::toggled,this,&EditingTools::antighostToolButtonToggled);
    connect(m_Ui->toolButtonPaint,&QAbstractButton::toggled,this,&EditingTools::antighostToolButtonPaintToggled);
    connect(m_Ui->agBrushSizeQSpinbox,SIGNAL(valueChanged(int)),m_previewWidget,SLOT(setBrushSize(int)));
    connect(m_Ui->agBrushStrengthQSpinbox,SIGNAL(valueChanged(int)),m_previewWidget,SLOT(setBrushStrength(int)));
    connect(m_Ui->maskColorButton,&QAbstractButton::clicked,this,&EditingTools::maskColorButtonClicked);
    connect(m_Ui->lassoColorButton,&QAbstractButton::clicked,this,&EditingTools::lassoColorButtonClicked);
    connect(m_Ui->toolButtonSaveMask,&QAbstractButton::clicked,m_previewWidget,&PreviewWidget::saveAgMask);
    connect(m_Ui->toolButtonSaveMask,&QAbstractButton::clicked,this,&EditingTools::saveAgMask);
    connect(m_Ui->toolButtonApplyMask,&QAbstractButton::clicked,this,&EditingTools::applySavedAgMask);
    connect(m_Ui->threshold_horizontalSlider, &QAbstractSlider::valueChanged, this, &EditingTools::updateThresholdSlider);
    connect(m_Ui->threshold_doubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateThresholdSpinBox(double)));

    connect(m_Ui->Next_Finishbutton,&QAbstractButton::clicked,this,&EditingTools::nextClicked);
    //connect(m_previewWidget, SIGNAL(moved(QPoint)), this, SLOT(updateScrollBars(QPoint)));
    connect(m_Ui->removeMaskRadioButton,&QAbstractButton::toggled,m_previewWidget,&PreviewWidget::setBrushMode);

    connect(m_hcm, &HdrCreationManager::imagesSaved, this, &EditingTools::restoreSaveImagesButtonState);
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
    for (HdrCreationItemContainer::iterator it = data.begin(),
         itEnd = data.end(); it != itEnd; ++it)
    {
        m_originalImagesList.push_back(&it->qimage());
    }
    int width = m_originalImagesList.at(0)->width();
    int height = m_originalImagesList.at(0)->height();
    m_gridX = width/agGridSize;
    m_gridY = height/agGridSize;

    m_previewWidget->removeSelection();
    m_previewWidget->setMovable(m_originalImagesList[m_Ui->movableListWidget->currentRow()]);
    m_previewWidget->setPivot(m_originalImagesList[m_Ui->referenceListWidget->currentRow()]);
    m_currentAgMaskIndex = m_Ui->movableListWidget->currentRow();
    m_previewWidget->setMask(m_antiGhostingMasksList[m_currentAgMaskIndex]);
    //restore fit
    if (m_Ui->fitButton->isChecked())
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
    m_Ui->Next_Finishbutton->setEnabled(false);
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    if (!m_imagesSaved)
        m_hcm->applyShiftsToItems(m_HV_offsets);
    if (m_doAutoAntighosting) {
        QStringList::ConstIterator it = m_fileList.begin();
        while( it != m_fileList.constEnd() ) {
            m_HV_offsets.append(qMakePair(0,0));
            ++it;
        }
        if (m_patchesEdited) {
            m_previewWidget->getPatches(m_patches);
            m_hcm->setPatches(m_patches);
        }
        else {
            float patchesPercent;
            m_agGoodImageIndex = m_hcm->computePatches(m_Ui->threshold_doubleSpinBox->value(), m_patches, patchesPercent, m_HV_offsets);
        }
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
    m_Ui->horizShiftSB->blockSignals(true);
    m_Ui->horizShiftSB->setValue(m_HV_offsets[newidx].first);
    m_Ui->horizShiftSB->blockSignals(false);
    m_Ui->vertShiftSB->blockSignals(true);
    m_Ui->vertShiftSB->setValue(m_HV_offsets[newidx].second);
    m_Ui->vertShiftSB->blockSignals(false);
    m_previewWidget->updatePreviewImage();
    m_histogram->setData(m_originalImagesList[newidx]);
    m_histogram->update();
}

void EditingTools::updatePivot(int newidx) {
    m_previewWidget->setPivot(m_originalImagesList[newidx],m_HV_offsets[newidx].first, m_HV_offsets[newidx].second);
    m_previewWidget->updatePreviewImage();
}

void EditingTools::upClicked() {
    m_Ui->vertShiftSB->setValue(m_Ui->vertShiftSB->value()-1-m_additionalShiftValue);
}
void EditingTools::downClicked() {
    m_Ui->vertShiftSB->setValue(m_Ui->vertShiftSB->value()+1+m_additionalShiftValue);
}
void EditingTools::rightClicked() {
    m_Ui->horizShiftSB->setValue(m_Ui->horizShiftSB->value()+1+m_additionalShiftValue);
}
void EditingTools::leftClicked() {
    m_Ui->horizShiftSB->setValue(m_Ui->horizShiftSB->value()-1-m_additionalShiftValue);
}

void EditingTools::vertShiftChanged(int v) {
    m_HV_offsets[m_Ui->movableListWidget->currentRow()].second=v;
    m_previewWidget->updateVertShiftMovable(v);
    m_previewWidget->updatePreviewImage();
}
void EditingTools::horizShiftChanged(int v) {
    m_HV_offsets[m_Ui->movableListWidget->currentRow()].first=v;
    m_previewWidget->updateHorizShiftMovable(v);
    m_previewWidget->updatePreviewImage();
}

void EditingTools::resetCurrent() {
    m_Ui->horizShiftSB->setValue(0);
    m_Ui->vertShiftSB->setValue(0);
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
    int prev=(m_Ui->movableListWidget->currentRow()==0) ? m_Ui->movableListWidget->count()-1 : m_Ui->movableListWidget->currentRow()-1;
    m_Ui->movableListWidget->setCurrentRow(prev);
}

void EditingTools::nextLeft() {
    int next=(m_Ui->movableListWidget->currentRow()==m_Ui->movableListWidget->count()-1) ? 0 : m_Ui->movableListWidget->currentRow()+1;
    m_Ui->movableListWidget->setCurrentRow(next);
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
    int prev=(m_Ui->referenceListWidget->currentRow()==0) ? m_Ui->referenceListWidget->count()-1 : m_Ui->referenceListWidget->currentRow()-1;
    m_Ui->referenceListWidget->setCurrentRow(prev);
}

void EditingTools::nextRight() {
    int next=(m_Ui->referenceListWidget->currentRow()==m_Ui->referenceListWidget->count()-1) ? 0 : m_Ui->referenceListWidget->currentRow()+1;
    m_Ui->referenceListWidget->setCurrentRow(next);
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
        m_Ui->stackedWidget->setCurrentIndex(1);
        m_antiGhosting = true;
        QImage* tmp = m_previewWidget->getMask();
        delete m_antiGhostingMasksList[m_currentAgMaskIndex];
        m_antiGhostingMasksList.replace(m_currentAgMaskIndex, tmp);
        m_currentAgMaskIndex = m_Ui->movableListWidget->currentRow();
        m_previewWidget->setMask(m_antiGhostingMasksList[m_currentAgMaskIndex]);
        m_previewWidget->hide();
        m_Ui->label_editable_list->setText(tr("Maskable"));
        m_Ui->label_reference_list->setText(tr("Good image"));
        m_Ui->saveImagesButton->setDisabled(true);
        m_Ui->prevBothButton->setIcon(QIcon::fromTheme(QStringLiteral("go-next"), QIcon(":/program-icons/go-next")));
        m_Ui->nextBothButton->setIcon(QIcon::fromTheme(QStringLiteral("go-previous"), QIcon(":/program-icons/go-previous")));
        m_Ui->prevBothButton->setToolTip(tr("Add good image"));
        m_Ui->nextBothButton->setToolTip(tr("Remove good image"));
        m_Ui->nextBothButton->setDisabled(true);
        disconnect(m_Ui->prevBothButton,&QAbstractButton::clicked,this,&EditingTools::prevBoth);
        disconnect(m_Ui->nextBothButton,&QAbstractButton::clicked,this,&EditingTools::nextBoth);
        disconnect(m_Ui->movableListWidget,&QListWidget::currentRowChanged,this,&EditingTools::updateMovable);
        disconnect(m_Ui->referenceListWidget,&QListWidget::currentRowChanged,this,&EditingTools::updatePivot);
        connect(m_Ui->prevBothButton,&QAbstractButton::clicked,this,&EditingTools::addGoodImage);
        connect(m_Ui->nextBothButton,&QAbstractButton::clicked,this,&EditingTools::removeGoodImage);
        m_Ui->referenceListWidget->clear();
        if (m_agGoodImageIndex != -1) {
            m_previewWidget->show();
            m_Ui->prevBothButton->setDisabled(true);
            m_Ui->nextBothButton->setDisabled(false);
            m_Ui->referenceListWidget->addItem(QFileInfo(m_fileList[m_agGoodImageIndex]).fileName());
            m_Ui->referenceListWidget->setCurrentRow(0);
            //movableListWidget->setCurrentRow(0);
            updatePivot(m_agGoodImageIndex);
            QString filename = m_Ui->movableListWidget->currentItem()->text();
            int idx = m_filesMap[filename];
            updateMovable(idx);
            m_previewWidget->updatePreviewImage();
        }
        connect(m_Ui->movableListWidget,&QListWidget::currentRowChanged,this,&EditingTools::updateAgMask);
    }
    else {
        m_Ui->stackedWidget->setCurrentIndex(0);
        m_antiGhosting = false;
        m_previewWidget->show();
        disconnect(m_Ui->movableListWidget,&QListWidget::currentRowChanged,this,&EditingTools::updateAgMask);
        m_Ui->label_editable_list->setText(tr("Ed&itable"));
        m_Ui->label_reference_list->setText(tr("R&eference"));
        m_Ui->prevBothButton->setIcon(QIcon::fromTheme(QStringLiteral("arrow-up"), QIcon(":/program-icons/uparrow")));
        m_Ui->nextBothButton->setIcon(QIcon::fromTheme(QStringLiteral("arrow-down"), QIcon(":/program-icons/downarrow")));
        m_Ui->prevBothButton->setToolTip(tr("Select the previous image in both lists"));
        m_Ui->nextBothButton->setToolTip(tr("Select the next image in both lists"));
        m_Ui->prevBothButton->setDisabled(false);
        m_Ui->nextBothButton->setDisabled(false);
        m_Ui->saveImagesButton->setDisabled(false);
        m_Ui->movableListWidget->clear();
        m_Ui->referenceListWidget->clear();
        disconnect(m_Ui->prevBothButton,&QAbstractButton::clicked,this,&EditingTools::addGoodImage);
        disconnect(m_Ui->nextBothButton,&QAbstractButton::clicked,this,&EditingTools::removeGoodImage);
        connect(m_Ui->prevBothButton,&QAbstractButton::clicked,this,&EditingTools::prevBoth);
        connect(m_Ui->nextBothButton,&QAbstractButton::clicked,this,&EditingTools::nextBoth);
        connect(m_Ui->movableListWidget,&QListWidget::currentRowChanged,this,&EditingTools::updateMovable);
        connect(m_Ui->referenceListWidget,&QListWidget::currentRowChanged,this,&EditingTools::updatePivot);
        foreach(QString s,m_fileList) {
            m_Ui->movableListWidget->addItem(QFileInfo(s).fileName());
            m_Ui->referenceListWidget->addItem(QFileInfo(s).fileName());
        }
        m_Ui->movableListWidget->setCurrentRow(1);
        m_Ui->referenceListWidget->setCurrentRow(0);
    }
}

void EditingTools::maskColorButtonClicked() {
    QColor returned = QColorDialog::getColor();
    if (returned.isValid()) {
        m_previewWidget->setBrushColor(returned);
        m_Ui->maskColorButton->setStyleSheet(QStringLiteral("background: rgb(%1,%2,%3)").arg(returned.red()).arg(returned.green()).arg(returned.blue()));
        m_luminanceOptions.setValue(KEY_MANUAL_AG_MASK_COLOR,returned.rgb());
    }
}

void EditingTools::lassoColorButtonClicked() {
    QColor returned = QColorDialog::getColor();
    if (returned.isValid()) {
        m_previewWidget->setLassoColor(returned);
        m_Ui->lassoColorButton->setStyleSheet(QStringLiteral("background: rgb(%1,%2,%3)").arg(returned.red()).arg(returned.green()).arg(returned.blue()));
        m_luminanceOptions.setValue(KEY_MANUAL_AG_LASSO_COLOR,returned.rgb());
    }
}

void EditingTools::blendModeCBIndexChanged(int newindex) {
    m_Ui->maskColorButton->setVisible(newindex == 4);
    m_Ui->lassoColorButton->setVisible(newindex == 4);
    if (newindex == 4 && !m_antiGhosting)
        ;//m_agWidget->show();
    else if (newindex != 4 && !m_antiGhosting)
        ;//m_agWidget->hide();
}

void EditingTools::saveImagesButtonClicked() {
    m_Ui->saveImagesButton->setEnabled(false);
    m_Ui->Next_Finishbutton->setEnabled(false);
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
    m_Ui->saveImagesButton->setEnabled(true);
    m_Ui->Next_Finishbutton->setEnabled(true);
    QApplication::restoreOverrideCursor();

    m_originalImagesList.clear();
    HdrCreationItemContainer data = m_hcm->getData();
    for ( HdrCreationItemContainer::iterator it = data.begin(),
          itEnd = data.end(); it != itEnd; ++it) {
        m_originalImagesList.push_back(&it->qimage());
    }

    m_previewWidget->setMovable(m_originalImagesList[m_Ui->movableListWidget->currentRow()]);
    m_previewWidget->setPivot(m_originalImagesList[m_Ui->referenceListWidget->currentRow()]);
}

void EditingTools::setAntiGhostingWidget(QImage *mask, QPair<int, int> HV_offset)
{
    m_previewWidget->setMask(mask);
    m_previewWidget->setHV_offset(HV_offset);
}

void EditingTools::addGoodImage()
{
    QString filename = m_Ui->movableListWidget->currentItem()->text();
    int idx = m_filesMap[filename];
    int idxGoodImage = m_Ui->movableListWidget->currentRow();
    m_Ui->referenceListWidget->addItem(QFileInfo(m_fileList[idx]).fileName());
    m_Ui->referenceListWidget->setCurrentRow(0);
    m_agGoodImageIndex = idx;
    m_antiGhostingMasksList[idx]->fill(qRgba(0,0,0,0));
    m_Ui->movableListWidget->item(idxGoodImage)->setBackground(QColor(Qt::yellow));

    m_Ui->prevBothButton->setDisabled(true);
    m_Ui->nextBothButton->setDisabled(false);
    updatePivot(idx);
    m_previewWidget->show();
}

void EditingTools::removeGoodImage()
{
    QString filename = m_Ui->referenceListWidget->currentItem()->text();
    int idx = m_filesMap[filename];
    m_Ui->movableListWidget->item(idx)->setBackground(QColor(Qt::white));
    m_Ui->referenceListWidget->takeItem(0);
    m_Ui->prevBothButton->setDisabled(false);
    m_Ui->nextBothButton->setDisabled(true);
    m_agGoodImageIndex = -1;
    m_previewWidget->hide();
}

void EditingTools::updateAgMask(int)
{
    QString filename = m_Ui->movableListWidget->currentItem()->text();
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
    m_Ui->toolButtonApplyMask->setEnabled(true);
}

void EditingTools::applySavedAgMask()
{
    QString filename = m_Ui->movableListWidget->currentItem()->text();
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
    m_patchesEdited = false;
    m_Ui->recomputePatches_pushButton->setEnabled(false);
    m_agGoodImageIndex = m_hcm->computePatches(m_Ui->threshold_doubleSpinBox->value(), m_patches, patchesPercent, m_HV_offsets);
    m_previewWidget->switchViewPatchesMode(true, m_patches, m_gridX, m_gridY);
    m_previewWidget->renderPatchesMask();
    m_Ui->totalPatches_lineEdit->setText(QString::number(patchesPercent,'g', 2)+"%");
    m_Ui->recomputePatches_pushButton->setEnabled(true);
    QApplication::restoreOverrideCursor();
}

void EditingTools::on_autoAG_checkBox_toggled(bool toggled)
{
    if (toggled) {
        m_doAutoAntighosting = true;
        m_patchesEdited = false;
        m_Ui->antighostToolButton->setEnabled(false);
        m_previewWidget->switchViewPatchesMode(true, m_patches, m_gridX, m_gridY);
    }
    else {
        m_doAutoAntighosting = false;
        m_Ui->antighostToolButton->setEnabled(true);
        m_previewWidget->switchViewPatchesMode(false, m_patches, m_gridX, m_gridY);
        m_agGoodImageIndex = -1;
    }
}

void EditingTools::updateThresholdSlider(int newValue)
{
    float newThreshold = ((float)newValue)/10000.f;
    bool oldState = m_Ui->threshold_doubleSpinBox->blockSignals(true);
    m_Ui->threshold_doubleSpinBox->setValue( newThreshold );
    m_Ui->threshold_doubleSpinBox->blockSignals(oldState);
}

void EditingTools::updateThresholdSpinBox(double newThreshold)
{
    bool oldState = m_Ui->threshold_horizontalSlider->blockSignals(true);
    m_Ui->threshold_horizontalSlider->setValue( (int)(newThreshold*10000) );
    m_Ui->threshold_horizontalSlider->blockSignals(oldState);
}

void EditingTools::setPatchesEdited()
{
    m_patchesEdited = true;
}
