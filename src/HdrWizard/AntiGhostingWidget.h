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

#ifndef ANTIGHOSTINGWIDGET_H
#define ANTIGHOSTINGWIDGET_H

#include <QGraphicsItem>
#include <QImage>
#include <QPaintEvent>
#include <QGraphicsSceneMouseEvent>
#include <QResizeEvent>
#include <QEvent>

#include "Viewers/IGraphicsView.h"

class AntiGhostingWidget : public QObject, public QGraphicsItem
{
Q_OBJECT

public:
    AntiGhostingWidget(QImage *mask, IGraphicsView *view, QGraphicsItem* parent = 0);
    ~AntiGhostingWidget();
    
    QSize sizeHint () const {
        return m_agMask->size();
    }
    
    void setMask(QImage *mask) {
        m_agMask = mask;
        m_mx = m_my = 0;
    }

    void setHV_offset(QPair<int, int> HV_offset) {
        m_mx = HV_offset.first;
        m_my = HV_offset.second;
    }

    void updateVertShift(int);
    void updateHorizShift(int);

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                QWidget *widget);

public Q_SLOTS:
    void switchAntighostingMode(bool);
    void setBrushSize(const int);
    void setBrushStrength(const int);
    void setBrushColor(const QColor);
    void setBrushMode(bool);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
//    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void timerEvent(QTimerEvent *event);

private:
    QImage *m_agMask;
    QPoint m_mousePos;
    int m_timerid;
    QPixmap *m_agcursorPixmap;
    int m_requestedPixmapSize;
    int m_requestedPixmapStrength;
    QColor m_requestedPixmapColor;
    bool m_brushAddMode;//false means brush is in remove mode.
    void fillAntiGhostingCursorPixmap();
    
    float m_scaleFactor;
    int m_mx, m_my;
    QGraphicsView *m_view;

signals:
    void moved(QPoint diff);
};

#endif
