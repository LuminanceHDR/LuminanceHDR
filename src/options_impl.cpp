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
 */

#include "options_impl.h"
#include <QColorDialog>

QtpfsguiOptions::QtpfsguiOptions(QWidget *p, qtpfsgui_opts *orig_opts, QSettings *s) : QDialog(p), opts(orig_opts), infnancolor(opts->naninfcolor), negcolor(opts->negcolor), settings(s) {
	setupUi(this);
	from_options_to_gui(); //update the gui in order to show the options
	connect(negative_color_button,SIGNAL(clicked()),this,SLOT(negative_clicked()));
	connect(infnan_color_button,SIGNAL(clicked()),this,SLOT(infnan_clicked()));
	connect(OptionListWidget,SIGNAL(currentRowChanged(int)),stackedWidget,SLOT(setCurrentIndex(int)));
	connect(okButton,SIGNAL(clicked()),this,SLOT(ok_clicked()));
}

void QtpfsguiOptions::negative_clicked() {
	negcolor = QColorDialog::getColor(negcolor, this);
	change_color_of(negative_color_button,&negcolor);
}
void QtpfsguiOptions::infnan_clicked() {
	infnancolor = QColorDialog::getColor(infnancolor, this);
	change_color_of(infnan_color_button,&infnancolor);
}

void QtpfsguiOptions::change_color_of(QPushButton *button, QColor *newcolor) {
	if (newcolor->isValid()) {
#if QT_VERSION <= 0x040200
		QPalette modified_palette(button->palette());
		modified_palette.setColor(QPalette::Active,QPalette::Button,*newcolor);
		button->setPalette( modified_palette );
#else
		button->setStyleSheet(QString("background: rgb(%1,%2,%3)").arg(newcolor->red()).arg(newcolor->green()).arg(newcolor->blue()));
#endif
	}
}

void QtpfsguiOptions::ok_clicked() {
	settings->beginGroup(GROUP_DCRAW);
		if (checkBox_camwb->isChecked() != opts->dcraw_options.camera_wb) {
	// 		qDebug("switching camera_wb from %d to %d",opts->dcraw_options.camera_wb,checkBox_camwb->isChecked());
			opts->dcraw_options.camera_wb=checkBox_camwb->isChecked();
			settings->setValue(KEY_CAMERAWB,checkBox_camwb->isChecked());
		}
		if (checkBox_autowb->isChecked() != opts->dcraw_options.auto_wb) {
	// 		qDebug("switching auto_wb from %d to %d",opts->dcraw_options.auto_wb,checkBox_autowb->isChecked());
			opts->dcraw_options.auto_wb=checkBox_autowb->isChecked();
			settings->setValue(KEY_AUTOWB,checkBox_autowb->isChecked());
		}
		if (checkBox_4colors->isChecked() != opts->dcraw_options.four_colors) {
	// 		qDebug("switching 4colors from %d to %d",opts->dcraw_options.four_colors,checkBox_4colors->isChecked());
			opts->dcraw_options.four_colors=checkBox_4colors->isChecked();
			settings->setValue(KEY_4COLORS,checkBox_4colors->isChecked());
		}
		if (spinBox_highlights->value() != opts->dcraw_options.highlights) {
	// 		qDebug("switching highlights from %d to %d",opts->dcraw_options.highlights,spinBox_highlights->value());
			opts->dcraw_options.highlights=spinBox_highlights->value();
			settings->setValue(KEY_HIGHLIGHTS,spinBox_highlights->value());
		}
		if (comboBox_outcolspace->currentIndex() != opts->dcraw_options.output_color_space) {
	// 		qDebug("switching out_color_space from %d to %d",opts->dcraw_options.output_color_space,comboBox_outcolspace->currentIndex());
			opts->dcraw_options.output_color_space=comboBox_outcolspace->currentIndex();
			settings->setValue(KEY_OUTCOLOR,comboBox_outcolspace->currentIndex());
		}
		if (comboBox_quality->currentIndex() != opts->dcraw_options.quality) {
	// 		qDebug("switching quality from %d to %d",opts->dcraw_options.quality,comboBox_quality->currentIndex());
			opts->dcraw_options.quality=comboBox_quality->currentIndex();
			settings->setValue(KEY_QUALITY,comboBox_quality->currentIndex());
		}
	settings->endGroup();

	settings->beginGroup(GROUP_HDRVISUALIZATION);
		if(negcolor.rgba() != opts->negcolor) {
			opts->negcolor=negcolor.rgba();
			settings->setValue(KEY_NEGCOLOR,negcolor.rgba());
		}
		if(infnancolor.rgba() != opts->naninfcolor) {
			opts->naninfcolor=infnancolor.rgba();
			settings->setValue(KEY_NANINFCOLOR,infnancolor.rgba());
		}
	settings->endGroup();

	settings->beginGroup(GROUP_TONEMAPPING);
		if (checkBox_currentsize->isChecked() != opts->keepsize) {
			opts->keepsize=checkBox_currentsize->isChecked();
			settings->setValue(KEY_KEEPSIZE,checkBox_currentsize->isChecked());
		}
	settings->endGroup();

	settings->beginGroup(GROUP_TIFF);
		if (radioButtonLogLuv->isChecked() != opts->saveLogLuvTiff) {
			opts->saveLogLuvTiff=radioButtonLogLuv->isChecked();
			settings->setValue(KEY_SAVE_LOGLUV,radioButtonLogLuv->isChecked());
		}
	settings->endGroup();

	accept();
}

void QtpfsguiOptions::from_options_to_gui() {
// 	qDebug("autowb=%s", opts->dcraw_options.auto_wb ? "true":"false");
	checkBox_autowb->setChecked(opts->dcraw_options.auto_wb);
// 	qDebug("camwb=%s", opts->dcraw_options.camera_wb ? "true":"false");
	checkBox_camwb->setChecked(opts->dcraw_options.camera_wb);
// 	qDebug("4col=%s", opts->dcraw_options.four_colors ? "true":"false");
	checkBox_4colors->setChecked(opts->dcraw_options.four_colors);
// 	qDebug("highlights=%d",opts->dcraw_options.highlights);
	spinBox_highlights->setValue(opts->dcraw_options.highlights);
// 	qDebug("output_color_space=%d",opts->dcraw_options.output_color_space);
	comboBox_outcolspace->setCurrentIndex(opts->dcraw_options.output_color_space);
// 	qDebug("quality=%d",opts->dcraw_options.quality);
	comboBox_quality->setCurrentIndex(opts->dcraw_options.quality);
	change_color_of(negative_color_button,&negcolor);
	change_color_of(infnan_color_button,&infnancolor);
// 	qDebug("keepsize=%d",opts->keepsize);
	checkBox_currentsize->setChecked(opts->keepsize);
	radioButtonLogLuv->setChecked(opts->saveLogLuvTiff);
	radioButtonFloatTiff->setChecked(!opts->saveLogLuvTiff);
}

QtpfsguiOptions::~QtpfsguiOptions() {
}
