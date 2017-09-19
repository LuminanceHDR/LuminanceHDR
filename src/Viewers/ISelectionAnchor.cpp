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

#include <QDebug>
#include <QGraphicsView>

#include "Viewers/ISelectionAnchor.h"

ISelectionAnchor::ISelectionAnchor(AnchorPosition position,
                                   QGraphicsItem *parent)
    : QGraphicsItem(parent),
      mAnchorColor(Qt::black),
      mSize(ANCHOR_SIZE),
      mPosition(position),
      mMouseState(MOUSE_BUTTON_RELEASED) {
    setParentItem(parent);

    this->setAcceptHoverEvents(true);
}

// we have to implement the mouse events to keep the linker happy,
// just set accepted to false since are not actually handling them

// void ISelectionAnchor::mouseMoveEvent(QGraphicsSceneDragDropEvent *event)
//{
//    event->setAccepted(false);
//}

// void ISelectionAnchor::mousePressEvent(QGraphicsSceneDragDropEvent *event)
//{
//    event->setAccepted(false);
//}

void ISelectionAnchor::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    mMouseState = MOUSE_BUTTON_RELEASED;
    event->setAccepted(false);
}

void ISelectionAnchor::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    mMouseState = MOUSE_BUTTON_PRESSED;
    event->setAccepted(false);
}

void ISelectionAnchor::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    mMouseState = MOUSE_MOVING;
    event->setAccepted(false);
}

// change the color on hover events to indicate to the use the object has
// been captured by the mouse

void ISelectionAnchor::hoverLeaveEvent(QGraphicsSceneHoverEvent *) {
    unsetCursor();
    mAnchorColor = Qt::black;
    this->update();
}

void ISelectionAnchor::hoverEnterEvent(QGraphicsSceneHoverEvent *) {
    setCursor(Qt::CrossCursor);

    //    switch (mPosition)
    //    {
    //    case TOP_LEFT:
    //    case BOTTOM_RIGHT:
    //        setCursor(Qt::SizeFDiagCursor);
    //        break;
    //    case TOP_RIGHT:
    //    case BOTTOM_LEFT:
    //        setCursor(Qt::SizeBDiagCursor);
    //        break;
    //    case LEFT: case RIGHT:
    //        setCursor(Qt::SizeHorCursor);
    //        break;
    //    case TOP: case BOTTOM:
    //        setCursor(Qt::SizeVerCursor);
    //        break;
    //    }

    mAnchorColor = Qt::red;
    this->update();
}

QRectF ISelectionAnchor::boundingRect() const {
    return QRectF(-mSize / qreal(2.), -mSize / qreal(2.), mSize, mSize)
        .normalized();
}

#include <qmath.h>

void ISelectionAnchor::paint(QPainter *painter,
                             const QStyleOptionGraphicsItem *, QWidget *) {
    QList<QGraphicsView *> views = this->scene()->views();

    // I only consider the first one for speed, being sure that
    if (views.count() >= 1) {
        QGraphicsView *view = views.at(0);
        qreal sf = 1.0 / view->transform().m11();

        mSize = qFloor(ANCHOR_SIZE * sf);
    } else {
        // default values
        mSize = ANCHOR_SIZE;
    }

    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(mAnchorColor, Qt::SolidPattern));
    painter->drawEllipse(
        QRectF(-mSize / qreal(2.), -mSize / qreal(2.), mSize, mSize)
            .normalized());
}
