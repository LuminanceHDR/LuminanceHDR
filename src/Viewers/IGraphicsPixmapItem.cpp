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
#include "Viewers/ISelectionBox.h"

IGraphicsPixmapItem::IGraphicsPixmapItem(QGraphicsItem *parent)
    : QGraphicsPixmapItem(parent),
      mDropShadow(new QGraphicsDropShadowEffect()),
      mSelectionBox(NULL),
      mIsSelectionEnabled(true) {
    mDropShadow->setBlurRadius(10);
    mDropShadow->setOffset(0, 0);
    // this->setGraphicsEffect(mDropShadow);
    setTransformationMode(Qt::SmoothTransformation);
}

IGraphicsPixmapItem::~IGraphicsPixmapItem() {
    if (mSelectionBox) delete mSelectionBox;

    delete mDropShadow;
}

QRect IGraphicsPixmapItem::getSelectionRect() {
    if (mSelectionBox) {
        return mSelectionBox->getSelection().toRect();
    } else {
        return QRect(0, 0, 0, 0);
    }
}

void IGraphicsPixmapItem::removeSelection() {
    if (mSelectionBox) {
        delete mSelectionBox;
        mSelectionBox = NULL;
        // scene()->update();

        emit selectionReady(false);
    }
}

void IGraphicsPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (!mIsSelectionEnabled && event->button() == Qt::LeftButton) {
        emit startDragging();
        return;
    }

    if (event->button() == Qt::LeftButton) {
        setCursor(Qt::CrossCursor);
    }
}

void IGraphicsPixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if (!mIsSelectionEnabled) {
        event->ignore();
        return;
    }

    mMouseState = SELECTING;

    setCursor(Qt::CrossCursor);

    if (mSelectionBox) {
        QPointF origin = ISelectionBox::checkBorders(
            event->buttonDownScenePos(Qt::LeftButton), this);
        QPointF current = ISelectionBox::checkBorders(event->scenePos(), this);
        mSelectionBox->setSelection(QRectF(origin, current));
    } else {
        mSelectionBox = new ISelectionBox(this);

        QPointF origin = ISelectionBox::checkBorders(
            event->buttonDownScenePos(Qt::LeftButton), this);
        mSelectionBox->setSelection(QRectF(origin, QSizeF()));
        mSelectionBox->show();
    }
}

void IGraphicsPixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if (!mIsSelectionEnabled) return;

    if (event->button() == Qt::LeftButton) {
        switch (mMouseState) {
            case SELECTING: {
                if (mSelectionBox) {
                    QPointF origin = ISelectionBox::checkBorders(
                        event->buttonDownScenePos(Qt::LeftButton), this);
                    QPointF current =
                        ISelectionBox::checkBorders(event->scenePos(), this);
                    mSelectionBox->setSelection(QRectF(origin, current));

                    emit selectionReady(true);
                }
                mMouseState = IDLE;
            } break;

            default:
            case IDLE: {
                if (mSelectionBox) removeSelection();
            } break;
        }

        unsetCursor();
    } else
        event->ignore();
}
