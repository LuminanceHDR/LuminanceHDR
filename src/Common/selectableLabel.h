/**
 * This file is a part of Qtpfsgui package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2009 Franco Comida
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
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#ifndef SELECTABLELABEL_H
#define SELECTABLELABEL_H

#include <QLabel>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QRubberBand>

enum Actions { NOACTION, PANNING, START_SELECTING, SELECTING, MOVING, RESIZING_LEFT, RESIZING_RIGHT, RESIZING_TOP, RESIZING_BOTTOM, RESIZING_LEFT_TOP, RESIZING_LEFT_BOTTOM, RESIZING_RIGHT_TOP, RESIZING_RIGHT_BOTTOM};	

class SelectableLabel : public QLabel
{
Q_OBJECT
public:
	SelectableLabel(QWidget *parent=0);
	~SelectableLabel();
	QRect getSelectionRect();
	void removeSelection();
signals:
        void selectionReady();
        void selectionRemoved();
        void moved(QPoint diff);
protected:
        void mousePressEvent(QMouseEvent *e);
        void mouseMoveEvent(QMouseEvent *e);
        void mouseReleaseEvent(QMouseEvent *e);
	void paintEvent(QPaintEvent *e);
	void resizeEvent(QResizeEvent *e);
private:
        QRubberBand *rubberBand;
        QPoint mousePos, origin;
	Actions action;
        bool isSelectionReady;
};
#endif
