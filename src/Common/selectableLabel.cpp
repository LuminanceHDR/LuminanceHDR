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

#include "selectableLabel.h"
#include <iostream> // for debug

SelectableLabel::SelectableLabel(QWidget *parent) : QLabel(parent), selecting(false), isSelectionReady(false), movingSelection(false) {

	rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
        setMouseTracking(true); // need for moving and resizing selection
}

SelectableLabel::~SelectableLabel() {
	delete rubberBand;
}

void SelectableLabel::mousePressEvent(QMouseEvent *e) {
	mousePos = e->globalPos();
	if (e->modifiers()==Qt::ControlModifier) { // Selection 
		selecting = true; // start selection
		isSelectionReady = false;
		origin = e->pos();
     		rubberBand->setGeometry(QRect(origin, QSize()));
     		rubberBand->show();
	}
	if (isSelectionReady) {
		if (rubberBand->geometry().contains(e->pos())) {
			movingSelection = true;	
		}
	}
}

void SelectableLabel::mouseMoveEvent(QMouseEvent *e) {
	if (e->buttons()==Qt::LeftButton) { // That's better for laptop users
		if (selecting) { // Selection
			rubberBand->setGeometry(QRect(origin, e->pos()).normalized());
		}
		else if(movingSelection) {
			QPoint diff = (e->globalPos() - mousePos);
			int x1, y1, x2, y2;
			rubberBand->geometry().getCoords(&x1, &y1, &x2, &y2);
			rubberBand->move(QPoint(x1,y1)+diff);
		}
		else {
			QPoint diff = (e->globalPos() - mousePos);
			if (e->modifiers()==Qt::ShiftModifier)
				diff*=5;
			emit moved(diff);	
		}
		mousePos = e->globalPos();
	}
	if (isSelectionReady) {
		int x1, y1, x2, y2, mouseX, mouseY;
		rubberBand->geometry().getCoords(&x1, &y1, &x2, &y2);
		mouseX = e->x();
		mouseY = e->y();
		
		//std::cout << "x1: " << x1 << " y1: " << y1 << std::endl;
		//std::cout << "x2: " << x2 << " y2: " << y2 << std::endl;
		//std::cout << "mouseX: " << mouseX << " mouseY: " << mouseY << std::endl;
		
		if (rubberBand->geometry().contains(e->pos()))
			setCursor( QCursor(Qt::OpenHandCursor) );
		else if (( mouseX == x1) || (mouseX == x2))
			setCursor( QCursor(Qt::SizeHorCursor) );
		else if ((mouseY == y1) || (mouseY == y2)) 		
			setCursor( QCursor(Qt::SizeVerCursor) );
		else
			setCursor( QCursor(Qt::ArrowCursor) );
		
	}

}

void SelectableLabel::mouseReleaseEvent(QMouseEvent *e) {
	if (selecting) {
		emit selectionReady();
		selecting = false; // end of selection  
		isSelectionReady = true;
	}
	else if (movingSelection) {
		movingSelection = false;
	}
}

QRect SelectableLabel::getSelectionRect() {

	QRect selectionRect;

	int x1, y1, x2, y2; // Selection Area corners
	int img_x1, img_y1, img_x2, img_y2;
	int w, h;
	
	rect().getCoords(&img_x1, &img_y1, &img_x2, &img_y2);
	rubberBand->geometry().getCoords(&x1, &y1, &x2, &y2);

	if (x1 < 0) x1 = 0;
	if (y1 < 0) y1 = 0;
	if (x2 > img_x2) x2 = img_x2;
	if (y2 > img_y2) y2 = img_y2;

	w = x2-x1+1; // width
	h = y2-y1+1; // hight

	selectionRect = QRect(x1, y1, w, h);

	// TODO: Image zoomed or fitted to window
	//std::cout << "x1: " << x1 << " y1: " << y1 << std::endl;
	//std::cout << "x2: " << x2 << " y2: " << y2 << std::endl;
	//std::cout << "img_x1: " << img_x1 << " img_y1: " << img_y1 << std::endl;
	//std::cout << "img_x2: " << img_x2 << " img_y2: " << img_y2 << std::endl;

	return selectionRect;
}

