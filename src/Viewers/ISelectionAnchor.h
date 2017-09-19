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

#ifndef ISELECTIONANCHOR_H
#define ISELECTIONANCHOR_H

#include <QColor>
#include <QGraphicsItem>
#include <QGraphicsRectItem>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QObject>
#include <QPainter>
#include <QPen>
#include <QPointF>

#define ANCHOR_SIZE (8)

enum AnchorPosition {
    TOP_LEFT,
    TOP,
    TOP_RIGHT,
    LEFT,
    RIGHT,
    BOTTOM_LEFT,
    BOTTOM,
    BOTTOM_RIGHT
};
enum AnchorMouseState {
    MOUSE_BUTTON_RELEASED,
    MOUSE_BUTTON_PRESSED,
    MOUSE_MOVING
};

class ISelectionAnchor : public QGraphicsItem {
   public:
    explicit ISelectionAnchor(AnchorPosition position,
                              QGraphicsItem *parent = 0);

    // allows the owner to find out which coner this is
    inline AnchorPosition getCorner() { return mPosition; }
    // allows the owner to record the current mouse state
    inline void setMouseState(AnchorPosition position) { mPosition = position; }
    // allows the owner to get the current mouse state
    inline AnchorMouseState getMouseState() { return mMouseState; }

    qreal getAnchorSize() { return mSize; }

   private:
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

    // once the hover event handlers are implemented in this class,
    // the mouse events must allow be implemented because of
    // some linkage issue - apparrently there is some connection
    // between the hover events and mouseMove/Press/Release
    // events which triggers a vtable issue
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    // virtual void mouseMoveEvent(QGraphicsSceneDragDropEvent *event);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    // virtual void mousePressEvent(QGraphicsSceneDragDropEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    // the hover event handlers will toggle this between red and black
    QColor mAnchorColor;

    qreal mSize;
    AnchorPosition mPosition;
    AnchorMouseState mMouseState;
};

#endif  // ISELECTIONANCHOR_H
