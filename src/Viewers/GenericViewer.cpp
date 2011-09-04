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

#include <QScrollBar>
//#include <QtOpenGL/QGLWidget>

#include "GenericViewer.h"
#include "UI/UMessageBox.h"

// define the number of pixels to count as border of the image, because of the shadow
#define BORDER_SIZE 30

GenericViewer::GenericViewer(QWidget *parent, bool ns, bool ncf):
    QWidget(parent), NeedsSaving(ns), noCloseFlag(ncf),
    mImage(NULL),
    mViewerMode(NORMAL_SIZE)
{
    setAttribute(Qt::WA_DeleteOnClose);

    mVBL = new QVBoxLayout(this);
    mVBL->setSpacing(0);
    mVBL->setMargin(0);

    toolBar = new QToolBar("",this);
    toolBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    toolBar->setFixedHeight(40);
    mVBL->addWidget(toolBar);

    // RUBBISH
    //scrollArea = new SmartScrollArea(this, imageLabel);
    //scrollArea->setBackgroundRole(QPalette::Shadow);
    //mVBL->addWidget(scrollArea);

    cornerButton = new QToolButton(this);
    cornerButton->setToolTip(tr("Pan the image to a region"));
    cornerButton->setIcon(QIcon(":/new/prefix1/images/move.png"));
    //scrollArea->setCornerWidget(cornerButton);
    connect(cornerButton, SIGNAL(pressed()), this, SLOT(slotCornerButtonPressed()));
    //connect(scrollArea, SIGNAL(selectionReady(bool)), this, SIGNAL(selectionReady(bool)));
    //connect(scrollArea, SIGNAL(changed(void)), this, SLOT(route_changed(void)));
    // RUBBISH

    mScene = new QGraphicsScene(this);
    mScene->setBackgroundBrush(Qt::darkGray);
    mView = new IGraphicsView(mScene, this);
    connect(mView, SIGNAL(zoomIn()), this, SLOT(zoomIn()));
    connect(mView, SIGNAL(zoomOut()), this, SLOT(zoomOut()));
    connect(mView, SIGNAL(viewAreaChangedSize()), this, SLOT(updateView()));
    //mView->setViewport(new QGLWidget()); //OpenGL viewer
    mView->setCornerWidget(cornerButton);

    mVBL->addWidget(mView);
    mView->show();

    mPixmap = new QGraphicsPixmapItem();
    QGraphicsDropShadowEffect* x = new QGraphicsDropShadowEffect();
    x->setBlurRadius(10);
    x->setXOffset(0);
    x->setYOffset(0);
    mPixmap->setGraphicsEffect(x);
}

GenericViewer::~GenericViewer()
{
    if ( mImage != NULL ) delete mImage;
}

void GenericViewer::fitToWindow(bool /* checked */)
{
    // DO NOT de-comment: this line is not an optimization, it's a nice way to stop everything working correctly!
    //if ( mViewerMode == FIT_WINDOW ) return;

    mViewerMode = FIT_WINDOW;

    const int w = mView->width(); // - mView->verticalScrollBar()->width() - 2;
    const int h = mView->height(); // - mView->horizontalScrollBar()->height() - 2;

    qreal w_ratio = qreal(w)/(mCols + 2*BORDER_SIZE);
    qreal h_ratio = qreal(h)/(mRows + 2*BORDER_SIZE);

    qreal sf = qMin(w_ratio, h_ratio)/getScaleFactor();

    if (sf != 1.0) mView->scale(sf,sf);

    emit changed(this);
}

bool GenericViewer::isFittedToWindow()
{
    return ((mViewerMode == FIT_WINDOW) ? true : false);
}


void GenericViewer::fillToWindow()
{
    // DO NOT de-comment: this line is not an optimization, it's a nice way to stop everything working correctly!
    //if ( mViewerMode == FILL_WINDOW ) return;

    mViewerMode = FILL_WINDOW;

    const int w = mView->width() - mView->verticalScrollBar()->width() - 2;
    const int h = mView->height() - mView->horizontalScrollBar()->height() - 2;

    qreal w_ratio = qreal(w)/(mCols);
    qreal h_ratio = qreal(h)/(mRows);

    qreal sf = qMax(w_ratio, h_ratio)/getScaleFactor();

    if (sf != 1.0) mView->scale(sf,sf);

    emit changed(this);
}

bool GenericViewer::isFilledToWindow()
{
    return ((mViewerMode == FILL_WINDOW) ? true : false);
}

void GenericViewer::normalSize()
{
    // DO NOT de-comment: this line is not an optimization, it's a nice way to stop everything working correctly!
    //if ( mViewerMode == NORMAL_SIZE ) return;

    mViewerMode = NORMAL_SIZE;

    qreal curr_scale_factor = getScaleFactor();
    qreal scale_by = 1.0f/curr_scale_factor;

    mView->scale(scale_by, scale_by);

    emit changed(this);
}

bool GenericViewer::isNormalSize()
{
    return ((mViewerMode == NORMAL_SIZE) ? true : false);
}

void GenericViewer::zoomIn()
{
    // update the current view
    switch (mViewerMode)
    {
    case NORMAL_SIZE:
        normalSize();
        break;
    case FILL_WINDOW:
        normalSize();
        break;
    case FIT_WINDOW:
        fillToWindow();
        break;
    }

    emit changed(this);
}

void GenericViewer::zoomOut()
{
    // update the current view
    switch (mViewerMode)
    {
    case NORMAL_SIZE:
        fillToWindow();
        break;
    case FILL_WINDOW:
        fitToWindow();
        break;
    case FIT_WINDOW:
        fitToWindow();
        break;
    }

    emit changed(this);
}

void GenericViewer::updateView()
{
    switch (mViewerMode)
    {
    case NORMAL_SIZE:
        normalSize();
        break;
    case FILL_WINDOW:
        fillToWindow();
        break;
    case FIT_WINDOW:
        fitToWindow();
        break;
    }
}

// Factor is a number between 0.01 and 1.0f (1% to 100%)
void GenericViewer::zoomToFactor(float /*factor*/)
{
    //scrollArea->zoomToFactor(factor);
    emit changed(this);
}

const QRect GenericViewer::getSelectionRect(void)
{
        return QRect();//scrollArea->getSelectionRect();
}

void GenericViewer::setSelectionTool(bool toggled)
{
        //scrollArea->setSelectionTool( toggled );
}

void GenericViewer::removeSelection(void)
{
        //scrollArea->removeSelection();
}

bool GenericViewer::hasSelection(void)
{
        return false; //scrollArea->hasSelection();
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
    panIconWidget = new PanIconWidget(this);
    panIconWidget->setImage(mImage);

    float zf = this->getScaleFactor(); //getImageScaleFactor(); // come minchia lo calcolo lo scaling factor?!
    float leftviewpos = (float)(mView->horizontalScrollBar()->value());
    float topviewpos = (float)(mView->verticalScrollBar()->value());
    float wps_w = (float)(mView->maximumViewportSize().width());
    float wps_h = (float)(mView->maximumViewportSize().height());
    QRect r((int)(leftviewpos/zf), (int)(topviewpos/zf), (int)(wps_w/zf), (int)(wps_h/zf));
    panIconWidget->setRegionSelection(r);
    panIconWidget->setMouseFocus();
    connect(panIconWidget, SIGNAL(selectionMoved(QRect)), this, SLOT(slotPanIconSelectionMoved(QRect)));
    connect(panIconWidget, SIGNAL(finished()), this, SLOT(slotPanIconHidden()));
    QPoint g = mView->mapToGlobal(mView->viewport()->pos());
    g.setX(g.x()+ mView->viewport()->size().width());
    g.setY(g.y()+ mView->viewport()->size().height());
    panIconWidget->popup(QPoint(g.x() - panIconWidget->width()/2, g.y() - panIconWidget->height()/2));
    panIconWidget->setCursorToLocalRegionSelectionCenter();
}

void GenericViewer::slotPanIconSelectionMoved(QRect gotopos)
{
    mView->horizontalScrollBar()->setValue((int)(gotopos.x()*this->getScaleFactor()));
    mView->verticalScrollBar()->setValue((int)(gotopos.y()*this->getScaleFactor()));
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
    return mView->horizontalScrollBar()->value();
}

int GenericViewer::getVertScrollBarValue()
{
    return mView->verticalScrollBar()->value();
}

void GenericViewer::setHorizScrollBarValue(int value)
{
    mView->horizontalScrollBar()->setValue(value);
    //scrollArea->setHorizScrollBarValue(value);
}

void GenericViewer::setVertScrollBarValue(int value)
{
    mView->verticalScrollBar()->setValue(value);
    //scrollArea->setVertScrollBarValue(value);
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

