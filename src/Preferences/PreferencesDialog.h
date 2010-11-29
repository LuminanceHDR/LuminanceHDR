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

#ifndef OPTIONS_IMPL_H
#define OPTIONS_IMPL_H

#include <QDialog>
#include <QMap>

#include "ui_PreferencesDialog.h"
#include "Common/options.h"
#include "Common/global.h"

class PreferencesDialog : public QDialog, private Ui::PreferencesDialog
{
Q_OBJECT
public:
	PreferencesDialog(QWidget *parent);
	~PreferencesDialog();
private:
	void change_color_of(QPushButton *,QColor *);
	void from_options_to_gui();
	LuminanceOptions *luminance_options;
	QColor infnancolor, negcolor;
	QMap<QString, int> fromIso639ToGuiIndex;
	QMap<int, QString> fromGuiIndexToIso639;
	QStringList sanitizeAISparams();
	QStringList sanitizeDCRAWparams();
private slots:
	void negative_clicked();
	void infnan_clicked();
	void ok_clicked();
	void updateLineEditString();
	void helpDcrawParamsButtonClicked();
	void enterWhatsThis();
	void wb_method_comboBox_currentIndexChanged(int i);
	void use_black_CB_stateChanged(int);
	void use_sat_CB_stateChanged(int);
	void use_noise_CB_stateChanged(int);
	void use_chroma_CB_stateChanged(int);
	void brightness_horizontalSlider_valueChanged(int);
	void brightness_doubleSpinBox_valueChanged(double);
	void red_horizontalSlider_valueChanged(int);
	void red_doubleSpinBox_valueChanged(double);
	void green_horizontalSlider_valueChanged(int);
	void green_doubleSpinBox_valueChanged(double);
};
#endif
