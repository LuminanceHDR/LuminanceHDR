/**
 * This file is a part of LuminanceHDR package.
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
 *
 */

#ifndef ISELECTIONTOOL_H
#define ISELECTIONTOOL_H

#include <QGraphicsSceneMouseEvent>
#include "PreviewWidget.h"
#include "Viewers/IGraphicsView.h"

class ISelectionTool : public QObject, public QGraphicsItem
{
Q_OBJECT
public:
    ISelectionTool(IGraphicsView *view, PreviewWidget *widget, QGraphicsItem *parent = 0);
    QRect getSelectionRect();
    bool hasSelection();
    void removeSelection();
    void enable();
    void disable();
    void setScaleFactor(qreal);
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
signals:
    void selectionReady(bool);
    void scroll(int x, int y, int w, int h);
protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *e);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *e);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *e);
    QRectF m_selectionRect;
    QPointF m_mousePos, m_origin;
    bool isSelectionReady;
    enum { NOACTION, PANNING, START_SELECTING, SELECTING, MOVING, RESIZING_LEFT, RESIZING_RIGHT, RESIZING_TOP, RESIZING_BOTTOM, RESIZING_XY} m_action;
    qreal m_scaleFactor;
    IGraphicsView * m_view;
    PreviewWidget *m_widget;
};
#endif
