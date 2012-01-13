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
#include <QDebug>
//#include <QtOpenGL/QGLWidget>

#include "Viewers/GenericViewer.h"

#include "Common/PanIconWidget.h"
#include "Viewers/IGraphicsView.h"
#include "Viewers/IGraphicsPixmapItem.h"
#include "Libpfs/frame.h"


namespace
{
// define the number of pixels to count as border of the image, because of the shadow
static const int BORDER_SIZE = 30;
}

GenericViewer::GenericViewer(pfs::Frame* frame, QWidget *parent, bool ns):
    QWidget(parent),
    mNeedsSaving(ns),
    mViewerMode(FIT_WINDOW),
    mFrame(frame)
{
    mVBL = new QVBoxLayout(this);
    mVBL->setSpacing(0);
    mVBL->setMargin(0);

    mToolBar = new QToolBar("",this);
    mToolBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    mToolBar->setFixedHeight(40);
    mVBL->addWidget(mToolBar);

    mScene = new QGraphicsScene(this);
    mScene->setBackgroundBrush(Qt::darkGray);
    mView = new IGraphicsView(mScene, this);
    //mView->setViewport(new QGLWidget()); //OpenGL viewer
    mView->setCacheMode(QGraphicsView::CacheBackground);
    mView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    connect(mView, SIGNAL(zoomIn()), this, SLOT(zoomIn()));
    connect(mView, SIGNAL(zoomOut()), this, SLOT(zoomOut()));
    connect(mView, SIGNAL(viewAreaChangedSize()), this, SLOT(updateView()));
    mView->horizontalScrollBar()->setTracking(true);
    mView->verticalScrollBar()->setTracking(true);
    connect(mView->horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(scrollBarChanged(int)));
    connect(mView->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(scrollBarChanged(int)));

    mCornerButton = new QToolButton(this);
    mCornerButton->setToolTip(tr("Pan the image to a region"));
    mCornerButton->setIcon(QIcon(":/new/prefix1/images/move.png"));

    mView->setCornerWidget(mCornerButton);

    connect(mCornerButton, SIGNAL(pressed()), this, SLOT(slotCornerButtonPressed()));

    mVBL->addWidget(mView);
    mView->show();

    mPixmap = new IGraphicsPixmapItem();
    mScene->addItem(mPixmap);
    connect(mPixmap, SIGNAL(selectionReady(bool)), this, SIGNAL(selectionReady(bool)));
}

GenericViewer::~GenericViewer()
{
    delete mFrame;
}

void GenericViewer::fitToWindow(bool /* checked */)
{
    // DO NOT de-comment: this line is not an optimization, it's a nice way to stop everything working correctly!
    //if ( mViewerMode == FIT_WINDOW ) return;

    mScene->setSceneRect(mPixmap->boundingRect());
    mViewerMode = FIT_WINDOW;

    const int w = mView->viewport()->size().width() - 2*BORDER_SIZE;
    const int h = mView->viewport()->size().height() - 2*BORDER_SIZE;

    qreal w_ratio = qreal(w)/getWidth();
    qreal h_ratio = qreal(h)/getHeight();

    qreal sf = qMin(w_ratio, h_ratio)/getScaleFactor();

    // update only if the change is below the 0.005%
    if ( qAbs(sf - 1.0) > 0.005 )
    {
#ifdef QT_DEBUG
        //qDebug() << "void GenericViewer::fitToWindow().sf = " << sf;
#endif
        mView->scale(sf,sf);

        emit changed(this);
    }
}

bool GenericViewer::isFittedToWindow()
{
    return ((mViewerMode == FIT_WINDOW) ? true : false);
}

void GenericViewer::fillToWindow()
{
    // DO NOT de-comment: this line is not an optimization, it's a nice way to stop everything working correctly!
    //if ( mViewerMode == FILL_WINDOW ) return;

    mScene->setSceneRect(mPixmap->boundingRect());
    mViewerMode = FILL_WINDOW;

    const int w = mView->viewport()->size().width();
    const int h = mView->viewport()->size().height();

    qreal w_ratio = qreal(w)/getWidth();
    qreal h_ratio = qreal(h)/getHeight();

    qreal sf = qMax(w_ratio, h_ratio)/getScaleFactor();

    // update only if the change is below the 0.005%
    if ( qAbs(sf - 1.0) > 0.005 )
    {
#ifdef QT_DEBUG
        //qDebug() << "void GenericViewer::fillToWindow().sf = " << sf;
#endif
        mView->scale(sf,sf);

        emit changed(this);
    }
}

bool GenericViewer::isFilledToWindow()
{
    return ((mViewerMode == FILL_WINDOW) ? true : false);
}

void GenericViewer::normalSize()
{
    //if ( mViewerMode == NORMAL_SIZE ) return;

    mScene->setSceneRect(mPixmap->boundingRect());
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

GenericViewer::ViewerMode GenericViewer::getViewerMode()
{
    return mViewerMode;
}

void GenericViewer::setViewerMode(GenericViewer::ViewerMode viewer_mode)
{
    switch (viewer_mode)
    {
    case NORMAL_SIZE:
        normalSize();
        break;
    case FIT_WINDOW:
        fitToWindow();
        break;
    case FILL_WINDOW:
        fillToWindow();
        break;
    }
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
#ifdef QT_DEBUG
    qDebug() << "void GenericViewer::updateView()";
#endif

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

QRect GenericViewer::getSelectionRect(void)
{
    return mPixmap->getSelectionRect();
}

void GenericViewer::setSelectionTool(bool toggled)
{
    if (toggled) mPixmap->enableSelectionTool();
    else mPixmap->disableSelectionTool();
}

void GenericViewer::removeSelection(void)
{
    mPixmap->removeSelection();
}

bool GenericViewer::hasSelection(void)
{
    return mPixmap->hasSelection();
}

bool GenericViewer::needsSaving(void)
{
    return mNeedsSaving;
}

void GenericViewer::setNeedsSaving(bool s)
{
    mNeedsSaving = s;
}

QString GenericViewer::getFileName()
{
    return mFileName;
}

void GenericViewer::setFileName(const QString& fn)
{
    mFileName = fn;
}

void GenericViewer::slotCornerButtonPressed()
{
    mPanIconWidget = new PanIconWidget(this);

    // is there a way to avoid this call?
    // how expensive is to call this function?
    QImage image = this->getQImage();
    mPanIconWidget->setImage(&image);

    float zf = this->getScaleFactor();
    float leftviewpos = (float)(mView->horizontalScrollBar()->value());
    float topviewpos = (float)(mView->verticalScrollBar()->value());
    float wps_w = (float)(mView->maximumViewportSize().width());
    float wps_h = (float)(mView->maximumViewportSize().height());
    QRect r((int)(leftviewpos/zf), (int)(topviewpos/zf), (int)(wps_w/zf), (int)(wps_h/zf));
    mPanIconWidget->setRegionSelection(r);
    mPanIconWidget->setMouseFocus();
    connect(mPanIconWidget, SIGNAL(selectionMoved(QRect)), this, SLOT(slotPanIconSelectionMoved(QRect)));
    connect(mPanIconWidget, SIGNAL(finished()), this, SLOT(slotPanIconHidden()));
    QPoint g = mView->mapToGlobal(mView->viewport()->pos());
    g.setX(g.x()+ mView->viewport()->size().width());
    g.setY(g.y()+ mView->viewport()->size().height());
    mPanIconWidget->popup(QPoint(g.x() - mPanIconWidget->width()/2, g.y() - mPanIconWidget->height()/2));
    mPanIconWidget->setCursorToLocalRegionSelectionCenter();
}

void GenericViewer::slotPanIconSelectionMoved(QRect gotopos)
{
    mView->horizontalScrollBar()->setValue((int)(gotopos.x()*this->getScaleFactor()));
    mView->verticalScrollBar()->setValue((int)(gotopos.y()*this->getScaleFactor()));
    emit changed(this);
}

void GenericViewer::slotPanIconHidden()
{
    mPanIconWidget->close();
    mCornerButton->blockSignals(true);
    mCornerButton->animateClick();
    mCornerButton->blockSignals(false);
}

void GenericViewer::scrollBarChanged(int /*value*/)
{
    emit changed(this);
}

void GenericViewer::syncViewer(GenericViewer *src)
{
    if (src == NULL) return;
    if (src == this) return;

    setViewerMode( src->getViewerMode() );

    mView->horizontalScrollBar()->setValue( src->mView->horizontalScrollBar()->value() );
    mView->verticalScrollBar()->setValue( src->mView->verticalScrollBar()->value() );
}

float GenericViewer::getScaleFactor()
{
    return mView->transform().m11();
}

QImage GenericViewer::getQImage() const
{
    return mPixmap->pixmap().toImage();
}

void GenericViewer::setQImage(const QImage& qimage)
{
    mPixmap->setPixmap(QPixmap::fromImage(qimage));
}

int GenericViewer::getWidth()
{
    return mFrame->getWidth();
}

int GenericViewer::getHeight()
{
    return mFrame->getHeight();
}

void GenericViewer::setFrame(pfs::Frame *new_frame, TonemappingOptions* tmopts)
{
    delete mFrame;
    mFrame = new_frame;

    // call virtual protected function
    updatePixmap();

    // update tonemappingoptions (if available)
    // in the current implementation, only LdrViewer redefines this function
    setTonemappingOptions(tmopts);

    // reset boundaries
    updateView();
}

pfs::Frame* GenericViewer::getFrame() const
{
    return mFrame;
}
