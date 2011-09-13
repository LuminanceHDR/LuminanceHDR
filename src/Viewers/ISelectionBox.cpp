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
#include <QGraphicsScene>

#include "Viewers/ISelectionBox.h"

ISelectionBox::ISelectionBox(QGraphicsPixmapItem * parent):
    QGraphicsItem(parent),
    mParent(parent), mSelectedArea(0, 0, 0, 0), mIsEnable(true)
{
    // 0 ---- 1 -----2
    // |             |
    // 7      +      3
    // |             |
    // 6------5------4

    // create anchors
    mAnchors[0] = new ISelectionAnchor(TOP_LEFT, this);
    mAnchors[1] = new ISelectionAnchor(TOP, this);
    mAnchors[2] = new ISelectionAnchor(TOP_RIGHT, this);
    mAnchors[3] = new ISelectionAnchor(RIGHT, this);
    mAnchors[4] = new ISelectionAnchor(BOTTOM_RIGHT, this);
    mAnchors[5] = new ISelectionAnchor(BOTTOM, this);
    mAnchors[6] = new ISelectionAnchor(BOTTOM_LEFT, this);
    mAnchors[7] = new ISelectionAnchor(LEFT, this);

    mAnchors[0]->installSceneEventFilter(this);
    mAnchors[1]->installSceneEventFilter(this);
    mAnchors[2]->installSceneEventFilter(this);
    mAnchors[3]->installSceneEventFilter(this);
    mAnchors[4]->installSceneEventFilter(this);
    mAnchors[5]->installSceneEventFilter(this);
    mAnchors[6]->installSceneEventFilter(this);
    mAnchors[7]->installSceneEventFilter(this);

    setCornerPositions();
}

ISelectionBox::~ISelectionBox()
{
    delete mAnchors[0];
    delete mAnchors[1];
    delete mAnchors[2];
    delete mAnchors[3];
    delete mAnchors[4];
    delete mAnchors[5];
    delete mAnchors[6];
    delete mAnchors[7];
}

void ISelectionBox::setCornerPositions()
{
    mAnchors[0]->setPos(mSelectedArea.topLeft());
    mAnchors[1]->setPos((mSelectedArea.left() + mSelectedArea.right())/2, mSelectedArea.top()); // top
    mAnchors[2]->setPos(mSelectedArea.topRight());
    mAnchors[3]->setPos(mSelectedArea.right(), (mSelectedArea.top()+mSelectedArea.bottom())/2); // right
    mAnchors[4]->setPos(mSelectedArea.bottomRight());
    mAnchors[5]->setPos((mSelectedArea.left() + mSelectedArea.right())/2, mSelectedArea.bottom()); // bottom
    mAnchors[6]->setPos(mSelectedArea.bottomLeft());
    mAnchors[7]->setPos(mSelectedArea.left(), (mSelectedArea.top()+mSelectedArea.bottom())/2); // left
}

void ISelectionBox::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
#ifdef QT_DEBUG
    qDebug() << "ISelectionBox::mousePressEvent()";
#endif

    if (!mIsEnable) return;
}

void ISelectionBox::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
#ifdef QT_DEBUG
    qDebug() << "ISelectionBox::mouseMoveEvent()";
#endif

}

void ISelectionBox::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
#ifdef QT_DEBUG
    qDebug() << "ISelectionBox::mouseReleaseEvent()";
#endif


}

void ISelectionBox::hoverEnterEvent(QGraphicsSceneHoverEvent * event)
{

}

void ISelectionBox::hoverLeaveEvent(QGraphicsSceneHoverEvent * event)
{

}

QRectF ISelectionBox::boundingRect() const
{
    return mSelectedArea;
}

// example of a drop shadow effect on a box, using QLinearGradient and two boxes

void ISelectionBox::paint (QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{

    setCornerPositions();
}

bool ISelectionBox::sceneEventFilter(QGraphicsItem * watched, QEvent * event)
{
#ifdef QT_DEBUG
    qDebug() << " QEvent == " + QString::number(event->type());
#endif

    ISelectionAnchor* anchor = dynamic_cast<ISelectionAnchor *>(watched);
    if (anchor == NULL) return false; // not expected to get here

    QGraphicsSceneMouseEvent* mouse_event = dynamic_cast<QGraphicsSceneMouseEvent*>(event);
    // this is not one of the mouse events we are interested in
    if (mouse_event == NULL) return false;

    return true;
}

void ISelectionBox::mouseMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    event->setAccepted(false);
}

void ISelectionBox::mousePressEvent(QGraphicsSceneDragDropEvent *event)
{
    event->setAccepted(false);
}
