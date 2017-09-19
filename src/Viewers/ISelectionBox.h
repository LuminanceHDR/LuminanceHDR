/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2011 Davide Anastasia
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
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *
 */

#ifndef ISELECTIONTOOL_H
#define ISELECTIONTOOL_H

#include <QGraphicsItem>
#include <QGraphicsObject>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsSceneMouseEvent>
#include <QPointF>

#include "Viewers/ISelectionAnchor.h"

class ISelectionBox : public QGraphicsItem {
   public:
    ISelectionBox(QGraphicsPixmapItem *parent = 0);
    ~ISelectionBox();

    void setSelection(QRectF selection);
    QRectF getSelection();

    // checks and modifies that a certain point is not outside the parent's area
    static QPointF checkBorders(QPointF, QGraphicsItem *);

   protected:
    // must be re-implemented in this class to provide the diminsions of the box
    // to the QGraphicsView
    virtual QRectF boundingRect() const;
    // must be re-implemented here to pain the box on the paint-event
    virtual void paint(QPainter *painter,
                       const QStyleOptionGraphicsItem *option, QWidget *widget);
    // must be re-implemented to handle mouse hover enter events
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    // must be re-implemented to handle mouse hover leave events
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

    // allows the main object to be moved in the scene by capturing the mouse
    // move
    // events
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    // virtual void mouseMoveEvent(QGraphicsSceneDragDropEvent *event);
    // virtual void mousePressEvent(QGraphicsSceneDragDropEvent *event);
    virtual bool sceneEventFilter(QGraphicsItem *watched, QEvent *event);

    void setCornerPositions();

    QGraphicsPixmapItem *mParent;

    ISelectionAnchor *mAnchors[8];

    QRectF mSelectedArea;
};
#endif
