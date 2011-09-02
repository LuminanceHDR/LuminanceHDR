/**
 * This file is a part of LuminanceHDR package.
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

#ifndef SELECTIONTOOL_H
#define SELECTIONTOOL_H

#include <QMouseEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QRubberBand>

class SelectionTool : public QWidget
{
Q_OBJECT
public:
	SelectionTool(QWidget *parent=0);
	bool setON(bool isON);
	QRect getSelectionRect();
	bool hasSelection();
	void removeSelection();
	void enable();
	void disable();
signals:
        void selectionReady(bool);
        void moved(QPoint diff);
        void scroll(int x, int y, int w, int h);
protected:
        void mousePressEvent(QMouseEvent *e);
        void mouseMoveEvent(QMouseEvent *e);
        void mouseReleaseEvent(QMouseEvent *e);
	void paintEvent(QPaintEvent *e);
	bool eventFilter(QObject *obj, QEvent *event);
private:
        QWidget *m_parent;
 	QRect m_selectionRect;
	QPoint m_mousePos, m_origin;
        bool isSelectionReady;
	enum { NOACTION, PANNING, START_SELECTING, SELECTING, MOVING, RESIZING_LEFT, RESIZING_RIGHT, RESIZING_TOP, RESIZING_BOTTOM, RESIZING_XY} m_action;
};
#endif
