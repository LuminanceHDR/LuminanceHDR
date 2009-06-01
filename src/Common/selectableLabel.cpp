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

#include <QPainter> 

#include "selectableLabel.h"
#include <iostream> // for debug

bool inRange(int a, int b, int c) {
	return (a >= (b - c)) && (a <= (b + c));
}

SelectableLabel::SelectableLabel(QWidget *parent) : QLabel(parent), action(NOACTION), isSelectionReady(false) {

	rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
        setMouseTracking(true); // needed for moving and resizing selection
}

SelectableLabel::~SelectableLabel() {
	delete rubberBand;
}

void SelectableLabel::mousePressEvent(QMouseEvent *e) {
	mousePos = e->globalPos();
	if (e->modifiers()==Qt::ControlModifier) { // Selection 
		action = SELECTING;
		setCursor( QCursor(Qt::CrossCursor) );
		isSelectionReady = false;
		origin = e->pos();
     		rubberBand->setGeometry(QRect(origin, QSize()));
     		rubberBand->show();
	}
	if (isSelectionReady) {
		int x1, y1, x2, y2;
		int mouseX, mouseY;
		
		rubberBand->geometry().getCoords(&x1, &y1, &x2, &y2);

		mouseX = e->x();
		mouseY = e->y();
		
		QRect innerArea = rubberBand->geometry();
		innerArea.adjust(5, 5, -5, -5);			
		
		if (innerArea.contains(e->pos()))
			action = MOVING;	
		else if ( inRange(mouseX, x1, 5) && inRange(mouseY, y1, 5))
			action = RESIZING_LEFT_TOP;
		else if ( inRange(mouseX, x1, 5) && inRange(mouseY, y2, 5))
			action = RESIZING_LEFT_BOTTOM;
		else if ( inRange(mouseX, x2, 5) && inRange(mouseY, y1, 5))
			action = RESIZING_RIGHT_TOP;
		else if ( inRange(mouseX, x2, 5) && inRange(mouseY, y2, 5))
			action = RESIZING_RIGHT_BOTTOM;
		else if ( inRange(mouseX, x1, 5) )
			action = RESIZING_LEFT;	
		else if ( inRange(mouseX, x2, 5) )
			action = RESIZING_RIGHT;	
		else if ( inRange(mouseY, y1, 5) ) 		
			action = RESIZING_TOP;	
		else if ( inRange(mouseY, y2, 5) ) 		
			action = RESIZING_BOTTOM;	
	}
}

void SelectableLabel::mouseMoveEvent(QMouseEvent *e) {
	int x1, y1, x2, y2;
	QRect newRect;
	rubberBand->geometry().getCoords(&x1, &y1, &x2, &y2);
	
	if (e->buttons()==Qt::LeftButton) { // That's better for laptop users
		QPoint diff = (e->globalPos() - mousePos);
		if (rubberBand->isHidden()) {
			setCursor( QCursor(Qt::ArrowCursor) );
			if (e->modifiers()==Qt::ShiftModifier)
				diff*=5;
			emit moved(diff);	
			mousePos = e->globalPos();
			return;
		}	
		
		switch (action) {
			case SELECTING: 
				rubberBand->setGeometry(QRect(origin, e->pos()).normalized());
				break;
			case MOVING:
				rubberBand->move(QPoint(x1,y1)+diff);
				break;
			case RESIZING_LEFT:
				newRect = rubberBand->geometry();
				newRect.adjust(diff.x(),0,0,0);
				rubberBand->setGeometry(newRect);
				break;
			case RESIZING_RIGHT:
				newRect = rubberBand->geometry();
				newRect.adjust(0,0,diff.x(),0);
				rubberBand->setGeometry(newRect);
				break;
			case RESIZING_TOP:
				newRect = rubberBand->geometry();
				newRect.adjust(0,diff.y(),0,0);
				rubberBand->setGeometry(newRect);
				break;
			case RESIZING_BOTTOM:
				newRect = rubberBand->geometry();
				newRect.adjust(0,0,0,diff.y());
				rubberBand->setGeometry(newRect);
				break;
			case RESIZING_LEFT_TOP:
				newRect = rubberBand->geometry();
				newRect.adjust(diff.x(),diff.y(),0,0);
				rubberBand->setGeometry(newRect);
				break;
			case RESIZING_LEFT_BOTTOM:
				newRect = rubberBand->geometry();
				newRect.adjust(diff.x(),0,0,diff.y());
				rubberBand->setGeometry(newRect);
				break;
			case RESIZING_RIGHT_TOP:
				newRect = rubberBand->geometry();
				newRect.adjust(0,diff.y(),diff.x(),0);
				rubberBand->setGeometry(newRect);
				break;
			case RESIZING_RIGHT_BOTTOM:
				newRect = rubberBand->geometry();
				newRect.adjust(0,0,diff.x(),diff.y());
				rubberBand->setGeometry(newRect);
				break;
			default:
				if (e->modifiers()==Qt::ShiftModifier)
					diff*=5;
				emit moved(diff);	
		}
		mousePos = e->globalPos();
	}
	if (isSelectionReady) {
		int mouseX, mouseY;
		mouseX = e->x();
		mouseY = e->y();
		
		//std::cout << "x1: " << x1 << " y1: " << y1 << std::endl;
		//std::cout << "x2: " << x2 << " y2: " << y2 << std::endl;
		//std::cout << "mouseX: " << mouseX << " mouseY: " << mouseY << std::endl;
		QRect innerArea = rubberBand->geometry();
		innerArea.adjust(5, 5, -5, -5);			
		if (innerArea.contains(e->pos()))
			setCursor( QCursor(Qt::OpenHandCursor) );
		else if ( inRange(mouseX, x1, 5) && inRange(mouseY, y1, 5))
			setCursor( QCursor(Qt::SizeFDiagCursor) );
		else if ( inRange(mouseX, x1, 5) && inRange(mouseY, y2, 5))
			setCursor( QCursor(Qt::SizeBDiagCursor) );
		else if ( inRange(mouseX, x2, 5) && inRange(mouseY, y1, 5))
			setCursor( QCursor(Qt::SizeBDiagCursor) );
		else if ( inRange(mouseX, x2, 5) && inRange(mouseY, y2, 5))
			setCursor( QCursor(Qt::SizeFDiagCursor) );
		else if ( inRange(mouseX, x1, 5) || inRange(mouseX, x2, 5))
			setCursor( QCursor(Qt::SizeHorCursor) );
		else if ( inRange(mouseY, y1, 5) || inRange(mouseY, y2, 5)) 		
			setCursor( QCursor(Qt::SizeVerCursor) );
		else
			setCursor( QCursor(Qt::ArrowCursor) );
		
	}
}

void SelectableLabel::mouseReleaseEvent(QMouseEvent *e) {
	switch (action) {
		case SELECTING:
			emit selectionReady();
			isSelectionReady = true;
		default:
			action = NOACTION;  
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

void SelectableLabel::paintEvent(QPaintEvent *e) {
	QPainter painter(this);
	
	if (pixmap()==NULL) return;

	painter.drawPixmap(0, 0, *pixmap());
	
	if (!isSelectionReady && action != SELECTING) return;

	painter.setPen(Qt::NoPen);
  	painter.setBrush(QColor(255,0,0,127));
	painter.drawRect(geometry());
  	painter.setBrush(QColor(255,0,0,0));
	painter.drawPixmap(rubberBand->geometry(),*pixmap(), rubberBand->geometry());
	update();

}

void SelectableLabel::hideRubberBand() {
	rubberBand->hide();
	isSelectionReady = false;
	action = NOACTION;
	std::cout << "SelectableLabel hideRubberBand" << std::endl;
}

