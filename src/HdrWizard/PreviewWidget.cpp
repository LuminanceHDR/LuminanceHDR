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

PreviewWidget::PreviewWidget(QWidget *parent, /*const*/ QImage *m, const QImage *p) : QWidget(parent), movableImage(m), pivotImage(p), prev_computed(), agcursor_pixmap(NULL) {
// 	this->setAttribute(Qt::WA_PaintOnScreen);
// 	this->setAttribute(Qt::WA_StaticContents);
// 	this->setAttribute(Qt::WA_OpaquePaintEvent);
// 	this->setAttribute(Qt::WA_NoSystemBackground);
	mx=my=px=py=0;
	setFocusPolicy(Qt::StrongFocus);
	previewImage=new QImage(movableImage->size(),QImage::Format_ARGB32);
	previewImage->fill(qRgba(255,0,0,255));
	blendmode=&PreviewWidget::computeDiffRgba;
	leftButtonMode=LB_nomode;
	setMouseTracking(true);
	//set internal brush values to their default
	brushAddMode=true;
	setBrushSize(32);
	previousPixmapSize=-1;
	setBrushStrength(255);
	previousPixmapStrength=-1;
	previousPixmapColor=QColor();
	fillAntiGhostingCursorPixmap();
}

PreviewWidget::~PreviewWidget() {
	delete previewImage;
	delete agcursor_pixmap;
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

			if (pivotImage==movableImage && blendmode != & PreviewWidget::computeAntiGhostingMask)
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

	QPoint mousepos=event->pos();
	if (event->buttons()==Qt::LeftButton) {
		if (leftButtonMode ==  LB_antighostingmode && scaleFactor == 1)
			timerid=this->startTimer(0);
	}
	event->ignore();
}

void PreviewWidget::mouseMoveEvent(QMouseEvent *event) {
	if (event->buttons()==Qt::MidButton) {
		//moving mouse with middle button pans the preview
		QPoint diff = (event->globalPos() - mousePos);
		if (event->modifiers()==Qt::ShiftModifier)
			diff*=5;
		emit moved(diff);
		//scrollArea->verticalScrollBar()->setValue(scrollArea->verticalScrollBar()->value() + diff.y());
		//scrollArea->horizontalScrollBar()->setValue(scrollArea->horizontalScrollBar()->value() + diff.x());
		mousePos=event->globalPos();
	}
	event->ignore();
}

void PreviewWidget::mouseReleaseEvent(QMouseEvent *event) {
	if (event->button()==Qt::LeftButton) {
		if (leftButtonMode == LB_antighostingmode && scaleFactor == 1)
			this->killTimer(timerid);
	}
}

void PreviewWidget::timerEvent(QTimerEvent *) {
	if (scaleFactor!=1)
		return;

	QPoint relativeToWidget=mapFromGlobal(QCursor::pos());

	QPoint imagereferredPoint= relativeToWidget/scaleFactor -QPoint(mx,my+1);

	QPoint halfsize((int)(requestedPixmapSize/(2*scaleFactor)), (int)(requestedPixmapSize/(2*scaleFactor)));

	QRect imagereferredBoundingRect( imagereferredPoint-halfsize, imagereferredPoint+halfsize );
// 	QRect localBoundingRect ( localPoint-QPoint(requestedPixmapSize/2,requestedPixmapSize/2), localPoint+QPoint(requestedPixmapSize/2,requestedPixmapSize/2) );

	QRegion imagereferredBrushArea(imagereferredBoundingRect,QRegion::Ellipse);

	for (int row=qMax(0,imagereferredBoundingRect.top()); row <= qMin(movableImage->size().height()-1,imagereferredBoundingRect.bottom()); row++) {
		for (int col=qMax(0,imagereferredBoundingRect.left()); col <= qMin(movableImage->size().width()-1,imagereferredBoundingRect.right()); col++) {
			QPoint p(col,row);
			if (imagereferredBrushArea.contains(p)) {
				QColor pxval= QColor::fromRgba(movableImage->pixel(p.x(),p.y()));
				pxval.setAlpha(qMin(255,qMax(0,pxval.alpha()-requestedPixmapStrength)));
				movableImage->setPixel(p.x(), p.y(), pxval.rgba());
			}
		}
	}
	prev_computed-=imagereferredBrushArea.translated(mx,my);
	update();
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
	else if (newindex==4)
		blendmode=& PreviewWidget::computeAntiGhostingMask;

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

void PreviewWidget::switchAntighostingMode(bool ag) {
	if (ag) {
		leftButtonMode=LB_antighostingmode;
		this->setCursor(*agcursor_pixmap);
	} else {
		leftButtonMode=LB_nomode;
		this->unsetCursor();
	}
}

void PreviewWidget::setBrushSize (const int newsize) {
	requestedPixmapSize=newsize;
}

void PreviewWidget::setBrushMode(bool removemode) {
	requestedPixmapStrength *= -1;
	brushAddMode=!removemode;
}

void PreviewWidget::setBrushStrength (const int newstrength) {
	requestedPixmapStrength=newstrength;
	requestedPixmapColor.setAlpha(qMax(60,requestedPixmapStrength));
	requestedPixmapStrength *= (!brushAddMode) ? -1 : 1;
}

void PreviewWidget::setBrushColor (const QColor newcolor) {
	requestedPixmapColor=newcolor;
	prev_computed=QRegion();
	update();
}

void PreviewWidget::enterEvent(QEvent *) {
	if (leftButtonMode==LB_antighostingmode) {
		fillAntiGhostingCursorPixmap();
		this->unsetCursor();
		this->setCursor(*agcursor_pixmap);
	}
}

void PreviewWidget::fillAntiGhostingCursorPixmap() {
	if (requestedPixmapSize != previousPixmapSize || requestedPixmapStrength != previousPixmapStrength || requestedPixmapColor.rgb() != previousPixmapColor.rgb()) {
		if (agcursor_pixmap)
			delete agcursor_pixmap;
		previousPixmapSize=requestedPixmapSize;
		previousPixmapStrength=requestedPixmapStrength;
		previousPixmapColor=requestedPixmapColor;
		agcursor_pixmap=new QPixmap(requestedPixmapSize,requestedPixmapSize);
		agcursor_pixmap->fill(Qt::transparent);
		QPainter painter(agcursor_pixmap);
		painter.setPen(Qt::NoPen);
		painter.setBrush(QBrush(requestedPixmapColor,Qt::SolidPattern));
		painter.drawEllipse(0,0,requestedPixmapSize,requestedPixmapSize);
	}
}
