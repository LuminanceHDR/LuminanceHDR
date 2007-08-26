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
 *
 */

#ifndef ALIGNMENTDIALOG_IMPL_H
#define ALIGNMENTDIALOG_IMPL_H

#include <QColor>
#include "../generated_uic/ui_aligndialog.h"
#include "../../smart_scroll_area.h"
class HistogramLDR;
//defined in mtb_alignment.cpp
QImage* shiftQImage(const QImage *in, int dx, int dy);

class LabelWithRubberBand : public QLabel
{
Q_OBJECT
public:
	LabelWithRubberBand(QWidget *parent=0);
	~LabelWithRubberBand();
	QRect getCropArea() const;
	void hideRubberBand() {
		rubberBand->hide();
		emit validCropArea(false);
	}
signals:
	void validCropArea(bool);
protected:
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void resizeEvent(QResizeEvent *event);
private:
	QRubberBand *rubberBand;
	QPoint origin;
};

class AlignmentDialog : public QDialog, private Ui::AlignmentDialog
{
Q_OBJECT
public:
	AlignmentDialog(QWidget *parent, QList<QImage*>&ldrlist, QList<bool> &ldr_tiff_input, QStringList fileStringList, Qt::ToolButtonStyle s);
	~AlignmentDialog();
private:
	LabelWithRubberBand *imageLabel;
	SmartScrollArea *scrollArea;
	QImage *diffImage;
	QImage *movableImage,*pivotImage;
	QList<QImage*> &original_ldrlist;
	QList<bool> &ldr_tiff_input;
	QList< QPair<int,int> > HV_offsets;
	HistogramLDR *histogram;
	bool dont_change_shift,dont_recompute;
	Qt::ToolButtonStyle style;
	
	inline void recomputeDiffImage();
	inline QRgb computeDiffRgba(const QRgb *Mrgba, const QRgb *Prgba)const {
		int ro,go,bo;
		int Mred=  qRed(*Mrgba);
		int Mgreen=qGreen(*Mrgba);
		int Mblue= qBlue(*Mrgba);
		int Malphaweight=(int)(qAlpha(*Mrgba)/255.0f);
		int Pred  =qRed(*Prgba);
		int Pgreen=qGreen(*Prgba);
		int Pblue =qBlue(*Prgba);
		int Palphaweight=(int)(qAlpha(*Prgba)/255.0f);
	
		//when both images have alpha==0 we return an opaque black
		if (Malphaweight==0 && Palphaweight==0)
			return qRgba(0,0,0,255);
		else {
			//blend samples using alphas as weights
			ro=qAbs(Pred*Palphaweight - Mred*Malphaweight);
			go=qAbs(Pgreen*Palphaweight - Mgreen*Malphaweight);
			bo=qAbs(Pblue*Palphaweight - Mblue*Malphaweight);
			//the output image still has alpha=255 (opaque)
			return qRgba(ro,go,bo,255);
		}
	}
private slots:
	void updatePivot(int);
	void updateMovable(int);
	void upClicked();
	void rightClicked();
	void downClicked();
	void leftClicked();
	void horizShiftChanged(int);
	void vertShiftChanged(int);
	void resetCurrent();
	void resetAll();
	void prevLeft();
	void nextLeft();
	void prevBoth();
	void nextBoth();
	void prevRight();
	void nextRight();
	void enterWhatsThis();
	void zoomIn();
	void zoomOut();
	void fitDiff(bool);
	void origSize();
	void crop_stack();
	
	void nextClicked();
};



#endif
