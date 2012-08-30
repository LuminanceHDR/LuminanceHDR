/**
 * This file is a part of Luminance HDR package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2012 Franco Comida
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
 * @author Franco Comida <fcomida@users.sourceforge.net>
 */

#include <QDebug>

#include <QApplication>
#include <QPainter>

#include "AntiGhostingWidget.h"

AntiGhostingWidget::AntiGhostingWidget(QWidget *parent, QImage *mask): 
    QWidget(parent),
    m_agMask(mask),
    m_agcursorPixmap(NULL),
    m_mx(0),
    m_my(0) 
{
    qDebug() << "AntiGhostingWidget::AntiGhostingWidget";
    //set internal brush values to their default
    m_brushAddMode = true;
    setBrushSize(32);
    m_previousPixmapSize = -1;
    setBrushStrength(255);
    m_previousPixmapStrength = -1;
    m_previousPixmapColor = QColor();
    fillAntiGhostingCursorPixmap();
    setMouseTracking(true);
} 

AntiGhostingWidget::~AntiGhostingWidget()
{
    qDebug() << "~AntiGhostingWidget::AntiGhostingWidget";
    if (m_agcursorPixmap)
        delete m_agcursorPixmap;
}

void AntiGhostingWidget::paintEvent(QPaintEvent *event)
{
    QRect paintrect = event->rect();
    QRect srcrect = QRect(paintrect.topLeft().x()/m_scaleFactor - m_mx, paintrect.topLeft().y()/m_scaleFactor - m_my, paintrect.width()/m_scaleFactor, paintrect.height()/m_scaleFactor);
    QPainter p(this);
    p.drawImage(paintrect, *m_agMask, srcrect);
}

void AntiGhostingWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::MidButton) {
        QApplication::setOverrideCursor( QCursor(Qt::ClosedHandCursor) );
        m_mousePos = event->globalPos();
    }

    if (event->buttons() == Qt::LeftButton) {
        m_timerid = this->startTimer(0);
    }
    event->ignore();
}

void AntiGhostingWidget::mouseMoveEvent(QMouseEvent *event)
{
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

void AntiGhostingWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        this->killTimer(m_timerid);
    }
    else if (event->button() == Qt::MidButton) {
        QApplication::restoreOverrideCursor();      
        fillAntiGhostingCursorPixmap();
        this->unsetCursor();
        this->setCursor(*m_agcursorPixmap);
    }
    event->ignore();
}

void AntiGhostingWidget::resizeEvent(QResizeEvent *event)
{
    m_scaleFactor = (float)(event->size().width())/(float)(m_agMask->size().width());
}

void AntiGhostingWidget::timerEvent(QTimerEvent *) 
{
    QPoint relativeToWidget = mapFromGlobal(QCursor::pos());
    int sx = relativeToWidget.x()/m_scaleFactor - m_mx;
    int sy = relativeToWidget.y()/m_scaleFactor - m_my;
    QPoint scaled(sx,sy);
    QPainter p(m_agMask);
    p.setPen(Qt::NoPen);
    p.setBrush(QBrush(m_requestedPixmapColor, Qt::SolidPattern));
    if (!m_brushAddMode)
        p.setCompositionMode(QPainter::CompositionMode_Clear);
    int pixSize = m_requestedPixmapSize/(2*m_scaleFactor);
    p.drawEllipse(scaled, pixSize, pixSize);
    update();
}

void AntiGhostingWidget::setBrushSize (const int newsize) {
    m_requestedPixmapSize = newsize;
}

void AntiGhostingWidget::setBrushMode(bool removemode) {
    m_requestedPixmapStrength *= -1;
    m_brushAddMode = !removemode;
}

void AntiGhostingWidget::setBrushStrength (const int newstrength) {
    m_requestedPixmapStrength = newstrength;
    m_requestedPixmapColor.setAlpha(qMax(60,m_requestedPixmapStrength));
    m_requestedPixmapStrength *= (!m_brushAddMode) ? -1 : 1;
}

void AntiGhostingWidget::setBrushColor (const QColor newcolor) {
    m_requestedPixmapColor = newcolor;
    update();
}

void AntiGhostingWidget::enterEvent(QEvent *) {
    fillAntiGhostingCursorPixmap();
    this->unsetCursor();
    this->setCursor(*m_agcursorPixmap);
}

void AntiGhostingWidget::switchAntighostingMode(bool ag) {
    if (ag) {
        this->setCursor(*m_agcursorPixmap);
    } else {
        this->unsetCursor();
    }
}

void AntiGhostingWidget::fillAntiGhostingCursorPixmap() {
    if (m_agcursorPixmap)
        delete m_agcursorPixmap;
    m_previousPixmapSize = m_requestedPixmapSize;
    m_previousPixmapStrength = m_requestedPixmapStrength;
    m_previousPixmapColor = m_requestedPixmapColor;
    m_agcursorPixmap = new QPixmap(m_requestedPixmapSize,m_requestedPixmapSize);
    m_agcursorPixmap->fill(Qt::transparent);
    QPainter painter(m_agcursorPixmap);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(m_requestedPixmapColor,Qt::SolidPattern));
    painter.drawEllipse(0,0,m_requestedPixmapSize,m_requestedPixmapSize);
}

void AntiGhostingWidget::updateVertShift(int v) {
    m_my = v;
}

void AntiGhostingWidget::updateHorizShift(int h) {
    m_mx = h;
}

