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
#include <QCursor> 
#include <cmath>

#include "selectableLabel.h"
#include <iostream> // for debug

bool inRange(int a, int b, int c) {
	return (a >= (b - c)) && (a <= (b + c));
}

SelectableLabel::SelectableLabel(QWidget *parent) : QLabel(parent), action(NOACTION), isSelectionReady(false) {
	rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
        setMouseTracking(true); // needed for moving and resizing selection
	sizeX = rect().width();
}

SelectableLabel::~SelectableLabel() {
	delete rubberBand;
}

void SelectableLabel::mousePressEvent(QMouseEvent *e) {
	mousePos = e->globalPos();

	if (e->buttons()==Qt::MidButton) {  
		action = PANNING;
		setCursor( QCursor(Qt::SizeAllCursor) );
     		return;
	}

	if (e->buttons()==Qt::LeftButton) {  
		if (!isSelectionReady) {
			action = START_SELECTING;
			origin = e->pos();
     			rubberBand->setGeometry(QRect(origin, QSize()));
			return;
		}
	
		int x1, y1, x2, y2;
		int mouseX, mouseY;
		
		rubberBand->geometry().getCoords(&x1, &y1, &x2, &y2);

		mouseX = e->x();
		mouseY = e->y();
		
		QRect innerArea = rubberBand->geometry();
		innerArea.adjust(5, 5, -5, -5);			
		
		if (innerArea.contains(e->pos()))
			action = MOVING;	
		else if ( inRange(mouseX, x1, 5) && inRange(mouseY, y1, 5)) {
			action = RESIZING_XY;
			origin = QPoint(x2, y2);
		}
		else if ( inRange(mouseX, x1, 5) && inRange(mouseY, y2, 5)) {
			action = RESIZING_XY;
			origin = QPoint(x2, y1);
		}
		else if ( inRange(mouseX, x2, 5) && inRange(mouseY, y1, 5)) {
			action = RESIZING_XY;
			origin = QPoint(x1, y2);
		}
		else if ( inRange(mouseX, x2, 5) && inRange(mouseY, y2, 5)) {
			action = RESIZING_XY;
			origin = QPoint(x1, y1);
		}
		else if ( inRange(mouseX, x1, 5) ) {
			action = RESIZING_LEFT;	
			origin = QPoint(x2, y2);
		}
		else if ( inRange(mouseX, x2, 5) ) {
			action = RESIZING_RIGHT;
			origin = QPoint(x1, y2);
		}	
		else if ( inRange(mouseY, y1, 5) ) {
			action = RESIZING_TOP;	
			origin = QPoint(x2, y2);
		}
		else if ( inRange(mouseY, y2, 5) ) {
			action = RESIZING_BOTTOM;
			origin = QPoint(x1, y1);
		}	
		else {
			removeSelection();
			action = START_SELECTING;
			origin = e->pos();
     			rubberBand->setGeometry(QRect(origin, QSize()));
		     }
	}
}

void SelectableLabel::mouseMoveEvent(QMouseEvent *e) {
	int x1, y1, x2, y2;
	int mouseX, mouseY;
	
	mouseX = e->x();
	mouseY = e->y();
	
	QRect newRect;
	QPoint diff = (e->globalPos() - mousePos);

	rubberBand->geometry().getCoords(&x1, &y1, &x2, &y2);
	
	switch (action) {
		case NOACTION:
			break;
		case PANNING: 
			if (e->modifiers()==Qt::ShiftModifier)
				diff*=5;
			emit moved(diff);	
			break;
		case START_SELECTING: 
			setCursor( QCursor(Qt::CrossCursor) );
			action = SELECTING;
		case SELECTING: 
			rubberBand->setGeometry(QRect(origin, e->pos()).normalized());
			break;
		case MOVING:
			rubberBand->move(QPoint(x1,y1)+diff);
			break;
		case RESIZING_LEFT:
			rubberBand->setGeometry(QRect(origin, QPoint(mouseX,y1)).normalized());
			break;
		case RESIZING_RIGHT:
			rubberBand->setGeometry(QRect(origin, QPoint(mouseX,y1)).normalized());
			break;
		case RESIZING_TOP:
			rubberBand->setGeometry(QRect(origin, QPoint(x1,mouseY)).normalized());
			break;
		case RESIZING_BOTTOM:
			rubberBand->setGeometry(QRect(origin, QPoint(x2,mouseY)).normalized());
			break;
		case RESIZING_XY:
			rubberBand->setGeometry(QRect(origin, e->pos()).normalized());
			break;
	}
	//
	//TODO: improve mouse outside visible region ----------------------
	//
	if (!visibleRegion().contains(e->pos())) {
		//int x1, y1, x2, y2;

		//visibleRegion().boundingRect().getCoords(&x1, &y1, &x2, &y2);
		//QCursor::setPos( mapToGlobal(e->pos()));
		emit moved(2*diff);
	}
	//
	//-------------------------------------------------------------------
	//

	mousePos = e->globalPos();
	
	if (isSelectionReady) {
		
		QRect innerArea = rubberBand->geometry();
		innerArea.adjust(5, 5, -5, -5);			
		if (innerArea.contains(e->pos())) 
			((action == MOVING) || (action == PANNING) ) ? setCursor( QCursor(Qt::SizeAllCursor) ) : 
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
		else if ( action == PANNING )
			setCursor( QCursor(Qt::SizeAllCursor) );
		else 
			setCursor( QCursor(Qt::ArrowCursor) );
		
	}
	if ( !( (rubberBand->size() == QSize()) || 
		(rubberBand->width() == 0) || 
		(rubberBand->height() == 0) || 
		(rubberBand->height() == 1) || 
		(rubberBand->height() == 2) ) 
	   ) { 
			rubberBand->setGeometry(rect().intersected(rubberBand->geometry()));
	}
	repaint();
}

void SelectableLabel::mouseReleaseEvent(QMouseEvent *e) {
	if ( (rubberBand->size() == QSize()) || 
		(rubberBand->width() == 0) || 
		(rubberBand->height() == 0) || 
		(rubberBand->height() == 1) || 
		(rubberBand->height() == 2) 
	   ) { 
		removeSelection();
	}
	switch (action) {
		case START_SELECTING:
			removeSelection();
			action = NOACTION;
			break;
		case SELECTING:			
			emit selectionReady();
			isSelectionReady = true;
		default:
			action = NOACTION;  
	}
	setCursor( QCursor(Qt::ArrowCursor) );
	repaint();
}

QRect SelectableLabel::getSelectionRect() {
	int x1, y1, x2, y2; 
	int img_x1, img_y1, img_x2, img_y2;
	float scaleFactor = (float) pixmap()->width() / (float) sizeX;

	rect().getCoords(&img_x1, &img_y1, &img_x2, &img_y2);
	rubberBand->geometry().getCoords(&x1, &y1, &x2, &y2);

	if (x1 < 0) x1 = 0;
	if (y1 < 0) y1 = 0;
	if (x2 > img_x2) x2 = img_x2;
	if (y2 > img_y2) y2 = img_y2;

	x1 = (int)lround( (float) x1 * scaleFactor );
	y1 = (int)lround( (float) y1 * scaleFactor );
	x2 = (int)lround( (float) x2 * scaleFactor );
	y2 = (int)lround( (float) y2 * scaleFactor );

	return QRect(x1, y1, x2-x1+1, y2-y1+1);
}

void SelectableLabel::paintEvent(QPaintEvent *e) {
	if (pixmap()==NULL) return;
	QLabel::paintEvent(e);
	QPainter painter(this);
	QRect sourceRect;

	if (!isSelectionReady && action != SELECTING) return;

	int x1, y1, x2, y2; 
	int img_x1, img_y1, img_x2, img_y2;
	int pw = 2; //Pen Width
	int dpw = 2*pw;
	int cw = 10; //Corner Width

	rubberBand->geometry().getCoords(&x1, &y1, &x2, &y2);
	rect().getCoords(&img_x1, &img_y1, &img_x2, &img_y2);

	painter.setPen(Qt::NoPen);
	painter.setBrush(QColor(127,127,127,127));
	
	if (abs(y2-y1) == 1) {
		painter.drawRect(0, 0, img_x2, img_y2);
		return;
	}

	painter.drawRect(0, 0, img_x2, y1);
	painter.drawRect(0, y2, img_x2, img_y2);
	painter.drawRect(0, y1, x1, y2-y1);
	painter.drawRect(x2, y1, img_x2 - x2, y2-y1);

	QPen pen(QColor(255,255,255,255));
	pen.setWidth(pw);
	painter.setPen(pen);
	painter.setBrush(QColor(255,0,0,0));
	painter.drawRect(x1-pw, y1-pw, x2-x1+dpw, y2-y1+dpw);

	painter.drawRect(x1-pw, y1-pw, cw, cw);
	painter.drawRect(x2+pw-cw, y1-pw, cw, cw);
	painter.drawRect(x1-pw, y2+pw-cw, cw, cw);
	painter.drawRect(x2+pw-cw, y2+pw-cw, cw, cw);
}

void SelectableLabel::resizeEvent(QResizeEvent *e) {
	int x1, y1, x2, y2;

	int newSize = rect().width();
	float scaleFactor = (float) newSize / (float) sizeX;
	
	sizeX = newSize;

	rubberBand->geometry().getCoords(&x1, &y1, &x2, &y2);
	
	x1 = (int)lround( (float) x1 * scaleFactor );
	y1 = (int)lround( (float) y1 * scaleFactor );
	x2 = (int)lround( (float) x2 * scaleFactor );
	y2 = (int)lround( (float) y2 * scaleFactor );

	rubberBand->setGeometry(x1 , y1, x2-x1+1, y2-y1+1);
}

void SelectableLabel::removeSelection() {
	isSelectionReady = false;
	action = NOACTION;
	emit selectionRemoved();
	repaint();
}

