/**
 * This file is a part of Qtpfsgui package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2006,2007 Giuseppe Rota
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
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 */

#include "show_image.h"
#include <QScrollBar>
#include <cstdlib>

////////////////////////////////////////////////////////////////////////////////////////////////
//			ScrollArea
////////////////////////////////////////////////////////////////////////////////////////////////
ScrollArea::ScrollArea(ShowImage * data): QScrollArea(),mousePos(0,0) {
	setBackgroundRole(QPalette::Dark);
	setWidgetResizable(false);
	setWidget(data);
}

void ScrollArea::mouseMoveEvent(QMouseEvent *e) {
	qDebug("move SA");
	if (e->buttons()==Qt::MidButton) {
		QPoint diff = e->pos() - mousePos;
		if (e->modifiers()==Qt::ShiftModifier)
			diff*=5;
		mousePos = e->pos();
		verticalScrollBar()->setValue(verticalScrollBar()->value() + 
	diff.y());
		horizontalScrollBar()->setValue(horizontalScrollBar()->value() + 
	diff.x());
	}
}

ScrollArea::~ScrollArea() {
// 	qDebug("~ShowImage()");
}

////////////////////////////////////////////////////////////////////////////////////////////////
//			IMAGE VIEWER
////////////////////////////////////////////////////////////////////////////////////////////////
ShowImage::ShowImage(QList<Image *> *p, int listimgs_own_idx, QList<QPair<CP*,CP*> > *_pairs/*, bool _left*/) : QWidget(), ownindex(listimgs_own_idx), ptrlistimages(p), pairs(_pairs), otherindex(0), data(p->at(ownindex)), temp(NULL), dragged(NULL)/*, left(_left)*/ {
	//FIXME: otherindex=0 now, should we add it to the constructor instead?
	this->setCursor(QCursor(QPixmap(":/new/prefix1/images/crosshair.png"),15,15));
// 	qDebug("ShowImage()");
}

void ShowImage::paintEvent(QPaintEvent *e) {
	QPainter painter(this);
	painter.drawPixmap(0,0,QPixmap::fromImage(*(data->payload),Qt::ColorOnly)); //FIXME performance, use e
	for (int i=0;i<data->listcps.count();i++){
		for (int pair_idx=0; pair_idx<pairs->count(); pair_idx++) {
			QPair<CP*,CP*> p=pairs->at(pair_idx);
			if ( (p.first==data->listcps[i] && p.second->whobelongsto==ptrlistimages->at(otherindex)) || (p.second==data->listcps[i] && p.first->whobelongsto==ptrlistimages->at(otherindex)) ) {
// 				qDebug(left? "left: found a point" : "right: found a point");
				painter.setPen(Qt::yellow);
				painter.drawEllipse((int)data->listcps[i]->x-9, (int)data->listcps[i]->y-9, 19,19);
				break;
			}
		}
	}
	if (temp) { //draw temp point
		painter.setPen(Qt::red);
// 		painter.setBrush(Qt::red);
		painter.drawEllipse((int)temp->x-9, (int)temp->y-9, 19, 19);
	}
	if (dragged) {
		painter.setPen(Qt::red);
		painter.drawEllipse((int)dragged->x-9, (int)dragged->y-9, 19, 19);
	}
}

void ShowImage::mousePressEvent(QMouseEvent *e) {
	if (e->button()==Qt::LeftButton) {
		qDebug("press LEFT");
		int mx=e->pos().x(); int my=e->pos().y();
		//there should be no point being dragged
		assert(dragged==NULL);
		if (!temp) {
			//pristine situation, i.e. pair not initialized:
			//let's see if our click is valid for a drag
			for (int i=0;i<data->listcps.count();i++){
				if (abs(mx-(int)data->listcps[i]->x)<9 && abs(my-(int)data->listcps[i]->y)<9)
					dragged=data->listcps[i];
			}
		} else {
			//this happens when the pair is not completed yet (red circle), but we click again the leftbutton: 
			//we drag the temp point.
			dragged=temp;
		}
	}
}

void ShowImage::mouseMoveEvent(QMouseEvent *e) {
	if (e->buttons()==Qt::LeftButton) {
		qDebug("move LEFT");
		if (dragged) {
			qDebug("draggin' now");
			dragged->x=e->pos().x();
			dragged->y=e->pos().y();
			update();
		}
	}
	if (e->buttons()==Qt::MidButton) {
		e->ignore(); //this will send the event to ScrollArea as well
	}
}

void ShowImage::mouseReleaseEvent(QMouseEvent *e) {
	if (e->button()==Qt::LeftButton) {
		qDebug("release LEFT");
		if (!dragged) {
			emit finished_point_in_image(e->pos().x(), e->pos().y());
		} else {
			emit point_finished_moving(dragged);
		}
		dragged=NULL;
		update();
	}
}

void ShowImage::update_other_idx(int other_idx) {
	otherindex=other_idx;
// 	qDebug(left? "left: updating other to %d" : "right: updating other to %d",other_idx);
	update();
}

void ShowImage::clear_temp_CP() {
	temp=NULL;
	update();
}

void ShowImage::update_firstPoint(CP* newcp) {
	temp=newcp;
	update();
}

ShowImage::~ShowImage(){
	//TODO check with valgrind the mem leaks
// 	qDebug("~ShowImage()");
	//data and temp have to be left untouched
// 	if (data) {
// 		qDebug("deleting data->payload"); delete data->payload; data->payload=NULL;
// 		qDebug("deleting data"); delete data; data=NULL;
// 	}
}


