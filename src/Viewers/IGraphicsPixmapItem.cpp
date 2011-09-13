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
    qDebug() << "IGraphicsPixmapItem::mousePressEvent()";

    mOrigin = event->pos();
    if (!mSelectionBox)
    {
        mSelectionBox = new ISelectionBox(this);
        //this->scene()->addItem(mSelectionBox);
    }
    mSelectionBox->setSelection(QRectF(mOrigin, QSizeF()));
    mSelectionBox->show();
}

void IGraphicsPixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    qDebug() << "IGraphicsPixmapItem::mouseMoveEvent()";

    if (mSelectionBox)
    {
        mSelectionBox->setSelection(QRectF(mOrigin, event->pos()));
    }
}

void IGraphicsPixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    qDebug() << "IGraphicsPixmapItem::mouseReleaseEvent()";

    if (mSelectionBox)
    {
        mSelectionBox->setSelection(QRectF(mOrigin, event->pos()));
    }
}




