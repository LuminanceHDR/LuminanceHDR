/**
 * This file is a part of Luminance HDR package.
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

namespace Ui
{
    class PreferencesDialog;
}

class PreferencesDialog : public QDialog
{
Q_OBJECT
private:
    QScopedPointer<Ui::PreferencesDialog> m_Ui;

public:
	PreferencesDialog(QWidget *parent);
	~PreferencesDialog();
private:
    void from_options_to_gui();

    QColor infnancolor, negcolor;
	QMap<QString, int> fromIso639ToGuiIndex;
	QMap<int, QString> fromGuiIndexToIso639;

protected:
	virtual void changeEvent(QEvent* event);

private Q_SLOTS:
	void negative_clicked();
	void infnan_clicked();
	void ok_clicked();
	void cancel_clicked();
	void updateLineEditString();
	void enterWhatsThis();
	
	void user_qual_comboBox_currentIndexChanged(int);
	void med_passes_spinBox_valueChanged(int);
	void wb_method_comboBox_currentIndexChanged(int);
	void TK_spinBox_valueChanged(int);
	void highlights_comboBox_currentIndexChanged(int);
	void level_spinBox_valueChanged(int);
	void user_black_spinBox_valueChanged(int);
	void user_sat_spinBox_valueChanged(int);
	void threshold_spinBox_valueChanged(int);
	void use_black_CB_stateChanged(int);
	void use_sat_CB_stateChanged(int);
	void use_noise_CB_stateChanged(int);
	void use_chroma_CB_stateChanged(int);
	void brightness_horizontalSlider_valueChanged(int);
	void brightness_doubleSpinBox_valueChanged(double);
	void red_horizontalSlider_valueChanged(int);
	void red_doubleSpinBox_valueChanged(double);
	void blue_horizontalSlider_valueChanged(int);
	void blue_doubleSpinBox_valueChanged(double);
	void green_horizontalSlider_valueChanged(int);
	void green_doubleSpinBox_valueChanged(double);
	void camera_comboBox_currentIndexChanged(int);

 	void user_qual_toolButton_clicked();
	void med_passes_toolButton_clicked();
	void wb_method_toolButton_clicked();
	void TK_toolButton_clicked();
	void highlights_toolButton_clicked();
	void level_toolButton_clicked();
	void brightness_toolButton_clicked();
	void user_black_toolButton_clicked();
	void user_sat_toolButton_clicked();
	void threshold_toolButton_clicked();
	void red_toolButton_clicked();
	void blue_toolButton_clicked();
	void green_toolButton_clicked();
	void camera_toolButton_clicked();

	void toolButtonInterface_clicked();
	void toolButtonHDR_clicked();
	void toolButtonTM_clicked();
	void toolButtonRAW_clicked();
	void toolButtonExtTool_clicked();

    void batchTmFormatSelector(int);
};
#endif
