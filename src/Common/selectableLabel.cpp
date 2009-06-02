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
#include <cmath>

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
     			rubberBand->show();
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
		else {
			removeSelection();
			action = START_SELECTING;
			origin = e->pos();
     			rubberBand->setGeometry(QRect(origin, QSize()));
     			rubberBand->show();
		     }
	}
}

void SelectableLabel::mouseMoveEvent(QMouseEvent *e) {
	int x1, y1, x2, y2;
	QRect newRect;
	
	QPoint diff = (e->globalPos() - mousePos);
	rubberBand->geometry().getCoords(&x1, &y1, &x2, &y2);
	
	switch (action) {
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
			newRect = rubberBand->geometry();
			newRect.adjust(diff.x(),0,0,0);
			rubberBand->setGeometry(newRect.normalized());
			break;
		case RESIZING_RIGHT:
			newRect = rubberBand->geometry();
			newRect.adjust(0,0,diff.x(),0);
			rubberBand->setGeometry(newRect.normalized());
			break;
		case RESIZING_TOP:
			newRect = rubberBand->geometry();
			newRect.adjust(0,diff.y(),0,0);
			rubberBand->setGeometry(newRect.normalized());
			break;
		case RESIZING_BOTTOM:
			newRect = rubberBand->geometry();
			newRect.adjust(0,0,0,diff.y());
			rubberBand->setGeometry(newRect.normalized());
			break;
		case RESIZING_LEFT_TOP:
			newRect = rubberBand->geometry();
			newRect.adjust(diff.x(),diff.y(),0,0);
			rubberBand->setGeometry(newRect.normalized());
			break;
		case RESIZING_LEFT_BOTTOM:
			newRect = rubberBand->geometry();
			newRect.adjust(diff.x(),0,0,diff.y());
			rubberBand->setGeometry(newRect.normalized());
			break;
		case RESIZING_RIGHT_TOP:
			newRect = rubberBand->geometry();
			newRect.adjust(0,diff.y(),diff.x(),0);
			rubberBand->setGeometry(newRect.normalized());
			break;
		case RESIZING_RIGHT_BOTTOM:
			newRect = rubberBand->geometry();
			newRect.adjust(0,0,diff.x(),diff.y());
			rubberBand->setGeometry(newRect.normalized());
			break;
	}
	mousePos = e->globalPos();
	
	if (isSelectionReady) {
		int mouseX, mouseY;
		mouseX = e->x();
		mouseY = e->y();
		
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
	repaint();
}

void SelectableLabel::mouseReleaseEvent(QMouseEvent *e) {
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

	int x1, y1, x2, y2; // Selection Area corners
	int img_x1, img_y1, img_x2, img_y2;
	float scaleFactor = (float) pixmap()->width() / (float) geometry().width();
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
	float scaleFactor = (float) pixmap()->width() / (float) geometry().width();
	rubberBand->geometry().getCoords(&x1, &y1, &x2, &y2);
	
	x1 = (int)lround( (float) x1 * scaleFactor );
	y1 = (int)lround( (float) y1 * scaleFactor );
	x2 = (int)lround( (float) x2 * scaleFactor );
	y2 = (int)lround( (float) y2 * scaleFactor );
		
	sourceRect = QRect(x1, y1, x2-x1+1, y2-y1+1);

	painter.setPen(Qt::NoPen);
	painter.setBrush(QColor(127,127,127,127));
	painter.drawRect(rect());
	painter.setBrush(QColor(255,0,0,0));
	painter.drawPixmap(rubberBand->geometry(), *pixmap(), sourceRect);
}

void SelectableLabel::resizeEvent(QResizeEvent *e) {
	int x1, y1, x2, y2;
	float scaleFactor = (float) e->size().width() / (float) e->oldSize().width();
	if (((1.0 - scaleFactor) < 0.01) && ((1.0 - scaleFactor) > -0.01)) return;
	if (scaleFactor < -2.0) return;

	rubberBand->geometry().getCoords(&x1, &y1, &x2, &y2);
	
	x1 = (int)lround( (float) x1 * scaleFactor );
	y1 = (int)lround( (float) y1 * scaleFactor );
	x2 = (int)lround( (float) x2 * scaleFactor );
	y2 = (int)lround( (float) y2 * scaleFactor );

	rubberBand->setGeometry(x1 , y1, x2-x1+1, y2-y1+1);
}

void SelectableLabel::removeSelection() {
	rubberBand->hide();
	isSelectionReady = false;
	action = NOACTION;
	emit selectionRemoved();
	repaint();
}

