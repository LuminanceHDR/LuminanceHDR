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

#ifndef PREVIEWWIDGET_H
#define PREVIEWWIDGET_H

#include <QColor>
#include <QResizeEvent>
#include <QScrollBar>
#include <QScrollArea>
#include <QImage>

class PreviewWidget : public QWidget
{
Q_OBJECT
public:
	PreviewWidget(QWidget *parent, /*const*/ QImage *m, const QImage *p);
	~PreviewWidget();
	QSize sizeHint () const {
		return previewImage->size();
	}
	float getScaleFactor() {
		return scaleFactor;
	}
	QRect getCropArea() const {
		return rubberband;
	}
	QImage & getPreviewImage() {
		renderPreviewImage(blendmode);
		return *previewImage;
	}
	void setPivot(QImage *p, int _px, int _py);
	void setPivot(QImage *p);
	void setMovable(QImage *m, int _mx, int _my);
	void setMovable(QImage *m);
	void updateVertShiftMovable(int v);
	void updateHorizShiftMovable(int h);
	void updateHorizShiftPivot(int h);
	void updateVertShiftPivot(int v);
	void hideRubberBand();

public slots:
	void requestedBlendMode(int);
	void switchAntighostingMode(bool);
	void setBrushSize(const int);
	void setBrushStrength(const int);
	void setBrushColor(const QColor);
	void setBrushMode(bool);
signals:
	void validCropArea(bool);
	void moved(QPoint diff);
protected:
	void paintEvent( QPaintEvent * );
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void resizeEvent(QResizeEvent *event);
	void timerEvent(QTimerEvent *event);
	void enterEvent(QEvent *event);

private:
	//5 blending modes
	inline QRgb computeOnlyMovable(const QRgb *Mrgba, const QRgb */*Prgba*/) const {
		return *Mrgba;
	}
	inline QRgb computeOnlyPivot(const QRgb */*Mrgba*/, const QRgb *Prgba) const {
		return *Prgba;
	}
	inline QRgb computeAddRgba(const QRgb *Mrgba, const QRgb *Prgba) const {
		int ro,go,bo;
		int Mred   = qRed(*Mrgba);
		int Mgreen = qGreen(*Mrgba);
		int Mblue  = qBlue(*Mrgba);
		int Malpha = qAlpha(*Mrgba);
		int Pred   = qRed(*Prgba);
		int Pgreen = qGreen(*Prgba);
		int Pblue  = qBlue(*Prgba);
		int Palpha = qAlpha(*Prgba);
		//blend samples using alphas as weights
		ro = ( Pred*Palpha + Mred*Malpha )/510;
		go = ( Pgreen*Palpha + Mgreen*Malpha )/510;
		bo = ( Pblue*Palpha + Mblue*Malpha )/510;
		//the output image still has alpha=255 (opaque)
		return qRgba(ro,go,bo,255);
	}
	inline QRgb computeDiffRgba(const QRgb *Mrgba, const QRgb *Prgba) const {
		int ro,go,bo;
		int Mred        = qRed(*Mrgba);
		int Mgreen      = qGreen(*Mrgba);
		int Mblue       = qBlue(*Mrgba);
		int Malpha      = qAlpha(*Mrgba);
		int Pred        = qRed(*Prgba);
		int Pgreen      = qGreen(*Prgba);
		int Pblue       = qBlue(*Prgba);
		int Palpha      = qAlpha(*Prgba);
		//blend samples using alphas as weights
		ro = qAbs( Pred*Palpha - Mred*Malpha )/255;
		go = qAbs( Pgreen*Palpha - Mgreen*Malpha )/255;
		bo = qAbs( Pblue*Palpha - Mblue*Malpha )/255;
		//the output image still has alpha=255 (opaque)
		return qRgba(ro,go,bo,255);
	}
	inline QRgb computeAntiGhostingMask(const QRgb *Mrgba, const QRgb */*Prgba*/) const {
		int ro,go,bo;
		int Mred   = qRed(*Mrgba);
		int Mgreen = qGreen(*Mrgba);
		int Mblue  = qBlue(*Mrgba);
		int Malpha = qAlpha(*Mrgba);
		//blend samples using alphas as weights
		ro = ( requestedPixmapColor.red()*(255-Malpha) + Mred*Malpha )/255;
		go = ( requestedPixmapColor.green()*(255-Malpha) + Mgreen*Malpha )/255;
		bo = ( requestedPixmapColor.blue()*(255-Malpha) + Mblue*Malpha )/255;
		//the output image still has alpha=255 (opaque)
		return qRgba(ro,go,bo,255);
	}
	QRgb(PreviewWidget::*blendmode)(const QRgb*,const QRgb*)const;
	void renderPreviewImage(QRgb(PreviewWidget::*f)(const QRgb*,const QRgb*)const,const QRect a = QRect());

	// the out and 2 in images
	QImage *previewImage;
	/*const*/ QImage *movableImage;
	const QImage *pivotImage;

	//QScrollArea *scrollArea;
	QRegion prev_computed;

	//movable and pivot's x,y shifts
	int mx,my,px,py;
	//zoom factor
	float scaleFactor;

	//for panning with mid-button
	QPoint mousePos;
	//for cropping
	QRect rubberband;
	//assigned when starting to create a rubberband
	QPoint rubberbandInitialCreationPoint;
	//used for additional painting
	int timerid;
	QPixmap *agcursor_pixmap;
	int requestedPixmapSize,previousPixmapSize;
	int requestedPixmapStrength,previousPixmapStrength;
	QColor requestedPixmapColor,previousPixmapColor;
	bool brushAddMode;//false means brush is in remove mode.
	void fillAntiGhostingCursorPixmap();

	enum dragging_mode {DRAGGING_LEFT, DRAGGING_RIGHT, DRAGGING_TOP, DRAGGING_BOTTOM, DRAGGING_TOPLEFT, DRAGGING_TOPRIGHT, DRAGGING_BOTTOMRIGHT, DRAGGING_BOTTOMLEFT, DRAGGING_NONE } dragging_mode;
	enum {LB_croppingmode,LB_antighostingmode} leftButtonMode;
};

#endif
