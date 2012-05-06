/**
 * This file is a part of Luminance HDR package.
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
 */

#include <QDebug>

#include <QApplication>
#include <QPainter>

#include "AntiGhostingWidget.h"

AntiGhostingWidget::AntiGhostingWidget(QWidget *parent, QImage *mask): 
	QWidget(parent),
	agMask(mask),
	agcursor_pixmap(NULL),
	mx(0),
	my(0) 
{
	qDebug() << "AntiGhostingWidget::AntiGhostingWidget";
	//set internal brush values to their default
	brushAddMode = true;
	setBrushSize(32);
	previousPixmapSize = -1;
	setBrushStrength(255);
	previousPixmapStrength = -1;
	previousPixmapColor = QColor();
	fillAntiGhostingCursorPixmap();
	setMouseTracking(true);
} 

AntiGhostingWidget::~AntiGhostingWidget()
{
	qDebug() << "~AntiGhostingWidget::AntiGhostingWidget";
	if (agcursor_pixmap)
		delete agcursor_pixmap;
}

void AntiGhostingWidget::paintEvent(QPaintEvent *event)
{
	QRect paintrect = event->rect();
	QRect srcrect = QRect(paintrect.topLeft().x()/scaleFactor - mx, paintrect.topLeft().y()/scaleFactor - my, paintrect.width()/scaleFactor, paintrect.height()/scaleFactor);
	QPainter p(this);
	p.drawImage(paintrect, *agMask, srcrect);
}

void AntiGhostingWidget::mousePressEvent(QMouseEvent *event)
{
	if (event->buttons() == Qt::MidButton) {
		QApplication::setOverrideCursor( QCursor(Qt::ClosedHandCursor) );
		mousePos = event->globalPos();
	}

	if (event->buttons() == Qt::LeftButton) {
		timerid = this->startTimer(0);
	}
	event->ignore();
}

void AntiGhostingWidget::mouseMoveEvent(QMouseEvent *event)
{
	if (event->buttons() == Qt::MidButton) {
		//moving mouse with middle button pans the preview
		QPoint diff = (event->globalPos() - mousePos);
		if (event->modifiers() == Qt::ShiftModifier)
			diff *= 5;
		emit moved(diff);
		mousePos = event->globalPos();
	}
	event->ignore();
}

void AntiGhostingWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		this->killTimer(timerid);
	}
	else if (event->button() == Qt::MidButton) {
		QApplication::restoreOverrideCursor();		
		fillAntiGhostingCursorPixmap();
		this->unsetCursor();
		this->setCursor(*agcursor_pixmap);
	}
	event->ignore();
}

void AntiGhostingWidget::resizeEvent(QResizeEvent *event)
{
	scaleFactor = (float)(event->size().width())/(float)(agMask->size().width());
}

void AntiGhostingWidget::timerEvent(QTimerEvent *) 
{
	QPoint relativeToWidget = mapFromGlobal(QCursor::pos());
	int sx = relativeToWidget.x()/scaleFactor - mx;
 	int sy = relativeToWidget.y()/scaleFactor - my;
	QPoint scaled(sx,sy);
	QPainter p(agMask);
	p.setPen(Qt::NoPen);
	if (brushAddMode)
		p.setBrush(QBrush(requestedPixmapColor, Qt::SolidPattern));
	else
		p.setBrush(QBrush(qRgba(0,0,0,0), Qt::SolidPattern));
	int pixSize = requestedPixmapSize/(2*scaleFactor);
	p.drawEllipse(scaled, pixSize, pixSize);
	update();
}

void AntiGhostingWidget::setBrushSize (const int newsize) {
	requestedPixmapSize = newsize;
}

void AntiGhostingWidget::setBrushMode(bool removemode) {
	requestedPixmapStrength *= -1;
	brushAddMode = !removemode;
}

void AntiGhostingWidget::setBrushStrength (const int newstrength) {
	requestedPixmapStrength = newstrength;
	requestedPixmapColor.setAlpha(qMax(60,requestedPixmapStrength));
	requestedPixmapStrength *= (!brushAddMode) ? -1 : 1;
}

void AntiGhostingWidget::setBrushColor (const QColor newcolor) {
	requestedPixmapColor = newcolor;
	update();
}

void AntiGhostingWidget::enterEvent(QEvent *) {
	fillAntiGhostingCursorPixmap();
	this->unsetCursor();
	this->setCursor(*agcursor_pixmap);
}

void AntiGhostingWidget::switchAntighostingMode(bool ag) {
	if (ag) {
		this->setCursor(*agcursor_pixmap);
	} else {
		this->unsetCursor();
	}
}

void AntiGhostingWidget::fillAntiGhostingCursorPixmap() {
//	if (requestedPixmapSize != previousPixmapSize || requestedPixmapStrength != previousPixmapStrength || requestedPixmapColor.rgb() != previousPixmapColor.rgb()) {
		if (agcursor_pixmap)
			delete agcursor_pixmap;
		previousPixmapSize = requestedPixmapSize;
		previousPixmapStrength = requestedPixmapStrength;
		previousPixmapColor = requestedPixmapColor;
		agcursor_pixmap = new QPixmap(requestedPixmapSize,requestedPixmapSize);
		agcursor_pixmap->fill(Qt::transparent);
		QPainter painter(agcursor_pixmap);
		painter.setPen(Qt::NoPen);
		painter.setBrush(QBrush(requestedPixmapColor,Qt::SolidPattern));
		painter.drawEllipse(0,0,requestedPixmapSize,requestedPixmapSize);
//	}
}

void AntiGhostingWidget::updateVertShift(int v) {
	my=v;
}

void AntiGhostingWidget::updateHorizShift(int h) {
	mx=h;
}

