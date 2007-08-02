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

#include <QColorDialog>
#include <QFileDialog>
#include "options_impl.h"
#include "../config.h"

QtpfsguiOptions::QtpfsguiOptions(QWidget *p, qtpfsgui_opts *orig_opts, QSettings *s) : QDialog(p), opts(orig_opts), infnancolor(opts->naninfcolor), negcolor(opts->negcolor), settings(s) {
	setupUi(this);
	from_options_to_gui(); //update the gui in order to show the options
	connect(negative_color_button,SIGNAL(clicked()),this,SLOT(negative_clicked()));
	connect(infnan_color_button,SIGNAL(clicked()),this,SLOT(infnan_clicked()));
	connect(okButton,SIGNAL(clicked()),this,SLOT(ok_clicked()));
	connect(chooseCachePathButton,SIGNAL(clicked()),this,SLOT(updateLineEditString()));
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
			opts->dcraw_options.camera_wb=checkBox_camwb->isChecked();
			settings->setValue(KEY_CAMERAWB,checkBox_camwb->isChecked());
		}
		if (checkBox_autowb->isChecked() != opts->dcraw_options.auto_wb) {
			opts->dcraw_options.auto_wb=checkBox_autowb->isChecked();
			settings->setValue(KEY_AUTOWB,checkBox_autowb->isChecked());
		}
		if (checkBox_4colors->isChecked() != opts->dcraw_options.four_colors) {
			opts->dcraw_options.four_colors=checkBox_4colors->isChecked();
			settings->setValue(KEY_4COLORS,checkBox_4colors->isChecked());
		}
		if (spinBox_highlights->value() != opts->dcraw_options.highlights) {
			opts->dcraw_options.highlights=spinBox_highlights->value();
			settings->setValue(KEY_HIGHLIGHTS,spinBox_highlights->value());
		}
		if (comboBox_outcolspace->currentIndex() != opts->dcraw_options.output_color_space) {
			opts->dcraw_options.output_color_space=comboBox_outcolspace->currentIndex();
			settings->setValue(KEY_OUTCOLOR,comboBox_outcolspace->currentIndex());
		}
		if (comboBox_quality->currentIndex() != opts->dcraw_options.quality) {
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
		if (lineEditTempPath->text() != opts->tempfilespath) {
			opts->tempfilespath=lineEditTempPath->text();
			settings->setValue(KEY_TEMP_RESULT_PATH,lineEditTempPath->text());
		}
		if (imageformat_comboBox->currentText() != opts->batch_ldr_format) {
			opts->batch_ldr_format=imageformat_comboBox->currentText();
			settings->setValue(KEY_BATCH_LDR_FORMAT,imageformat_comboBox->currentText());
		}
		if (thread_spinBox->value() != opts->num_batch_threads) {
			opts->num_batch_threads=thread_spinBox->value();
			settings->setValue(KEY_NUM_BATCH_THREADS,thread_spinBox->value());
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
	lineEditTempPath->setText(opts->tempfilespath);
	if (opts->batch_ldr_format=="JPEG")
		imageformat_comboBox->setCurrentIndex(0);
	else if (opts->batch_ldr_format=="PNG")
		imageformat_comboBox->setCurrentIndex(1);
	else if (opts->batch_ldr_format=="PPM")
		imageformat_comboBox->setCurrentIndex(2);
	else if (opts->batch_ldr_format=="PBM")
		imageformat_comboBox->setCurrentIndex(3);
	else if (opts->batch_ldr_format=="BMP")
		imageformat_comboBox->setCurrentIndex(4);
	thread_spinBox->setValue(opts->num_batch_threads);
	checkBox_autowb->setChecked(opts->dcraw_options.auto_wb);
	checkBox_camwb->setChecked(opts->dcraw_options.camera_wb);
	checkBox_4colors->setChecked(opts->dcraw_options.four_colors);
	spinBox_highlights->setValue(opts->dcraw_options.highlights);
	comboBox_outcolspace->setCurrentIndex(opts->dcraw_options.output_color_space);
	comboBox_quality->setCurrentIndex(opts->dcraw_options.quality);
	radioButtonLogLuv->setChecked(opts->saveLogLuvTiff);
	radioButtonFloatTiff->setChecked(!opts->saveLogLuvTiff);
	change_color_of(negative_color_button,&negcolor);
	change_color_of(infnan_color_button,&infnancolor);
}

QtpfsguiOptions::~QtpfsguiOptions() {
}

void QtpfsguiOptions::updateLineEditString() {
	QString dir=QFileDialog::getExistingDirectory(
	this,
	tr("Choose a directory"),
	QDir::currentPath(),
	QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks);
	if (!dir.isEmpty()) {
		lineEditTempPath->setText(dir);
	}
}
