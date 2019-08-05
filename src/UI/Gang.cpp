/**
 * This file is a part of Luminance HDR package.
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
 *
 * Original Work
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 *
 * Improvements, bugfixing
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#include <math.h>

#include "Gang.h"

//#include <iostream>
using namespace std;

Gang::Gang(QSlider *slider, QDoubleSpinBox *doublespinbox, QCheckBox *checkbox1,
           QCheckBox *checkbox2, QRadioButton *radiobutton1, QRadioButton *radiobutton2,
           const float minvalue, const float maxvalue, const float defaultvalue,
           const bool logs)
    : slider(slider),
      doublespinbox(doublespinbox),
      checkbox1(checkbox1),
      checkbox2(checkbox2),
      radiobutton1(radiobutton1),
      radiobutton2(radiobutton2),
      minvalue(minvalue),
      maxvalue(maxvalue),
      defaultvalue(defaultvalue),
      logscaling(logs),
      undoState(false),
      redoState(false) {
    if (checkbox1) isCbx1Checked_default = checkbox1->isChecked();
    if (checkbox2) isCbx2Checked_default = checkbox2->isChecked();
    if (radiobutton1) isRb1Checked_default = radiobutton1->isChecked();
    if (radiobutton2) isRb2Checked_default = radiobutton2->isChecked();

    tmoSettingsList = new TmoSettingsList();
    graphics_only = false;

    if (slider) slider->setTracking(false);

    if (slider) {
        connect(slider, &QAbstractSlider::sliderMoved, this, &Gang::sliderMoved);
        connect(slider, &QAbstractSlider::valueChanged, this,
                &Gang::sliderValueChanged);
    }

    if (doublespinbox)
        // connect( dsb, SIGNAL(editingFinished()), this,
        // SLOT(spinboxFocusEnter()));
        connect(doublespinbox, SIGNAL(valueChanged(double)), this,
                SLOT(spinboxValueChanged(double)));

    if (checkbox1)
        connect(checkbox1, &QAbstractButton::toggled, this, &Gang::checkBox1Checked);

    if (checkbox2)
        connect(checkbox2, &QAbstractButton::toggled, this, &Gang::checkBox2Checked);

    if (radiobutton1)
        connect(radiobutton1, &QAbstractButton::clicked, this,
                &Gang::radioButton1Checked);

    if (radiobutton2)
        connect(radiobutton2, &QAbstractButton::clicked, this,
                &Gang::radioButton2Checked);

    setDefault();
}

Gang::~Gang() { delete tmoSettingsList; }

float Gang::p2v(const int p) const {
    float x = (p - slider->minimum()) / ((float)(slider->maximum() - slider->minimum()));
    if (logscaling) {
        // cout << "p:  " << p << ", x:  " << x << ", " <<
        // minv*exp(log(maxv/minv)*x
        // ) << endl;
        return minvalue * exp(log(maxvalue / minvalue) * x);
    }
    return (maxvalue - minvalue) * x + minvalue;
}

int Gang::v2p(const float x) const {
    float y = (x - minvalue) / (maxvalue - minvalue);
    if (logscaling) {
        y = (log(x) - log(minvalue)) / (log(maxvalue) - log(minvalue));
        // cout << "x:  " << x << ", y:  " << y << ", " << log(x) << endl;
    }
    return (int)((slider->maximum() - slider->minimum()) * y + slider->minimum());
}

void Gang::sliderMoved(int p) {
    // qDebug("Slider moved");
    if (value_from_text) {
        value_from_text = false;
        // qDebug("bailing out");
        return;
    }
    value_from_slider = true;
    value = p2v(p);
    doublespinbox->setValue(value);
    changed_ = true;
    value_from_slider = false;
}
void Gang::sliderValueChanged(int p) {
    // qDebug("Slider changed");
    if (value_from_text) {
        value_from_text = false;
        // qDebug("bailing out");
        if (!graphics_only) emit finished();
        return;
    }
    value_from_slider = true;
    value = p2v(p);
    doublespinbox->setValue(value);
    value = doublespinbox->value();
    changed_ = true;
    value_from_slider = false;
    if (!graphics_only) emit finished();
}

// void Gang::spinboxFocusEnter()
// {
//     qDebug("Spinbox lost_focus/enter");
//     if( value_from_slider ) {
//         value_from_slider = false;
//         qDebug("bailing out");
//         return;
//     }
//     double x = dsb->value();
//     value = x;
// //     value_from_text = true;
//     qDebug("0");
//     s->setValue( v2p(value ) );
//     qDebug("1");
//     changed_ = true;
//     return;
// }

void Gang::spinboxValueChanged(double x) {
    // qDebug("Spinbox value_changed");
    if (value_from_slider) {
        value_from_slider = false;
        // qDebug("bailing out");
        return;
    }
    value = x;
    value_from_text = true;
    slider->setValue(v2p(value));
    changed_ = true;
}

void Gang::checkBox1Checked(bool b) { isCbx1Checked = b; }

void Gang::checkBox2Checked(bool b) { isCbx2Checked = b; }

void Gang::radioButton1Checked(bool b) {
    isRb1Checked = b;
    isRb2Checked = !b;
    // cout << "isRb1Checked: " << b << endl;
    // cout << "isRb2Checked: " << !b << endl;
}

void Gang::radioButton2Checked(bool b) {
    isRb2Checked = b;
    isRb1Checked = !b;
    // cout << "isRb1Checked: " << !b << endl;
    // cout << "isRb2Checked: " << b << endl;
}

void Gang::setDefault() {
    //     qDebug("def");
    graphics_only = true;
    value = defaultvalue;
    value_from_slider = true;
    if (doublespinbox) {
        doublespinbox->setValue(value);
        value = doublespinbox->value();
    }
    value_from_text = true;
    if (slider) slider->setValue(v2p(value));
    changed_ = false;
    value_from_text = false;
    value_from_slider = false;
    graphics_only = false;
    if (checkbox1) isCbx1Checked = isCbx1Checked_default;
    if (checkbox2) isCbx2Checked = isCbx2Checked_default;
    if (radiobutton1) isRb1Checked = isRb1Checked_default;
    if (radiobutton2) isRb2Checked = isRb2Checked_default;

    // cout << "Gang::setDefault()" << endl;
    // cout << "v: " << value << endl;
    // cout << "cbx1: " << isCbx1Checked << endl;
    // cout << "cbx2: " << isCbx2Checked << endl;
    // cout << "rb1: " << isRb1Checked << endl;
    // cout << "rb2: " << isRb2Checked << endl;
    // cout << "/Gang::setDefault()" << endl;
}

QString Gang::flag(const QString &f) const {
    if (!changed()) return QLatin1String("");

    return QStringLiteral(" %1 %2").arg(f).arg(value);
}

QString Gang::fname(const QString &f) const {
    if (!changed()) return QLatin1String("");

    return QStringLiteral(".%1%2").arg(f).arg(value);
}

void Gang::setupUndo() {
    bool isCbx1Checked = false;
    bool isCbx2Checked = false;
    bool isRb1Checked = false;
    bool isRb2Checked = false;
    float v = 0.0;

    if (slider) v = value;
    if (checkbox1) isCbx1Checked = checkbox1->isChecked();
    if (checkbox2) isCbx2Checked = checkbox2->isChecked();
    if (radiobutton1) isRb1Checked = radiobutton1->isChecked();
    if (radiobutton2) isRb2Checked = radiobutton2->isChecked();

    tmoSettingsList->append(TmoSettings(this, v, isCbx1Checked, isCbx2Checked,
                                        isRb1Checked, isRb2Checked));
    if (tmoSettingsList->index() == 1) {
        emit enableUndo(true);
        undoState = true;
    }
    if (tmoSettingsList->index() == tmoSettingsList->size() - 1) {
        emit enableRedo(false);
        redoState = false;
    }

    // cout << "Gang::setupUndo()" << endl;
    // cout << "size: " << tmoSettingsList->size() << endl;
    // cout << "index: " << tmoSettingsList->index() << endl;
    // cout << "v: " << v << endl;
    // cout << "cbx1: " << isCbx1Checked << endl;
    // cout << "cbx2: " << isCbx2Checked << endl;
    // cout << "rb: " << isRbChecked << endl;
    // cout << "/Gang::setupUndo()" << endl;
}

void Gang::undo() {
    // cout << "Gang::undo(): size: " << tmoSettingsList->size() << endl;
    // cout << "Gang::undo(): index: " << tmoSettingsList->index() << endl;
    if (tmoSettingsList->index() == tmoSettingsList->size() - 1) {
        emit enableRedo(true);
        redoState = true;
    }

    tmoSettingsList->previous();

    if (tmoSettingsList->index() == 0) {
        emit enableUndo(false);
        undoState = false;
    }
    // cout << "/Gang::undo(): size: " << tmoSettingsList->size() << endl;
    // cout << "/Gang::undo(): index: " << tmoSettingsList->index() << endl;
}

void Gang::redo() {
    // cout << "Gang::redo()" << endl;
    // cout << "Gang::redo(): size: " << tmoSettingsList->size() << endl;
    // cout << "Gang::redo(): index: " << tmoSettingsList->index() << endl;
    if (tmoSettingsList->index() == 0) {
        emit enableUndo(true);
        undoState = true;
    }

    tmoSettingsList->next();

    if (tmoSettingsList->index() == tmoSettingsList->size() - 1) {
        emit enableRedo(false);
        redoState = false;
    }

    // cout << "/Gang::redo(): size: " << tmoSettingsList->size() << endl;
    // cout << "/Gang::redo(): index: " << tmoSettingsList->index() << endl;
}

void Gang::updateUndoState() {
    // cout << "Gang::updateUndoState()" << endl;
    // cout << "undoState: " << undoState << endl;
    // cout << "redoState: " << redoState << endl;
    // cout << "/Gang::updateUndoState()" << endl;
    emit enableUndo(undoState);
    emit enableRedo(redoState);
}

//
//===================================== Undo/Redo
//============================================
//
TmoSettings::TmoSettings(Gang *gangPtr, float v, bool isCbx1, bool isCbx2,
                         bool isRB1C, bool isRB2C)
    : gangPtr(gangPtr) {
    if (gangPtr->checkbox1) {
        isCbx1Checked = isCbx1;
    }
    if (gangPtr->checkbox2) {
        isCbx2Checked = isCbx2;
    }
    if (gangPtr->radiobutton1) {
        isRb1Checked = isRB1C;
    }
    if (gangPtr->radiobutton2) {
        isRb2Checked = isRB2C;
    }
    if (gangPtr->slider) {
        value = v;
    }
}

void TmoSettings::apply() const {
    // cout << "TmoSettings::apply()" << endl;
    if (gangPtr->slider) gangPtr->slider->setValue(gangPtr->v2p(value));
    if (gangPtr->doublespinbox) {
        gangPtr->doublespinbox->setValue(value);
        gangPtr->value = gangPtr->doublespinbox->value();
    }
    if (gangPtr->checkbox1) gangPtr->checkbox1->setChecked(isCbx1Checked);
    if (gangPtr->checkbox2) gangPtr->checkbox2->setChecked(isCbx2Checked);
    if (gangPtr->radiobutton1) gangPtr->radiobutton1->setChecked(isRb1Checked);
    if (gangPtr->radiobutton2) gangPtr->radiobutton2->setChecked(isRb2Checked);
}

//
// TmoSettingsList Implementation
//
TmoSettingsList::TmoSettingsList() : QList<TmoSettings>(), m_index(-1) {}
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

int TmoSettingsList::index() { return m_index; }

void TmoSettingsList::append(const TmoSettings &value) {
    QList<TmoSettings>::append(value);
    m_index++;
}
