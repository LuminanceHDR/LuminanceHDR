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
 *
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#include <QColorDialog>
#include <QFileDialog>
#include <QWhatsThis>
#include <QMessageBox>
#include <cmath>

#include <iostream>

#include "Common/config.h"
#include "PreferencesDialog.h"


static double pos2value(int pos, int minpos, int maxpos, double minv, double maxv) {
	double x = (pos - minpos)/( (double) (maxpos - minpos));
	return minv*exp(log(maxv/minv)*x );
}

static int value2pos(double value, int minpos, int maxpos, double minv, double maxv) {
	double y = (log(value)-log(minv))/(log(maxv)-log(minv));
	return (int) ((maxpos - minpos)*y) + minpos;
}

PreferencesDialog::PreferencesDialog(QWidget *p) : QDialog(p) {
	setupUi(this);

	fromIso639ToGuiIndex["cs"]=0;
	fromIso639ToGuiIndex["en"]=1;
	fromIso639ToGuiIndex["fi"]=2;
	fromIso639ToGuiIndex["fr"]=3;
	fromIso639ToGuiIndex["de"]=4;
	fromIso639ToGuiIndex["id"]=5;
	fromIso639ToGuiIndex["it"]=6;
	fromIso639ToGuiIndex["pl"]=7;
	fromIso639ToGuiIndex["ro"]=8;
	fromIso639ToGuiIndex["ru"]=9;
	fromIso639ToGuiIndex["es"]=10;
	fromIso639ToGuiIndex["tr"]=11;
	fromIso639ToGuiIndex["hu"]=12;

	fromGuiIndexToIso639[0]="cs";
	fromGuiIndexToIso639[1]="en";
	fromGuiIndexToIso639[2]="fi";
	fromGuiIndexToIso639[3]="fr";
	fromGuiIndexToIso639[4]="de";
	fromGuiIndexToIso639[5]="id";
	fromGuiIndexToIso639[6]="it";
	fromGuiIndexToIso639[7]="pl";
	fromGuiIndexToIso639[8]="ro";
	fromGuiIndexToIso639[9]="ru";
	fromGuiIndexToIso639[10]="es";
	fromGuiIndexToIso639[11]="tr";
	fromGuiIndexToIso639[12]="hu";

        negcolor = luminance_options.getViewerNegColor();
        infnancolor = luminance_options.getViewerNanInfColor();

	from_options_to_gui(); //update the gui in order to show the options

	framePreferences->setStyleSheet("QFrame { background-color: lightgray; }");

	connect(negativeColorButton,SIGNAL(clicked()),this,SLOT(negative_clicked()));
	connect(ifnanColorButton,SIGNAL(clicked()),this,SLOT(infnan_clicked()));
	connect(okButton,SIGNAL(clicked()),this,SLOT(ok_clicked()));
	connect(cancelButton,SIGNAL(clicked()),this,SLOT(cancel_clicked()));
	connect(chooseCachePathButton,SIGNAL(clicked()),this,SLOT(updateLineEditString()));

	connect(user_qual_comboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(user_qual_comboBox_currentIndexChanged(int)));
	connect(med_passes_spinBox,SIGNAL(valueChanged(int)),this,SLOT(med_passes_spinBox_valueChanged(int)));
	connect(wb_method_comboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(wb_method_comboBox_currentIndexChanged(int)));
	connect(TK_spinBox,SIGNAL(valueChanged(int)),this,SLOT(TK_spinBox_valueChanged(int)));
	connect(highlights_comboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(highlights_comboBox_currentIndexChanged(int)));
	connect(level_spinBox,SIGNAL(valueChanged(int)),this,SLOT(level_spinBox_valueChanged(int)));
	connect(user_black_spinBox,SIGNAL(valueChanged(int)),this,SLOT(user_black_spinBox_valueChanged(int)));
	connect(user_sat_spinBox,SIGNAL(valueChanged(int)),this,SLOT(user_sat_spinBox_valueChanged(int)));
	connect(threshold_spinBox,SIGNAL(valueChanged(int)),this,SLOT(threshold_spinBox_valueChanged(int)));
	connect(use_black_CB,SIGNAL(stateChanged(int)),this,SLOT(use_black_CB_stateChanged(int)));
	connect(use_sat_CB,SIGNAL(stateChanged(int)),this,SLOT(use_sat_CB_stateChanged(int)));
	connect(use_noise_CB,SIGNAL(stateChanged(int)),this,SLOT(use_noise_CB_stateChanged(int)));
	connect(use_chroma_CB,SIGNAL(stateChanged(int)),this,SLOT(use_chroma_CB_stateChanged(int)));
	connect(brightness_horizontalSlider,SIGNAL(valueChanged(int)),this,SLOT(brightness_horizontalSlider_valueChanged(int)));
	connect(brightness_doubleSpinBox,SIGNAL(valueChanged(double)),this,SLOT(brightness_doubleSpinBox_valueChanged(double)));
	connect(red_horizontalSlider,SIGNAL(valueChanged(int)),this,SLOT(red_horizontalSlider_valueChanged(int)));
	connect(red_doubleSpinBox,SIGNAL(valueChanged(double)),this,SLOT(red_doubleSpinBox_valueChanged(double)));
	connect(blue_horizontalSlider,SIGNAL(valueChanged(int)),this,SLOT(blue_horizontalSlider_valueChanged(int)));
	connect(blue_doubleSpinBox,SIGNAL(valueChanged(double)),this,SLOT(blue_doubleSpinBox_valueChanged(double)));
	connect(green_horizontalSlider,SIGNAL(valueChanged(int)),this,SLOT(green_horizontalSlider_valueChanged(int)));
	connect(green_doubleSpinBox,SIGNAL(valueChanged(double)),this,SLOT(green_doubleSpinBox_valueChanged(double)));
	
	connect(user_qual_toolButton,SIGNAL(clicked()),this,SLOT(user_qual_toolButton_clicked()));
	connect(med_passes_toolButton,SIGNAL(clicked()),this,SLOT(med_passes_toolButton_clicked()));
	connect(wb_method_toolButton,SIGNAL(clicked()),this,SLOT(wb_method_toolButton_clicked()));
	connect(TK_toolButton,SIGNAL(clicked()),this,SLOT(TK_toolButton_clicked()));
	connect(highlights_toolButton,SIGNAL(clicked()),this,SLOT(highlights_toolButton_clicked()));
	connect(level_toolButton,SIGNAL(clicked()),this,SLOT(level_toolButton_clicked()));
	connect(brightness_toolButton,SIGNAL(clicked()),this,SLOT(brightness_toolButton_clicked()));
	connect(user_black_toolButton,SIGNAL(clicked()),this,SLOT(user_black_toolButton_clicked()));
	connect(user_sat_toolButton,SIGNAL(clicked()),this,SLOT(user_sat_toolButton_clicked()));
	connect(threshold_toolButton,SIGNAL(clicked()),this,SLOT(threshold_toolButton_clicked()));
	connect(red_toolButton,SIGNAL(clicked()),this,SLOT(red_toolButton_clicked()));
	connect(blue_toolButton,SIGNAL(clicked()),this,SLOT(blue_toolButton_clicked()));
	connect(green_toolButton,SIGNAL(clicked()),this,SLOT(green_toolButton_clicked()));


	connect(toolButtonInterface,SIGNAL(clicked()),this,SLOT(toolButtonInterface_clicked()));
	connect(toolButtonHDR,SIGNAL(clicked()),this,SLOT(toolButtonHDR_clicked()));
	connect(toolButtonTM,SIGNAL(clicked()),this,SLOT(toolButtonTM_clicked()));
	connect(toolButtonRAW,SIGNAL(clicked()),this,SLOT(toolButtonRAW_clicked()));
	connect(toolButtonExtTool,SIGNAL(clicked()),this,SLOT(toolButtonExtTool_clicked()));

/**	connect(whatsThisButton,SIGNAL(clicked()),this,SLOT(enterWhatsThis())); 
        Qt::ToolButtonStyle style = (Qt::ToolButtonStyle)luminance_options.value(KEY_TOOLBAR_MODE,Qt::ToolButtonTextUnderIcon).toInt();
*	whatsThisButton->setToolButtonStyle(style); */

	// Hide unused check boxes
	checkBoxTMOWindowsMax->hide();
	checkBoxTMOWindowsHDR->hide();
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
	} 
	//check if we have '-v'
	temp_ais_options=aisParamsLineEdit->text().split(" ",QString::SkipEmptyParts);
	idx_a=temp_ais_options.indexOf("-v");
	if (idx_a==-1) {
// 		qDebug("missing, adding");
		temp_ais_options+="-v";
	}
	else
	{
		align_opt_was_ok=true;
	}
	if (!align_opt_was_ok) {
		QMessageBox::information(this,tr("Option -v -a..."),tr("LuminanceHDR requires align_image_stack to be executed with the \"-v -a aligned_\" options. Command line options have been corrected."));
	}
	return temp_ais_options;
}

void PreferencesDialog::ok_clicked() {
        if (luminance_options.getGuiLang()!=fromGuiIndexToIso639[languageComboBox->currentIndex()]) {
                QMessageBox::information(this,
                                         tr("Please restart..."),
                                         tr("Please restart LuminanceHDR to use the new language (%1).").arg(languageComboBox->currentText()));
        }
        luminance_options.setGuiLang( fromGuiIndexToIso639[languageComboBox->currentIndex()] );

//        luminance_options.beginGroup(GROUP_EXTERNALTOOLS);
        luminance_options.setAlignImageStackOptions( sanitizeAISparams() );
//        luminance_options.endGroup();

//        luminance_options.beginGroup(GROUP_HDRVISUALIZATION);
//        if(negcolor.rgba() != luminance_options.negcolor) {
//            luminance_options.negcolor=negcolor.rgba();
//            luminance_options.setValue(KEY_NEGCOLOR,negcolor.rgba());
//        }
        luminance_options.setViewerNegColor(negcolor.rgba());
//        if(infnancolor.rgba() != luminance_options.naninfcolor) {
//            luminance_options.naninfcolor=infnancolor.rgba();
//            luminance_options.setValue(KEY_NANINFCOLOR,infnancolor.rgba());
//        }
        luminance_options.setViewerNanInfColor(infnancolor.rgba());
//        luminance_options.endGroup();

        //luminance_options.beginGroup(GROUP_TONEMAPPING);
//                if (lineEditTempPath->text() != luminance_options.tempfilespath) {
//                        luminance_options.tempfilespath=lineEditTempPath->text();
//                        luminance_options.setValue(KEY_TEMP_RESULT_PATH,lineEditTempPath->text());
//		}
        luminance_options.setTempDir( lineEditTempPath->text() );
//                if (batchLdrFormatComboBox->currentText() != luminance_options.batch_ldr_format) {
//                        luminance_options.batch_ldr_format=batchLdrFormatComboBox->currentText();
//                        luminance_options.setValue(KEY_BATCH_LDR_FORMAT,batchLdrFormatComboBox->currentText());
//		}
                luminance_options.setBatchTmLdrFormat( batchLdrFormatComboBox->currentText() );

//                if (numThreadspinBox->value() != luminance_options.num_threads) {
//                        luminance_options.num_threads=numThreadspinBox->value();
//                        luminance_options.setValue(KEY_NUM_BATCH_THREADS,numThreadspinBox->value());
//		}
                luminance_options.setBatchTmNumThreads( numThreadspinBox->value() );
        //luminance_options.endGroup();

//        luminance_options.beginGroup(GROUP_TIFF);
//                if (logLuvRadioButton->isChecked() != luminance_options.saveLogLuvTiff) {
//                        luminance_options.saveLogLuvTiff=logLuvRadioButton->isChecked();
//                        luminance_options.setValue(KEY_SAVE_LOGLUV,logLuvRadioButton->isChecked());
//		}
//        luminance_options.endGroup();
        luminance_options.setSaveLogLuvTiff( logLuvRadioButton->isChecked() );

//        luminance_options.beginGroup(GROUP_TMOWINDOW);
                //if (previewsWidthSpinBox->value() != luminance_options.previews_width) {
                        //luminance_options.previews_width = previewsWidthSpinBox->value();
                        luminance_options.setValue(KEY_TMOWINDOW_PREVIEWS_WIDTH, previewsWidthSpinBox->value());
                //}
//                if (checkBoxTMOWindowsMax->isChecked() != luminance_options.tmowindow_max) {
//                        luminance_options.tmowindow_max=checkBoxTMOWindowsMax->isChecked();
//                        luminance_options.setValue(KEY_TMOWINDOW_MAX,checkBoxTMOWindowsMax->isChecked());
//		}
//                if (checkBoxTMOWindowsHDR->isChecked() != luminance_options.tmowindow_showprocessed) {
//                        luminance_options.tmowindow_showprocessed = checkBoxTMOWindowsHDR->isChecked();
//                        luminance_options.setValue(KEY_TMOWINDOW_SHOWPROCESSED,checkBoxTMOWindowsHDR->isChecked());
//		}
//                if (checkBoxTMOWindowsPreviewPanel->isChecked() != luminance_options.tmowindow_showpreviewpanel) {
//                        luminance_options.tmowindow_showpreviewpanel = checkBoxTMOWindowsPreviewPanel->isChecked();
//                        luminance_options.setValue(KEY_TMOWINDOW_SHOWPREVIEWPANEL,checkBoxTMOWindowsPreviewPanel->isChecked());
//		}
//        luminance_options.endGroup();

        //luminance_options.beginGroup(GROUP_HDR_WIZARD);
        //        if (checkBoxWizardShowFirstPage->isChecked() != luminance_options.wizard_show_firstpage) {
        //                luminance_options.wizard_show_firstpage = checkBoxWizardShowFirstPage->isChecked();
        //                luminance_options.setValue(KEY_WIZARD_SHOWFIRSTPAGE,checkBoxWizardShowFirstPage->isChecked());
        //	}
        luminance_options.setShowFirstPageWizard( checkBoxWizardShowFirstPage->isChecked() );
        //luminance_options.endGroup();

        //luminance_options.beginGroup(GROUP_RAW_CONVERSION_OPTIONS);
		
        //luminance_options.four_color_rgb = four_color_rgb_CB->isChecked();
        //luminance_options.setValue(KEY_FOUR_COLOR_RGB, four_color_rgb_CB->isChecked());
        luminance_options.setRawFourColorRGB( four_color_rgb_CB->isChecked() );

        //luminance_options.do_not_use_fuji_rotate = do_not_use_fuji_rotate_CB->isChecked();
        //luminance_options.setValue(KEY_DO_NOT_USE_FUJI_ROTATE, do_not_use_fuji_rotate_CB->isChecked());
        luminance_options.setRawDoNotUseFujiRotate( do_not_use_fuji_rotate_CB->isChecked() );

        //luminance_options.user_qual = user_qual_comboBox->currentIndex();
        //luminance_options.setValue(KEY_USER_QUAL, user_qual_comboBox->currentIndex());
        luminance_options.setRawUserQuality( user_qual_comboBox->currentIndex() );

        //luminance_options.med_passes = med_passes_spinBox->value();
        //luminance_options.setValue(KEY_MED_PASSES, med_passes_spinBox->value());
        luminance_options.setRawMedPasses( med_passes_spinBox->value() );

        //luminance_options.wb_method = wb_method_comboBox->currentIndex();
        //luminance_options.setValue(KEY_WB_METHOD, wb_method_comboBox->currentIndex());
        luminance_options.setRawWhiteBalanceMethod( wb_method_comboBox->currentIndex() );

        //luminance_options.TK = TK_spinBox->value();
        //luminance_options.setValue(KEY_TK, TK_spinBox->value());
        luminance_options.setRawTemperatureKelvin( TK_spinBox->value() );

        //luminance_options.green = green_doubleSpinBox->value();
        //luminance_options.setValue(KEY_GREEN, green_doubleSpinBox->value());
        luminance_options.setRawGreen( green_doubleSpinBox->value() );

        //luminance_options.highlights = highlights_comboBox->currentIndex();
        //luminance_options.setValue(KEY_HIGHLIGHTS, highlights_comboBox->currentIndex());
        luminance_options.setRawHighlightsMode( highlights_comboBox->currentIndex() );

        //luminance_options.level = level_spinBox->value();
        //luminance_options.setValue(KEY_LEVEL, level_spinBox->value());
        luminance_options.setRawLevel( level_spinBox->value() );

        //luminance_options.auto_bright = auto_bright_CB->isChecked();
        //luminance_options.setValue(KEY_AUTO_BRIGHT, auto_bright_CB->isChecked());
        luminance_options.setRawAutoBrightness( auto_bright_CB->isChecked() );

        //luminance_options.brightness = brightness_doubleSpinBox->value();
        //luminance_options.setValue(KEY_BRIGHTNESS, brightness_doubleSpinBox->value());
        luminance_options.setRawBrightness( brightness_doubleSpinBox->value() );

        //luminance_options.use_black = use_black_CB->isChecked();
        //luminance_options.setValue(KEY_USE_BLACK, use_black_CB->isChecked());
        luminance_options.setRawUseBlack( use_black_CB->isChecked() );

        //luminance_options.use_sat = use_sat_CB->isChecked();
        //luminance_options.setValue(KEY_USE_SAT, use_sat_CB->isChecked());
        luminance_options.setRawUseSaturation( use_sat_CB->isChecked() );

        //luminance_options.use_noise = use_noise_CB->isChecked();
        //luminance_options.setValue(KEY_USE_NOISE, use_noise_CB->isChecked());
        luminance_options.setRawUseNoiseReduction( use_noise_CB->isChecked() );

        //luminance_options.use_chroma = use_chroma_CB->isChecked();
        //luminance_options.setValue(KEY_USE_CHROMA, use_chroma_CB->isChecked());
        luminance_options.setRawUseChroma( use_chroma_CB->isChecked() );

        //luminance_options.threshold = threshold_spinBox->value();
        //luminance_options.setValue(KEY_THRESHOLD, threshold_spinBox->value());
        luminance_options.setRawNoiseReductionThreshold( threshold_spinBox->value() );

        //luminance_options.user_black = user_black_spinBox->value();
        //luminance_options.setValue(KEY_USER_BLACK, user_black_spinBox->value());
        luminance_options.setRawUserBlack( user_black_spinBox->value() );

        //luminance_options.user_sat = user_sat_spinBox->value();
        //luminance_options.setValue(KEY_USER_SAT, user_sat_spinBox->value());
        luminance_options.setRawUserSaturation( user_sat_spinBox->value() );

        //luminance_options.aber_0 = red_doubleSpinBox->value();
        //luminance_options.setValue(KEY_ABER_0, red_doubleSpinBox->value());
        luminance_options.setRawAber0( red_doubleSpinBox->value() );

        //luminance_options.aber_2 = blue_doubleSpinBox->value();
        //luminance_options.setValue(KEY_ABER_2, blue_doubleSpinBox->value());
        luminance_options.setRawAber2( blue_doubleSpinBox->value() );

        //////////////////////////////////////////////////////////////

        luminance_options.setValue(KEY_USER_QUAL_TOOLBUTTON, user_qual_toolButton->isEnabled());
        luminance_options.setValue(KEY_MED_PASSES_TOOLBUTTON, med_passes_toolButton->isEnabled());
        luminance_options.setValue(KEY_WB_METHOD_TOOLBUTTON, wb_method_toolButton->isEnabled());
        luminance_options.setValue(KEY_TK_TOOLBUTTON, TK_toolButton->isEnabled());
        luminance_options.setValue(KEY_HIGHLIGHTS_TOOLBUTTON, highlights_toolButton->isEnabled());
        luminance_options.setValue(KEY_LEVEL_TOOLBUTTON, level_toolButton->isEnabled());
        luminance_options.setValue(KEY_BRIGHTNESS_TOOLBUTTON, brightness_toolButton->isEnabled());
        luminance_options.setValue(KEY_USER_BLACK_TOOLBUTTON, user_black_toolButton->isEnabled());
        luminance_options.setValue(KEY_USER_SAT_TOOLBUTTON, user_sat_toolButton->isEnabled());
        luminance_options.setValue(KEY_THRESHOLD_TOOLBUTTON, threshold_toolButton->isEnabled());
        luminance_options.setValue(KEY_RED_TOOLBUTTON, red_toolButton->isEnabled());
        luminance_options.setValue(KEY_BLUE_TOOLBUTTON, blue_toolButton->isEnabled());
        luminance_options.setValue(KEY_GREEN_TOOLBUTTON, green_toolButton->isEnabled());
		
        //luminance_options.endGroup();
	
	accept();
}

void PreferencesDialog::cancel_clicked() {
	reject();
}

void PreferencesDialog::user_qual_comboBox_currentIndexChanged(int i) {
	if (i == 0)
		user_qual_toolButton->setEnabled(false);
	else
		user_qual_toolButton->setEnabled(true);
}

void PreferencesDialog::med_passes_spinBox_valueChanged(int value) {
	if (value == 0)
		med_passes_toolButton->setEnabled(false);
	else
		med_passes_toolButton->setEnabled(true);
}

void PreferencesDialog::wb_method_comboBox_currentIndexChanged(int i) {
	if (i == 3) {	// Manual method
		TK_label->setEnabled(true);
		TK_horizontalSlider->setEnabled(true);
		TK_spinBox->setEnabled(true);
		green_label->setEnabled(true);
		green_horizontalSlider->setEnabled(true);
		green_doubleSpinBox->setEnabled(true);		
	}
	else {
		TK_label->setEnabled(false);
		TK_horizontalSlider->setEnabled(false);
		TK_spinBox->setEnabled(false);
		green_label->setEnabled(false);
		green_horizontalSlider->setEnabled(false);
		green_doubleSpinBox->setEnabled(false);		
	}
	if (i == 1)
		wb_method_toolButton->setEnabled(false);
	else
		wb_method_toolButton->setEnabled(true);
}

void PreferencesDialog::TK_spinBox_valueChanged(int value) {
	if (value == 6500)
		TK_toolButton->setEnabled(false);
	else
		TK_toolButton->setEnabled(true);
}

void PreferencesDialog::highlights_comboBox_currentIndexChanged(int i) {
	if (i == 0)
		highlights_toolButton->setEnabled(false);
	else
		highlights_toolButton->setEnabled(true);
}

void PreferencesDialog::level_spinBox_valueChanged(int value) {
	if (value == 0)
		level_toolButton->setEnabled(false);
	else
		level_toolButton->setEnabled(true);
}

void PreferencesDialog::user_black_spinBox_valueChanged(int value) {
	if (value == 0)
		user_black_toolButton->setEnabled(false);
	else
		user_black_toolButton->setEnabled(true);
}

void PreferencesDialog::user_sat_spinBox_valueChanged(int value) {
	if (value == 20000)
		user_sat_toolButton->setEnabled(false);
	else
		user_sat_toolButton->setEnabled(true);
}

void PreferencesDialog::threshold_spinBox_valueChanged(int value) {
	if (value == 100)
		threshold_toolButton->setEnabled(false);
	else
		threshold_toolButton->setEnabled(true);
}

void PreferencesDialog::use_black_CB_stateChanged(int) {
	if (use_black_CB->isChecked()) {
		user_black_horizontalSlider->setEnabled(true);
		user_black_spinBox->setEnabled(true);
	}
	else {
		user_black_horizontalSlider->setEnabled(false);
		user_black_spinBox->setEnabled(false);
	}
}

void PreferencesDialog::use_sat_CB_stateChanged(int) {
	if (use_sat_CB->isChecked()) {
		user_sat_horizontalSlider->setEnabled(true);
		user_sat_spinBox->setEnabled(true);
	}
	else {
		user_sat_horizontalSlider->setEnabled(false);
		user_sat_spinBox->setEnabled(false);
	}
}

void PreferencesDialog::use_noise_CB_stateChanged(int) {
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

void PreferencesDialog::use_chroma_CB_stateChanged(int) {
	if (use_chroma_CB->isChecked()) {
		red_label->setEnabled(true);
		red_horizontalSlider->setEnabled(true);
		red_doubleSpinBox->setEnabled(true);
		blue_label->setEnabled(true);
		blue_horizontalSlider->setEnabled(true);
		blue_doubleSpinBox->setEnabled(true);
	}
	else {
		red_label->setEnabled(false);
		red_horizontalSlider->setEnabled(false);
		red_doubleSpinBox->setEnabled(false);
		blue_label->setEnabled(false);
		blue_horizontalSlider->setEnabled(false);
		blue_doubleSpinBox->setEnabled(false);
	}
}

/***********************************************************/

void PreferencesDialog::brightness_horizontalSlider_valueChanged( int value) {
	brightness_doubleSpinBox->setValue(((double) value)/brightness_doubleSpinBox->maximum());
}

void PreferencesDialog::brightness_doubleSpinBox_valueChanged( double value) {
	brightness_horizontalSlider->setValue((int) (value*brightness_doubleSpinBox->maximum()));
	if (fabs(value - 1.0) < 1e-4)
		brightness_toolButton->setEnabled(false);
	else
		brightness_toolButton->setEnabled(true);
}

void PreferencesDialog::red_horizontalSlider_valueChanged( int pos) {
	int minpos = red_horizontalSlider->minimum();
	int maxpos = red_horizontalSlider->maximum();
	double minv = red_doubleSpinBox->minimum();
	double maxv = red_doubleSpinBox->maximum();
	double value = pos2value(pos, minpos, maxpos, minv, maxv);
	red_doubleSpinBox->setValue(value);
}

void PreferencesDialog::red_doubleSpinBox_valueChanged( double value) {
	int minpos = red_horizontalSlider->minimum();
	int maxpos = red_horizontalSlider->maximum();
	double minv = red_doubleSpinBox->minimum();
	double maxv = red_doubleSpinBox->maximum();
	int pos = value2pos(value, minpos, maxpos, minv, maxv);
	red_horizontalSlider->setValue(pos);
	if (fabs(value - 1.0) < 1e-4)
		red_toolButton->setEnabled(false);
	else
		red_toolButton->setEnabled(true);
}

void PreferencesDialog::blue_horizontalSlider_valueChanged( int pos) {
	int minpos = blue_horizontalSlider->minimum();
	int maxpos = blue_horizontalSlider->maximum();
	double minv = blue_doubleSpinBox->minimum();
	double maxv = blue_doubleSpinBox->maximum();
	double value = pos2value(pos, minpos, maxpos, minv, maxv);
	blue_doubleSpinBox->setValue(value);
}

void PreferencesDialog::blue_doubleSpinBox_valueChanged( double value) {
	int minpos = blue_horizontalSlider->minimum();
	int maxpos = blue_horizontalSlider->maximum();
	double minv = blue_doubleSpinBox->minimum();
	double maxv = blue_doubleSpinBox->maximum();
	int pos = value2pos(value, minpos, maxpos, minv, maxv);
	blue_horizontalSlider->setValue(pos);
	if (fabs(value - 1.0) < 1e-4)
		blue_toolButton->setEnabled(false);
	else
		blue_toolButton->setEnabled(true);
}

void PreferencesDialog::green_horizontalSlider_valueChanged( int pos) {
	int minpos = green_horizontalSlider->minimum();
	int maxpos = green_horizontalSlider->maximum();
	double minv = green_doubleSpinBox->minimum();
	double maxv = green_doubleSpinBox->maximum();
	double value = pos2value(pos, minpos, maxpos, minv, maxv);
	green_doubleSpinBox->setValue(value);
}

void PreferencesDialog::green_doubleSpinBox_valueChanged( double value) {
	int minpos = green_horizontalSlider->minimum();
	int maxpos = green_horizontalSlider->maximum();
	double minv = green_doubleSpinBox->minimum();
	double maxv = green_doubleSpinBox->maximum();
	int pos = value2pos(value, minpos, maxpos, minv, maxv);
	blue_horizontalSlider->setValue(pos);
	if (fabs(value - 1.0) < 1e-4)
		green_toolButton->setEnabled(false);
	else
		green_toolButton->setEnabled(true);
}

/*********************************************************/

void PreferencesDialog::user_qual_toolButton_clicked() {
	user_qual_comboBox->setCurrentIndex(0);
	user_qual_toolButton->setEnabled(false);
}

void PreferencesDialog::med_passes_toolButton_clicked() {
	med_passes_horizontalSlider->setValue(0);
	med_passes_spinBox->setValue(0);
	med_passes_toolButton->setEnabled(false);
}

void PreferencesDialog::wb_method_toolButton_clicked() {
	wb_method_comboBox->setCurrentIndex(1);
	wb_method_toolButton->setEnabled(false);
}

void PreferencesDialog::TK_toolButton_clicked() {
	TK_horizontalSlider->setValue(6500);
	TK_spinBox->setValue(6500);
	TK_toolButton->setEnabled(false);
}

void PreferencesDialog::highlights_toolButton_clicked() {
	highlights_comboBox->setCurrentIndex(0);
	highlights_toolButton->setEnabled(false);
}

void PreferencesDialog::level_toolButton_clicked() {
	level_horizontalSlider->setValue(0);
	level_spinBox->setValue(0);
	level_toolButton->setEnabled(false);
}

void PreferencesDialog::brightness_toolButton_clicked() {
	brightness_horizontalSlider->setValue(10);
	brightness_doubleSpinBox->setValue(1.0);
	brightness_toolButton->setEnabled(false);
}

void PreferencesDialog::user_black_toolButton_clicked() {
	user_black_horizontalSlider->setValue(0);
	user_black_spinBox->setValue(0);
	user_black_toolButton->setEnabled(false);
}

void PreferencesDialog::user_sat_toolButton_clicked() {
	user_sat_horizontalSlider->setValue(20000);
	user_sat_spinBox->setValue(20000);
	user_sat_toolButton->setEnabled(false);
}

void PreferencesDialog::threshold_toolButton_clicked() {
	use_noise_CB->setChecked(true);
	threshold_horizontalSlider->setValue(100);
	threshold_spinBox->setValue(100);
	threshold_toolButton->setEnabled(false);
}

void PreferencesDialog::red_toolButton_clicked() {
	int minpos = red_horizontalSlider->minimum();
	int maxpos = red_horizontalSlider->maximum();
	double minv = red_doubleSpinBox->minimum();
	double maxv = red_doubleSpinBox->maximum();
	int pos = value2pos(1.0, minpos, maxpos, minv, maxv);
	red_horizontalSlider->setValue(pos);
	red_doubleSpinBox->setValue(1.0);
	red_toolButton->setEnabled(false);
}

void PreferencesDialog::blue_toolButton_clicked() {
	int minpos = blue_horizontalSlider->minimum();
	int maxpos = blue_horizontalSlider->maximum();
	double minv = blue_doubleSpinBox->minimum();
	double maxv = blue_doubleSpinBox->maximum();
	int pos = value2pos(1.0, minpos, maxpos, minv, maxv);
	blue_horizontalSlider->setValue(pos);
	blue_doubleSpinBox->setValue(1.0);
	blue_toolButton->setEnabled(false);
}

void PreferencesDialog::green_toolButton_clicked() {
	int minpos = green_horizontalSlider->minimum();
	int maxpos = green_horizontalSlider->maximum();
	double minv = green_doubleSpinBox->minimum();
	double maxv = green_doubleSpinBox->maximum();
	int pos = value2pos(1.0, minpos, maxpos, minv, maxv);
	green_horizontalSlider->setValue(pos);
	green_doubleSpinBox->setValue(1.0);
	green_toolButton->setEnabled(false);
}

void PreferencesDialog::toolButtonInterface_clicked() {
	stackedPagesWidget->setCurrentIndex(0);
}

void PreferencesDialog::toolButtonHDR_clicked() {
	stackedPagesWidget->setCurrentIndex(1);
}

void PreferencesDialog::toolButtonTM_clicked() {
	stackedPagesWidget->setCurrentIndex(2);
}

void PreferencesDialog::toolButtonRAW_clicked() {
	stackedPagesWidget->setCurrentIndex(3);
}

void PreferencesDialog::toolButtonExtTool_clicked() {
	stackedPagesWidget->setCurrentIndex(4);
}

/********************************************************/

void PreferencesDialog::from_options_to_gui()
{
        //language: if by any chance luminance_options.gui_lang does NOT contain one of the valid 2 chars
	//codes which are key for the fromIso639ToGuiIndex QMap, provide the default "en"
        if (!fromIso639ToGuiIndex.contains(luminance_options.getGuiLang())) {
            luminance_options.setGuiLang("en");
        }
        languageComboBox->setCurrentIndex(fromIso639ToGuiIndex.value(luminance_options.getGuiLang()));

        // Temp directory
        lineEditTempPath->setText(luminance_options.getTempDir());

        // Batch TM output format
        {
            QString out_format = luminance_options.getBatchTmLdrFormat();
            if (out_format=="JPEG")
                batchLdrFormatComboBox->setCurrentIndex(0);
            else if (out_format=="PNG")
                batchLdrFormatComboBox->setCurrentIndex(1);
            else if (out_format=="PPM")
                batchLdrFormatComboBox->setCurrentIndex(2);
            else if (out_format=="PBM")
                batchLdrFormatComboBox->setCurrentIndex(3);
            else if (out_format=="BMP")
                batchLdrFormatComboBox->setCurrentIndex(4);
            else if (out_format=="TIFF")
                batchLdrFormatComboBox->setCurrentIndex(5);
        }
        numThreadspinBox->setValue(luminance_options.getNumThreads());
        aisParamsLineEdit->setText( luminance_options.getAlignImageStackOptions().join(" ") );
        logLuvRadioButton->setChecked(luminance_options.isSaveLogLuvTiff());
        floatTiffRadioButton->setChecked(!luminance_options.isSaveLogLuvTiff());

        change_color_of(negativeColorButton,&negcolor); // TO CHECK
        change_color_of(ifnanColorButton,&infnancolor); // TO CHECK

        //previewsWidthSpinBox->setValue(luminance_options.previews_width); // TO CHECK
        //checkBoxTMOWindowsMax->setChecked(luminance_options.tmowindow_max); // TO CHECK
        //checkBoxTMOWindowsHDR->setChecked(luminance_options.tmowindow_showprocessed); // TO CHECK
        checkBoxTMOWindowsPreviewPanel->setChecked(luminance_options.isPreviewPanelActive());
        checkBoxWizardShowFirstPage->setChecked(luminance_options.isShowFirstPageWizard());

        // RAW Processing
        four_color_rgb_CB->setChecked(luminance_options.isRawFourColorRGB());
        do_not_use_fuji_rotate_CB->setChecked(luminance_options.isRawDoNotUseFujiRotate());
        user_qual_comboBox->setCurrentIndex(luminance_options.getRawUserQuality());
        med_passes_horizontalSlider->setValue(luminance_options.getRawMedPasses());
        med_passes_spinBox->setValue(luminance_options.getRawMedPasses());
        wb_method_comboBox->setCurrentIndex(luminance_options.getRawWhiteBalanceMethod());
        if (luminance_options.getRawWhiteBalanceMethod() < 3) {
		//TODO
		TK_label->setEnabled(false);
		TK_horizontalSlider->setEnabled(false);
		TK_spinBox->setEnabled(false);
		green_label->setEnabled(false);
		green_horizontalSlider->setEnabled(false);
		green_doubleSpinBox->setEnabled(false);
	}
	else {
		TK_label->setEnabled(true);
		TK_horizontalSlider->setEnabled(true);
		TK_spinBox->setEnabled(true);
		green_label->setEnabled(true);
		green_horizontalSlider->setEnabled(true);
		green_doubleSpinBox->setEnabled(true);
	}
        TK_horizontalSlider->setValue(luminance_options.getRawTemperatureKelvin());
        TK_spinBox->setValue(luminance_options.getRawTemperatureKelvin());
        highlights_comboBox->setCurrentIndex(luminance_options.getRawHighlightsMode());
        level_horizontalSlider->setValue(luminance_options.getRawLevel());
        level_spinBox->setValue(luminance_options.getRawLevel());
        //false_colors_CB->setChecked(luminance_options.false_colors);
        auto_bright_CB->setChecked(luminance_options.isRawAutoBrightness());
        brightness_horizontalSlider->setValue((int) 10.0*luminance_options.getRawBrightness());
        brightness_doubleSpinBox->setValue(luminance_options.getRawBrightness());
        use_black_CB->setChecked(luminance_options.isRawUseBlack());
        if ( luminance_options.isRawUseBlack() ) {
		user_black_horizontalSlider->setEnabled(true);
		user_black_spinBox->setEnabled(true);
	}
	else {
		user_black_horizontalSlider->setEnabled(false);
		user_black_spinBox->setEnabled(false);
	}
        user_black_horizontalSlider->setValue(luminance_options.getRawUserBlack());
        user_black_spinBox->setValue(luminance_options.getRawUserBlack());
        use_sat_CB->setChecked(luminance_options.isRawUseSaturation());
        if (luminance_options.isRawUseSaturation()) {
		user_sat_horizontalSlider->setEnabled(true);
		user_sat_spinBox->setEnabled(true);
	}
	else {
		user_sat_horizontalSlider->setEnabled(false);
		user_sat_spinBox->setEnabled(false);
	}
        user_sat_horizontalSlider->setValue(luminance_options.getRawUserSaturation());
        user_sat_spinBox->setValue(luminance_options.getRawUserSaturation());
        use_noise_CB->setChecked(luminance_options.isRawUseNoiseReduction());
        if ( luminance_options.isRawUseNoiseReduction() ) {
		threshold_horizontalSlider->setEnabled(true);
		threshold_spinBox->setEnabled(true);
	}
	else {
		threshold_horizontalSlider->setEnabled(false);
		threshold_spinBox->setEnabled(false);
	}
        threshold_horizontalSlider->setValue( luminance_options.getRawNoiseReductionThreshold() );
        threshold_spinBox->setValue( luminance_options.getRawNoiseReductionThreshold() );
        use_chroma_CB->setChecked( luminance_options.isRawUseChroma() );
        if ( luminance_options.isRawUseChroma() ) {
		red_horizontalSlider->setEnabled(true);
		red_doubleSpinBox->setEnabled(true);
		blue_horizontalSlider->setEnabled(true);
		blue_doubleSpinBox->setEnabled(true);
	}
	else {
		red_horizontalSlider->setEnabled(false);
		red_doubleSpinBox->setEnabled(false);
		blue_horizontalSlider->setEnabled(false);
		blue_doubleSpinBox->setEnabled(false);
	}
	double  r_minv = red_doubleSpinBox->minimum(),
		r_maxv = red_doubleSpinBox->maximum(),
		r_minpos = red_horizontalSlider->minimum(),
		r_maxpos = red_horizontalSlider->maximum(),
		b_minv = blue_doubleSpinBox->minimum(),
		b_maxv = blue_doubleSpinBox->maximum(),
		b_minpos = blue_horizontalSlider->minimum(),
		b_maxpos = blue_horizontalSlider->maximum(),
		g_minv = green_doubleSpinBox->minimum(),
		g_maxv = green_doubleSpinBox->maximum(),
		g_minpos = green_horizontalSlider->minimum(),
		g_maxpos = green_horizontalSlider->maximum();
		
		
        red_horizontalSlider->setValue(value2pos(luminance_options.getRawAber0(), r_minpos, r_maxpos, r_minv, r_maxv));
        red_doubleSpinBox->setValue(luminance_options.getRawAber0());
        blue_horizontalSlider->setValue(value2pos(luminance_options.getRawAber2(), b_minpos, b_maxpos, b_minv, b_maxv));
        blue_doubleSpinBox->setValue(luminance_options.getRawAber2());
        green_horizontalSlider->setValue(value2pos(luminance_options.getRawGreen(), g_minpos, g_maxpos, g_minv, g_maxv));
        green_doubleSpinBox->setValue(luminance_options.getRawGreen());
	
        //luminance_options.beginGroup(GROUP_RAW_CONVERSION_OPTIONS);
        user_qual_toolButton->setEnabled( luminance_options.value(KEY_USER_QUAL_TOOLBUTTON).toBool());
        med_passes_toolButton->setEnabled( luminance_options.value(KEY_MED_PASSES_TOOLBUTTON).toBool());
        wb_method_toolButton->setEnabled( luminance_options.value(KEY_WB_METHOD_TOOLBUTTON).toBool());
        TK_toolButton->setEnabled( luminance_options.value(KEY_TK_TOOLBUTTON).toBool());
        highlights_toolButton->setEnabled( luminance_options.value(KEY_HIGHLIGHTS_TOOLBUTTON).toBool());
        level_toolButton->setEnabled( luminance_options.value(KEY_LEVEL_TOOLBUTTON).toBool());
        brightness_toolButton->setEnabled( luminance_options.value(KEY_BRIGHTNESS_TOOLBUTTON).toBool());
        user_black_toolButton->setEnabled( luminance_options.value(KEY_USER_BLACK_TOOLBUTTON).toBool());
        user_sat_toolButton->setEnabled( luminance_options.value(KEY_USER_SAT_TOOLBUTTON).toBool());
        threshold_toolButton->setEnabled( luminance_options.value(KEY_THRESHOLD_TOOLBUTTON).toBool());
        red_toolButton->setEnabled( luminance_options.value(KEY_RED_TOOLBUTTON).toBool());
        blue_toolButton->setEnabled( luminance_options.value(KEY_BLUE_TOOLBUTTON).toBool());
        green_toolButton->setEnabled( luminance_options.value(KEY_GREEN_TOOLBUTTON).toBool());
        //luminance_options.endGroup();
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

void PreferencesDialog::enterWhatsThis() {
	QWhatsThis::enterWhatsThisMode();
}

