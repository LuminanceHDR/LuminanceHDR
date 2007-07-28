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

#include "gamma_and_levels.h"
#include <QCloseEvent>
#include <QLinearGradient>
#include <QPainter>
#include <cmath>
#include <cassert>


GammaAndLevels::GammaAndLevels(QWidget *parent, const QImage data) : QDialog(parent) {
	setupUi(this);
	connect(cancelButton,SIGNAL(clicked()),this,SIGNAL(closing()));
	connect(okButton,SIGNAL(clicked()),this,SIGNAL(closing()));
	LUT=new unsigned char[256];
	blackin=0;
	gamma=1.0f;
	whitein=255;
	blackout=0;
	whiteout=255;

	QVBoxLayout *qvl=new QVBoxLayout;
	qvl->setMargin(0);
	qvl->setSpacing(1);

	histogram=new HistogramLDR(this,data);
	gb1=new GrayBar(inputStuffFrame);

	connect(black_in_spinbox,SIGNAL(valueChanged(int)),gb1,SLOT(changeBlack(int)));
	connect(gamma_spinbox,SIGNAL(valueChanged(double)),gb1,SLOT(changeGamma(double)));
	connect(white_in_spinbox,SIGNAL(valueChanged(int)),gb1,SLOT(changeWhite(int)));

	connect(gb1,SIGNAL(black_changed(int)),this,SLOT(updateBlackIn(int)));
	connect(gb1,SIGNAL(gamma_changed(double)),this,SLOT(updateGamma(double)));
	connect(gb1,SIGNAL(white_changed(int)),this,SLOT(updateWhiteIn(int)));
	connect(gb1,SIGNAL(default_gamma_black_white()),this,SLOT(defaultGammaBlackWhiteIn()));

	qvl->addWidget(histogram);
	qvl->addWidget(gb1);
	inputStuffFrame->setLayout(qvl);

	QVBoxLayout *qvl2=new QVBoxLayout;
	qvl2->setMargin(0);
	qvl2->setSpacing(1);
	gb2=new GrayBar(out_levels,true);
	connect(black_out_spinbox,SIGNAL(valueChanged(int)),gb2,SLOT(changeBlack(int)));
	connect(white_out_spinbox,SIGNAL(valueChanged(int)),gb2,SLOT(changeWhite(int)));

	connect(gb2,SIGNAL(black_changed(int)),this,SLOT(updateBlackOut(int)));
	connect(gb2,SIGNAL(white_changed(int)),this,SLOT(updateWhiteOut(int)));
	connect(gb2,SIGNAL(default_black_white()),this,SLOT(defaultBlackWhiteOut()));

	connect(ResetButton,SIGNAL(clicked()),gb1,SLOT(resetvalues()));
	connect(ResetButton,SIGNAL(clicked()),gb2,SLOT(resetvalues()));
	connect(ResetButton,SIGNAL(clicked()),this,SLOT(resetValues()));

	qvl2->addWidget(gb2);
	out_levels->setLayout(qvl2);
}

GammaAndLevels::~GammaAndLevels() {
	delete gb1; delete gb2; delete histogram;
	emit closing();
// 	qDebug("~GammaAndLevels");
}

void GammaAndLevels::defaultGammaBlackWhiteIn() {
	qDebug("this::defaultGammaBlackWhiteIn");
	blackin=0;
	gamma=1.0f;
	whitein=255;
}

void GammaAndLevels::defaultBlackWhiteOut() {
	qDebug("this::defaultBlackWhiteOut");
	blackout=0;
	whiteout=255;
}

void GammaAndLevels::closeEvent(QCloseEvent *) {
// 	emit closing();
}

void GammaAndLevels::updateBlackIn(int v) {
	qDebug("this::updateBlackIn");
	black_in_spinbox->setValue(v);
	blackin=v;
	refreshLUT();
}
void GammaAndLevels::updateGamma(double v) {
	qDebug("this::updateGamma");
	gb1->dont_emit=true;
	gamma_spinbox->setValue(v);
	gamma=v;
	refreshLUT();
	gb1->dont_emit=false;
}
void GammaAndLevels::updateWhiteIn(int v) {
	qDebug("this::updateWhiteIn");
	white_in_spinbox->setValue(v);
	whitein=v;
	refreshLUT();
}
void GammaAndLevels::updateBlackOut(int v) {
	qDebug("this::updateBlackOut");
	black_out_spinbox->setValue(v);
	blackout=v;
	refreshLUT();
}
void GammaAndLevels::updateWhiteOut(int v) {
	qDebug("this::updateWhiteOut");
	white_out_spinbox->setValue(v);
	whiteout=v;
	refreshLUT();
}

void GammaAndLevels::resetValues() {
	qDebug("this::resetValues");
	blackin=0;
	gamma=1.0f;
	whitein=255;
	blackout=0;
	whiteout=255;
	refreshLUT();
}

static inline unsigned char clamp( const float v, const unsigned char minV, const unsigned char maxV )
{
    if( v < minV ) return minV;
    if( v > maxV ) return maxV;
    return (unsigned char)v;
}

void GammaAndLevels::refreshLUT() {
	qDebug("this::refreshLUT");
	//values in 0..1 range
	float bin=(float)blackin/255.0f;
	float win=(float)whitein/255.0f;
	float expgamma=1.0f/gamma;
	for (int i=0; i<256; i++) {
		float value=powf( ( ((float)(i)/255.0f) - bin ) / (win-bin), expgamma);
		LUT[i]=clamp(blackout+value*(whiteout-blackout),0,255);
// 		qDebug("LUT[%d]=%f char=%d",i,(blackout+value*(whiteout-blackout)),LUT[i]);
	}
	emit LUTrefreshed(LUT);
}
//////////////////////////////////////////////////////////////////////////////////////

HistogramLDR::HistogramLDR(QWidget *parent, const QImage data, int accuracy) : QWidget(parent), accuracy(accuracy){
	P = new float[256];

	//initialize to 0
	for( int i = 0; i < 256; i++ )
		P[i]=0;

	//increment bins
	for (int i=0; i<data.width()*data.height(); i+=accuracy) {
		int v=qGray(*((QRgb*)(data.bits())+i));
		assert(v>=0 && v<=255);
		P[v] += 1;
	}

	//find max
	float max=-1;
	for( int i = 0; i < 256; i++ )
		if (P[i]>max)
			max=P[i];

	//normalize to make maxvalue=1
	for( int i = 0; i < 256; i++ )
		P[i] /= max;
	
}
void HistogramLDR::paintEvent( QPaintEvent * ) {
	QPainter painter(this);
	for (int i=0; i<256; i++) {
		painter.fillRect( i*(int)((float)(this->width())/256.f), this->height()-(int)(P[i]*(height()-2)), (int)( (float)(this->width())/256.f ), (int)(P[i]*(height()-2)), QBrush(Qt::black) );
	}
}
// int HistogramLDR::getClipFromBlack() {
// 	for (int i=0;i<=255;i++) {
// 		if (P[i]!=0)
// 			return i;
// 	}
// 	return 255;
// }
// int HistogramLDR::getClipFromWhite() {
// 	for (int i=255;i>=0;i--) {
// 		if (P[i]!=0)
// 			return i;
// 	}
// 	return 0;
// }
QSize HistogramLDR::sizeHint () const {
	return QSize( 500, 30 );
}
QSize HistogramLDR::minimumSizeHint () const {
	return QSize( 400, 20 );
}
HistogramLDR::~HistogramLDR() {
	delete [] P;
}

//////////////////////////////////////////////////////////////////////////////////////
GrayBar::GrayBar(QWidget *parent, bool two_handles) : QWidget(parent), dont_emit(false) {
	twohandles=two_handles;
	dragging=DRAGNONE;
// 	qDebug("width=%d, height=%d",width(),height());
}

QSize GrayBar::sizeHint () const {
	return QSize( 500, 12 );
}
QSize GrayBar::minimumSizeHint () const {
	return QSize( 400, 12 );
}

void GrayBar::mouseMoveEvent( QMouseEvent * e ) {
	if (dragging==DRAGNONE)
		return;

	//here we have to make sure that we keep the "order": black,(gray),white
	if (dragging==DRAGBLACK) {
		if (e->x()<=whitepos && e->x()>=0) {
			//update graphical position of gray handle
			gammapos=e->x()+(int)(blackgrayratio*(whitepos-e->x()));
			//update graphical position of black handle
			blackpos=e->x();
			update();
		}
		return;
	}
	if (dragging==DRAGWHITE) {
		if (e->x()>=blackpos && e->x()<=width()) {
			//update graphical position of gray handle
			gammapos=e->x()-(int)((1.0f-blackgrayratio)*(e->x()-blackpos));
			//update graphical position of white handle
			whitepos=e->x();
			update();
		}
		return;
	}
	if (dragging==DRAGGRAY) {
		if (e->x()>=blackpos && e->x()<=whitepos) {
			//update graphical position of gray handle
			blackgrayratio=(float)(e->x()-blackpos)/(float)(whitepos-blackpos);
			//update graphical position of white handle
			gammapos=e->x();
			update();
		}
		return;
	}
}

void GrayBar::mousePressEvent( QMouseEvent * e ) {
	dragging=findHandle(e->x(),e->y());
}

void GrayBar::mouseReleaseEvent( QMouseEvent * e) {
	if (dragging==DRAGBLACK) {
		emit black_changed( (int)(255*((float)(blackpos)/(float)(width()))) );
	}
	if (dragging==DRAGWHITE) {
		emit white_changed( (int)(255*((float)(whitepos)/(float)(width()))) );
	}
	if (dragging==DRAGGRAY) {
		float mediumpos = (float)blackpos+ ((float)whitepos-(float)blackpos)/2.0f;
		if (e->x()>mediumpos) {
//exp10f is not defined on MinGW in windows.
#ifdef _GNU_SOURCE
			emit gamma_changed( exp10f( (mediumpos-(float)e->x())/((float)(whitepos)-mediumpos) ) );
		} else {
			emit gamma_changed( exp10f( (mediumpos-(float)e->x())/(mediumpos-(float)(blackpos)) ) );
#else
			emit gamma_changed( powf(10.0f, (mediumpos-(float)e->x())/((float)(whitepos)-mediumpos) ) );
		} else {
			emit gamma_changed( powf(10.0f, (mediumpos-(float)e->x())/(mediumpos-(float)(blackpos)) ) );
#endif
		}
	}
	dragging=DRAGNONE;
	update();
}

GrayBar::draggingT GrayBar::findHandle(int x, int y) {

	QRect black_rect(blackpos-5,1+(height()-1)/2, 10, (height()-1)-(1+(height()-1)/2));
	QRect white_rect(whitepos-5,1+(height()-1)/2, 10, (height()-1)-(1+(height()-1)/2));

	//mouse click belongs to both white and black rects, and there's some space on the left of the black coordinate
	if ( black_rect.contains(x,y,false) && white_rect.contains(x,y,false) && blackpos!=0 ) {
		return DRAGBLACK;
	}
	//mouse click belongs to both white and black rects, and there's some space on the right of the white coordinate
	if ( black_rect.contains(x,y,false) && white_rect.contains(x,y,false) && whitepos!=width() ) {
		return DRAGWHITE;
	}

	//check if we clicked on black handle
	if ( black_rect.contains(x,y,false) ) {
		return DRAGBLACK;
	}

	//check if we clicked on white handle
	if ( white_rect.contains(x,y,false) ) {
		return DRAGWHITE;
	}

	//check if we clicked on gray handle
	if (!twohandles) {
		QRect gray_rect(gammapos-5,1+(height() - 1)/2, 10, (height() - 1)-(1+(height() - 1)/2));
		if ( gray_rect.contains(x,y,false) ) {
			return DRAGGRAY;
		}
	}
	return DRAGNONE;
}

void GrayBar::resizeEvent ( QResizeEvent * ) {
	qDebug("GrayBar::resizeEvent");
	resetvalues();
//this one below does not work, we resetvalues for the time being.
// 	float factor=(float)(e->size().width())/(float)(e->oldSize().width());
// 	qDebug("factor=%f",factor);
// 	blackpos=(int)( (float)blackpos*( factor ) );
// 	whitepos=(int)( (float)whitepos*( factor ) );
// 	gammapos=(int)( (float)gammapos*( factor ) );
// 	update();
}

void GrayBar::paintEvent( QPaintEvent * ) {
	QPainter painter(this);
	QLinearGradient linearGradient(0, height()/2, width(), height()/2);
	linearGradient.setColorAt(0.0, Qt::black);
	linearGradient.setColorAt(0.5, Qt::darkGray);
	linearGradient.setColorAt(1.0, Qt::white);
	painter.setBrush(linearGradient);
	painter.setPen(Qt::NoPen);
	painter.drawRect(QRect(0, 0, width() - 1, (height() - 1)/2));

	painter.setPen(Qt::black);
	//draw black triangle
	static QPoint black_tri[3] = {
		QPoint(blackpos, 1+(height() - 1)/2),
		QPoint(blackpos-5, (height() - 1)),
		QPoint(blackpos+5, (height() - 1)),
	};
	black_tri[0].setX(blackpos);
	black_tri[1].setX(blackpos-5);
	black_tri[2].setX(blackpos+5);
	black_tri[0].setY(1+(height() - 1)/2);
	black_tri[1].setY(height()-1);
	black_tri[2].setY(height()-1);
	painter.setBrush(QBrush(Qt::black));
	painter.drawPolygon(black_tri,3);
	//draw white triangle
	static QPoint white_tri[3] = {
		QPoint(whitepos, 1+(height() - 1)/2),
		QPoint(whitepos-5, (height() - 1)),
		QPoint(whitepos+5, (height() - 1)),
	};
	white_tri[0].setX(whitepos);
	white_tri[1].setX(whitepos-5);
	white_tri[2].setX(whitepos+5);
	white_tri[0].setY(1+(height()-1)/2);
	white_tri[1].setY(height()-1);
	white_tri[2].setY(height()-1);
	painter.setBrush(QBrush(Qt::white));
	painter.drawPolygon(white_tri,3);
	//in case, draw gray triangle
	if (!twohandles) {
		static QPoint gray_tri[3] = {
			QPoint(gammapos, 1+(height() - 1)/2),
			QPoint(gammapos-5, (height() - 1)),
			QPoint(gammapos+5, (height() - 1)),
		};
		gray_tri[0].setX(gammapos);
		gray_tri[1].setX(gammapos-5);
		gray_tri[2].setX(gammapos+5);
		gray_tri[0].setY(1+(height()-1)/2);
		gray_tri[1].setY(height()-1);
		gray_tri[2].setY(height()-1);
		painter.setBrush(QBrush(Qt::darkGray));
		painter.drawPolygon(gray_tri,3);
	}
// 	qDebug("paint width=%d, height=%d",width(),height());
// 	qDebug("blackpos=%d, gammapos=%d, whitepos=%d",blackpos,gammapos,whitepos);
}

void GrayBar::resetvalues() {
	qDebug("GrayBar::resetvalues");
	blackpos=0;
	gammapos=width()/2;
	blackgrayratio=0.5f;
	whitepos=width();

	if (twohandles)
		emit default_black_white();
	else
		emit default_gamma_black_white();
	update();
}

void GrayBar::changeBlack(int v) {
	if ((int)(255*((float)(blackpos)/(float)(width()))) == v)
		return;
	qDebug("GrayBar::changeBlack");
	blackpos=(int) (v*width()/255.0f) < whitepos ? (int) (v*width()/255.0f) : blackpos;
	gammapos=blackpos+(int)(blackgrayratio*(whitepos-blackpos));
	update();
	emit black_changed(v);
}

void GrayBar::changeGamma(double v) {

	float mediumpos = (float)blackpos+ ((float)whitepos-(float)blackpos)/2.0f;
	if (v<1.0f) {
		gammapos=(int)( mediumpos-((float)(whitepos)-mediumpos)*log10f(v) );
	} else {
		gammapos=(int)( mediumpos-(mediumpos-(float)(blackpos))*log10f(v) );
	}
	qDebug("GrayBar::changeGamma %f",v);
	blackgrayratio=(float)(gammapos-blackpos)/(float)(whitepos-blackpos);
	update();
	if (dont_emit) {
		dont_emit=false;
		return;
	}
// 	qDebug("setting dont_emit false");
// 	dont_emit=false;
	emit gamma_changed (v);
}

void GrayBar::changeWhite(int v) {
	if ((int)(255*((float)(whitepos)/(float)(width()))) == v)
		return;
	qDebug("GrayBar::changeWhite, %d", v);
	whitepos=(int) (v*width()/255.0f) > blackpos ? (int) (v*width()/255.0f) : whitepos;
	gammapos=whitepos-(int)((1.0f-blackgrayratio)*(whitepos-blackpos));
	update();
	emit white_changed(v);
}

