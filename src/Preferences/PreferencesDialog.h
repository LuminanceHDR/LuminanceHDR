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
#include <QLineEdit>
#include <QMap>
#include <QSignalMapper>

#include "LibpfsAdditions/formathelper.h"

namespace Ui {
class PreferencesDialog;
}

class PreferencesDialog : public QDialog {
    Q_OBJECT

   private:
    QScopedPointer<Ui::PreferencesDialog> m_Ui;

   public:
    PreferencesDialog(QWidget *parent, int tab = 0);
    ~PreferencesDialog();

   private:
    void from_options_to_gui();

    QColor infnancolor, negcolor;
    QMap<QString, int> fromIso639ToGuiIndex;
    QMap<int, QString> fromGuiIndexToIso639;
    QSignalMapper *toolButtonMapper;
    pfsadditions::FormatHelper m_formatHelper;

   protected:
    virtual void changeEvent(QEvent *event);

   private Q_SLOTS:
    void on_okButton_clicked();
    void on_cancelButton_clicked();
    void on_chooseCachePathButton_clicked();
    void enterWhatsThis();

    void on_user_qual_comboBox_currentIndexChanged(int);
    void on_med_passes_spinBox_valueChanged(int);
    void on_wb_method_comboBox_currentIndexChanged(int);
    void on_TK_spinBox_valueChanged(int);
    void on_highlights_comboBox_currentIndexChanged(int);
    void on_level_spinBox_valueChanged(int);
    void on_user_black_spinBox_valueChanged(int);
    void on_user_sat_spinBox_valueChanged(int);
    void on_threshold_spinBox_valueChanged(int);
    void on_use_black_CB_stateChanged(int);
    void on_use_sat_CB_stateChanged(int);
    void on_use_noise_CB_stateChanged(int);
    void on_use_chroma_CB_stateChanged(int);
    void on_brightness_horizontalSlider_valueChanged(int);
    void on_brightness_doubleSpinBox_valueChanged(double);
    void on_red_horizontalSlider_valueChanged(int);
    void on_red_doubleSpinBox_valueChanged(double);
    void on_blue_horizontalSlider_valueChanged(int);
    void on_blue_doubleSpinBox_valueChanged(double);
    void on_green_horizontalSlider_valueChanged(int);
    void on_green_doubleSpinBox_valueChanged(double);

    void on_user_qual_toolButton_clicked();
    void on_med_passes_toolButton_clicked();
    void on_wb_method_toolButton_clicked();
    void on_TK_toolButton_clicked();
    void on_highlights_toolButton_clicked();
    void on_level_toolButton_clicked();
    void on_brightness_toolButton_clicked();
    void on_user_black_toolButton_clicked();
    void on_user_sat_toolButton_clicked();
    void on_threshold_toolButton_clicked();
    void on_red_toolButton_clicked();
    void on_blue_toolButton_clicked();
    void on_green_toolButton_clicked();

    void on_exportFileButton_clicked();

    void on_camera_toolButton_clicked();
    void on_monitor_toolButton_clicked();
    void on_printer_toolButton_clicked();

    void toolButton_clicked(int);

    void openColorProfile(QLineEdit *lineEdit);

    void on_themeChanged();
};
#endif
