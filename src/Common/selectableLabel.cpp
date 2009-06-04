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

bool inRange(int a, int b, int c) {
	return (a >= (b - c)) && (a <= (b + c));
}
//
//----------------------------------------------------------------------------
//

SelectableLabel::SelectableLabel(QWidget *parent) : QLabel(parent), action(NOACTION), isSelectionReady(false) {
	rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
        setMouseTracking(true); // needed for moving and resizing selection
	sizeX = rect().width();
}
//
// SelectableLabel::SelectableLabel(QWidget *parent) 
//

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
		const int bw = 5; // border width
	
		rubberBand->geometry().getCoords(&x1, &y1, &x2, &y2);

		mouseX = e->x();
		mouseY = e->y();
		
		QRect innerArea = rubberBand->geometry();
		innerArea.adjust(bw, bw, -bw, -bw);			
		
		if (innerArea.contains(e->pos()))
			action = MOVING;	
		else if ( inRange(mouseX, x1, bw) && inRange(mouseY, y1, bw)) {
			action = RESIZING_XY;
			origin = QPoint(x2, y2);
		}
		else if ( inRange(mouseX, x1, bw) && inRange(mouseY, y2, bw)) {
			action = RESIZING_XY;
			origin = QPoint(x2, y1);
		}
		else if ( inRange(mouseX, x2, bw) && inRange(mouseY, y1, bw)) {
			action = RESIZING_XY;
			origin = QPoint(x1, y2);
		}
		else if ( inRange(mouseX, x2, bw) && inRange(mouseY, y2, bw)) {
			action = RESIZING_XY;
			origin = QPoint(x1, y1);
		}
		else if ( inRange(mouseX, x1, bw) ) {
			action = RESIZING_LEFT;	
			origin = QPoint(x2, y2);
		}
		else if ( inRange(mouseX, x2, bw) ) {
			action = RESIZING_RIGHT;
			origin = QPoint(x1, y2);
		}	
		else if ( inRange(mouseY, y1, bw) ) {
			action = RESIZING_TOP;	
			origin = QPoint(x2, y2);
		}
		else if ( inRange(mouseY, y2, bw) ) {
			action = RESIZING_BOTTOM;
			origin = QPoint(x1, y1);
		}	
		else {
			removeSelection();
			action = START_SELECTING;
			origin = e->pos();
     			rubberBand->setGeometry(QRect(origin, QSize()));
		     }
	}// if (e->buttons()==Qt::LeftButton) 
}
//
// void SelectableLabel::mousePressEvent(QMouseEvent *e)
//

void SelectableLabel::mouseMoveEvent(QMouseEvent *e) {
	int x1, y1, x2, y2;
	int mouseX, mouseY;
	const int bw = 5; // border width	
 	bool updateMousePosition = true;

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
				diff *= 5;
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
			if (!rect().contains(rubberBand->geometry())) {
				rubberBand->move(QPoint(x1,y1)); 
				QCursor::setPos(mousePos);
				updateMousePosition = false;
			}
			else {
				updateMousePosition = true;
			}
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
	//TODO: improve mouse outside visible region
	//	mouse cursor position and moving selection
	//
	const int margin = 40;
	QRect visibleRect = visibleRegion().boundingRect();
	visibleRect.adjust(-margin,-margin,margin,margin);

	if (!(visibleRect.contains(e->pos()))) {
		int x1, y1, x2, y2;
		int dx = 10*diff.x();
		int dy = 10*diff.y();
		
		if (abs(dx) < 2)
			dx *= 10;
		if (abs(dy) < 2)
			dy *= 10;

		visibleRegion().boundingRect().getCoords(&x1, &y1, &x2, &y2);
		
		if (mouseX < x1) {
			if (dx == 0) dx = -20;
			if ( dx < 0 )
				emit moved(QPoint(dx,0));
		}
		else if (mouseX > x2) {
			if (dx == 0) dx = 20;
			if ( dx > 0 )
				emit moved(QPoint(dx,0));
		}
		if (mouseY < y1) {
			if (dy == 0) dy = -20;
			if ( dy < 0 )
				emit moved(QPoint(0,dy));
		}
		else if (mouseY > y2) {
			if (dy == 0) dy = 20;
			if ( dy > 0 )
				emit moved(QPoint(0,dy));
		}
		QCursor::setPos(mousePos);
		updateMousePosition = false;
		
	} 
	else if (action != MOVING) {
		updateMousePosition = true;
	}
	
	if (isSelectionReady) {
		QRect innerArea = rubberBand->geometry();
		innerArea.adjust(bw, bw, -bw, -bw);			
		if (innerArea.contains(e->pos())) 
			((action == MOVING) || (action == PANNING) ) ? setCursor( QCursor(Qt::SizeAllCursor) ) : 
				setCursor( QCursor(Qt::OpenHandCursor) );
		else if ( inRange(mouseX, x1, bw) && inRange(mouseY, y1, bw))
			setCursor( QCursor(Qt::SizeFDiagCursor) );
		else if ( inRange(mouseX, x1, bw) && inRange(mouseY, y2, bw))
			setCursor( QCursor(Qt::SizeBDiagCursor) );
		else if ( inRange(mouseX, x2, bw) && inRange(mouseY, y1, bw))
			setCursor( QCursor(Qt::SizeBDiagCursor) );
		else if ( inRange(mouseX, x2, bw) && inRange(mouseY, y2, bw))
			setCursor( QCursor(Qt::SizeFDiagCursor) );
		else if ( inRange(mouseX, x1, bw) || inRange(mouseX, x2, bw))
			setCursor( QCursor(Qt::SizeHorCursor) );
		else if ( inRange(mouseY, y1, bw) || inRange(mouseY, y2, bw)) 	
			setCursor( QCursor(Qt::SizeVerCursor) );
		else if ( action == PANNING )
			setCursor( QCursor(Qt::SizeAllCursor) );
		else 
			setCursor( QCursor(Qt::ArrowCursor) );
		
	}// if (isSelectionReady) 

	if ( !( (rubberBand->size() == QSize()) || 
		(rubberBand->width() == 0) || 
		(rubberBand->height() == 0) )
	   ) { 
			rubberBand->setGeometry(rect().intersected(rubberBand->geometry()));
	} 
	else {
		if (action == MOVING)
			removeSelection();
	}

	if (updateMousePosition)
		mousePos = e->globalPos();
	
	repaint();
}
//
// void SelectableLabel::mouseMoveEvent(QMouseEvent *e) 
//


void SelectableLabel::mouseReleaseEvent(QMouseEvent *e) {
	if ( (rubberBand->size() == QSize()) || 
		(rubberBand->width() == 0) || 
		(rubberBand->height() == 0)  
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
//
// void SelectableLabel::mouseReleaseEvent(QMouseEvent *e) 
//

QRect SelectableLabel::getSelectionRect() {
	int x1, y1, x2, y2; 
	float scaleFactor = (float) pixmap()->width() / (float) sizeX;

	rubberBand->geometry().getCoords(&x1, &y1, &x2, &y2);

	x1 = (int)lround( (float) x1 * scaleFactor );
	y1 = (int)lround( (float) y1 * scaleFactor );
	x2 = (int)lround( (float) x2 * scaleFactor );
	y2 = (int)lround( (float) y2 * scaleFactor );

	return QRect(x1, y1, x2-x1+1, y2-y1+1);
}
//
// QRect SelectableLabel::getSelectionRect() 
//

void SelectableLabel::paintEvent(QPaintEvent *e) {
	if (pixmap()==NULL) return;
	QLabel::paintEvent(e);

	if (!isSelectionReady && action != SELECTING) return;

	int x1, y1, x2, y2; 
	int pw = 2; //Pen Width
	int cw = 10; //Corner Width

	QPainter painter(this);
	QRect selectedRect = rubberBand->geometry();

	rubberBand->geometry().getCoords(&x1, &y1, &x2, &y2);
	QRegion outsideArea = QRegion(rect()) - QRegion(selectedRect);

	painter.setPen(Qt::NoPen);
	painter.setBrush(QColor(127,127,127,127));

	painter.drawRects(outsideArea.rects());
	
	QPen pen(QColor(255,255,255,255));
	pen.setWidth(pw);
	painter.setPen(pen);
	painter.setBrush(QBrush());
	painter.drawRect(selectedRect.adjusted(-pw,-pw,+pw,+pw));

	painter.drawRect(x1-pw, y1-pw, cw, cw);
	painter.drawRect(x2+pw-cw+1, y1-pw, cw, cw);
	painter.drawRect(x1-pw, y2+pw-cw+1, cw, cw);
	painter.drawRect(x2+pw-cw+1, y2+pw-cw+1, cw, cw);
}
//
// void SelectableLabel::paintEvent(QPaintEvent *e)
//

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
//
// void SelectableLabel::resizeEvent(QResizeEvent *e) 
//

void SelectableLabel::removeSelection() {
	isSelectionReady = false;
	action = NOACTION;
	emit selectionRemoved();
}
//
// void SelectableLabel::removeSelection() 
//

