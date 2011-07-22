/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2009 Franco Comida
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
 * Improvements, bugfixing
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#include <QMessageBox>

#include "GenericViewer.h"
#include "UI/UMessageBox.h"

GenericViewer::GenericViewer(QWidget *parent, bool ns, bool ncf):
        QWidget(parent), NeedsSaving(ns), noCloseFlag(ncf)
{
    // Set Minimum Dimension of the Viewer Window
    // setMinimumSize(450, 300);

    VBL_L = new QVBoxLayout(this);
    VBL_L->setSpacing(0);
    VBL_L->setMargin(0);

    toolBar = new QToolBar("",this);
    toolBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    toolBar->setFixedHeight(40);
    VBL_L->addWidget(toolBar);

    scrollArea = new SmartScrollArea(this, imageLabel);
    scrollArea->setBackgroundRole(QPalette::Shadow);
    VBL_L->addWidget(scrollArea);

    cornerButton = new QToolButton(this);
    cornerButton->setToolTip(tr("Pan the image to a region"));
    cornerButton->setIcon(QIcon(":/new/prefix1/images/move.png"));
    scrollArea->setCornerWidget(cornerButton);
    connect(cornerButton, SIGNAL(pressed()), this, SLOT(slotCornerButtonPressed()));
    connect(scrollArea, SIGNAL(selectionReady(bool)), this, SIGNAL(selectionReady(bool)));
    connect(scrollArea, SIGNAL(changed(void)), this, SLOT(route_changed(void)));

    statusBar = new QStatusBar(this);
    statusBar->setSizeGripEnabled(false);
    VBL_L->addWidget(statusBar);
}

GenericViewer::~GenericViewer()
{
    if ( m_image != NULL ) delete m_image;
    this->deleteLater();
}

void GenericViewer::setLabelPixmap(const QPixmap pix)
{
	imageLabel.setPixmap(pix);
	imageLabel.adjustSize();
}

void GenericViewer::fitToWindow(bool checked)
{
	scrollArea->fitToWindow(checked);
	emit changed(this);
}

void GenericViewer::zoomIn()
{
	scrollArea->zoomIn();
	emit changed(this);
}

void GenericViewer::zoomOut()
{
	scrollArea->zoomOut();
	emit changed(this);
}

void GenericViewer::zoomToFactor(float factor)
{
	scrollArea->zoomToFactor(factor);
	emit changed(this);
}

void GenericViewer::normalSize()
{
	scrollArea->normalSize();
	emit changed(this);
}

bool GenericViewer::isFittedToWindow()
{
	return scrollArea->isFittedToWindow();
}

float GenericViewer::getScaleFactor()
{
	return scrollArea->getScaleFactor();
}

const QRect GenericViewer::getSelectionRect(void)
{
	return scrollArea->getSelectionRect();
}

void GenericViewer::setSelectionTool(bool toggled)
{
	scrollArea->setSelectionTool( toggled );
}

void GenericViewer::removeSelection(void)
{
	scrollArea->removeSelection();
}

bool GenericViewer::hasSelection(void)
{
	return scrollArea->hasSelection();
}

bool GenericViewer::needsSaving(void)
{
	return NeedsSaving;
}

void GenericViewer::setNeedsSaving(bool s)
{
	NeedsSaving = s;
}

const QString GenericViewer::getFileName(void)
{
	return filename;
}

void GenericViewer::setFileName(const QString fn)
{
	filename = fn;
}

void GenericViewer::slotCornerButtonPressed()
{
    panIconWidget=new PanIconWidget(this);
    panIconWidget->setImage(m_image);

    float zf=scrollArea->getScaleFactor();
    float leftviewpos=(float)(scrollArea->horizontalScrollBar()->value());
    float topviewpos=(float)(scrollArea->verticalScrollBar()->value());
    float wps_w=(float)(scrollArea->maximumViewportSize().width());
    float wps_h=(float)(scrollArea->maximumViewportSize().height());
    QRect r((int)(leftviewpos/zf), (int)(topviewpos/zf), (int)(wps_w/zf), (int)(wps_h/zf));
    panIconWidget->setRegionSelection(r);
    panIconWidget->setMouseFocus();
    connect(panIconWidget, SIGNAL(selectionMoved(QRect)), this, SLOT(slotPanIconSelectionMoved(QRect)));
    connect(panIconWidget, SIGNAL(finished()), this, SLOT(slotPanIconHidden()));
    QPoint g = scrollArea->mapToGlobal(scrollArea->viewport()->pos());
    g.setX(g.x()+ scrollArea->viewport()->size().width());
    g.setY(g.y()+ scrollArea->viewport()->size().height());
    panIconWidget->popup(QPoint(g.x() - panIconWidget->width()/2, g.y() - panIconWidget->height()/2));
    panIconWidget->setCursorToLocalRegionSelectionCenter();
}

void GenericViewer::slotPanIconSelectionMoved(QRect gotopos )
{
    scrollArea->horizontalScrollBar()->setValue((int)(gotopos.x()*scrollArea->getScaleFactor()));
    scrollArea->verticalScrollBar()->setValue((int)(gotopos.y()*scrollArea->getScaleFactor()));
    emit changed(this);
}

void GenericViewer::slotPanIconHidden()
{
    panIconWidget->close();
    cornerButton->blockSignals(true);
    cornerButton->animateClick();
    cornerButton->blockSignals(false);
}

int  GenericViewer::getHorizScrollBarValue()
{
    return scrollArea->getHorizScrollBarValue();
}

int  GenericViewer::getVertScrollBarValue()
{
    return scrollArea->getVertScrollBarValue();
}

float  GenericViewer::getImageScaleFactor()
{
    return scrollArea->getImageScaleFactor();
}

void GenericViewer::setHorizScrollBarValue(int value)
{
    scrollArea->setHorizScrollBarValue(value);
}

void GenericViewer::setVertScrollBarValue(int value)
{
    scrollArea->setVertScrollBarValue(value);
}

void GenericViewer::route_changed()
{
    emit changed(this);
}

void GenericViewer::closeEvent( QCloseEvent * event )
{
    if (NeedsSaving)
    {
        int ret = UMessageBox::warning(tr("Unsaved changes..."),
                                       tr("This image has unsaved changes.<br>Are you sure you want to close it?"),
                                       this);

        if ( ret == QMessageBox::Yes )
        {
            event->accept();
        }
        else
        {
            event->ignore();
        }
    }
    else if (noCloseFlag)
    {
        emit closeRequested(false);
        event->ignore();
    }
    else
    {
        event->accept();
    }
}

