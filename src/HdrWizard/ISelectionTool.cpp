/**
 * This file is a part of LuminanceHDR package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2012 Franco Comida
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

#include <QDebug>

#include <QCursor> 

#include "ISelectionTool.h"

namespace {
bool inRange(qreal a, qreal b, qreal c) {
	return (a >= (b - c)) && (a <= (b + c));
}
}
//
//----------------------------------------------------------------------------
//
ISelectionTool::ISelectionTool(IGraphicsView *view, PreviewWidget* widget, QGraphicsItem* parent) : 
    QGraphicsItem(parent), m_view(view), m_widget(widget) 
{
}
//
// ISelectionTool::ISelectionTool(QGraphicsView *view, PreviewWidget* widget, QGraphicsItem* parent) 
//

void ISelectionTool::mousePressEvent(QGraphicsSceneMouseEvent *e) 
{
	m_mousePos = m_origin = e->pos();
	
	if (e->buttons() == Qt::MidButton) {  
		m_action = PANNING;
		setCursor(QCursor(Qt::SizeAllCursor));
		return;
	}

	if (e->buttons() == Qt::LeftButton) {  
		if (!isSelectionReady) {
			m_action = START_SELECTING;
			m_selectionRect = QRectF(m_origin, QSize());
			setCursor(QCursor(Qt::CrossCursor));
			return;
		}
	
		qreal x1, y1, x2, y2;
		qreal mouseX, mouseY;
		const int bw = (int) 5.0f / m_scaleFactor; // border width
	
		m_selectionRect.getCoords(&x1, &y1, &x2, &y2);

		mouseX = e->pos().x();
		mouseY = e->pos().y();
		
		QRectF innerArea = m_selectionRect;
		innerArea.adjust(bw, bw, -bw, -bw);			
		
		if (innerArea.contains(m_mousePos)) {
			m_action = MOVING;	
			setCursor(QCursor(Qt::SizeAllCursor));
		}
		else if ( inRange(mouseX, x1, bw) && inRange(mouseY, y1, bw)) {
			m_action = RESIZING_XY;
			m_origin = QPointF(x2, y2);
		}
		else if ( inRange(mouseX, x1, bw) && inRange(mouseY, y2, bw)) {
			m_action = RESIZING_XY;
			m_origin = QPointF(x2, y1);
		}
		else if ( inRange(mouseX, x2, bw) && inRange(mouseY, y1, bw)) {
			m_action = RESIZING_XY;
			m_origin = QPointF(x1, y2);
		}
		else if ( inRange(mouseX, x2, bw) && inRange(mouseY, y2, bw)) {
			m_action = RESIZING_XY;
			m_origin = QPointF(x1, y1);
        }
		else if ( inRange(mouseX, x1, bw) ) {
			m_action = RESIZING_LEFT;	
			m_origin = QPointF(x2, y2);
		}
		else if ( inRange(mouseX, x2, bw) ) {
			m_action = RESIZING_RIGHT;
			m_origin = QPointF(x1, y2);
		}	
		else if ( inRange(mouseY, y1, bw) ) {
			m_action = RESIZING_TOP;	
			m_origin = QPointF(x2, y2);
		}
		else if ( inRange(mouseY, y2, bw) ) {
			m_action = RESIZING_BOTTOM;
			m_origin = QPointF(x1, y1);
		}	
		else { // user clicked outside selected region
			removeSelection();
			m_action = START_SELECTING;
			m_selectionRect = QRectF(m_origin, QSize());
		}
	}// if (e->buttons()==Qt::LeftButton) 
    update(boundingRect());
}
//
// void ISelectionTool::mousePressEvent(QMouseEvent *e)
//

void ISelectionTool::mouseMoveEvent(QGraphicsSceneMouseEvent *e) 
{
	if ((e->buttons() == Qt::NoButton) && (!isSelectionReady)) 
	  return;	

	qreal x1, y1, x2, y2;
	qreal mouseX, mouseY;
	qreal bw = (int) 5.0f / m_scaleFactor; // border width	
 	bool updateMousePosition = true;

	mouseX = e->pos().x();
	mouseY =  e->pos().y();
	
	QPointF diff = (e->pos() - m_mousePos);

	m_selectionRect.getCoords(&x1, &y1, &x2, &y2);

	if (isSelectionReady && m_action == NOACTION ) {
		QRectF innerArea = m_selectionRect;
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
			m_view->centerOn(e->pos()); //TODO doesen't work	
			break;
		case START_SELECTING: 
			m_action = SELECTING;
		case SELECTING: 
			m_selectionRect = QRectF(m_origin, e->pos()).normalized();
			break;
		case MOVING:
			m_selectionRect.translate(diff);
			if (!(m_widget->boundingRect().contains(m_selectionRect))) { // stop moving outside pixmap
				m_selectionRect.translate(-diff);
				QCursor::setPos(m_view->mapToGlobal((m_view->mapFromScene(e->pos().toPoint()))));
				updateMousePosition = false;
			}
			else { //moving inside pixmap and visible region
				updateMousePosition = true;
			}
			break;
		case RESIZING_LEFT:
			m_selectionRect = QRectF(m_origin, QPointF(mouseX,y1)).normalized();
			break;
		case RESIZING_RIGHT:
			m_selectionRect = QRectF(m_origin, QPointF(mouseX,y1)).normalized();
			break;
		case RESIZING_TOP:
			m_selectionRect = QRectF(m_origin, QPointF(x1,mouseY)).normalized();
			break;
		case RESIZING_BOTTOM:
			m_selectionRect = QRectF(m_origin, QPointF(x2,mouseY)).normalized();
			break;
		case RESIZING_XY:
			m_selectionRect = QRectF(m_origin, QPointF(mouseX, mouseY)).normalized();
			break;
	}
	if ( !( (m_selectionRect.size() == QSize()) || 
		    (m_selectionRect.width() == 0) 		|| 
		    (m_selectionRect.height() == 0) 
	      )
	   ) 
	{ 
		m_selectionRect = m_widget->boundingRect().intersected(m_selectionRect);
	} 
	if (updateMousePosition)
		m_mousePos = e->pos();

    update(boundingRect());
}
//
// void ISelectionTool::mouseMoveEvent(QMouseEvent *e) 
//


void ISelectionTool::mouseReleaseEvent(QGraphicsSceneMouseEvent *) 
{
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
    update(boundingRect());
}
//
// void ISelectionTool::mouseReleaseEvent(QMouseEvent *e) 
//

QRect ISelectionTool::getSelectionRect() 
{
    return m_selectionRect.toRect();
}
//
// QRect ISelectionTool::getSelectionRect() 
//

void ISelectionTool::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	if (m_widget == NULL) return;

	if (!isSelectionReady && m_action != SELECTING) {
		painter->setPen(Qt::NoPen);
		painter->setBrush(QColor(255,127,127,0));
		painter->drawRect(scene()->sceneRect());
		return;
	}

	qreal pw = 1.0f / m_scaleFactor; //Pen Width
	qreal mpw = 1.0f / m_scaleFactor; // pw >> 1;
	qreal cw = 6.0f / m_scaleFactor; //Corner Width

	//QRegion outsideArea = QRegion(m_widget->boundingRect().toRect()) - QRegion(m_selectionRect.toRect());
	QRegion outsideArea = QRegion(scene()->sceneRect().toRect()) - QRegion(m_selectionRect.toRect());

	painter->setPen(Qt::NoPen);
	painter->setBrush(QColor(127,127,127,127));

	painter->drawRects(outsideArea.rects());
	
	QPen pen(Qt::yellow);
	pen.setWidth(pw);
	pen.setCapStyle(Qt::RoundCap);
	pen.setJoinStyle(Qt::MiterJoin);
    pen.setStyle(Qt::DashLine);
	painter->setPen(pen);
	painter->setBrush(QBrush());
	painter->drawRect(m_selectionRect.adjusted(-mpw, -mpw, mpw, mpw));

	qreal x1, y1, x2, y2; 
	m_selectionRect.getCoords(&x1, &y1, &x2, &y2);
    painter->setPen( Qt::NoPen );
    painter->setBrush( QBrush(Qt::black, Qt::SolidPattern) );
    painter->drawEllipse( x1-cw/2, y1-cw/2, cw, cw);
    painter->drawEllipse( x2-cw/2, y2-cw/2, cw, cw);
    painter->drawEllipse( x1-cw/2, y2-cw/2, cw, cw);
    painter->drawEllipse( x2-cw/2, y1-cw/2, cw, cw);
}
//
// void ISelectionTool::paintEvent(QPaintEvent *e)
//

bool ISelectionTool::hasSelection() 
{
	return isSelectionReady;
}

void ISelectionTool::removeSelection() 
{
	isSelectionReady = false;
	m_action = NOACTION;
	emit selectionReady(false);
	m_selectionRect = QRect();
}
//
// void ISelectionTool::removeSelection() 
//

void ISelectionTool::enable() 
{
	show();
}
//
// void ISelectionTool::removeSelection() 
//

void ISelectionTool::disable() 
{
	hide();
}
//
// void ISelectionTool::disable() 
//

void ISelectionTool::setScaleFactor(qreal sf)
{
    m_scaleFactor = sf;
}

QRectF ISelectionTool::boundingRect() const
{
    return m_widget->boundingRect();
}
