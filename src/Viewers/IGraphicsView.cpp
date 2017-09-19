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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  US
 * ----------------------------------------------------------------------
 *
 * Original Work
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *
 */

#include "IGraphicsView.h"

#include <QDebug>

IGraphicsView::IGraphicsView(QWidget *parent) : QGraphicsView(parent) {
    init();
}

IGraphicsView::IGraphicsView(QGraphicsScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent) {
    init();
}

void IGraphicsView::init() {
    // setDragMode(QGraphicsView::RubberBandDrag);
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
}

IGraphicsView::~IGraphicsView() {}

void IGraphicsView::wheelEvent(QWheelEvent *event) {
    // get the focus!
    setFocus();

    if (event->modifiers() == Qt::ControlModifier) {
        if (event->delta() > 0) emit zoomIn();
        if (event->delta() < 0) emit zoomOut();
    } else {
        // scrolls up/down - left/right
        QGraphicsView::wheelEvent(event);
    }
}

void IGraphicsView::resizeEvent(QResizeEvent *event) {
    // default resizeEvent handling
    // keeps the center
    QGraphicsView::resizeEvent(event);

    emit viewAreaChangedSize();
}
