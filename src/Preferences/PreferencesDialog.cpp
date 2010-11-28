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
 *
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#include <QColorDialog>
#include <QFileDialog>
#include <QWhatsThis>
#include <QMessageBox>

#include "Common/config.h"
#include "PreferencesDialog.h"

/* From UFRAW sourcecode
 * Convert between Temperature and RGB.
 * Base on information from http://www.brucelindbloom.com/
 * The fit for D-illuminant between 4000K and 15000K are from CIE
 * The generalization to 2000K < T < 4000K and the blackbody fits
 * are my own and should be taken with a grain of salt.
 */
static const double XYZ_to_RGB[3][3] = {
    { 3.24071,	-0.969258,  0.0556352 },
    {-1.53726,	1.87599,    -0.203996 },
    {-0.498571,	0.0415557,  1.05707 } };

void Temperature_to_RGB(double T, double RGB[3])
{
    int c;
    double xD, yD, X, Y, Z, max;
    // Fit for CIE Daylight illuminant
    if (T<= 4000) {
	xD = 0.27475e9/(T*T*T) - 0.98598e6/(T*T) + 1.17444e3/T + 0.145986;
    } else if (T<= 7000) {
	xD = -4.6070e9/(T*T*T) + 2.9678e6/(T*T) + 0.09911e3/T + 0.244063;
    } else {
	xD = -2.0064e9/(T*T*T) + 1.9018e6/(T*T) + 0.24748e3/T + 0.237040;
    }
    yD = -3*xD*xD + 2.87*xD - 0.275;

    // Fit for Blackbody using CIE standard observer function at 2 degrees
    //xD = -1.8596e9/(T*T*T) + 1.37686e6/(T*T) + 0.360496e3/T + 0.232632;
    //yD = -2.6046*xD*xD + 2.6106*xD - 0.239156;

    // Fit for Blackbody using CIE standard observer function at 10 degrees
    //xD = -1.98883e9/(T*T*T) + 1.45155e6/(T*T) + 0.364774e3/T + 0.231136;
    //yD = -2.35563*xD*xD + 2.39688*xD - 0.196035;

    X = xD/yD;
    Y = 1;
    Z = (1-xD-yD)/yD;
    max = 0;
    for (c=0; c<3; c++) {
	RGB[c] = X*XYZ_to_RGB[0][c] + Y*XYZ_to_RGB[1][c] + Z*XYZ_to_RGB[2][c];
	if (RGB[c]>max) max = RGB[c];
    }
    for (c=0; c<3; c++) RGB[c] = RGB[c]/max;
}

void RGB_to_Temperature(double RGB[3], double *T, double *Green)
{
    double Tmax, Tmin, testRGB[3];
    Tmin = 2000;
    Tmax = 15000;
    for (*T=(Tmax+Tmin)/2; Tmax-Tmin>10; *T=(Tmax+Tmin)/2) {
	Temperature_to_RGB(*T, testRGB);
	if (testRGB[2]/testRGB[0] > RGB[2]/RGB[0])
	    Tmax = *T;
	else
	    Tmin = *T;
    }
    *Green = (testRGB[1]/testRGB[0]) / (RGB[1]/RGB[0]);
}


PreferencesDialog::PreferencesDialog(QWidget *p) : QDialog(p) {
	setupUi(this);

	fromIso639ToGuiIndex["cs"]=0;
	fromIso639ToGuiIndex["en"]=1;
	fromIso639ToGuiIndex["fr"]=2;
	fromIso639ToGuiIndex["de"]=3;
	fromIso639ToGuiIndex["id"]=4;
	fromIso639ToGuiIndex["it"]=5;
	fromIso639ToGuiIndex["pl"]=6;
	fromIso639ToGuiIndex["ru"]=7;
	fromIso639ToGuiIndex["es"]=8;
	fromIso639ToGuiIndex["tr"]=9;
	fromIso639ToGuiIndex["hu"]=10;

	fromGuiIndexToIso639[0]="cs";
	fromGuiIndexToIso639[1]="en";
	fromGuiIndexToIso639[2]="fr";
	fromGuiIndexToIso639[3]="de";
	fromGuiIndexToIso639[4]="id";
	fromGuiIndexToIso639[5]="it";
	fromGuiIndexToIso639[6]="pl";
	fromGuiIndexToIso639[7]="ru";
	fromGuiIndexToIso639[8]="es";
	fromGuiIndexToIso639[9]="tr";
	fromGuiIndexToIso639[10]="hu";

	luminance_options=LuminanceOptions::getInstance();
	negcolor=luminance_options->negcolor;
	infnancolor=luminance_options->naninfcolor;

	from_options_to_gui(); //update the gui in order to show the options
	connect(negativeColorButton,SIGNAL(clicked()),this,SLOT(negative_clicked()));
	connect(ifnanColorButton,SIGNAL(clicked()),this,SLOT(infnan_clicked()));
	connect(okButton,SIGNAL(clicked()),this,SLOT(ok_clicked()));
	connect(chooseCachePathButton,SIGNAL(clicked()),this,SLOT(updateLineEditString()));
	connect(wb_method_comboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(wb_method_comboBox_currentIndexChanged(int)));
	connect(use_black_CB,SIGNAL(stateChanged(int)),this,SLOT(use_black_CB_stateChanged(int)));
	connect(use_sat_CB,SIGNAL(stateChanged(int)),this,SLOT(use_sat_CB_stateChanged(int)));
	connect(use_noise_CB,SIGNAL(stateChanged(int)),this,SLOT(use_noise_CB_stateChanged(int)));
	connect(use_chroma_CB,SIGNAL(stateChanged(int)),this,SLOT(use_chroma_CB_stateChanged(int)));
	connect(brightness_horizontalSlider,SIGNAL(valueChanged(int)),this,SLOT(brightness_horizontalSlider_valueChanged(int)));
	connect(brightness_doubleSpinBox,SIGNAL(valueChanged(double)),this,SLOT(brightness_doubleSpinBox_valueChanged(double)));
	
/**	connect(whatsThisButton,SIGNAL(clicked()),this,SLOT(enterWhatsThis())); 
	Qt::ToolButtonStyle style = (Qt::ToolButtonStyle)settings.value(KEY_TOOLBAR_MODE,Qt::ToolButtonTextUnderIcon).toInt();
*	whatsThisButton->setToolButtonStyle(style); */
}

void PreferencesDialog::negative_clicked() {
	negcolor = QColorDialog::getColor(negcolor, this);
	change_color_of(negativeColorButton,&negcolor);
}
void PreferencesDialog::infnan_clicked() {
	infnancolor = QColorDialog::getColor(infnancolor, this);
	change_color_of(ifnanColorButton,&infnancolor);
}

void PreferencesDialog::change_color_of(QPushButton *button, QColor *newcolor) {
	if (newcolor->isValid()) {
		button->setStyleSheet(QString("background: rgb(%1,%2,%3)").arg(newcolor->red()).arg(newcolor->green()).arg(newcolor->blue()));
	}
}

QStringList PreferencesDialog::sanitizeAISparams() {
	bool align_opt_was_ok=false;
	//check if we have '-a "aligned_"'
	QStringList temp_ais_options=aisParamsLineEdit->text().split(" ",QString::SkipEmptyParts);
	int idx_a=temp_ais_options.indexOf("-a");
	//if we don't have -a
	if (idx_a==-1) {
// 		qDebug("missing, adding");
		temp_ais_options+="-a";
		temp_ais_options+="aligned_";
	}
	//if we have -a at the very end (without the prefix)
	else if (idx_a==temp_ais_options.size()-1) {
		temp_ais_options+="aligned_";
// 		qDebug("-a at end, adding aligned_");
	}
	//if we have -a in the middle without the prefix
	else if ( (idx_a!=-1 && temp_ais_options.at(idx_a+1) != "aligned_") ) {
// 		qDebug("-a in the middle without the prefix after");
		if (!temp_ais_options.at(idx_a+1).startsWith("-")) {
// 			qDebug("next is bad prefix, removing");
			temp_ais_options.removeAt(idx_a+1);
		}
// 		qDebug("now adding");
		temp_ais_options.insert(idx_a+1,"aligned_");
	} else {
		align_opt_was_ok=true;
	}
	if (!align_opt_was_ok) {
		QMessageBox::information(this,tr("Option -a..."),tr("LuminanceHDR requires align_image_stack to be executed with the \"-a aligned_\" option. Command line options have been corrected."));
	}
	return temp_ais_options;
}

void PreferencesDialog::ok_clicked() {
	if (luminance_options->gui_lang!=fromGuiIndexToIso639[languageComboBox->currentIndex()])
		QMessageBox::information(this,tr("Please restart..."),tr("Please restart LuminanceHDR to use the new language (%1).").arg(languageComboBox->currentText()));
	luminance_options->gui_lang=fromGuiIndexToIso639[languageComboBox->currentIndex()];
	settings.setValue(KEY_GUI_LANG,luminance_options->gui_lang);

	settings.beginGroup(GROUP_EXTERNALTOOLS);
		settings.setValue(KEY_EXTERNAL_DCRAW_OPTIONS,luminance_options->dcraw_options);

		luminance_options->align_image_stack_options=sanitizeAISparams();
		settings.setValue(KEY_EXTERNAL_AIS_OPTIONS,luminance_options->align_image_stack_options);
	settings.endGroup();

	settings.beginGroup(GROUP_HDRVISUALIZATION);
		if(negcolor.rgba() != luminance_options->negcolor) {
			luminance_options->negcolor=negcolor.rgba();
			settings.setValue(KEY_NEGCOLOR,negcolor.rgba());
		}
		if(infnancolor.rgba() != luminance_options->naninfcolor) {
			luminance_options->naninfcolor=infnancolor.rgba();
			settings.setValue(KEY_NANINFCOLOR,infnancolor.rgba());
		}
	settings.endGroup();

	settings.beginGroup(GROUP_TONEMAPPING);
		if (lineEditTempPath->text() != luminance_options->tempfilespath) {
			luminance_options->tempfilespath=lineEditTempPath->text();
			settings.setValue(KEY_TEMP_RESULT_PATH,lineEditTempPath->text());
		}
		if (batchLdrFormatComboBox->currentText() != luminance_options->batch_ldr_format) {
			luminance_options->batch_ldr_format=batchLdrFormatComboBox->currentText();
			settings.setValue(KEY_BATCH_LDR_FORMAT,batchLdrFormatComboBox->currentText());
		}
		if (numThreadspinBox->value() != luminance_options->num_threads) {
			luminance_options->num_threads=numThreadspinBox->value();
			settings.setValue(KEY_NUM_BATCH_THREADS,numThreadspinBox->value());
		}
	settings.endGroup();

	settings.beginGroup(GROUP_TIFF);
		if (logLuvRadioButton->isChecked() != luminance_options->saveLogLuvTiff) {
			luminance_options->saveLogLuvTiff=logLuvRadioButton->isChecked();
			settings.setValue(KEY_SAVE_LOGLUV,logLuvRadioButton->isChecked());
		}
	settings.endGroup();

	settings.beginGroup(GROUP_TMOWINDOW);
		if (checkBoxTMOWindowsMax->isChecked() != luminance_options->tmowindow_max) {
			luminance_options->tmowindow_max=checkBoxTMOWindowsMax->isChecked();
			settings.setValue(KEY_TMOWINDOW_MAX,checkBoxTMOWindowsMax->isChecked());
		}

		if (checkBoxTMOWindowsHDR->isChecked() != luminance_options->tmowindow_showprocessed) {
			luminance_options->tmowindow_showprocessed = checkBoxTMOWindowsHDR->isChecked();
			settings.setValue(KEY_TMOWINDOW_SHOWPROCESSED,checkBoxTMOWindowsHDR->isChecked());
		}
	settings.endGroup();

	settings.beginGroup(GROUP_RAW_CONVERSION_OPTIONS);
		
		luminance_options->four_color_rgb = four_color_rgb_CB->isChecked();
		settings.setValue(KEY_FOUR_COLOR_RGB, four_color_rgb_CB->isChecked());

		luminance_options->use_fuji_rotate = use_fuji_rotate_CB->isChecked();
		settings.setValue(KEY_USE_FUJI_ROTATE, use_fuji_rotate_CB->isChecked());
		
		luminance_options->user_qual = user_qual_comboBox->currentIndex();
		settings.setValue(KEY_USER_QUAL, user_qual_comboBox->currentIndex());

		luminance_options->med_passes = med_passes_spinBox->value();
		settings.setValue(KEY_MED_PASSES, med_passes_spinBox->value());
		
		luminance_options->wb_method = wb_method_comboBox->currentIndex();
		settings.setValue(KEY_WB_METHOD, wb_method_comboBox->currentIndex());
	
		luminance_options->highlights = highlights_comboBox->currentIndex();
		settings.setValue(KEY_HIGHLIGHTS, highlights_comboBox->currentIndex());

		luminance_options->level = level_spinBox->value();
		settings.setValue(KEY_LEVEL, level_spinBox->value());

		luminance_options->auto_bright = auto_bright_CB->isChecked();
		settings.setValue(KEY_AUTO_BRIGHT, auto_bright_CB->isChecked());
		
		luminance_options->brightness = brightness_doubleSpinBox->value();
		settings.setValue(KEY_BRIGHTNESS, brightness_doubleSpinBox->value());

		luminance_options->use_black = use_black_CB->isChecked();
		settings.setValue(KEY_USE_BLACK, use_black_CB->isChecked());
		
		luminance_options->use_sat = use_sat_CB->isChecked();
		settings.setValue(KEY_USE_SAT, use_sat_CB->isChecked());
		
		luminance_options->use_noise = use_noise_CB->isChecked();
		settings.setValue(KEY_USE_NOISE, use_noise_CB->isChecked());
		
		luminance_options->use_chroma = use_chroma_CB->isChecked();
		settings.setValue(KEY_USE_CHROMA, use_chroma_CB->isChecked());

		luminance_options->threshold = threshold_spinBox->value();
		settings.setValue(KEY_THRESHOLD, threshold_spinBox->value());
		
		luminance_options->user_black = user_black_spinBox->value();
		settings.setValue(KEY_USER_BLACK, user_black_spinBox->value());
		
		luminance_options->user_sat = user_sat_spinBox->value();
		settings.setValue(KEY_USER_SAT, user_sat_spinBox->value());
		
		luminance_options->aber_0 = red_doubleSpinBox->value();
		settings.setValue(KEY_ABER_0, red_doubleSpinBox->value());
		
		luminance_options->aber_2 = green_doubleSpinBox->value();
		settings.setValue(KEY_ABER_2, green_doubleSpinBox->value());
		
		luminance_options->user_mul_0 = R_doubleSpinBox->value();
		settings.setValue(KEY_USER_MUL_0, R_doubleSpinBox->value());
		
		luminance_options->user_mul_1 = G_doubleSpinBox->value();
		settings.setValue(KEY_USER_MUL_1, G_doubleSpinBox->value());
		
		luminance_options->user_mul_2 = B_doubleSpinBox->value();
		settings.setValue(KEY_USER_MUL_2, B_doubleSpinBox->value());
		
		luminance_options->user_mul_3 = G2_doubleSpinBox->value();
		settings.setValue(KEY_USER_MUL_3, G2_doubleSpinBox->value());
		
	settings.endGroup();
	
	accept();
}

void PreferencesDialog::wb_method_comboBox_currentIndexChanged(int i) {
	if (i == 2) {	// Manual method
		label_4->setEnabled(true);
		R_label->setEnabled(true);
		G_label->setEnabled(true);
		B_label->setEnabled(true);
		G2_label->setEnabled(true);
		R_doubleSpinBox->setEnabled(true);
		G_doubleSpinBox->setEnabled(true);
		B_doubleSpinBox->setEnabled(true);
		G2_doubleSpinBox->setEnabled(true);
	}
	else {
		label_4->setEnabled(false);
		R_label->setEnabled(false);
		G_label->setEnabled(false);
		B_label->setEnabled(false);
		G2_label->setEnabled(false);
		R_doubleSpinBox->setEnabled(false);
		G_doubleSpinBox->setEnabled(false);
		B_doubleSpinBox->setEnabled(false);
		G2_doubleSpinBox->setEnabled(false);
	}
}

void PreferencesDialog::use_black_CB_stateChanged(int i) {
	if (use_black_CB->isChecked()) {
		user_black_horizontalSlider->setEnabled(true);
		user_black_spinBox->setEnabled(true);
	}
	else {
		user_black_horizontalSlider->setEnabled(false);
		user_black_spinBox->setEnabled(false);
	}
}

void PreferencesDialog::use_sat_CB_stateChanged(int i) {
	if (use_sat_CB->isChecked()) {
		user_sat_horizontalSlider->setEnabled(true);
		user_sat_spinBox->setEnabled(true);
	}
	else {
		user_sat_horizontalSlider->setEnabled(false);
		user_sat_spinBox->setEnabled(false);
	}
}

void PreferencesDialog::use_noise_CB_stateChanged(int i) {
	if (use_noise_CB->isChecked()) {
		threshold_label->setEnabled(true);
		threshold_horizontalSlider->setEnabled(true);
		threshold_spinBox->setEnabled(true);
	}
	else {
		threshold_label->setEnabled(false);
		threshold_horizontalSlider->setEnabled(false);
		threshold_spinBox->setEnabled(false);
	}
}

void PreferencesDialog::use_chroma_CB_stateChanged(int i) {
	if (use_chroma_CB->isChecked()) {
		red_label->setEnabled(true);
		red_horizontalSlider->setEnabled(true);
		red_doubleSpinBox->setEnabled(true);
		green_label->setEnabled(true);
		green_horizontalSlider->setEnabled(true);
		green_doubleSpinBox->setEnabled(true);
	}
	else {
		red_label->setEnabled(false);
		red_horizontalSlider->setEnabled(false);
		red_doubleSpinBox->setEnabled(false);
		green_label->setEnabled(false);
		green_horizontalSlider->setEnabled(false);
		green_doubleSpinBox->setEnabled(false);
	}
}

void PreferencesDialog::brightness_horizontalSlider_valueChanged( int value) {
	brightness_doubleSpinBox->setValue((double) value/10.0);
}

void PreferencesDialog::brightness_doubleSpinBox_valueChanged( double value) {
	brightness_horizontalSlider->setValue((int) value*10);
}

void PreferencesDialog::from_options_to_gui() {
	//language: if by any chance luminance_options->gui_lang does NOT contain one of the valid 2 chars
	//codes which are key for the fromIso639ToGuiIndex QMap, provide the default "en"
	if (!fromIso639ToGuiIndex.contains(luminance_options->gui_lang))
		luminance_options->gui_lang="en";
	languageComboBox->setCurrentIndex(fromIso639ToGuiIndex.value(luminance_options->gui_lang));
	lineEditTempPath->setText(luminance_options->tempfilespath);
	if (luminance_options->batch_ldr_format=="JPEG")
		batchLdrFormatComboBox->setCurrentIndex(0);
	else if (luminance_options->batch_ldr_format=="PNG")
		batchLdrFormatComboBox->setCurrentIndex(1);
	else if (luminance_options->batch_ldr_format=="PPM")
		batchLdrFormatComboBox->setCurrentIndex(2);
	else if (luminance_options->batch_ldr_format=="PBM")
		batchLdrFormatComboBox->setCurrentIndex(3);
	else if (luminance_options->batch_ldr_format=="BMP")
		batchLdrFormatComboBox->setCurrentIndex(4);
	numThreadspinBox->setValue(luminance_options->num_threads);
	aisParamsLineEdit->setText(luminance_options->align_image_stack_options.join(" "));
	logLuvRadioButton->setChecked(luminance_options->saveLogLuvTiff);
	floatTiffRadioButton->setChecked(!luminance_options->saveLogLuvTiff);
	change_color_of(negativeColorButton,&negcolor);
	change_color_of(ifnanColorButton,&infnancolor);
	checkBoxTMOWindowsMax->setChecked(luminance_options->tmowindow_max);
	checkBoxTMOWindowsHDR->setChecked(luminance_options->tmowindow_showprocessed);
}

PreferencesDialog::~PreferencesDialog() {
}

void PreferencesDialog::updateLineEditString() {
	QString dir=QFileDialog::getExistingDirectory(
	this,
	tr("Choose a directory"),
	QDir::currentPath(),
	QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks);
	if (!dir.isEmpty()) {
		lineEditTempPath->setText(dir);
	}
}

//TODO
void PreferencesDialog::helpDcrawParamsButtonClicked() {
	/*
	QDialog *help=new QDialog();
	help->setAttribute(Qt::WA_DeleteOnClose);
	Ui::HelpDialog ui;
	ui.setupUi(help);
	ui.tb->setSearchPaths(QStringList("/usr/share/luminance/html") << "/usr/local/share/luminance/html" << "./html");
	ui.tb->setSource(QUrl("dcraw.html"));
	help->exec();
	*/
}

void PreferencesDialog::enterWhatsThis() {
	QWhatsThis::enterWhatsThisMode();
}
