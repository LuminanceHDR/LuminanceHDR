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

#include <QColor>
#include <QResizeEvent>
#include <QScrollBar>
#include <QScrollArea>

class PreviewWidget : public QWidget
{
Q_OBJECT
public:
	PreviewWidget(QWidget *parent, QImage *m, QImage *p);
	~PreviewWidget();
	void renderPreviewImage(QRgb(PreviewWidget::*f)(const QRgb*,const QRgb*)const,const QRect a = QRect());
	QSize sizeHint () const {
		return previewImage->size();
	}
	float getScaleFactor() {
		return scaleFactor;
	}
	QRect getCropArea() const {
		return rubberband;
	}
	QImage* getPreviewImage() {
		renderPreviewImage(blendmode);
		return previewImage;
	}
	void setPivot(QImage *p, int _px, int _py);
	void setMovable(QImage *m, int _mx, int _my);
	void updateVertShiftMovable(int v);
	void updateHorizShiftMovable(int h);
	void updateHorizShiftPivot(int h);
	void updateVertShiftPivot(int v);
	void hideRubberBand();

public slots:
	void requestedBlendMode(int);
signals:
	void validCropArea(bool);
protected:
	void paintEvent( QPaintEvent * );
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void resizeEvent(QResizeEvent *event);
private:
	//4 blending modes
	inline QRgb computeOnlyMovable(const QRgb *Mrgba, const QRgb */*Prgba*/) const {
		return *Mrgba;
	}
	inline QRgb computeOnlyPivot(const QRgb */*Mrgba*/, const QRgb *Prgba) const {
		return *Prgba;
	}
	inline QRgb computeAddRgba(const QRgb *Mrgba, const QRgb *Prgba) const {
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
		ro = ( Pred*Palpha + Mred*Malpha )/512;
		go = ( Pgreen*Palpha + Mgreen*Malpha )/512;
		bo = ( Pblue*Palpha + Mblue*Malpha )/512;
		//the output image still has alpha=255 (opaque)
		return qRgba(ro,go,bo,255);
	}
	inline QRgb computeDiffRgba(const QRgb *Mrgba, const QRgb *Prgba) const {
		int ro,go,bo;
		int Mred        = qRed(*Mrgba);
		int Mgreen      = qGreen(*Mrgba);
		int Mblue       = qBlue(*Mrgba);
		int Malphaweight= (int)(qAlpha(*Mrgba)/255.0f);
		int Pred        = qRed(*Prgba);
		int Pgreen      = qGreen(*Prgba);
		int Pblue       = qBlue(*Prgba);
		int Palphaweight= (int)(qAlpha(*Prgba)/255.0f);
	
		//blend samples using alphas as weights
		ro = qAbs( Pred*Palphaweight - Mred*Malphaweight );
		go = qAbs( Pgreen*Palphaweight - Mgreen*Malphaweight );
		bo = qAbs( Pblue*Palphaweight - Mblue*Malphaweight );
		//the output image still has alpha=255 (opaque)
		return qRgba(ro,go,bo,255);
	}
	QRgb(PreviewWidget::*blendmode)(const QRgb*,const QRgb*)const;
	QImage *previewImage;
	QImage *movableImage,*pivotImage;
	QScrollArea *scrollArea;
	QRegion prev_computed;
	int mx,my,px,py;
	float scaleFactor;
	QPoint mousePos;
	QRect rubberband;
	bool dragging_rubberband_left;
	bool dragging_rubberband_right;
	bool dragging_rubberband_top;
	bool dragging_rubberband_bottom;
	bool dragging_rubberband_topleft;
	bool dragging_rubberband_topright;
	bool dragging_rubberband_bottomright;
	bool dragging_rubberband_bottomleft;

};
