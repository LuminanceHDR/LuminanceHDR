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
#include <QWhatsThis>
#include "preferencesDialog.h"
#include "../Common/config.h"
#include "../generated_uic/ui_documentation.h"

PreferenceDialog::PreferenceDialog(QWidget *p) : QDialog(p) {
	setupUi(this);

	qtpfsgui_options=QtpfsguiOptions::getInstance();
	negcolor=qtpfsgui_options->negcolor;
	infnancolor=qtpfsgui_options->naninfcolor;

	from_options_to_gui(); //update the gui in order to show the options
	connect(negative_color_button,SIGNAL(clicked()),this,SLOT(negative_clicked()));
	connect(infnan_color_button,SIGNAL(clicked()),this,SLOT(infnan_clicked()));
	connect(okButton,SIGNAL(clicked()),this,SLOT(ok_clicked()));
	connect(chooseCachePathButton,SIGNAL(clicked()),this,SLOT(updateLineEditString()));
	connect(helpDcrawParamsButton,SIGNAL(clicked()),this,SLOT(helpDcrawParamsButtonClicked()));
	connect(whatsThisButton,SIGNAL(clicked()),this,SLOT(enterWhatsThis()));

	Qt::ToolButtonStyle style = (Qt::ToolButtonStyle)settings.value(KEY_TOOLBAR_MODE,Qt::ToolButtonTextUnderIcon).toInt();
	whatsThisButton->setToolButtonStyle(style);
}

void PreferenceDialog::negative_clicked() {
	negcolor = QColorDialog::getColor(negcolor, this);
	change_color_of(negative_color_button,&negcolor);
}
void PreferenceDialog::infnan_clicked() {
	infnancolor = QColorDialog::getColor(infnancolor, this);
	change_color_of(infnan_color_button,&infnancolor);
}

void PreferenceDialog::change_color_of(QPushButton *button, QColor *newcolor) {
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

void PreferenceDialog::ok_clicked() {
	settings.beginGroup(GROUP_DCRAW);
		qtpfsgui_options->dcraw_options=commandlineParamsLineEdit->text().split(" ",QString::SkipEmptyParts);
		settings.setValue(KEY_EXTERNAL_DCRAW_OPTIONS,qtpfsgui_options->dcraw_options);
	settings.endGroup();

	settings.beginGroup(GROUP_HDRVISUALIZATION);
		if(negcolor.rgba() != qtpfsgui_options->negcolor) {
			qtpfsgui_options->negcolor=negcolor.rgba();
			settings.setValue(KEY_NEGCOLOR,negcolor.rgba());
		}
		if(infnancolor.rgba() != qtpfsgui_options->naninfcolor) {
			qtpfsgui_options->naninfcolor=infnancolor.rgba();
			settings.setValue(KEY_NANINFCOLOR,infnancolor.rgba());
		}
	settings.endGroup();

	settings.beginGroup(GROUP_TONEMAPPING);
		if (lineEditTempPath->text() != qtpfsgui_options->tempfilespath) {
			qtpfsgui_options->tempfilespath=lineEditTempPath->text();
			settings.setValue(KEY_TEMP_RESULT_PATH,lineEditTempPath->text());
		}
		if (imageformat_comboBox->currentText() != qtpfsgui_options->batch_ldr_format) {
			qtpfsgui_options->batch_ldr_format=imageformat_comboBox->currentText();
			settings.setValue(KEY_BATCH_LDR_FORMAT,imageformat_comboBox->currentText());
		}
		if (thread_spinBox->value() != qtpfsgui_options->num_threads) {
			qtpfsgui_options->num_threads=thread_spinBox->value();
			settings.setValue(KEY_NUM_BATCH_THREADS,thread_spinBox->value());
		}
	settings.endGroup();

	settings.beginGroup(GROUP_TIFF);
		if (radioButtonLogLuv->isChecked() != qtpfsgui_options->saveLogLuvTiff) {
			qtpfsgui_options->saveLogLuvTiff=radioButtonLogLuv->isChecked();
			settings.setValue(KEY_SAVE_LOGLUV,radioButtonLogLuv->isChecked());
		}
	settings.endGroup();

	accept();
}

void PreferenceDialog::from_options_to_gui() {
	lineEditTempPath->setText(qtpfsgui_options->tempfilespath);
	if (qtpfsgui_options->batch_ldr_format=="JPEG")
		imageformat_comboBox->setCurrentIndex(0);
	else if (qtpfsgui_options->batch_ldr_format=="PNG")
		imageformat_comboBox->setCurrentIndex(1);
	else if (qtpfsgui_options->batch_ldr_format=="PPM")
		imageformat_comboBox->setCurrentIndex(2);
	else if (qtpfsgui_options->batch_ldr_format=="PBM")
		imageformat_comboBox->setCurrentIndex(3);
	else if (qtpfsgui_options->batch_ldr_format=="BMP")
		imageformat_comboBox->setCurrentIndex(4);
	thread_spinBox->setValue(qtpfsgui_options->num_threads);
	commandlineParamsLineEdit->setText(qtpfsgui_options->dcraw_options.join(" "));
	radioButtonLogLuv->setChecked(qtpfsgui_options->saveLogLuvTiff);
	radioButtonFloatTiff->setChecked(!qtpfsgui_options->saveLogLuvTiff);
	change_color_of(negative_color_button,&negcolor);
	change_color_of(infnan_color_button,&infnancolor);
}

PreferenceDialog::~PreferenceDialog() {
}

void PreferenceDialog::updateLineEditString() {
	QString dir=QFileDialog::getExistingDirectory(
	this,
	tr("Choose a directory"),
	QDir::currentPath(),
	QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks);
	if (!dir.isEmpty()) {
		lineEditTempPath->setText(dir);
	}
}

void PreferenceDialog::helpDcrawParamsButtonClicked() {
	QDialog *help=new QDialog();
	help->setAttribute(Qt::WA_DeleteOnClose);
	Ui::HelpDialog ui;
	ui.setupUi(help);
	ui.tb->setSearchPaths(QStringList("/usr/share/qtpfsgui/html") << "/usr/local/share/qtpfsgui/html" << "./html");
	ui.tb->setSource(QUrl("dcraw.html"));
	help->exec();
}

void PreferenceDialog::enterWhatsThis() {
	QWhatsThis::enterWhatsThisMode();
}
