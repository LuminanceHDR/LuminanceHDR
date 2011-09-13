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
    QGraphicsPixmapItem(parent)
{
    mSelectionBox = NULL;
}

IGraphicsPixmapItem::~IGraphicsPixmapItem()
{
    if ( mSelectionBox ) delete mSelectionBox;
}

void IGraphicsPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
#ifdef QT_DEBUG
        qDebug() << "IGraphicsPixmapItem::mousePressEvent()";
#endif
        if (!mSelectionBox)
        {
            mSelectionBox = new ISelectionBox(this);
            //this->scene()->addItem(mSelectionBox);    // not necessary
        }
        mSelectionBox->setSelection(QRectF(event->buttonDownScenePos(Qt::LeftButton), QSizeF()));
        mSelectionBox->show();
    }

    if (event->button() == Qt::RightButton)
    {
        if (mSelectionBox)
        {
            delete mSelectionBox;
            mSelectionBox = NULL;
            scene()->update();
        }
    }
}

void IGraphicsPixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
//    if (event->button() == Qt::LeftButton)
//    {
#ifdef QT_DEBUG
        qDebug() << "IGraphicsPixmapItem::mouseMoveEvent()";
#endif
        if (mSelectionBox)
        {
            mSelectionBox->setSelection(QRectF(event->buttonDownScenePos(Qt::LeftButton), event->scenePos()));
        }
//    }
}

void IGraphicsPixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
#ifdef QT_DEBUG
        qDebug() << "IGraphicsPixmapItem::mouseReleaseEvent()";
#endif
        if (mSelectionBox)
        {
            mSelectionBox->setSelection(QRectF(event->buttonDownScenePos(Qt::LeftButton), event->scenePos()));
        }
    }
}




