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

#include <QPainter> 
#include <QCursor> 
//#include <iostream> // debug

#include "SelectionTool.h"

bool inRange(int a, int b, int c) {
	return (a >= (b - c)) && (a <= (b + c));
}

//
//----------------------------------------------------------------------------
//
SelectionTool::SelectionTool(QWidget *parent) : 
	QWidget(parent), isSelectionReady(false), m_action(NOACTION) {
	setMouseTracking(true); // needed for moving and resizing selection
	m_parent = parent;
	m_parent->installEventFilter(this);
}
//
// SelectionTool::SelectionTool(QWidget *parent) 
//


bool SelectionTool::eventFilter(QObject *obj, QEvent *event) {
	if (event->type() == QEvent::Resize) {
		float scaleFactor = (float) m_parent->width() / (float) width();
		resize(m_parent->size());
       	m_selectionRect = QRect(m_selectionRect.topLeft()*scaleFactor, m_selectionRect.size()*scaleFactor);
		return false; // propagate the event
	} 
	else {
		// standard event processing
		return QObject::eventFilter(obj, event);
	}
}
//
// bool SelectionTool::eventFilter(QObject *obj, QEvent *event)
//

void SelectionTool::mousePressEvent(QMouseEvent *e) {
	m_mousePos = e->globalPos();
	m_origin = e->pos();
	
	if (e->buttons() == Qt::MidButton) {  
		m_action = PANNING;
		setCursor(QCursor(Qt::SizeAllCursor));
		return;
	}

	if (e->buttons() == Qt::LeftButton) {  
		if (!isSelectionReady) {
			m_action = START_SELECTING;
			m_selectionRect = QRect(m_origin, QSize());
			setCursor(QCursor(Qt::CrossCursor));
			return;
		}
	
		int x1, y1, x2, y2;
		int mouseX, mouseY;
		const int bw = 5; // border width
	
		m_selectionRect.getCoords(&x1, &y1, &x2, &y2);

		mouseX = e->x();
		mouseY = e->y();
		
		QRect innerArea = m_selectionRect;
		innerArea.adjust(bw, bw, -bw, -bw);			
		
		if (innerArea.contains(e->pos())) {
			m_action = MOVING;	
		}
		else if ( inRange(mouseX, x1, bw) && inRange(mouseY, y1, bw)) {
			m_action = RESIZING_XY;
			m_origin = QPoint(x2, y2);
		}
		else if ( inRange(mouseX, x1, bw) && inRange(mouseY, y2, bw)) {
			m_action = RESIZING_XY;
			m_origin = QPoint(x2, y1);
		}
		else if ( inRange(mouseX, x2, bw) && inRange(mouseY, y1, bw)) {
			m_action = RESIZING_XY;
			m_origin = QPoint(x1, y2);
		}
		else if ( inRange(mouseX, x2, bw) && inRange(mouseY, y2, bw)) {
			m_action = RESIZING_XY;
			m_origin = QPoint(x1, y1);
		}
		else if ( inRange(mouseX, x1, bw) ) {
			m_action = RESIZING_LEFT;	
			m_origin = QPoint(x2, y2);
		}
		else if ( inRange(mouseX, x2, bw) ) {
			m_action = RESIZING_RIGHT;
			m_origin = QPoint(x1, y2);
		}	
		else if ( inRange(mouseY, y1, bw) ) {
			m_action = RESIZING_TOP;	
			m_origin = QPoint(x2, y2);
		}
		else if ( inRange(mouseY, y2, bw) ) {
			m_action = RESIZING_BOTTOM;
			m_origin = QPoint(x1, y1);
		}	
		else { // user clicked outside selected region
			removeSelection();
			m_action = START_SELECTING;
			m_selectionRect = QRect(m_origin, QSize());
		}
	}// if (e->buttons()==Qt::LeftButton) 
}
//
// void SelectionTool::mousePressEvent(QMouseEvent *e)
//

void SelectionTool::mouseMoveEvent(QMouseEvent *e) {
	if ((e->buttons() == Qt::NoButton) && (!isSelectionReady)) 
	  return;	

	int x1, y1, x2, y2;
	int mouseX, mouseY;
	const int bw = 5; // border width	
 	bool updateMousePosition = true;
	QRect visibleRect; 

	mouseX = e->x();
	mouseY = e->y();
	
	QPoint diff = (e->globalPos() - m_mousePos);

	m_selectionRect.getCoords(&x1, &y1, &x2, &y2);

	if (isSelectionReady && m_action == NOACTION ) {
		QRect innerArea = m_selectionRect;
		innerArea.adjust(bw, bw, -bw, -bw);			
		if (innerArea.contains(e->pos())) 
			setCursor(QCursor(Qt::OpenHandCursor));
		else if ( inRange(mouseX, x1, bw) && inRange(mouseY, y1, bw))
			setCursor(QCursor(Qt::SizeFDiagCursor));
		else if ( inRange(mouseX, x1, bw) && inRange(mouseY, y2, bw))
			setCursor(QCursor(Qt::SizeBDiagCursor));
		else if ( inRange(mouseX, x2, bw) && inRange(mouseY, y1, bw))
			setCursor(QCursor(Qt::SizeBDiagCursor));
		else if ( inRange(mouseX, x2, bw) && inRange(mouseY, y2, bw))
			setCursor(QCursor(Qt::SizeFDiagCursor));
		else if ( (inRange(mouseX, x1, bw) || inRange(mouseX, x2, bw)) && (mouseY >= y1) && (mouseY <= y2) )
			setCursor(QCursor(Qt::SizeHorCursor));
		else if ( (inRange(mouseY, y1, bw) || inRange(mouseY, y2, bw)) && (mouseX >= x1) && (mouseX <= x2) ) 	
			setCursor(QCursor(Qt::SizeVerCursor));
		else 
			setCursor(QCursor(Qt::ArrowCursor));
		return;
	}// if (isSelectionReady) 

	
	switch (m_action) {
		case NOACTION:
			break;
		case PANNING: 
			if (e->modifiers() == Qt::ShiftModifier)
				diff *= 5;
			emit moved(diff);	
			break;
		case START_SELECTING: 
			m_action = SELECTING;
		case SELECTING: 
			m_selectionRect = QRect(m_origin, e->pos()).normalized();
			break;
		case MOVING:
			setCursor(QCursor(Qt::SizeAllCursor));
			visibleRect = m_parent->visibleRegion().boundingRect();
			m_selectionRect.translate(diff);
			if (!m_parent->rect().contains(m_selectionRect)) { // stop moving outside pixmap
				m_selectionRect.translate(-diff);
				QCursor::setPos(m_mousePos);
				updateMousePosition = false;
			}
			else { //moving inside pixmap and visible region
				updateMousePosition = true;
			}
			break;
		case RESIZING_LEFT:
			m_selectionRect = QRect(m_origin, QPoint(mouseX,y1)).normalized();
			break;
		case RESIZING_RIGHT:
			m_selectionRect = QRect(m_origin, QPoint(mouseX,y1)).normalized();
			break;
		case RESIZING_TOP:
			m_selectionRect = QRect(m_origin, QPoint(x1,mouseY)).normalized();
			break;
		case RESIZING_BOTTOM:
			m_selectionRect = QRect(m_origin, QPoint(x2,mouseY)).normalized();
			break;
		case RESIZING_XY:
			m_selectionRect = QRect(m_origin, e->pos()).normalized();
			break;
	}
	//
	//TODO: improve moving selection while mouse outside visible region
	//

	if (!(m_parent->visibleRegion().contains(e->pos()))) {
		int x1, y1, x2, y2;
		int dx = 6*diff.x();
		int dy = 6*diff.y();
			
		m_parent->visibleRegion().boundingRect().getCoords(&x1, &y1, &x2, &y2);

		if (m_action == PANNING)
			return;

		if (m_action != MOVING) {

			if (mouseX < x1) {
				if (dx == 0) 
					dx = -20;
				if ( dx < 0 )
					emit moved(QPoint(dx,0));
			}
			else if (mouseX > x2) {
				if (dx == 0) 
					dx = 20;
				if ( dx > 0 )
					emit moved(QPoint(dx,0));
			}
			if (mouseY < y1) {
				if (dy == 0) 
					dy = -20;
				if ( dy < 0 )
					emit moved(QPoint(0,dy));
			}
			else if (mouseY > y2) {
				if (dy == 0) 
					dy = 20;
				if ( dy > 0 )
					emit moved(QPoint(0,dy));
			}
			QCursor::setPos(m_mousePos);
			updateMousePosition = false;
		}
	} 
	else if (m_action != MOVING) {
		updateMousePosition = true;
	}

	if ( !( (m_selectionRect.size() == QSize()) || 
		    (m_selectionRect.width() == 0) 		|| 
		    (m_selectionRect.height() == 0) 
	      )
	   ) 
	{ 
		m_selectionRect = m_parent->rect().intersected(m_selectionRect);
	} 

	if (updateMousePosition)
		m_mousePos = e->globalPos();
	
	repaint();
}
//
// void SelectionTool::mouseMoveEvent(QMouseEvent *e) 
//


void SelectionTool::mouseReleaseEvent(QMouseEvent *) {
	if ( (m_selectionRect.size() == QSize()) || 
		(m_selectionRect.width() == 0) || 
		(m_selectionRect.height() == 0)  
	   ) { 
		removeSelection();
	}
	switch (m_action) {
		case START_SELECTING:
			removeSelection();
			emit selectionReady(false);
			m_action = NOACTION;
			break;
		case SELECTING:
			emit selectionReady(true);
			isSelectionReady = true;
		default:
			m_action = NOACTION;  
	}
	setCursor(QCursor(Qt::ArrowCursor));
	repaint();
}
//
// void SelectionTool::mouseReleaseEvent(QMouseEvent *e) 
//

QRect SelectionTool::getSelectionRect() {
	int x1, y1, x2, y2;
	m_selectionRect.getCoords(&x1, &y1, &x2, &y2);
	return m_selectionRect;
}
//
// QRect SelectionTool::getSelectionRect() 
//

void SelectionTool::paintEvent(QPaintEvent *e) {
	if (m_parent == NULL) return;

	QPainter painter(this);
	
	if (!isSelectionReady && m_action != SELECTING) {
		painter.setPen(Qt::NoPen);
		painter.setBrush(QColor(127,127,127,0));
		painter.drawRect(e->rect());
		return;
	}

	int x1, y1, x2, y2; 
	const int pw = 1; //Pen Width
	const int mpw = 1; // pw >> 1;
	const int cw = 6; //Corner Width

	m_selectionRect.getCoords(&x1, &y1, &x2, &y2);
	QRegion outsideArea = QRegion(rect()) - QRegion(m_selectionRect);

	painter.setPen(Qt::NoPen);
	painter.setBrush(QColor(127,127,127,127));

	painter.drawRects(outsideArea.rects());
	
	QPen pen(QColor(255,255,255,255));
	pen.setWidth(pw);
	pen.setCapStyle(Qt::FlatCap);
	pen.setJoinStyle(Qt::MiterJoin);
	painter.setPen(pen);
	painter.setBrush(QBrush());
	painter.drawRect(m_selectionRect.adjusted(-mpw, -mpw, mpw, mpw));

	painter.drawRect(x1-mpw, y1-mpw, cw, cw);
	painter.drawRect(x2+mpw-cw+1, y1-mpw, cw, cw);
	painter.drawRect(x1-mpw, y2+mpw-cw+1, cw, cw);
	painter.drawRect(x2+mpw-cw+1, y2+mpw-cw+1, cw, cw);
}
//
// void SelectionTool::paintEvent(QPaintEvent *e)
//

bool SelectionTool::hasSelection() {
	return isSelectionReady;
}

void SelectionTool::removeSelection() {
	isSelectionReady = false;
	m_action = NOACTION;
	emit selectionReady(false);
	m_selectionRect = QRect();
	repaint();
}
//
// void SelectionTool::removeSelection() 
//

void SelectionTool::enable() {
	setMouseTracking(true); 
	show();
}
//
// void SelectionTool::removeSelection() 
//

void SelectionTool::disable() {
	setMouseTracking(false); 
	hide();
}
//
// void SelectionTool::disable() 
//



