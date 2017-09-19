/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2007 Giuseppe Rota
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
 * Implementation based on QGraphicsView, anti-ghosting:
 * @author Franco Comida <fcomida@users.sourceforge.net>
 */

#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <cassert>

#include "Viewers/GenericViewer.h"
#include "Viewers/IGraphicsPixmapItem.h"
#include "Viewers/IGraphicsView.h"
#include "Viewers/PanIconWidget.h"

#include "PreviewWidget.h"

namespace {
// define the number of pixels to count as border of the image, because of the
// shadow
static const int BORDER_SIZE = 30;
}

PreviewWidget::PreviewWidget(QWidget *parent, QImage *m, const QImage *p)
    : QWidget(parent),
      m_movableImage(m),
      m_pivotImage(p),
      m_agMask(NULL),
      m_originalAgMask(NULL),
      m_patchesMask(NULL),
      m_agMaskPixmap(NULL),
      m_savedMask(NULL),
      m_prevComputed(),
      m_mx(0),
      m_my(0),
      m_px(0),
      m_py(0),
      m_old_mx(0),
      m_old_my(0),
      m_agcursorPixmap(NULL),
      m_drawingMode(BRUSH) {
    setFocusPolicy(Qt::StrongFocus);
    // setMouseTracking(true);
    // set internal brush values to their default
    m_brushAddMode = true;
    setBrushSize(32);
    m_previousPixmapSize = -1;
    setBrushStrength(255);
    m_previousPixmapStrength = -1;
    m_previousPixmapColor = QColor();
    fillAntiGhostingCursorPixmap();

    m_previewImage = new QImage(m_movableImage->size(), QImage::Format_ARGB32);
    m_previewImage->fill(qRgba(255, 0, 0, 255));
    blendmode = &PreviewWidget::computeDiffRgba;
    m_mode = EditingMode;
    m_rect = m_movableImage->rect();

    mVBL = new QVBoxLayout(this);
    mVBL->setSpacing(0);
    mVBL->setMargin(0);
    mScene = new QGraphicsScene(this);
    mScene->setBackgroundBrush(Qt::darkGray);
    mView = new IGraphicsView(mScene, this);
    // mView->setViewport(new QGLWidget()); //OpenGL viewer
    mView->setCacheMode(QGraphicsView::CacheBackground);
    mView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    mView->viewport()->installEventFilter(this);
    mView->viewport()->setMouseTracking(false);

    connect(mView, &IGraphicsView::zoomIn, this, &PreviewWidget::zoomIn);
    connect(mView, &IGraphicsView::zoomOut, this, &PreviewWidget::zoomOut);
    connect(mView, &IGraphicsView::viewAreaChangedSize, this,
            &PreviewWidget::updateView);
    mView->horizontalScrollBar()->setTracking(true);
    mView->verticalScrollBar()->setTracking(true);
    connect(mView->horizontalScrollBar(), &QAbstractSlider::valueChanged, this,
            &PreviewWidget::scrollBarChanged);
    connect(mView->verticalScrollBar(), &QAbstractSlider::valueChanged, this,
            &PreviewWidget::scrollBarChanged);

    mCornerButton = new QToolButton(this);
    mCornerButton->setIcon(QIcon::fromTheme(QStringLiteral("move"),
                                            QIcon(":/program-icons/move")));

    mView->setCornerWidget(mCornerButton);

    connect(mCornerButton, &QAbstractButton::pressed, this,
            &PreviewWidget::slotCornerButtonPressed);

    mVBL->addWidget(mView);
    mView->show();

    mPixmap = new IGraphicsPixmapItem();
    mPixmap->setZValue(0);
    renderPreviewImage(blendmode, m_rect);
    mPixmap->setPixmap(QPixmap::fromImage(*m_previewImage));
    fitToWindow();
    connect(mPixmap, &IGraphicsPixmapItem::selectionReady, this,
            &PreviewWidget::selectionReady);

    mAgPixmap = new IGraphicsPixmapItem(mPixmap);
    mAgPixmap->setZValue(1);
    mAgPixmap->setVisible(false);
    mScene->addItem(mPixmap);

    mAgPixmap->setAcceptedMouseButtons(0);
}

PreviewWidget::~PreviewWidget() {
    delete m_previewImage;
    delete mPixmap;
    if (m_agMaskPixmap) {
        delete m_agMaskPixmap;
        delete m_originalAgMask;
        delete m_agMask;
    }
}

QRgb outofbounds = qRgba(0, 0, 0, 255);

void PreviewWidget::renderPreviewImage(
    QRgb (PreviewWidget::*rendermode)(const QRgb *, const QRgb *) const,
    const QRect rect) {
    int originx = rect.x();
    int originy = rect.y();
    int W = rect.width();
    int H = rect.height();
    if (rect.isNull()) {
        // requested fullsize render
        QRegion areaToRender =
            QRegion(QRect(0, 0, m_previewImage->size().width(),
                          m_previewImage->size().height())) -
            m_prevComputed;
        if (!areaToRender.isEmpty()) {
            // render only what you have to
            originx = areaToRender.boundingRect().x();
            originy = areaToRender.boundingRect().y();
            W = areaToRender.boundingRect().width();
            H = areaToRender.boundingRect().height();
            m_prevComputed += areaToRender;
        } else  // image already rendered fullsize
            return;
    }
    // these kind of things can happen and lead to strange and nasty runtime
    // errors!
    // usually it's an error of 2,3 px
    if ((originy + H - 1) >= m_movableImage->height())
        H = m_movableImage->height() - originy;
    if ((originx + W - 1) >= m_movableImage->width())
        W = m_movableImage->width() - originx;

    const QRgb *movVal = NULL;
    const QRgb *pivVal = NULL;
    QRgb *movLine = NULL;
    QRgb *pivLine = NULL;
    QRgb *out = NULL;

// for all the rows that we have to paint
#pragma omp parallel for private(out, movVal, pivVal, movLine, pivLine)
    for (int i = originy; i < originy + H; i++) {
        out = (QRgb *)m_previewImage->scanLine(i);

        // if within bounds considering vertical offset
        if (!((i - m_my) < 0 || (i - m_my) >= m_movableImage->height()))
            movLine = (QRgb *)(m_movableImage->scanLine(i - m_my));
        else
            movLine = NULL;

        if (!((i - m_py) < 0 || (i - m_py) >= m_pivotImage->height()))
            pivLine = (QRgb *)(m_pivotImage->scanLine(i - m_py));
        else
            pivLine = NULL;

        // for all the columns that we have to paint
        for (int j = originx; j < originx + W; j++) {
            // if within bounds considering horizontal offset
            if (movLine == NULL || (j - m_mx) < 0 ||
                (j - m_mx) >= m_movableImage->width())
                movVal = &outofbounds;
            else
                movVal = &movLine[j - m_mx];

            if (pivLine == NULL || (j - m_px) < 0 ||
                (j - m_px) >= m_pivotImage->width())
                pivVal = &outofbounds;
            else
                pivVal = &pivLine[j - m_px];

            if (m_pivotImage == m_movableImage)
                out[j] = *movVal;
            else
                out[j] = (this->*rendermode)(movVal, pivVal);
        }
    }
}

namespace {
void paste(QImage *mask, QImage pixmap, const int mx, const int my) {
    const int W = mask->width();
    const int H = mask->height();

    for (int j = 0; j < H; j++) {
        for (int i = 0; i < W; i++) {
            if (((i + mx) >= 0 && (i + mx) < W) &&
                ((j + my) >= 0 && (j + my < H)))
                mask->setPixel(i, j, pixmap.pixel(i + mx, j + my));
        }
    }
}
}

void PreviewWidget::renderAgMask() {
    int W = 0, H = 0;
    const QRgb *maskVal = NULL;
    const QRgb *maskLine = NULL;
    QRgb *outMask = NULL;

    if (m_agMaskPixmap) {
        W = m_originalAgMask->width();
        H = m_originalAgMask->height();

        // for all the rows that we have to paint
        //#pragma omp parallel for private(outMask, maskVal, maskLine)
        for (int i = 0; i < H; i++) {
            outMask = (QRgb *)m_agMask->scanLine(i);

            if (!((i - m_my) < 0 || (i - m_my) >= H))
                maskLine = (QRgb *)m_originalAgMask->scanLine(i - m_my);
            else
                maskLine = NULL;

            // for all the columns that we have to paint
            for (int j = 0; j < W; j++) {
                // if within bounds considering horizontal offset
                if (maskLine == NULL || (j - m_mx) < 0 || (j - m_mx) >= W)
                    maskVal = &outofbounds;
                else
                    maskVal = &maskLine[j - m_mx];

                outMask[j] = *maskVal;
            }
        }
    }
}

// void PreviewWidget::renderPatchesMask(bool patches[][agGridSize], const int
// gridX, const int gridY)
void PreviewWidget::renderPatchesMask() {
    QPainter painter(m_patchesMask);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor::fromRgb(255, 255, 255, 60));
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.drawRect(0, 0, m_gridX * agGridSize, m_gridY * agGridSize);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    for (int i = 0; i < agGridSize; i++) {
        for (int j = 0; j < agGridSize; j++) {
            if (m_patches[i][j] == true)
                painter.drawRect(i * m_gridX, j * m_gridY, m_gridX, m_gridY);
        }
    }
    painter.end();
    renderPreviewImage(blendmode, m_rect);
    mPixmap->setPixmap(QPixmap::fromImage(*m_previewImage));
    delete m_agMaskPixmap;
    m_agMaskPixmap = new QPixmap(QPixmap::fromImage(*m_patchesMask));
    mAgPixmap->setPixmap(*m_agMaskPixmap);
}

void PreviewWidget::requestedBlendMode(int newindex) {
    if (newindex == 0)
        blendmode = &PreviewWidget::computeDiffRgba;
    else if (newindex == 1)
        blendmode = &PreviewWidget::computeAddRgba;
    else if (newindex == 2)
        blendmode = &PreviewWidget::computeOnlyMovable;
    else if (newindex == 3)
        blendmode = &PreviewWidget::computeOnlyPivot;

    m_prevComputed = QRegion();
    renderPreviewImage(blendmode, m_rect);
    mPixmap->setPixmap(QPixmap::fromImage(*m_previewImage));
    // updateView();
}

bool PreviewWidget::eventFilter(QObject *object, QEvent *event) {
    // if (m_mode == EditingMode || m_mode == ViewPatches) return false;
    if (m_mode == EditingMode) return false;
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouse = static_cast<QMouseEvent *>(event);
        if (mouse->buttons() == Qt::MidButton) {
            QApplication::setOverrideCursor(QCursor(Qt::ClosedHandCursor));
            m_mousePos = mView->mapToScene(mouse->pos());
        } else if (mouse->buttons() == Qt::LeftButton) {
            if (m_mode == ViewPatches) {
                QPointF relativeToWidget =
                    mView->mapToScene(mapFromGlobal(QCursor::pos()));
                int i = floor(relativeToWidget.x() / m_gridX);
                int j = floor(relativeToWidget.y() / m_gridY);
                m_patches[i][j] = !m_patches[i][j];
                renderPatchesMask();
                emit patchesEdited();
            } else {
                if (m_drawingMode == PATH) {
                    QPointF relativeToWidget =
                        mView->mapToScene(mapFromGlobal(QCursor::pos()));
                    // int sx = relativeToWidget.x() - m_mx;
                    // int sy = relativeToWidget.y() - m_my;
                    // QPointF shifted(sx,sy);
                    // m_firstPoint = m_lastPoint = m_currentPoint = shifted;
                    m_firstPoint = m_lastPoint = m_currentPoint =
                        relativeToWidget;
                    m_path = QPainterPath(m_firstPoint);
                    m_drawingPathEnded = false;
                } else if (m_drawingMode == BRUSH)
                    drawWithBrush();
                mAgPixmap->setPixmap(*m_agMaskPixmap);
                m_timerid = QObject::startTimer(0);
            }
        }
    } else if (event->type() == QEvent::MouseMove) {
        QMouseEvent *mouse = static_cast<QMouseEvent *>(event);
        if (mouse->buttons() == Qt::MidButton) {
            QPointF pos = mView->mapToScene(mouse->pos());
            // moving mouse with middle button pans the preview
            QPointF diff = pos - m_mousePos;
            if (mouse->modifiers() == Qt::ShiftModifier) diff *= 5;
            // emit moved(diff);
            m_mousePos = mView->mapToScene(mouse->pos());
        } else if (mouse->buttons() == Qt::LeftButton &&
                   m_drawingMode == PATH) {
            QPointF relativeToWidget =
                mView->mapToScene(mapFromGlobal(QCursor::pos()));
            // int sx = relativeToWidget.x() - m_mx;
            // int sy = relativeToWidget.y() - m_my;
            // QPointF shifted(sx,sy);
            // m_currentPoint = shifted;
            m_currentPoint = relativeToWidget;
        }
        mAgPixmap->setPixmap(*m_agMaskPixmap);
    } else if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouse = static_cast<QMouseEvent *>(event);
        if (mouse->button() == Qt::LeftButton) {
            QObject::killTimer(m_timerid);
            if (m_drawingMode == PATH) {
                m_path.lineTo(m_firstPoint);
                m_drawingPathEnded = true;
                drawPath();
            }
        } else if (mouse->button() == Qt::MidButton) {
            QApplication::restoreOverrideCursor();
        }
        paste(m_agMask, m_agMaskPixmap->toImage(), (m_mx - m_old_mx),
              (m_my - m_old_my));
        delete m_originalAgMask;
        m_originalAgMask = new QImage(*m_agMask);
        mAgPixmap->setPixmap(*m_agMaskPixmap);
    } else if (event->type() == QEvent::Enter) {
        if (m_mode == EditingMode)
            QApplication::restoreOverrideCursor();
        else if (m_mode == ViewPatches)
            QApplication::setOverrideCursor(Qt::PointingHandCursor);
        else {
            if (m_drawingMode == BRUSH) {
                fillAntiGhostingCursorPixmap();
                QApplication::setOverrideCursor(*m_agcursorPixmap);
            } else
                QApplication::setOverrideCursor(Qt::CrossCursor);
        }
    } else if (event->type() == QEvent::Leave)
        QApplication::restoreOverrideCursor();

    return false;
}

void PreviewWidget::setPivot(QImage *p, int p_px, int p_py) {
    m_pivotImage = p;
    m_px = p_px;
    m_py = p_py;
    m_prevComputed = QRegion();
}

void PreviewWidget::setPivot(QImage *p) { m_pivotImage = p; }

void PreviewWidget::setMovable(QImage *m, int p_mx, int p_my) {
    m_movableImage = m;
    m_mx = p_mx;
    m_my = p_my;
    m_prevComputed = QRegion();
}

void PreviewWidget::setMovable(QImage *m) {
    m_movableImage = m;
    // TODO: check this
    delete m_previewImage;
    m_previewImage = new QImage(m_movableImage->size(), QImage::Format_ARGB32);
    updateView();
}

void PreviewWidget::setMask(QImage *mask) {
    if (m_agMaskPixmap) delete m_agMaskPixmap;
    m_originalAgMask = new QImage(*mask);
    m_agMask = new QImage(*mask);
    m_agMaskPixmap = new QPixmap(QPixmap::fromImage(*m_agMask));
    mAgPixmap->setPixmap(*m_agMaskPixmap);
    m_mx = m_my = 0;
}

void PreviewWidget::setPatchesMask(QImage *mask) {
    if (m_agMaskPixmap) m_patchesMask = new QImage(*mask);
}

QImage *PreviewWidget::getMask() {
    if (m_agMaskPixmap) {
        return new QImage(*m_originalAgMask);
    }
    return NULL;
}

void PreviewWidget::updateVertShiftMovable(int v) {
    if (m_my != v)
        m_old_my = m_my;
    else
        m_old_my = v;
    m_my = v;
    m_old_mx = m_mx;
    m_prevComputed = QRegion();
}

void PreviewWidget::updateHorizShiftMovable(int h) {
    if (m_mx != h)
        m_old_mx = m_mx;
    else
        m_old_mx = h;
    m_mx = h;
    m_old_my = m_my;
    m_prevComputed = QRegion();
}

void PreviewWidget::updateVertShiftPivot(int v) {
    m_py = v;
    m_prevComputed = QRegion();
}

void PreviewWidget::updateHorizShiftPivot(int h) {
    m_px = h;
    m_prevComputed = QRegion();
}

void PreviewWidget::fitToWindow() {
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
#ifdef QT_DEBUG
// qDebug() << "void GenericViewer::fitToWindow().sf = " << sf;
#endif
        mView->scale(sf, sf);

        emit changed(this);
    }
}

bool PreviewWidget::isFittedToWindow() {
    return ((mViewerMode == FIT_WINDOW) ? true : false);
}

void PreviewWidget::fillToWindow() {
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
#ifdef QT_DEBUG
// qDebug() << "void GenericViewer::fillToWindow().sf = " << sf;
#endif
        mView->scale(sf, sf);

        emit changed(this);
    }
}

bool PreviewWidget::isFilledToWindow() {
    return ((mViewerMode == FILL_WINDOW) ? true : false);
}

void PreviewWidget::normalSize() {
    // if ( mViewerMode == NORMAL_SIZE ) return;

    mScene->setSceneRect(mPixmap->boundingRect());
    mViewerMode = NORMAL_SIZE;

    qreal curr_scale_factor = getScaleFactor();
    qreal scale_by = 1.0f / curr_scale_factor;

    mView->scale(scale_by, scale_by);

    emit changed(this);
}

bool PreviewWidget::isNormalSize() {
    return ((mViewerMode == NORMAL_SIZE) ? true : false);
}

PreviewWidget::ViewerMode PreviewWidget::getViewerMode() { return mViewerMode; }

void PreviewWidget::setViewerMode(PreviewWidget::ViewerMode viewer_mode) {
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

void PreviewWidget::zoomIn() {
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

void PreviewWidget::zoomOut() {
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

void PreviewWidget::updateView() {
#ifdef QT_DEBUG
    qDebug() << "void PreviewWidget::updateView()";
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

QRect PreviewWidget::getSelectionRect(void) {
    return mPixmap->getSelectionRect();
}

void PreviewWidget::setSelectionTool(bool toggled) {
    if (toggled)
        mPixmap->enableSelectionTool();
    else
        mPixmap->disableSelectionTool();
}

void PreviewWidget::removeSelection(void) { mPixmap->removeSelection(); }

bool PreviewWidget::hasSelection(void) { return mPixmap->hasSelection(); }

int PreviewWidget::getWidth() { return m_movableImage->width(); }

int PreviewWidget::getHeight() { return m_movableImage->height(); }

float PreviewWidget::getScaleFactor() { return mView->transform().m11(); }

void PreviewWidget::slotCornerButtonPressed() {
    mPanIconWidget = new PanIconWidget(this);

    // is there a way to avoid this call?
    // how expensive is to call this function?
    mPanIconWidget->setImage(m_previewImage);

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
            &PreviewWidget::slotPanIconSelectionMoved);
    connect(mPanIconWidget, &PanIconWidget::finished, this,
            &PreviewWidget::slotPanIconHidden);
    QPoint g = mView->mapToGlobal(mView->viewport()->pos());
    g.setX(g.x() + mView->viewport()->size().width());
    g.setY(g.y() + mView->viewport()->size().height());
    mPanIconWidget->popup(QPoint(g.x() - mPanIconWidget->width() / 2,
                                 g.y() - mPanIconWidget->height() / 2));
    mPanIconWidget->setCursorToLocalRegionSelectionCenter();
}

void PreviewWidget::slotPanIconSelectionMoved(QRect gotopos) {
    mView->horizontalScrollBar()->setValue(
        (int)(gotopos.x() * this->getScaleFactor()));
    mView->verticalScrollBar()->setValue(
        (int)(gotopos.y() * this->getScaleFactor()));
    emit changed(this);
}

void PreviewWidget::slotPanIconHidden() {
    mPanIconWidget->close();
    mCornerButton->blockSignals(true);
    mCornerButton->animateClick();
    mCornerButton->blockSignals(false);
}

void PreviewWidget::scrollBarChanged(int /*value*/) { emit changed(this); }

void PreviewWidget::updatePreviewImage() {
    renderPreviewImage(blendmode, m_rect);
    mPixmap->setPixmap(QPixmap::fromImage(*m_previewImage));
    if (m_mode == AntighostingMode) {
        if ((m_mx != m_old_mx) && (m_my != m_old_my)) {
            delete m_agMask;
            m_agMask = new QImage(*m_originalAgMask);
            paste(m_agMask, m_agMaskPixmap->toImage(), -(m_mx - m_old_mx),
                  -(m_my - m_old_my));
            delete m_agMaskPixmap;
            m_agMaskPixmap = new QPixmap(QPixmap::fromImage(*m_agMask));
            mAgPixmap->setPixmap(*m_agMaskPixmap);
        } else if ((m_mx != m_old_mx) && (m_my == m_old_my)) {
            delete m_agMask;
            m_agMask = new QImage(*m_originalAgMask);
            paste(m_agMask, m_agMaskPixmap->toImage(), -(m_mx - m_old_mx), 0);
            delete m_agMaskPixmap;
            m_agMaskPixmap = new QPixmap(QPixmap::fromImage(*m_agMask));
            mAgPixmap->setPixmap(*m_agMaskPixmap);
        } else if ((m_my != m_old_my) && (m_mx == m_old_mx)) {
            delete m_agMask;
            m_agMask = new QImage(*m_originalAgMask);
            paste(m_agMask, m_agMaskPixmap->toImage(), 0, -(m_my - m_old_my));
            delete m_agMaskPixmap;
            m_agMaskPixmap = new QPixmap(QPixmap::fromImage(*m_agMask));
            mAgPixmap->setPixmap(*m_agMaskPixmap);
        }
    }
}

void PreviewWidget::setDrawWithBrush() { m_drawingMode = BRUSH; }

void PreviewWidget::setDrawPath() { m_drawingMode = PATH; }

void PreviewWidget::setBrushSize(const int newsize) {
    m_requestedPixmapSize = newsize;
}

void PreviewWidget::setBrushMode(bool removemode) {
    m_requestedPixmapStrength *= -1;
    m_brushAddMode = !removemode;
}

void PreviewWidget::setBrushStrength(const int newstrength) {
    m_requestedPixmapStrength = newstrength;
    m_requestedPixmapColor.setAlpha(qMax(60, m_requestedPixmapStrength));
    m_requestedPixmapStrength *= (!m_brushAddMode) ? -1 : 1;
}

void PreviewWidget::setBrushColor(const QColor newcolor) {
    m_requestedPixmapColor = newcolor;
    update();
}

void PreviewWidget::setLassoColor(const QColor newcolor) {
    m_requestedLassoColor = newcolor;
    update();
}

void PreviewWidget::fillAntiGhostingCursorPixmap() {
    if (m_agcursorPixmap) delete m_agcursorPixmap;
    m_previousPixmapSize = m_requestedPixmapSize;
    m_previousPixmapStrength = m_requestedPixmapStrength;
    m_previousPixmapColor = m_requestedPixmapColor;
    m_agcursorPixmap =
        new QPixmap(m_requestedPixmapSize, m_requestedPixmapSize);
    m_agcursorPixmap->fill(Qt::transparent);
    QPainter painter(m_agcursorPixmap);
    painter.setPen(Qt::DashLine);
    painter.setBrush(QBrush(m_requestedPixmapColor, Qt::SolidPattern));
    painter.drawEllipse(0, 0, m_requestedPixmapSize, m_requestedPixmapSize);
}

void PreviewWidget::switchAntighostingMode(bool ag) {
    if (ag) {
        mPixmap->setAcceptedMouseButtons(0);
        delete m_agMaskPixmap;
        m_agMaskPixmap = new QPixmap(QPixmap::fromImage(*m_agMask));
        mAgPixmap->setPixmap(*m_agMaskPixmap);
        mAgPixmap->setVisible(true);
        m_mode = AntighostingMode;
    } else {
        mPixmap->setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton |
                                         Qt::MidButton);
        mAgPixmap->setVisible(false);
        m_mode = EditingMode;
    }
}

void PreviewWidget::switchViewPatchesMode(bool pp, bool patches[][agGridSize],
                                          const int gridX, const int gridY) {
    if (pp) {
        mPixmap->setAcceptedMouseButtons(0);
        mAgPixmap->setVisible(true);
        m_mode = ViewPatches;
        m_gridX = gridX;
        m_gridY = gridY;
        memcpy(m_patches, patches, agGridSize * agGridSize);
    } else {
        mPixmap->setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton |
                                         Qt::MidButton);
        mAgPixmap->setVisible(false);
        m_mode = EditingMode;
    }
}

void PreviewWidget::saveAgMask() {
    if (m_savedMask) delete m_savedMask;
    m_savedMask = new QImage(m_agMaskPixmap->toImage());
}

QImage *PreviewWidget::getSavedAgMask() { return m_savedMask; }

void PreviewWidget::timerEvent(QTimerEvent *) {
    (m_drawingMode == BRUSH) ? drawWithBrush() : drawPath();
}

void PreviewWidget::drawWithBrush() {
    QPointF relativeToWidget = mView->mapToScene(mapFromGlobal(QCursor::pos()));
    float scaleFactor = getScaleFactor();
    // int sx = relativeToWidget.x() - m_mx;
    // int sy = relativeToWidget.y() - m_my;
    // QPointF shifted(sx,sy);
    QPainter p(m_agMaskPixmap);
    p.setPen(Qt::NoPen);
    p.setBrush(QBrush(m_requestedPixmapColor, Qt::SolidPattern));
    if (!m_brushAddMode) p.setCompositionMode(QPainter::CompositionMode_Clear);
    int pixSize = m_requestedPixmapSize / (2 * scaleFactor);
    // p.drawEllipse(shifted, pixSize, pixSize);
    p.drawEllipse(relativeToWidget, pixSize, pixSize);
    p.end();
}

void PreviewWidget::drawPath() {
    QPainter painter(m_agMaskPixmap);
    painter.setPen(QPen(m_requestedLassoColor, 0, Qt::SolidLine, Qt::FlatCap,
                        Qt::MiterJoin));
    painter.setBrush(QBrush());

    if (m_drawingPathEnded) {
        painter.setCompositionMode(
            QPainter::CompositionMode_Clear);  // Nasty hack,
                                               // QPen does
                                               // not draw
                                               // semi
                                               // transparent
        painter.drawPath(m_path);
        if (m_brushAddMode)
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.fillPath(m_path, m_requestedPixmapColor);
        if (m_brushAddMode) {  // redraw the path
            painter.setPen(QPen(m_requestedPixmapColor, 0, Qt::SolidLine,
                                Qt::FlatCap, Qt::MiterJoin));
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            painter.drawPath(m_path);
        }
    } else {
        m_path.lineTo(m_lastPoint);
        m_lastPoint = m_currentPoint;
        painter.drawPath(m_path);
    }
    painter.end();
}

void PreviewWidget::getPatches(bool patches[][agGridSize]) {
    memcpy(patches, m_patches, agGridSize * agGridSize);
}
