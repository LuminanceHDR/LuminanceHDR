/**
 * This file is a part of LuminanceHDR package.
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

#ifndef GAMMA_AND_LEVELS_H
#define GAMMA_AND_LEVELS_H
#include <QWidget>

class GrayBar : public QWidget
{
Q_OBJECT
public:
	GrayBar(QWidget *parent, bool two_handles=false);
	QSize sizeHint () const;
	QSize minimumSizeHint () const;
	bool dont_emit;
protected:
	void paintEvent( QPaintEvent * );
	void resizeEvent ( QResizeEvent * event );
	void mouseMoveEvent ( QMouseEvent * event );
	void mousePressEvent ( QMouseEvent * event );
	void mouseReleaseEvent ( QMouseEvent * event );
private:
	//graphical coordinate, 0-width(), NOT 0-255
	int blackpos,gammapos,whitepos;
	float blackgrayratio;
	bool twohandles;
	enum draggingT {DRAGNONE,DRAGBLACK,DRAGGRAY,DRAGWHITE} dragging;
	draggingT findHandle(int x, int y);
public slots:
	void resetvalues();
	void changeBlack(int);
	void changeGamma(double);
	void changeWhite(int);
signals:
	void black_changed(int);
	void gamma_changed(double);
	void white_changed(int);
	void default_gamma_black_white();
	void default_black_white();
};

class HistogramLDR : public QWidget
{
Q_OBJECT
private:
	//LDR means 256 bins
	float *P;
	int accuracy;
public:
	HistogramLDR(QWidget *parent, int accuracy=1);
	~HistogramLDR();
	QSize sizeHint () const;
	QSize minimumSizeHint () const;
        void setData(const QImage* data);
protected:
	void paintEvent( QPaintEvent * );
// 	void resizeEvent ( QResizeEvent * );
};


#include "ui_GammaAndLevels.h"
class GammaAndLevels : public QDialog, public Ui::LevelsDialog
{
Q_OBJECT
private:
	unsigned char *LUT;
	int blackin, whitein, blackout, whiteout;
	float gamma;
	void refreshLUT();
public:
        GammaAndLevels(QWidget *parent, const QImage* image);
	~GammaAndLevels();
	GrayBar *gb1,*gb2;
	HistogramLDR *histogram;
protected:
	void closeEvent ( QCloseEvent * event );
signals:
	void closing();
	void LUTrefreshed(unsigned char *);

private slots:
	void resetValues();
	void updateBlackIn(int);
	void updateGamma(double);
	void updateWhiteIn(int);
	void updateBlackOut(int);
	void updateWhiteOut(int);
	void defaultGammaBlackWhiteIn();
	void defaultBlackWhiteOut();
};


#endif
