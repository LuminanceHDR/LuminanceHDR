/**
 * This file is a part of Luminance HDR package.
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

#include <cassert>
#include <QPainter>
#include <QApplication>

#include "PreviewWidget.h"

PreviewWidget::PreviewWidget(QWidget *parent, QImage *m, const QImage *p) : QWidget(parent), movableImage(m), pivotImage(p), prev_computed() {
	mx=my=px=py=0;
	setFocusPolicy(Qt::StrongFocus);
	previewImage=new QImage(movableImage->size(),QImage::Format_ARGB32);
	previewImage->fill(qRgba(255,0,0,255));
	blendmode=&PreviewWidget::computeDiffRgba;
	leftButtonMode=LB_nomode;
	setMouseTracking(true);
}

PreviewWidget::~PreviewWidget() {
	delete previewImage;
}

void PreviewWidget::paintEvent(QPaintEvent * event) {
	if (pivotImage==NULL || movableImage==NULL)
		return;
	assert(movableImage->size()==pivotImage->size());
	QRect paintrect=event->rect();
	QRect srcrect=QRect(paintrect.topLeft()/scaleFactor, paintrect.size()/scaleFactor);
	QPainter p(this);
	QRegion areatorender=QRegion(srcrect)-prev_computed;
	if (!areatorender.isEmpty()) {
		renderPreviewImage(blendmode,areatorender.boundingRect());
		prev_computed+=QRegion(srcrect);
	}
	p.drawImage(paintrect, *previewImage, srcrect);
}

QRgb outofbounds=qRgba(0,0,0,255);

void PreviewWidget::renderPreviewImage(QRgb(PreviewWidget::*rendermode)(const QRgb*,const QRgb*)const, const QRect rect ) {
	int originx=rect.x();
	int originy=rect.y();
	int W=rect.width();
	int H=rect.height();
	if (rect.isNull()) {
		//requested fullsize render
		QRegion areatorender= QRegion(QRect(0, 0, previewImage->size().width(), previewImage->size().height())) - prev_computed;
		if (!areatorender.isEmpty()) {
			//render only what you have to
			originx=areatorender.boundingRect().x();
			originy=areatorender.boundingRect().y();
			W=areatorender.boundingRect().width();
			H=areatorender.boundingRect().height();
			prev_computed+=areatorender;
		} else //image already rendered fullsize
			return;
	}
	//these kind of things can happen and lead to strange and nasty runtime errors!
	//usually it's an error of 2,3 px
	if ((originy+H-1)>=movableImage->height())
		H=movableImage->height()-originy;
	if ((originx+W-1)>=movableImage->width())
		W=movableImage->width()-originx;
	const QRgb *MovVal=NULL;
	const QRgb *PivVal=NULL;
	QRgb* mov_line=NULL;
	QRgb* piv_line=NULL;

	//for all the rows that we have to paint
	for(int i = originy; i < originy+H; i++) {
		QRgb* out = (QRgb*)previewImage->scanLine(i);

		//if within bounds considering vertical offset
		if ( !( (i-my)<0 || (i-my)>=movableImage->height()) )
			mov_line = (QRgb*)(movableImage->scanLine(i-my));
		else
			mov_line = NULL;

		if ( !( (i-py)<0 || (i-py)>=pivotImage->height()) )
			piv_line = (QRgb*)(pivotImage->scanLine(i-py));
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
				out[j]= (this->*rendermode)(MovVal,PivVal);
		}
	}
}

void PreviewWidget::resizeEvent(QResizeEvent *event) {
	if (event->size()==previewImage->size())
		scaleFactor=1; //done to prevent first spurious widget size (upon construction)
	else
		scaleFactor=(float)(event->size().width())/(float)(previewImage->size().width());
}

void PreviewWidget::mousePressEvent(QMouseEvent *event) {
	if (event->buttons()==Qt::MidButton) {
		QApplication::setOverrideCursor( QCursor(Qt::ClosedHandCursor) );
		mousePos = event->globalPos();
	}
}

void PreviewWidget::mouseMoveEvent(QMouseEvent *event) {
	if (event->buttons()==Qt::MidButton) {
		//moving mouse with middle button pans the preview
		QPoint diff = (event->globalPos() - mousePos);
		if (event->modifiers()==Qt::ShiftModifier)
			diff*=5;
		emit moved(diff);
		mousePos=event->globalPos();
	}
	event->ignore();
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

void PreviewWidget::setPivot(QImage *p, int p_px, int p_py) {
	pivotImage=p;
    px=p_px;
    py=p_py;
	prev_computed=QRegion();
}

void PreviewWidget::setPivot(QImage *p) {
	pivotImage = p;
}

void PreviewWidget::setMovable(QImage *m, int p_mx, int p_my) {
	movableImage=m;
    mx=p_mx;
    my=p_my;
	prev_computed=QRegion();
}

void PreviewWidget::setMovable(QImage *m) {
	movableImage = m;
	//TODO: check this
	delete previewImage;
        previewImage=new QImage(movableImage->size(),QImage::Format_ARGB32);
	resize(movableImage->size());
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

