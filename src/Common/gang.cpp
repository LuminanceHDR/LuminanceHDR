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

//#include <iostream>
#include <math.h>
#include "gang.h"

//using namespace std;

Gang::Gang(QSlider* slider, QDoubleSpinBox* doublespinbox, 
		QCheckBox *chkbox1, QCheckBox *chkbox2, QRadioButton *rbutton, 
		const float minvalue, const float maxvalue, 
		const float vv, const bool logs) : 
		s(slider), dsb(doublespinbox), cbx1(chkbox1), cbx2(chkbox2), rb(rbutton),  
		minv(minvalue), maxv(maxvalue), defaultv(vv), logscaling(logs),
		undoState(false), redoState(false)
{
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
		connect( cbx1, SIGNAL(stateChanged(int)), this, SLOT(checkBox1Checked(int)));
	
	if (cbx2)
		connect( cbx2, SIGNAL(stateChanged(int)), this, SLOT(checkBox2Checked(int)));

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

void Gang::checkBox1Checked(int) {
	isCbx1Checked = true;
}

void Gang::checkBox2Checked(int) {
	isCbx2Checked = true;
}

void Gang::radioButtonChecked(bool) {
	isRbChecked = true;
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
	Qt::CheckState cbx1CheckState = Qt::Unchecked;
	Qt::CheckState cbx2CheckState = Qt::Unchecked;
	bool isRbChecked = false;
	float v = 0.0;
	
	if (s)
		v = value;
	if (cbx1) 
		cbx1CheckState = cbx1->checkState();
	if  (cbx2) 
		cbx2CheckState = cbx2->checkState();
	if (rb) 
		isRbChecked = rb->isChecked();
	
	TmoSettings *tmoSettings = new  TmoSettings(this, v, cbx1CheckState, cbx2CheckState, isRbChecked);
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
TmoSettings::TmoSettings(Gang *gangPtr, float v, Qt::CheckState cbx1CS, Qt::CheckState cbx2CS, bool isRBC):
	gangPtr(gangPtr)
{
	if (gangPtr->cbx1) {
		cbx1CheckState = cbx1CS;
	}
	if (gangPtr->cbx2) {
		cbx2CheckState = cbx2CS;
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
		gangPtr->cbx1->setCheckState(cbx1CheckState);
	if (gangPtr->cbx2)
		gangPtr->cbx2->setCheckState(cbx2CheckState);
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
