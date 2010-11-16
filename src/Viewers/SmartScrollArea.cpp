/**
 * This file is a part of LuminanceHDR package.
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  US
 * ---------------------------------------------------------------------- 
 *
 * Original Work
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * Improvements, bugfixing
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#include <QPixmap>
#include <QRubberBand>

#include "SmartScrollArea.h"

SmartScrollArea::SmartScrollArea( QWidget *parent, QLabel &imagelabel ) : QScrollArea(parent), imageLabel(imagelabel), scaleFactor(1.0), previousScaleFactor(1.0), fittingwin(false), panning(false) {


	setBackgroundRole(QPalette::Light);
	imageLabel.setBackgroundRole(QPalette::Base);
	//the label ignores the pixmap's size
	imageLabel.setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	//the label will scale the image to fill all available space (to its -the label's- size)
	imageLabel.setScaledContents(true);
	//false (the default), the scroll area honors the size of its widget.
	//Regardless of this property, you can programmatically resize the widget using widget()->resize()
	//indeed, when I zoom in/out I call imageLabel.resize(...)
	setWidgetResizable(false);	
	imageLabel.adjustSize();

	selectionTool = new SelectionTool(&imageLabel); //plug a selection tool to imageLabel
	setWidget(&imageLabel);
	selectionTool->hide();

	connect(selectionTool, SIGNAL(selectionReady(bool)), this, SIGNAL(selectionReady(bool)));
	connect(selectionTool, SIGNAL(moved(QPoint)), this, SLOT(updateScrollBars(QPoint)));
	connect(selectionTool, SIGNAL(scroll(int, int, int, int)), this, SLOT(ensureVisible(int, int, int, int)));
	horizontalScrollBar()->setTracking(true);
	verticalScrollBar()->setTracking(true);
	connect(horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)));
	connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)));

}

void SmartScrollArea::valueChanged(int /*value*/) {
	emit changed();
}

void SmartScrollArea::zoomIn() {
	scaleImage(1.25);
}

void SmartScrollArea::zoomOut() {
	scaleImage(0.8);
}

void SmartScrollArea::zoomToFactor(float factor) {
	scaleImage(factor/scaleFactor);
}

void SmartScrollArea::fitToWindow(bool checked) {
	fittingwin=checked;
	if (checked) {
		previousScaleFactor = scaleFactor; // save zoom factot
		scaleLabelToFit();
		setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		emit changed();
	}
	else {
		// restore to the previous zoom factor
		scaleFactor = previousScaleFactor;
		setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		scaleImage(1);
		emit changed();
	}
}

bool SmartScrollArea::isFittedToWindow() {
	return fittingwin;
}

void SmartScrollArea::normalSize() {
	//use the image size for the label
	imageLabel.adjustSize();
	scaleFactor = 1.0;
}

void SmartScrollArea::scaleLabelToFit() {
	int sa_width=size().width();
	int sa_height=size().height();
	float imageratio=float(imageLabel.pixmap()->size().width())/float(imageLabel.pixmap()->size().height());
	float factor = 1;
	if (sa_width<imageratio*sa_height) {
		factor=float(sa_width)/float(imageLabel.pixmap()->size().width());
	} else {
		factor=float(sa_height)/float(imageLabel.pixmap()->size().height());
	}
	scaleFactor = factor;
	imageLabel.resize(factor * imageLabel.pixmap()->size());
}

void SmartScrollArea::scaleImage(double factor) {
	scaleFactor *= factor;
	imageLabel.resize(scaleFactor * imageLabel.pixmap()->size());
	adjustScrollBar(horizontalScrollBar(), factor);
	adjustScrollBar(verticalScrollBar(), factor);
}

void SmartScrollArea::scaleImage() {
	imageLabel.resize(scaleFactor * imageLabel.pixmap()->size());
}

void SmartScrollArea::adjustScrollBar(QScrollBar *scrollBar, double factor) {
	scrollBar->setValue(int(factor * scrollBar->value() + ((factor - 1) * scrollBar->pageStep()/2)));
}

void SmartScrollArea::resizeEvent ( QResizeEvent * /*e*/) {
// 	qDebug("smartscrollarea::resizeEvent this->size()=(%d,%d)",this->size().width(),this->size().height());
// 	qDebug("smartscrollarea::resizeEvent newsize()=(%d,%d)",e->size().width(),e->size().height());
// 	qDebug("smartscrollarea::resizeEvent oldsize()=(%d,%d)",e->oldSize().width(),e->oldSize().height());
	if (fittingwin) {
		scaleLabelToFit();
	} else {
		//this seems to be a bug in qt4
		imageLabel.resize(imageLabel.size()-QSize(0,1));
		imageLabel.resize(imageLabel.size()+QSize(0,1));

// 		QScrollBar *h=horizontalScrollBar();
// 		QScrollBar *v=verticalScrollBar();
// 		int fixwidth=imageLabel.size().width()+v->size().width()-this->size().width();
// 		if (fixwidth>0)
// 			h->setMaximum(qMax(0,fixwidth));
// 		else
// 			h->setMaximum(0);
// 		int fixheight=imageLabel.size().height()-this->size().height();
// 		if (fixheight>0)
// 			v->setMaximum(qMax(0,fixheight));
// 		else
// 			v->setMaximum(0);
	}
}

void SmartScrollArea::updateScrollBars(QPoint diff) {
	verticalScrollBar()->setValue(verticalScrollBar()->value() + diff.y());
	horizontalScrollBar()->setValue(horizontalScrollBar()->value() + diff.x());
}

void SmartScrollArea::ensureVisible(int x, int y, int w, int h) {
	QScrollArea::ensureVisible(x, y, w, h);
}

QRect SmartScrollArea::getSelectionRect() {
	QRect sr = selectionTool->getSelectionRect();
	return QRect(sr.topLeft()/scaleFactor, sr.size()/scaleFactor);
}

void SmartScrollArea::setSelectionTool(bool toggled) {
	if (toggled) {
    		setMouseTracking(false); 
		selectionTool->show();
	}
	else {
		setMouseTracking(true);
		selectionTool->hide();
	}
}

bool SmartScrollArea::hasSelection() {
	return selectionTool->hasSelection();
}

void SmartScrollArea::removeSelection() {
	selectionTool->removeSelection();
}

void SmartScrollArea::mousePressEvent(QMouseEvent *e) {
	m_mousePos = e->globalPos();
}

void SmartScrollArea::mouseMoveEvent(QMouseEvent *e) {
	if (e->buttons()==Qt::MidButton) {
		panning = true;
		setCursor(QCursor(Qt::SizeAllCursor));
		QPoint diff = (e->globalPos() - m_mousePos);
		if (e->modifiers()==Qt::ShiftModifier)
			diff *= 5;
		verticalScrollBar()->setValue(verticalScrollBar()->value() + diff.y());
		horizontalScrollBar()->setValue(horizontalScrollBar()->value() + diff.x());
		m_mousePos = e->globalPos();
		emit changed();
	}
}

void SmartScrollArea::mouseReleaseEvent(QMouseEvent *) {
	if (panning) {
		panning = false;
		emit changed();
	}
	setCursor(QCursor(Qt::ArrowCursor));
}

int  SmartScrollArea::getHorizScrollBarValue() {
	return horizontalScrollBar()->value();
}

int  SmartScrollArea::getVertScrollBarValue() {
	return verticalScrollBar()->value();
}

float SmartScrollArea::getScaleFactor() {
	return scaleFactor;
}

float SmartScrollArea::getImageScaleFactor() {
	float scaleFactor = float(imageLabel.size().width()) / (float) (imageLabel.pixmap()->size().width());
	return scaleFactor;
}

void SmartScrollArea::setHorizScrollBarValue(int value) {
	horizontalScrollBar()->setValue(value);
}

void SmartScrollArea::setVertScrollBarValue(int value) {
	verticalScrollBar()->setValue(value);
}

