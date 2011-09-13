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
#include <QRegion>

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

    setAcceptHoverEvents(true);

    this->update();
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
    // I assume all the anchors have the same size
    const qreal disp_r = mAnchors[0]->getAnchorSize()/2;
    QPointF disp = QPointF(disp_r, disp_r);

    mAnchors[0]->setPos(mSelectedArea.topLeft() - disp);
    mAnchors[1]->setPos(
                QPointF((mSelectedArea.left() + mSelectedArea.right())/2, mSelectedArea.top()) - disp
                ); // top
    mAnchors[2]->setPos(mSelectedArea.topRight() - disp);
    mAnchors[3]->setPos(
                QPointF(mSelectedArea.right(), (mSelectedArea.top()+mSelectedArea.bottom())/2) - disp
                ); // right
    mAnchors[4]->setPos(mSelectedArea.bottomRight() - disp);
    mAnchors[5]->setPos(
                QPointF((mSelectedArea.left() + mSelectedArea.right())/2, mSelectedArea.bottom()) - disp
                ); // bottom
    mAnchors[6]->setPos(mSelectedArea.bottomLeft() - disp);
    mAnchors[7]->setPos(
                QPointF(mSelectedArea.left(), (mSelectedArea.top()+mSelectedArea.bottom())/2 ) - disp
                ); // left
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
#ifdef QT_DEBUG
    qDebug() << "ISelectionBox::hoverEnterEvent()";
#endif
}

void ISelectionBox::hoverLeaveEvent(QGraphicsSceneHoverEvent * event)
{
#ifdef QT_DEBUG
    qDebug() << "ISelectionBox::hoverLeaveEvent()";
#endif
}

QRectF ISelectionBox::boundingRect() const
{
    return mSelectedArea;
}

// example of a drop shadow effect on a box, using QLinearGradient and two boxes

void ISelectionBox::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    // draw outside region
    QRegion outsideArea = mParent->boundingRegion(mParent->sceneTransform()) - QRegion(mSelectedArea.toRect());

    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(127,127,127,127));
    painter->drawRects(outsideArea.rects());

    // draw border
    QPen pen(QColor(255,255,255,255));
    pen.setWidth(1);
    pen.setCapStyle(Qt::FlatCap);
    pen.setJoinStyle(Qt::MiterJoin);
    painter->setPen(pen);
    painter->setBrush(QBrush());
    painter->drawRect(mSelectedArea);

    // draw corners
    setCornerPositions();
}

QPointF ISelectionBox::checkBorders(QPointF in)
{
    QRectF parents_rect = mParent->boundingRect();

    if (in.x() < parents_rect.left()) in.setX(parents_rect.left());
    else if (in.x() > parents_rect.right()) in.setX(parents_rect.right());

    if (in.y() < parents_rect.top()) in.setY(parents_rect.top());
    else if (in.y() > parents_rect.bottom()) in.setY(parents_rect.bottom());

    return in;
}

bool ISelectionBox::sceneEventFilter(QGraphicsItem* watched, QEvent* event)
{
    ISelectionAnchor* anchor = dynamic_cast<ISelectionAnchor *>(watched);
    if (anchor == NULL) return false; // not expected to get here

    QGraphicsSceneMouseEvent* mouse_event = dynamic_cast<QGraphicsSceneMouseEvent*>(event);
    // this is not one of the mouse events we are interested in
    if (mouse_event == NULL) return false;
    //if (mouse_event->button() != Qt::LeftButton) return true;

    switch (mouse_event->type())
    {
    case QEvent::GraphicsSceneMouseMove:
    case QEvent::GraphicsSceneMouseRelease:
    {
        QPointF currCoord = checkBorders(mouse_event->scenePos());

        // TOP_LEFT, TOP, TOP_RIGHT, LEFT, RIGHT, BOTTOM_LEFT, BOTTOM, BOTTOM_RIGHT
        switch(anchor->getCorner())
        {
        case TOP_LEFT:
        {
            mSelectedArea.setTopLeft(currCoord);
        }
            break;
        case TOP_RIGHT:
        {
            mSelectedArea.setTopRight(currCoord);
        }
            break;
        case BOTTOM_RIGHT:
        {
            mSelectedArea.setBottomRight(currCoord);
        }
            break;
        case BOTTOM_LEFT:
        {
            mSelectedArea.setBottomLeft(currCoord);
        }
            break;
        case TOP:
        {
            mSelectedArea.setTop(currCoord.y());
        }
            break;
        case BOTTOM:
        {
            mSelectedArea.setBottom(currCoord.y());
        }
            break;
        case LEFT:
        {
            mSelectedArea.setLeft(currCoord.x());
        }
            break;
        case RIGHT:
        {
            mSelectedArea.setRight(currCoord.x());
        }
            break;
        }
    }
    case QEvent::GraphicsSceneMousePress:
    {
        //save initial position
        //mOrigin = mouse_event->pos();
    }
        break;

    // so the compiler doesn't bitch
    default: { } break;

    }
    this->update();
    return true;
}

//void ISelectionBox::mouseMoveEvent(QGraphicsSceneDragDropEvent *event)
//{
//    event->setAccepted(false);
//}

//void ISelectionBox::mousePressEvent(QGraphicsSceneDragDropEvent *event)
//{
//    event->setAccepted(false);
//}
