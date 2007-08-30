/**
 * This file is a part of Qtpfsgui package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2007 Giuseppe Rota
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

#include <QPainter>
#include <QApplication>
#include "previewWidget.h"

PreviewWidget::PreviewWidget(QWidget *parent, QImage *m, QImage *p) : QWidget(parent), movableImage(m), pivotImage(p), scrollArea((QScrollArea*)parent), prev_computed() {
// 	this->setAttribute(Qt::WA_PaintOnScreen);
// 	this->setAttribute(Qt::WA_StaticContents);
// 	this->setAttribute(Qt::WA_OpaquePaintEvent);
// 	this->setAttribute(Qt::WA_NoSystemBackground);
	qDebug("PreviewWidget has m=%p and p=%p",m,p);
	mx=my=px=py=0;
	setFocusPolicy(Qt::StrongFocus);
	previewImage=new QImage(movableImage->size(),QImage::Format_ARGB32);
	previewImage->fill(qRgba(255,0,0,255));
	rubberband=QRect();
	blendmode=&PreviewWidget::computeDiffRgba;
	//track mouse position to draw specific cursor when on rubberband
	setMouseTracking(true);
	dragging_rubberband_left=false;
	dragging_rubberband_right=false;
	dragging_rubberband_top=false;
	dragging_rubberband_bottom=false;
	dragging_rubberband_topleft=false;
	dragging_rubberband_topright=false;
	dragging_rubberband_bottomright=false;
	dragging_rubberband_bottomleft=false;
}

PreviewWidget::~PreviewWidget() {
	delete previewImage;
}

void PreviewWidget::paintEvent(QPaintEvent * event) {
// 	qDebug("PreviewWidget::paintEvent");
	if (pivotImage==NULL || movableImage==NULL)
		return;
	QRect paintrect=event->rect();
	QRect srcrect=QRect(paintrect.topLeft()/scaleFactor, paintrect.size()/scaleFactor);
	QPainter p(this);
	QRegion areatorender=QRegion(srcrect)-prev_computed;
	if (!areatorender.isEmpty()) {
// 		qDebug("not cached! O=(%d,%d) w,h=(%dx%d)=%d",areatorender.boundingRect().left(),areatorender.boundingRect().top(),areatorender.boundingRect().width(),areatorender.boundingRect().height(),areatorender.boundingRect().width()*areatorender.boundingRect().height());
		renderPreviewImage(blendmode,areatorender.boundingRect());
		prev_computed+=QRegion(srcrect);
	} /*else {
		QRect br=prev_computed.boundingRect();
		qDebug("cached! : br= O(%d,%d) (%dx%d) src= O(%d,%d) (%dx%d)",br.left(),br.top(),br.width(),br.height(),srcrect.left(),srcrect.top(),srcrect.width(),srcrect.height());
	}*/
	p.drawImage(paintrect, *previewImage, srcrect);
	if (!rubberband.isNull()) {
// 		qDebug("paintEvent (%d,%d)->(%d,%d) w=%d h=%d",rubberband.left(),rubberband.top(),rubberband.right()+1,rubberband.bottom()+1,rubberband.width(),rubberband.height());
		QRegion outsidearea = QRegion(0, 0, int(previewImage->size().width()*scaleFactor), int(previewImage->size().height()*scaleFactor)) - QRegion(rubberband);
		p.setBrush(QBrush(Qt::black,Qt::Dense3Pattern));
		p.setPen(Qt::NoPen);
		p.drawRects(outsidearea.rects());
		p.setBrush(QBrush());
		p.setPen(QPen(Qt::blue, 1, Qt::SolidLine));
		p.drawRect(rubberband);
	}
}

void PreviewWidget::renderPreviewImage(QRgb(PreviewWidget::*fptr)(const QRgb*,const QRgb*)const, const QRect rect ) {
	qDebug("renderPreviewImage");

	int originx=rect.x();
	int originy=rect.y();
	int W=rect.width();
	int H=rect.height();
	if (rect.isNull()) { //requested fullsize render?
		qDebug("requested fullsize render");
		QRegion areatorender= QRegion(QRect(0, 0, previewImage->size().width(), previewImage->size().height())) - prev_computed;
		if (!areatorender.isEmpty()) {
			//render only what you have to
			originx=areatorender.boundingRect().x();
			originy=areatorender.boundingRect().y();
			W=areatorender.boundingRect().width();
			H=areatorender.boundingRect().height();
			prev_computed+=areatorender;
			qDebug("br XY=%d,%d (%d,%d)",originx,originy,W,H);
		} else { //image already rendered fullsize
			qDebug("image already rendered fullsize");
			return;
		}
	}
	QRgb outofbounds=qRgba(0,0,0,255);
	const QRgb *MovVal=NULL;
	const QRgb *PivVal=NULL;
	QRgb* mov_line=NULL;
	QRgb* piv_line=NULL;
	
	//for all the rows that we have to paint
	for(int i = originy; i < originy+H; i++) {
		QRgb* out = (QRgb*)previewImage->scanLine(i);

		//if within bounds considering vertical offset
		if ( !( (i-my)<0 || (i-my)>=movableImage->height()) )
			mov_line = (QRgb*)movableImage->scanLine(i-my);
		else
			mov_line = NULL;
			
		if ( !( (i-py)<0 || (i-py)>=pivotImage->height()) )
			piv_line = (QRgb*)pivotImage->scanLine(i-py);
		else
			piv_line = NULL;
			
		//for all the columns that we have to paint
		for(int j = originx; j < originx+W; j++) {
			//if within bounds considering horizontal offset
			if (mov_line==NULL || (j-mx)<0 || (j-mx)>=movableImage->width())
				MovVal=&outofbounds;
			else
				MovVal=&mov_line[j-mx];

			if (piv_line==NULL || (j-px)<0 || (j-px)>=pivotImage->width())
				PivVal=&outofbounds;
			else
				PivVal=&piv_line[j-px];

			if (pivotImage==movableImage)
				out[j]=*MovVal;
			else
				out[j]= (this->*fptr)(MovVal,PivVal);
		}
	}
}

void PreviewWidget::resizeEvent(QResizeEvent *event) {
	if (event->size()==previewImage->size())
		scaleFactor=1; //done to prevent first spurious widget size (upon construction)
	else
		scaleFactor=(float)(event->size().width())/(float)(previewImage->size().width());

	if (!rubberband.isNull()) {
		float newoldratioW = (float)(event->size().width())/(float)(event->oldSize().width());
		rubberband.setTopLeft(rubberband.topLeft()*newoldratioW);
		rubberband.setBottomRight(rubberband.bottomRight()*newoldratioW);
		if (rubberband.isNull()) {
			hideRubberBand();
		}
	}
	qDebug("PreviewWidget::resizeEvent newsize=(%d,%d) scaleFactor=%f",event->size().width(),event->size().height(),scaleFactor);
}

void PreviewWidget::mousePressEvent(QMouseEvent *event) {
	if (event->buttons()==Qt::MidButton) {
		QApplication::setOverrideCursor( QCursor(Qt::ClosedHandCursor) );
		mousePos = event->pos();
	}

	QPoint mousepos=event->pos();
	if (event->buttons()==Qt::LeftButton) {
		//detect if we are starting a drag
		if (rubberband.contains(mousepos) && !rubberband.contains(mousepos,true)) {
			int x=event->pos().x(); int y=event->pos().y();
			if ((x==rubberband.left())&&(y==rubberband.top()))
				dragging_rubberband_topleft=true;
			else if ((x==rubberband.right())&&(y==rubberband.top()))
				dragging_rubberband_topright=true;
			else if ((x==rubberband.right())&&(y==rubberband.bottom()))
				dragging_rubberband_bottomright=true;
			else if ((x==rubberband.left())&&(y==rubberband.bottom()))
				dragging_rubberband_bottomleft=true;
			else if (x==rubberband.left())
				dragging_rubberband_left=true;
			else if (x==rubberband.right())
				dragging_rubberband_right=true;
			else if (y==rubberband.bottom())
				dragging_rubberband_bottom=true;
			else if (y==rubberband.top())
				dragging_rubberband_top=true;
		} else {
			//single left-click, initialize the coordinates:
			//topleft=bottomright=mousepos. This makes the rubberband have 0-size
			QApplication::setOverrideCursor( QCursor(Qt::CrossCursor) );
			rubberband.setTopLeft(event->pos());
			rubberband.setBottomRight(event->pos()+QPoint(-1,-1));
// 			qDebug("single left-click (%d,%d)->(%d,%d) w=%d h=%d",rubberband.left(),rubberband.top(),rubberband.right()+1,rubberband.bottom()+1,rubberband.width(),rubberband.height());
			emit validCropArea(false);
		}
	}
	event->ignore();
}

void PreviewWidget::mouseMoveEvent(QMouseEvent *event) {
	if (event->buttons()==Qt::LeftButton) {
		int x=event->pos().x(); int y=event->pos().y();
		if (dragging_rubberband_topleft) {
			rubberband.setLeft(x);
			rubberband.setTop(y);
		} else if (dragging_rubberband_topright) {
			rubberband.setTop(y);
			rubberband.setRight(x-1);
		} else if (dragging_rubberband_bottomright) {
			rubberband.setBottom(y-1);
			rubberband.setRight(x-1);
		} else if (dragging_rubberband_bottomleft) {
			rubberband.setBottom(y-1);
			rubberband.setLeft(x);
		} else if (dragging_rubberband_left)
			rubberband.setLeft(x);
		else if (dragging_rubberband_right)
			rubberband.setRight(x-1);
		else if (dragging_rubberband_top)
			rubberband.setTop(y);
		else if (dragging_rubberband_bottom)
			rubberband.setBottom(y-1);
		else {//creating a new selection
			rubberband.setBottomRight(event->pos());
			rubberband=rubberband.normalized();
		}
		update();
	} else if (event->buttons()==Qt::NoButton) {
		//if mouse is over rubberband draw appropriate cursor
		QPoint mousepos=event->pos();
		if (rubberband.contains(mousepos) && !rubberband.contains(mousepos,true)) {
			int x=event->pos().x(); int y=event->pos().y();
			int left=rubberband.left();
			int right=rubberband.right();
			int bottom=rubberband.bottom();
			int top=rubberband.top();
			if ((x==left && y==top) || (x==right && y==bottom))
				QApplication::setOverrideCursor( QCursor(Qt::SizeFDiagCursor) );
			else if ((x==right && y==top) || (x==left && y==bottom))
				QApplication::setOverrideCursor( QCursor(Qt::SizeBDiagCursor) );
			else if (x==left || x==right)
				QApplication::setOverrideCursor( QCursor(Qt::SizeHorCursor) );
			else if (y==bottom || y==top)
				QApplication::setOverrideCursor( QCursor(Qt::SizeVerCursor) );
		} else
			QApplication::restoreOverrideCursor();
	} else if (event->buttons()==Qt::MidButton) {
		QPoint diff = (event->pos() - mousePos)/30.f;
		if (event->modifiers()==Qt::ShiftModifier)
			diff*=5;
		scrollArea->verticalScrollBar()->setValue(scrollArea->verticalScrollBar()->value() + diff.y());
		scrollArea->horizontalScrollBar()->setValue(scrollArea->horizontalScrollBar()->value() + diff.x());
	}
	event->ignore();
}

void PreviewWidget::mouseReleaseEvent(QMouseEvent *event) {
// 	qDebug("mouseReleaseEvent");
	if (event->button()==Qt::LeftButton) {
		dragging_rubberband_left=false;
		dragging_rubberband_right=false;
		dragging_rubberband_top=false;
		dragging_rubberband_bottom=false;
		dragging_rubberband_topleft=false;
		dragging_rubberband_topright=false;
		dragging_rubberband_bottomright=false;
		dragging_rubberband_bottomleft=false;
		rubberband=rubberband.normalized();

		if (!rubberband.isNull())
			emit validCropArea(true);
		rubberband.setLeft(qMax(0,rubberband.left()));
		rubberband.setRight(qMin(size().width()-2,rubberband.right()));
		rubberband.setTop(qMax(rubberband.top(),0));
		rubberband.setBottom(qMin(size().height()-2,rubberband.bottom()));
		QApplication::restoreOverrideCursor();
		update();
	}
	else if (event->button()==Qt::MidButton) {
		QApplication::restoreOverrideCursor();
	}
}

void PreviewWidget::requestedBlendMode(int newindex) {
	if (newindex==0)
		blendmode=& PreviewWidget::computeDiffRgba;
	else if (newindex==1)
		blendmode=& PreviewWidget::computeAddRgba;
	else if (newindex==2)
		blendmode=& PreviewWidget::computeOnlyMovable;
	else if (newindex==3)
		blendmode=& PreviewWidget::computeOnlyPivot;

	prev_computed=QRegion();
	this->update();
}

void PreviewWidget::setPivot(QImage *p, int _px, int _py) {
// 	qDebug("PreviewWidget has p=%p",p);
	pivotImage=p;
	px=_px;
	py=_py;
	prev_computed=QRegion();
}

void PreviewWidget::setMovable(QImage *m, int _mx, int _my) {
// 	qDebug("PreviewWidget has m=%p",m);
	movableImage=m;
	mx=_mx;
	my=_my;
	prev_computed=QRegion();
}

void PreviewWidget::updateVertShiftMovable(int v) {
	my=v;
	prev_computed=QRegion();
}

void PreviewWidget::updateHorizShiftMovable(int h) {
	mx=h;
	prev_computed=QRegion();
}

void PreviewWidget::updateVertShiftPivot(int v) {
	py=v;
	prev_computed=QRegion();
}

void PreviewWidget::updateHorizShiftPivot(int h) {
	px=h;
	prev_computed=QRegion();
}

void PreviewWidget::hideRubberBand() {
	rubberband=QRect();
	emit validCropArea(false);
}
