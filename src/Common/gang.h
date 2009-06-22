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

#ifndef GANG_H
#define GANG_H

#include <QSlider>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QList>

class TmoSettingsList;

class Gang : public QObject
{
	Q_OBJECT
public:
	Gang(QSlider* s = 0, QDoubleSpinBox* dsb = 0,
		QCheckBox *cbx1 = 0, QCheckBox *cbx2 = 0, QRadioButton *rb = 0, 
		const float minv = 0.0, const float maxv = 0.0, 
		const float vv = 0.0, const bool logs = false);
	~Gang();
	friend class TmoSettings;
	float v() const { return value; };
	bool isCheckBox1Checked() const { return isCbx1Checked; };
	bool isCheckBox2Checked() const { return isCbx2Checked; };
	bool isRadioButtonChecked() const { return isRbChecked; };
	float p2v(const int p) const;
	int v2p(const float x) const;
	void setDefault();
	bool changed() const { return changed_; };
	QString flag(const QString f) const;
	QString fname(const QString f) const;
	void setupUndo();
	void undo();
	void redo();
	void updateUndoState();
protected slots:
	void sliderMoved(int p);
	void sliderValueChanged(int p);
	void spinboxValueChanged(double);
	void checkBox1Checked(int);
	void checkBox2Checked(int);
	void radioButtonChecked(bool);
signals:
	void finished();
	void enableUndo(bool);
	void enableRedo(bool);
private:
	QSlider *s;
	QDoubleSpinBox *dsb;
	QCheckBox *cbx1;
	QCheckBox *cbx2;
	QRadioButton *rb;
	bool isCbx1Checked;
	bool isCbx2Checked;
	bool isRbChecked;
	float minv;
	float maxv;
	float defaultv;
	bool logscaling;
	float value;
	bool value_from_text;
	bool value_from_slider;
	bool graphics_only;
	bool changed_;
	bool undoState;
	bool redoState;
	TmoSettingsList *tmoSettingsList;
};

//
//==================================== Undo/Redo ===================================================
//
// TmoSettings stores current applied settings
//
class TmoSettings {
public:
	TmoSettings(Gang *gangPtr, float, Qt::CheckState, Qt::CheckState, bool);
	void apply() const;
protected:
	Gang *gangPtr;
	Qt::CheckState cbx1CheckState;
	Qt::CheckState cbx2CheckState;
	bool isRbChecked;
	float value;
};

class TmoSettingsList : public QList<TmoSettings> {
public:
	TmoSettingsList();
	~TmoSettingsList();
	void previous();
	void next();
	int index();
	void append(const TmoSettings &value);
private:
	int m_index;
};

#endif

