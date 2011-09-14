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
#include <QGraphicsView>

#include "Viewers/IGraphicsPixmapItem.h"

//
//----------------------------------------------------------------------------
//
IGraphicsPixmapItem::IGraphicsPixmapItem(QGraphicsItem *parent):
    QGraphicsPixmapItem(parent), mIsSelectionEnabled(true)
{
    mDropShadow = new QGraphicsDropShadowEffect();
    mDropShadow->setBlurRadius(10);
    mDropShadow->setOffset(0,0);
    //this->setGraphicsEffect(mDropShadow);

    mSelectionBox = NULL;
}

IGraphicsPixmapItem::~IGraphicsPixmapItem()
{
    if ( mSelectionBox ) delete mSelectionBox;

    delete mDropShadow;
}

QRect IGraphicsPixmapItem::getSelectionRect()
{
    if (mSelectionBox)
    {
        return mSelectionBox->getSelection().toRect();
    }
    else
    {
        return QRect(0,0,0,0);
    }
}

void IGraphicsPixmapItem::removeSelection()
{
    if (mSelectionBox)
    {
        delete mSelectionBox;
        mSelectionBox = NULL;
        //scene()->update();

        emit selectionReady(false);
    }
}

void IGraphicsPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (!mIsSelectionEnabled) return;

    if (event->button() == Qt::LeftButton)
    {
#ifdef QT_DEBUG
        //qDebug() << "IGraphicsPixmapItem::mousePressEvent()";
#endif
        setCursor(Qt::CrossCursor);
        if (!mSelectionBox)
        {
            mSelectionBox = new ISelectionBox(this);
            //this->scene()->addItem(mSelectionBox);    // not necessary
        }
        QPointF origin = ISelectionBox::checkBorders(event->buttonDownScenePos(Qt::LeftButton), this);
        mSelectionBox->setSelection(QRectF(origin, QSizeF()));
        mSelectionBox->show();
    }

    if (event->button() == Qt::RightButton)
    {
        removeSelection();
    }
}

void IGraphicsPixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!mIsSelectionEnabled) return;

#ifdef QT_DEBUG
    //qDebug() << "IGraphicsPixmapItem::mouseMoveEvent()";
#endif
    if (mSelectionBox)
    {
        setCursor(Qt::CrossCursor);

        QPointF origin = ISelectionBox::checkBorders(event->buttonDownScenePos(Qt::LeftButton), this);
        QPointF current = ISelectionBox::checkBorders(event->scenePos(), this);
        mSelectionBox->setSelection(QRectF(origin, current));
    }
}

void IGraphicsPixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (!mIsSelectionEnabled) return;

    if (event->button() == Qt::LeftButton)
    {
#ifdef QT_DEBUG
        //qDebug() << "IGraphicsPixmapItem::mouseReleaseEvent()";
#endif
        if (mSelectionBox)
        {
            QPointF origin = ISelectionBox::checkBorders(event->buttonDownScenePos(Qt::LeftButton), this);
            QPointF current = ISelectionBox::checkBorders(event->scenePos(), this);
            mSelectionBox->setSelection(QRectF(origin, current));

            emit selectionReady(true);
        }
        unsetCursor();
    }
}




