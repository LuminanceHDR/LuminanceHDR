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

#ifndef IGRAPHICSPIXMAPITEM_H
#define IGRAPHICSPIXMAPITEM_H

#include <QGraphicsDropShadowEffect>
#include <QGraphicsItem>
#include <QGraphicsObject>
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>
#include <QObject>
#include <QRect>

class ISelectionBox;  // forward declaration

class IGraphicsPixmapItem : public QObject, public virtual QGraphicsPixmapItem {
    Q_OBJECT
   public:
    IGraphicsPixmapItem(QGraphicsItem *parent = 0);
    ~IGraphicsPixmapItem();

    QRect getSelectionRect();
    void removeSelection();
    bool hasSelection();

    void enableSelectionTool();
    void disableSelectionTool();

   Q_SIGNALS:
    void selectionReady(bool);
    void startDragging();

   protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *e);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *e);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *e);

    // virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem
    // *option, QWidget *widget);

    QGraphicsDropShadowEffect *mDropShadow;
    ISelectionBox *mSelectionBox;

    bool mIsSelectionEnabled;

    enum { IDLE, SELECTING } mMouseState;
};

inline bool IGraphicsPixmapItem::hasSelection() {
    return (mSelectionBox != NULL);
}

inline void IGraphicsPixmapItem::enableSelectionTool() {
    mIsSelectionEnabled = true;
}

inline void IGraphicsPixmapItem::disableSelectionTool() {
    mIsSelectionEnabled = false;
    removeSelection();
}
#endif
