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
 * based on previous GPL code from qpfstmo, by Nicholas Phillips.
 * Original Work
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * Improvements, bugfixing
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#include <math.h>
#include "gang.h"

//#include <iostream>
//using namespace std;

Gang::Gang(QSlider* slider, QDoubleSpinBox* doublespinbox, 
		QCheckBox *chkbox1, QCheckBox *chkbox2, QRadioButton *rbutton, 
		const float minvalue, const float maxvalue, 
		const float vv, const bool logs) : 
		s(slider), dsb(doublespinbox), cbx1(chkbox1), cbx2(chkbox2), rb(rbutton),  
		minv(minvalue), maxv(maxvalue), defaultv(vv), logscaling(logs),
		undoState(false), redoState(false)
{
	if (cbx1)
		isCbx1Checked_default = cbx1->isChecked();
	if (cbx2)
		isCbx2Checked_default = cbx2->isChecked();
	if (rb)
		isRbChecked_default = rb->isChecked();

	if (s) {
		s->setMaximum((int)(100*maxvalue));
		s->setMinimum((int)(100*minvalue));
		s->setSingleStep((int)(maxvalue-minvalue));	
		s->setPageStep((int)(10*(maxvalue-minvalue)));	
	}
	if (dsb) {
		dsb->setMaximum((int)(100*maxvalue));
		dsb->setMinimum((int)(100*minvalue));
		dsb->setSingleStep((int)(maxvalue-minvalue));	
	}

	tmoSettingsList = new TmoSettingsList();
	graphics_only = false;

	if (s)
		s->setTracking(false);
	
	if (s) {	
		connect( s, SIGNAL(sliderMoved(int)),  this, SLOT(sliderMoved(int)));
		connect( s, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)));
	}

	if (dsb)
		//connect( dsb, SIGNAL(editingFinished()), this, SLOT(spinboxFocusEnter()));
		connect( dsb, SIGNAL(valueChanged(double)), this, SLOT(spinboxValueChanged(double)));

	if (cbx1)
		connect( cbx1, SIGNAL(toggled(bool)), this, SLOT(checkBox1Checked(bool)));
	
	if (cbx2)
		connect( cbx2, SIGNAL(toggled(bool)), this, SLOT(checkBox2Checked(bool)));

	if (rb)
		connect( rb, SIGNAL(clicked(bool)), this, SLOT(radioButtonChecked(bool)));
	
	setDefault();
}

Gang::~Gang() {
	delete tmoSettingsList;
} 

float Gang::p2v(const int p) const
{
	float x = (p-s->minimum())/( (float) (s->maximum() - s->minimum() ) ) ;
	if( logscaling ) {
		////cout << "p:  " << p << ", x:  " << x << ", " << minv*exp(log(maxv/minv)*x ) << endl;
		return minv*exp(log(maxv/minv)*x );
	}
	return (maxv-minv)*x + minv; 
}

int Gang::v2p(const float x) const
{
	float y = (x - minv)/(maxv - minv);
	if( logscaling ) {
		y = (log(x)-log(minv))/(log(maxv)-log(minv));
		////cout << "x:  " << x << ", y:  " << y << ", " << log(x) << endl;
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
	value_from_slider = true;
	value = p2v(p);
	dsb->setValue(value);
	changed_ = true;
	value_from_slider = false;
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
	value_from_slider = true;
	value = p2v(p);
	dsb->setValue(value);
	value = dsb->value();
	changed_ = true;
	value_from_slider = false;
	if (!graphics_only)
		emit finished();
}

// void Gang::spinboxFocusEnter()
// {
// 	qDebug("Spinbox lost_focus/enter");
// 	if( value_from_slider ) {
// 		value_from_slider = false;
// 		qDebug("bailing out");
// 		return;
// 	}
// 	double x = dsb->value();
// 	value = x;
// // 	value_from_text = true;
// 	qDebug("0");
// 	s->setValue( v2p(value ) );
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
	value = x;
	value_from_text = true;
	s->setValue( v2p(value ) );
	changed_ = true;
}

void Gang::checkBox1Checked(bool b) {
	isCbx1Checked = b;
}

void Gang::checkBox2Checked(bool b) {
	isCbx2Checked = b;
}

void Gang::radioButtonChecked(bool b) {
	isRbChecked = b;
}


void Gang::setDefault()
{
// 	qDebug("def");
	graphics_only = true;
	value = defaultv;
	value_from_slider = true;
	if (dsb) {
		dsb->setValue(value);
		value = dsb->value();
	}
	value_from_text = true;
	if (s)
		s->setValue( v2p(value) );
	changed_ = false;
	value_from_text = false;
	value_from_slider = false;
	graphics_only = false;
	if (cbx1) 
		isCbx1Checked = isCbx1Checked_default;
	if  (cbx2) 
		isCbx2Checked = isCbx2Checked_default;
	if (rb) 
		isRbChecked = isRbChecked_default;
}

QString Gang::flag(const QString f) const
{
    if( ! changed() )
	return "";
    return QString(" %1 %2").arg(f).arg(value);
}

QString Gang::fname(const QString f) const
{
    if( ! changed() )
	return "";
    return QString(".%1%2").arg(f).arg(value);
}

void Gang::setupUndo() {
	bool isCbx1Checked = false;
	bool isCbx2Checked = false;
	bool isRbChecked = false;
	float v = 0.0;
	
	if (s)
		v = value;
	if (cbx1) 
		isCbx1Checked = cbx1->isChecked();
	if  (cbx2) 
		isCbx2Checked = cbx2->isChecked();
	if (rb) 
		isRbChecked = rb->isChecked();
	
	TmoSettings *tmoSettings = new  TmoSettings(this, v, isCbx1Checked, isCbx2Checked, isRbChecked);
	tmoSettingsList->append(*tmoSettings);
	if (tmoSettingsList->index() == 1) {
		emit enableUndo(true);
		undoState = true;
	}
	if (tmoSettingsList->index() == tmoSettingsList->size() - 1) {
		emit enableRedo(false);
		redoState = false;
	}

	//cout << "Gang::setupUndo(): size: " << tmoSettingsList->size() << endl;
	//cout << "Gang::setupUndo(): index: " << tmoSettingsList->index() << endl;

	////cout << "Gang::setupUndo()" << endl;
	////cout << "v: " << v << endl;
	////cout << "cbx1: " << (int)cbx1CheckState << endl;
	////cout << "cbx2: " << (int)cbx2CheckState << endl;
	////cout << "rb: " << isRbChecked << endl;
	////cout << "count: " << undoStack->count() << endl;
}

void Gang::undo() {
	//cout << "Gang::undo(): size: " << tmoSettingsList->size() << endl;
	//cout << "Gang::undo(): index: " << tmoSettingsList->index() << endl;
	if (tmoSettingsList->index() == tmoSettingsList->size() - 1) {
		emit enableRedo(true);
		redoState = true;
	}
	
	tmoSettingsList->previous();

	if (tmoSettingsList->index() == 0) {
		emit enableUndo(false);
		undoState = false;
	}
	//cout << "/Gang::undo(): size: " << tmoSettingsList->size() << endl;
	//cout << "/Gang::undo(): index: " << tmoSettingsList->index() << endl;
}

void Gang::redo() {
	//cout << "Gang::redo(): size: " << tmoSettingsList->size() << endl;
	//cout << "Gang::redo(): index: " << tmoSettingsList->index() << endl;
	if (tmoSettingsList->index() == 0) {
		emit enableUndo(true);
		undoState = true;
	}

	tmoSettingsList->next();

	if (tmoSettingsList->index() == tmoSettingsList->size() - 1) {
		emit enableRedo(false);
		redoState = false;
	}

	//cout << "/Gang::redo(): size: " << tmoSettingsList->size() << endl;
	//cout << "/Gang::redo(): index: " << tmoSettingsList->index() << endl;
}

void Gang::updateUndoState() {
	//cout << "Gang::undoState(int)" << endl;
	//cout << "Gang::undoState(int): undoState: " << undoState << endl;
	//cout << "Gang::undoState(int): redoState: " << redoState << endl;
	emit enableUndo(undoState);
	emit enableRedo(redoState);
}

//
//===================================== Undo/Redo ============================================
//
TmoSettings::TmoSettings(Gang *gangPtr, float v, bool isCbx1, bool isCbx2, bool isRBC):
	gangPtr(gangPtr)
{
	if (gangPtr->cbx1) {
		isCbx1Checked = isCbx1;
	}
	if (gangPtr->cbx2) {
		isCbx2Checked = isCbx2;
	}
	if (gangPtr->rb) {
		isRbChecked = isRBC;

	}
	if (gangPtr->s) {
		value = v;
	}
}

void TmoSettings::apply() const {
	//cout << "TmoSettings::apply()" << endl;
 	if (gangPtr->s) 
		gangPtr->s->setValue(gangPtr->v2p(value));
	if (gangPtr->dsb) {
		gangPtr->dsb->setValue(value);
		gangPtr->value = gangPtr->dsb->value();	
	}
	if (gangPtr->cbx1)
		gangPtr->cbx1->setChecked(isCbx1Checked);
	if (gangPtr->cbx2)
		gangPtr->cbx2->setChecked(isCbx2Checked);
	if (gangPtr->rb)
		gangPtr->rb->setChecked(isRbChecked);
}

//
// TmoSettingsList Implementation
//
TmoSettingsList::TmoSettingsList(): QList<TmoSettings>(), m_index(-1) {}	
TmoSettingsList::~TmoSettingsList() {}	

void TmoSettingsList::previous() {
	if (m_index > 0) {
		m_index--;
		at(m_index).apply();
	}
}

void TmoSettingsList::next() {
	if (m_index < size()) {
		m_index++;
		at(m_index).apply();
	}
}

int TmoSettingsList::index() {
	return m_index;
}

void TmoSettingsList::append(const TmoSettings &value) {
	QList<TmoSettings>::append(value);
	m_index++;
}
