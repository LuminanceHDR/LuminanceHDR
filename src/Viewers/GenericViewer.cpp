/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2009-2012 Franco Comida, Davide Anastasia
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
 * Original Work
 * @author Franco Comida <fcomida@users.sourceforge.net>
 * Improvements, bugfixing
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *  Refactoring for QGraphicsView
 *
 */

#include <QDebug>
#include <QDrag>
#include <QMimeData>
#include <QScrollBar>

#include "Libpfs/frame.h"
#include "Viewers/GenericViewer.h"
#include "Viewers/IGraphicsPixmapItem.h"
#include "Viewers/IGraphicsView.h"
#include "Viewers/PanIconWidget.h"

namespace {
// define the number of pixels to count as border of the image, because of the
// shadow
static const int BORDER_SIZE = 30;
}

GenericViewer::GenericViewer(pfs::Frame *frame, QWidget *parent, bool ns)
    : QWidget(parent),
      mViewerMode(FIT_WINDOW),
      mNeedsSaving(ns),
      mFrame(frame) {
    mVBL = new QVBoxLayout(this);
    mVBL->setSpacing(0);
    mVBL->setMargin(0);

    mToolBar = new QToolBar(QLatin1String(""), this);
    mToolBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    mToolBar->setFixedHeight(40);
    mToolBar->setAutoFillBackground(true);

    mVBL->addWidget(mToolBar);

    mScene = new QGraphicsScene(this);
    mScene->setBackgroundBrush(Qt::darkGray);
    mView = new IGraphicsView(mScene, this);
    mView->setCacheMode(QGraphicsView::CacheBackground);
    mView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    connect(mView, &IGraphicsView::zoomIn, this, &GenericViewer::zoomIn);
    connect(mView, &IGraphicsView::zoomOut, this, &GenericViewer::zoomOut);
    connect(mView, &IGraphicsView::viewAreaChangedSize, this,
            &GenericViewer::updateView);
    mView->horizontalScrollBar()->setTracking(true);
    mView->verticalScrollBar()->setTracking(true);
    connect(mView->horizontalScrollBar(), &QAbstractSlider::valueChanged, this,
            &GenericViewer::scrollBarChanged);
    connect(mView->verticalScrollBar(), &QAbstractSlider::valueChanged, this,
            &GenericViewer::scrollBarChanged);

    mCornerButton = new QToolButton(this);
    mCornerButton->setIcon(QIcon::fromTheme(QStringLiteral("move"),
                                            QIcon(":/program-icons/move")));

    mView->setCornerWidget(mCornerButton);

    connect(mCornerButton, &QAbstractButton::pressed, this,
            &GenericViewer::slotCornerButtonPressed);

    mVBL->addWidget(mView);
    mView->show();

    mPixmap = new IGraphicsPixmapItem();
    mScene->addItem(mPixmap);
    connect(mPixmap, &IGraphicsPixmapItem::selectionReady, this,
            &GenericViewer::selectionReady);
    connect(mPixmap, &IGraphicsPixmapItem::startDragging, this,
            &GenericViewer::startDragging);
}

GenericViewer::~GenericViewer() {
    blockSignals(true);
}

void GenericViewer::retranslateUi() {
    mCornerButton->setToolTip(tr("Pan the image to a region"));
}

void GenericViewer::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange) retranslateUi();
    QWidget::changeEvent(event);
}

void GenericViewer::fitToWindow(bool /* checked */) {
    // DO NOT de-comment: this line is not an optimization, it's a nice way to
    // stop everything working correctly!
    // if ( mViewerMode == FIT_WINDOW ) return;

    mScene->setSceneRect(mPixmap->boundingRect());
    mViewerMode = FIT_WINDOW;

    const int w = mView->viewport()->size().width() - 2 * BORDER_SIZE;
    const int h = mView->viewport()->size().height() - 2 * BORDER_SIZE;

    qreal w_ratio = qreal(w) / getWidth();
    qreal h_ratio = qreal(h) / getHeight();

    qreal sf = qMin(w_ratio, h_ratio) / getScaleFactor();

    // update only if the change is above the 0.05%
    if (qAbs(sf - static_cast<qreal>(1.0)) > 0.05) {
        mView->scale(sf, sf);
        emit changed(this);
    }
}

bool GenericViewer::isFittedToWindow() {
    return ((mViewerMode == FIT_WINDOW) ? true : false);
}

void GenericViewer::fillToWindow() {
    // DO NOT de-comment: this line is not an optimization, it's a nice way to
    // stop everything working correctly!
    // if ( mViewerMode == FILL_WINDOW ) return;

    mScene->setSceneRect(mPixmap->boundingRect());
    mViewerMode = FILL_WINDOW;

    const int w = mView->viewport()->size().width();
    const int h = mView->viewport()->size().height();

    qreal w_ratio = qreal(w) / getWidth();
    qreal h_ratio = qreal(h) / getHeight();

    qreal sf = qMax(w_ratio, h_ratio) / getScaleFactor();

    // update only if the change is above the 0.05%
    if (qAbs(sf - static_cast<qreal>(1.0)) > 0.05) {
        mView->scale(sf, sf);
        emit changed(this);
    }
}

bool GenericViewer::isFilledToWindow() {
    return ((mViewerMode == FILL_WINDOW) ? true : false);
}

void GenericViewer::normalSize() {
    // if ( mViewerMode == NORMAL_SIZE ) return;

    mScene->setSceneRect(mPixmap->boundingRect());
    mViewerMode = NORMAL_SIZE;

    qreal curr_scale_factor = getScaleFactor();
    qreal scale_by = 1.0f / curr_scale_factor;

    mView->scale(scale_by, scale_by);

    emit changed(this);
}

bool GenericViewer::isNormalSize() {
    return ((mViewerMode == NORMAL_SIZE) ? true : false);
}

GenericViewer::ViewerMode GenericViewer::getViewerMode() { return mViewerMode; }

void GenericViewer::setViewerMode(GenericViewer::ViewerMode viewer_mode) {
    switch (viewer_mode) {
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

void GenericViewer::zoomIn() {
    // update the current view
    switch (mViewerMode) {
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

void GenericViewer::zoomOut() {
    // update the current view
    switch (mViewerMode) {
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

void GenericViewer::updateView() {
#ifdef QT_DEBUG
    qDebug() << "void GenericViewer::updateView()";
#endif

    switch (mViewerMode) {
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
void GenericViewer::zoomToFactor(float /*factor*/) {
    // scrollArea->zoomToFactor(factor);
    emit changed(this);
}

QRect GenericViewer::getSelectionRect(void) {
    return mPixmap->getSelectionRect();
}

void GenericViewer::setSelectionTool(bool toggled) {
    if (toggled)
        mPixmap->enableSelectionTool();
    else
        mPixmap->disableSelectionTool();
}

void GenericViewer::removeSelection(void) { mPixmap->removeSelection(); }

bool GenericViewer::hasSelection(void) { return mPixmap->hasSelection(); }

void GenericViewer::slotCornerButtonPressed() {
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
    QRect r((int)(leftviewpos / zf), (int)(topviewpos / zf), (int)(wps_w / zf),
            (int)(wps_h / zf));
    mPanIconWidget->setRegionSelection(r);
    mPanIconWidget->setMouseFocus();
    connect(mPanIconWidget, &PanIconWidget::selectionMoved, this,
            &GenericViewer::slotPanIconSelectionMoved);
    connect(mPanIconWidget, &PanIconWidget::finished, this,
            &GenericViewer::slotPanIconHidden);
    QPoint g = mView->mapToGlobal(mView->viewport()->pos());
    g.setX(g.x() + mView->viewport()->size().width());
    g.setY(g.y() + mView->viewport()->size().height());
    mPanIconWidget->popup(QPoint(g.x() - mPanIconWidget->width() / 2,
                                 g.y() - mPanIconWidget->height() / 2));
    mPanIconWidget->setCursorToLocalRegionSelectionCenter();
}

void GenericViewer::slotPanIconSelectionMoved(QRect gotopos) {
    mView->horizontalScrollBar()->setValue(
        (int)(gotopos.x() * this->getScaleFactor()));
    mView->verticalScrollBar()->setValue(
        (int)(gotopos.y() * this->getScaleFactor()));
    emit changed(this);
}

void GenericViewer::slotPanIconHidden() {
    mPanIconWidget->close();
    mCornerButton->blockSignals(true);
    mCornerButton->animateClick();
    mCornerButton->blockSignals(false);
}

void GenericViewer::scrollBarChanged(int /*value*/) { emit changed(this); }

void GenericViewer::syncViewer(GenericViewer *src) {
    if (src == NULL) return;
    if (src == this) return;

    setViewerMode(src->getViewerMode());

    mView->horizontalScrollBar()->setValue(
        src->mView->horizontalScrollBar()->value());
    mView->verticalScrollBar()->setValue(
        src->mView->verticalScrollBar()->value());
}

float GenericViewer::getScaleFactor() { return mView->transform().m11(); }

QImage GenericViewer::getQImage() const { return mPixmap->pixmap().toImage(); }

void GenericViewer::setQImage(const QImage &qimage) {
    QPixmap pixmap = QPixmap::fromImage(qimage);
    pixmap.setDevicePixelRatio(m_devicePixelRatio);
    mPixmap->setPixmap(pixmap);
}

int GenericViewer::getWidth() {
    if (mFrame)
        return mFrame->getWidth();
    else
        return 0;
}

int GenericViewer::getHeight() {
    if (mFrame)
        return mFrame->getHeight();
    else
        return 0;
}

void GenericViewer::setFrame(pfs::Frame *new_frame,
                             TonemappingOptions *tmopts) {
    mFrame.reset(new_frame);

    // call virtual protected function
    updatePixmap();

    // update tonemappingoptions (if available)
    // in the current implementation, only LdrViewer redefines this function
    if (tmopts != NULL) setTonemappingOptions(tmopts);

    // reset boundaries
    updateView();
}

pfs::Frame *GenericViewer::getFrame() const { return mFrame.get(); }

void GenericViewer::startDragging() {
    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    mimeData->setImageData(mPixmap->pixmap().toImage());
    drag->setMimeData(mimeData);
    drag->setPixmap(
        mPixmap->pixmap().scaledToHeight(mPixmap->pixmap().height() / 10));

    /*Qt::DropAction dropAction =*/drag->exec();
}

void GenericViewer::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_F10) {
        emit reparent(this);
    } else if (event->key() == Qt::Key_N) {
        emit goNext(this);
    } else if (event->key() == Qt::Key_P) {
        emit goPrevious(this);
    } else if (event->key() == Qt::Key_S) {
        emit syncViewers(this);
    } else if (event->key() == Qt::Key_Plus) {
        zoomIn();
    } else if (event->key() == Qt::Key_Minus) {
        zoomOut();
    } else if (event->key() == Qt::Key_W) {
        fitToWindow();
    } else if (event->key() == Qt::Key_F) {
        fillToWindow();
    } else if (event->key() == Qt::Key_O) {
        normalSize();
    }
}
