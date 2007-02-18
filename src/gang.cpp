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
 * based on previous GPL code from qpfstmo
 */

#include <iostream>
#include <math.h>
#include "gang.h"

using namespace std;


Gang::Gang(QSlider* slider_, QLineEdit* lineedit_, 
		const float minv_, const float maxv_, 
		const float vv, const bool logs) : 
			s(slider_), le(lineedit_), minv(minv_), maxv(maxv_), defaultv(vv), logscaling(logs)
{
// 	cerr << "Gang::Gang()" << endl;
	connect( s,  SIGNAL(valueChanged(int)), this, SLOT(sliderMoved(int)));
	connect( le, SIGNAL(returnPressed()),   this, SLOT(textChanged()));
	
	/*
	value_from_text = false;
	v_ = defaultv;
	s->setValue( v2p(v()) );
	changed_ = false;
	*/
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
	if( value_from_text ) {
		value_from_text = false;
		return;
	}
	v_ = p2v(p);
	le->setText( QString("%1").arg( v() )  );
	changed_ = true;
	return;
}

void Gang::textChanged()
{
	bool ok;
	
	float x = le->text().toFloat(&ok);
	if( ! ok ) 
		x = v();
	v_ = x;
	value_from_text = true;
	s->setValue( v2p(v() ) );
	changed_ = true;
	return;
}

void Gang::setDefault()
{
	v_ = defaultv;
	le->setText( QString("%1").arg(v()));
	value_from_text = true;
	s->setValue( v2p(v()) );
	changed_ = false;
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
