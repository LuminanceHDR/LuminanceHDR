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
 */

#include <cassert>
#include <QPainter>
#include <QApplication>

#include "PreviewWidget.h"

PreviewWidget::PreviewWidget(QWidget *parent, QImage *m, const QImage *p) : 
    QWidget(parent), 
    m_movableImage(m), 
    m_pivotImage(p), 
    m_prevComputed(),
    m_scaleFactor(1) 
{
    m_mx = m_my = m_px = m_py = 0;
    setFocusPolicy(Qt::StrongFocus);
    m_previewImage = new QImage(m_movableImage->size(),QImage::Format_ARGB32);
    m_previewImage->fill(qRgba(255,0,0,255));
    blendmode = &PreviewWidget::computeDiffRgba;
    m_leftButtonMode = LB_nomode;
    setMouseTracking(true);
}

PreviewWidget::~PreviewWidget() {
    delete m_previewImage;
}

void PreviewWidget::paintEvent(QPaintEvent * event) {
    if (m_pivotImage == NULL || m_movableImage == NULL)
        return;
    assert(m_movableImage->size() == m_pivotImage->size());
    QRect paintRect = event->rect();
    QRect srcRect = QRect(paintRect.topLeft()/m_scaleFactor, paintRect.size()/m_scaleFactor);
    QPainter p(this);
    QRegion areaToRender=QRegion(srcRect) - m_prevComputed;
    if (!areaToRender.isEmpty()) {
        renderPreviewImage(blendmode, areaToRender.boundingRect());
        m_prevComputed += QRegion(srcRect);
    }
    p.drawImage(paintRect, *m_previewImage, srcRect);
}

QRgb outofbounds = qRgba(0,0,0,255);

void PreviewWidget::renderPreviewImage(QRgb(PreviewWidget::*rendermode)(const QRgb*,const QRgb*)const, const QRect rect ) {
    int originx = rect.x();
    int originy = rect.y();
    int W = rect.width();
    int H = rect.height();
    if (rect.isNull()) {
        //requested fullsize render
        QRegion areaToRender= QRegion(QRect(0, 0, m_previewImage->size().width(), m_previewImage->size().height())) - m_prevComputed;
        if (!areaToRender.isEmpty()) {
            //render only what you have to
            originx = areaToRender.boundingRect().x();
            originy = areaToRender.boundingRect().y();
            W = areaToRender.boundingRect().width();
            H = areaToRender.boundingRect().height();
            m_prevComputed += areaToRender;
        } else //image already rendered fullsize
            return;
    }
    //these kind of things can happen and lead to strange and nasty runtime errors!
    //usually it's an error of 2,3 px
    if ((originy + H - 1) >= m_movableImage->height())
        H = m_movableImage->height() - originy;
    if ((originx + W - 1) >= m_movableImage->width())
        W = m_movableImage->width() - originx;

    const QRgb *movVal = NULL;
    const QRgb *pivVal = NULL;
    QRgb* movLine = NULL;
    QRgb* pivLine = NULL;

    //for all the rows that we have to paint
    for(int i = originy; i < originy+H; i++) {
        QRgb* out = (QRgb*)m_previewImage->scanLine(i);

        //if within bounds considering vertical offset
        if ( !( (i - m_my) < 0 || (i - m_my) >= m_movableImage->height()) )
            movLine = (QRgb*)(m_movableImage->scanLine(i - m_my));
        else
            movLine = NULL;

        if ( !( (i - m_py) < 0 || (i- m_py) >= m_pivotImage->height()) )
            pivLine = (QRgb*)(m_pivotImage->scanLine(i - m_py));
        else
            pivLine = NULL;

        //for all the columns that we have to paint
        for(int j = originx; j < originx + W; j++) {
            //if within bounds considering horizontal offset
            if (movLine == NULL || (j - m_mx) < 0 || (j - m_mx) >= m_movableImage->width())
                movVal = &outofbounds;
            else
                movVal = &movLine[j - m_mx];

            if (pivLine == NULL || (j - m_px) < 0 || (j - m_px) >= m_pivotImage->width())
                pivVal = &outofbounds;
            else
                pivVal = &pivLine[j - m_px];

            if (m_pivotImage == m_movableImage)
                out[j] = *movVal;
            else
                out[j] = (this->*rendermode)(movVal,pivVal);
        }
    }
}

void PreviewWidget::resizeEvent(QResizeEvent *event) {
    if (event->size() == m_previewImage->size())
        m_scaleFactor = 1; //done to prevent first spurious widget size (upon construction)
    else
        m_scaleFactor = (float)(event->size().width())/(float)(m_previewImage->size().width());
}

void PreviewWidget::mousePressEvent(QMouseEvent *event) {
    if (event->buttons() == Qt::MidButton) {
        QApplication::setOverrideCursor( QCursor(Qt::ClosedHandCursor) );
        m_mousePos = event->globalPos();
    }
    event->ignore();
}

void PreviewWidget::mouseReleaseEvent(QMouseEvent *event)
{
    QApplication::restoreOverrideCursor();      
    event->ignore();
}

void PreviewWidget::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() == Qt::MidButton) {
        //moving mouse with middle button pans the preview
        QPoint diff = (event->globalPos() - m_mousePos);
        if (event->modifiers() == Qt::ShiftModifier)
            diff *= 5;
        emit moved(diff);
        m_mousePos = event->globalPos();
    }
    event->ignore();
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

    m_prevComputed=QRegion();
    this->update();
}

void PreviewWidget::setPivot(QImage *p, int p_px, int p_py) {
    m_pivotImage = p;
    m_px = p_px;
    m_py = p_py;
    m_prevComputed = QRegion();
}

void PreviewWidget::setPivot(QImage *p) {
    m_pivotImage = p;
}

void PreviewWidget::setMovable(QImage *m, int p_mx, int p_my) {
    m_movableImage = m;
    m_mx = p_mx;
    m_my = p_my;
    m_prevComputed = QRegion();
}

void PreviewWidget::setMovable(QImage *m) {
    m_movableImage = m;
    //TODO: check this
    delete m_previewImage;
    m_previewImage = new QImage(m_movableImage->size(), QImage::Format_ARGB32);
    resize(m_movableImage->size());
}

void PreviewWidget::updateVertShiftMovable(int v) {
    m_my = v;
    m_prevComputed = QRegion();
}

void PreviewWidget::updateHorizShiftMovable(int h) {
    m_mx = h;
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

