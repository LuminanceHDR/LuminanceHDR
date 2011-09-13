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

#include "Viewers/ISelectionAnchor.h"

ISelectionAnchor::ISelectionAnchor(AnchorPosition position, QGraphicsItem *parent):
    QGraphicsItem(parent),
    mMouseDownX(0),
    mMouseDownY(0),
    _outterborderColor(Qt::black),
    _outterborderPen(),
    mWidth(6),
    mHeight(6),
    mPosition(position),
    mMouseState(MOUSE_RELEASED)
{
    setParentItem(parent);

    _outterborderPen.setWidth(2);
    _outterborderPen.setColor(_outterborderColor);

   this->setAcceptHoverEvents(true);
}

// we have to implement the mouse events to keep the linker happy,
// just set accepted to false since are not actually handling them

void ISelectionAnchor::mouseMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    event->setAccepted(false);
}

void ISelectionAnchor::mousePressEvent(QGraphicsSceneDragDropEvent *event)
{
    event->setAccepted(false);
}

void ISelectionAnchor::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    event->setAccepted(true);
}

void ISelectionAnchor::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    event->setAccepted(false);
}

void ISelectionAnchor::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
    event->setAccepted(false);
}


// change the color on hover events to indicate to the use the object has
// been captured by the mouse

void ISelectionAnchor::hoverLeaveEvent(QGraphicsSceneHoverEvent *)
{
    _outterborderColor = Qt::black;
    this->update(0,0,mWidth,mHeight);
}

void ISelectionAnchor::hoverEnterEvent (QGraphicsSceneHoverEvent *)
{
    _outterborderColor = Qt::red;
    this->update(0,0,mWidth,mHeight);
}

QRectF ISelectionAnchor::boundingRect() const
{
    return QRectF(0,0,mWidth,mHeight);
}


void ISelectionAnchor::paint (QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    // fill the box with solid color, use sharp corners
    _outterborderPen.setCapStyle(Qt::SquareCap);
    _outterborderPen.setStyle(Qt::SolidLine);
    painter->setPen(_outterborderPen);

    QPointF topLeft(0, 0);
    QPointF bottomRight(mWidth, mHeight);

    QRectF rect(topLeft, bottomRight);

    QBrush brush(Qt::SolidPattern);
    brush.setColor (_outterborderColor);
    painter->fillRect(rect,brush);

}
