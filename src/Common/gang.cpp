/**
 * This file is a part of Qtpfsgui package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2006,2007 Giuseppe Rota
 * Copyright (C) 2002-2005 Nicholas Phillips
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
 * based on previous GPL code from qpfstmo, by Nicholas Phillips.
 */

#include <iostream>
#include <math.h>
#include "gang.h"

using namespace std;


Gang::Gang(QSlider* slider_, QDoubleSpinBox* doublespinbox_, 
		const float minv_, const float maxv_, 
		const float vv, const bool logs) : 
			s(slider_), dsb(doublespinbox_), minv(minv_), maxv(maxv_), defaultv(vv), logscaling(logs)
{
	s->setTracking(false);
	graphics_only=false;
	connect( s, SIGNAL(sliderMoved(int)),  this, SLOT(sliderMoved(int)));
	connect( s, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)));
// 	connect( dsb, SIGNAL(editingFinished()), this, SLOT(spinboxFocusEnter()));
	connect( dsb, SIGNAL(valueChanged(double)), this, SLOT(spinboxValueChanged(double)));
	
	setDefault();
	return;
}

float Gang::p2v(const int p) const
{
	float x = (p-s->minimum())/( (float) (s->maximum() - s->minimum() ) ) ;
	if( logscaling ) {
		//cout << "p:  " << p << ", x:  " << x << ", " << minv*exp(log(maxv/minv)*x ) << endl;
		return minv*exp(log(maxv/minv)*x );
	}
	return (maxv-minv)*x + minv; 
}

int Gang::v2p(const float x) const
{
	float y = (x - minv)/(maxv - minv);
	if( logscaling ) {
		y = (log(x)-log(minv))/(log(maxv)-log(minv));
		//cout << "x:  " << x << ", y:  " << y << ", " << log(x) << endl;
	}
	return  (int) ( (s->maximum() - s->minimum() )*y + s->minimum() );
}

void Gang::sliderMoved(int p)
{
// 	qDebug("Slider moved");
	if( value_from_text ) {
		value_from_text = false;
// 		qDebug("bailing out");
		return;
	}
	value_from_slider=true;
	v_ = p2v(p);
	dsb->setValue(v());
	changed_ = true;
	value_from_slider=false;
	return;
}
void Gang::sliderValueChanged(int p)
{
// 	qDebug("Slider changed");
	if( value_from_text ) {
		value_from_text = false;
// 		qDebug("bailing out");
		if (!graphics_only)
			emit finished();
		return;
	}
	value_from_slider=true;
	v_ = p2v(p);
	dsb->setValue(v());
	v_ = dsb->value();
	changed_ = true;
	value_from_slider=false;
	if (!graphics_only)
		emit finished();
	return;
}

// void Gang::spinboxFocusEnter()
// {
// 	qDebug("Spinbox lost_focus/enter");
// 	if( value_from_slider ) {
// 		value_from_slider = false;
// 		qDebug("bailing out");
// 		return;
// 	}
// 	float x = dsb->value();
// 	v_ = x;
// // 	value_from_text = true;
// 	qDebug("0");
// 	s->setValue( v2p(v() ) );
// 	qDebug("1");
// 	changed_ = true;
// 	return;
// }
void Gang::spinboxValueChanged(double x)
{
// 	qDebug("Spinbox value_changed");
	if( value_from_slider ) {
		value_from_slider = false;
// 		qDebug("bailing out");
		return;
	}
	v_ = x;
	value_from_text = true;
	s->setValue( v2p(v() ) );
	changed_ = true;
	return;
}

void Gang::setDefault()
{
// 	qDebug("def");
	graphics_only=true;
	v_ = defaultv;
	value_from_slider=true;
	dsb->setValue(v());
	value_from_text = true;
	s->setValue( v2p(v()) );
	changed_ = false;
	value_from_text = false;
	value_from_slider=false;
	graphics_only=false;
	return;
}

QString Gang::flag(const QString f) const
{
    if( ! changed() )
	return "";
    return QString(" %1 %2").arg(f).arg(v());
}

QString Gang::fname(const QString f) const
{
    if( ! changed() )
	return "";
    return QString(".%1%2").arg(f).arg(v());
}
