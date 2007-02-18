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

// #include <qwidget.h>
#include <QSlider>
#include <QLineEdit>

class Gang : public QObject
{
	Q_OBJECT
public:
	Gang(QSlider* s_, QLineEdit* le_, 
		const float minv_, const float maxv_, 
		const float vv, const bool logs = false);
	
	float v() const { return v_; };
	void setDefault();
	bool changed() const { return changed_; };
	QString flag(const QString f) const;
	QString fname(const QString f) const;
	float p2v(const int p) const;
	int v2p(const float x) const;
public slots:
	void sliderMoved(int p);
	void textChanged();
private:
	QSlider *s;
	QLineEdit *le;
	float minv;
	float maxv;
	float defaultv;
	bool logscaling;
	float v_;
	bool value_from_text;
	bool changed_;
};

#endif
