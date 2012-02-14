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

#include <iostream>
#include <cmath>

#include "Common/global.h"
#include "Common/config.h"
#include "Common/LuminanceOptions.h"
#include "Preferences/PreferencesDialog.h"

// UI
#include "ui_PreferencesDialog.h"

#define KEY_USER_QUAL_TOOLBUTTON "preference_dialog/user_qual_toolButton"
#define KEY_MED_PASSES_TOOLBUTTON "preference_dialog/med_passes_toolButton"
#define KEY_WB_METHOD_TOOLBUTTON "preference_dialog/wb_method_toolButton"
#define KEY_TK_TOOLBUTTON "preference_dialog/TK_toolButton"
#define KEY_MULTIPLIERS_TOOLBUTTON "preference_dialog/multipliers_toolButton"
#define KEY_HIGHLIGHTS_TOOLBUTTON "preference_dialog/highlights_toolButton"
#define KEY_LEVEL_TOOLBUTTON "preference_dialog/level_toolButton"
#define KEY_BRIGHTNESS_TOOLBUTTON "preference_dialog/brightness_toolButton"
#define KEY_USER_BLACK_TOOLBUTTON "preference_dialog/user_black_toolButton"
#define KEY_USER_SAT_TOOLBUTTON "preference_dialog/user_sat_toolButton"
#define KEY_THRESHOLD_TOOLBUTTON "preference_dialog/threshold_toolButton"
#define KEY_RED_TOOLBUTTON "preference_dialog/red_toolButton"
#define KEY_BLUE_TOOLBUTTON "preference_dialog/blue_toolButton"
#define KEY_GREEN_TOOLBUTTON "preference_dialog/green_toolButton"

namespace
{
inline double pos2value(int pos, int minpos, int maxpos, double minv, double maxv)
{
    double x = (pos - minpos)/( (double) (maxpos - minpos));
    return minv*exp(log(maxv/minv)*x );
}

inline int value2pos(double value, int minpos, int maxpos, double minv, double maxv)
{
    double y = (log(value)-log(minv))/(log(maxv)-log(minv));
    return (int) ((maxpos - minpos)*y) + minpos;
}

inline void change_color_of(QPushButton& button, const QColor& newcolor)
{
    if (newcolor.isValid())
    {
        button.setStyleSheet(QString("background: rgb(%1,%2,%3)").arg(newcolor.red()).arg(newcolor.green()).arg(newcolor.blue()));
    }
}

QStringList sanitizeAISparams(const QString& input_parameter_string)
{
    bool align_opt_was_ok=false;
    //check if we have '-a "aligned_"'
    QStringList temp_ais_options = input_parameter_string.split(" ",QString::SkipEmptyParts);
    int idx_a = temp_ais_options.indexOf("-a");
    //if we don't have -a
    if (idx_a == -1) {
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
    else
        align_opt_was_ok=true;

    //check if we have '-v'
    if (temp_ais_options.indexOf("-v") < 0) {
// 		qDebug("missing, adding");
        temp_ais_options.insert(0, "-v");
        align_opt_was_ok = false;
    }

    if (!align_opt_was_ok) {
        QMessageBox::information(0,
			QObject::tr("Option -v -a..."),
			QObject::tr("LuminanceHDR requires align_image_stack to be executed with the \"-v -a aligned_\" options. Command line options have been corrected."));
    }
    return temp_ais_options;
}

int mappingBatchTmQStringToInt(const QString& string)
{
    if (string == "JPEG")
    {
        return 0;
    }
    else if (string=="PNG")
    {
        return 1;
    }
    else if (string=="PPM")
    {
        return 2;
    }
    else if (string=="PBM")
    {
        return 3;
    }
    else if (string=="BMP")
    {
        return 4;
    }
    else if (string=="TIFF")
    {
        return 5;
    }
}


}

PreferencesDialog::PreferencesDialog(QWidget *p):
    QDialog(p),
    m_Ui(new Ui::PreferencesDialog)
{
    m_Ui->setupUi(this);

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

    negcolor = LuminanceOptions().getViewerNegColor();
    infnancolor = LuminanceOptions().getViewerNanInfColor();

    from_options_to_gui(); //update the gui in order to show the options

    connect(m_Ui->negativeColorButton,SIGNAL(clicked()),this,SLOT(negative_clicked()));
    connect(m_Ui->ifnanColorButton,SIGNAL(clicked()),this,SLOT(infnan_clicked()));
    connect(m_Ui->okButton,SIGNAL(clicked()),this,SLOT(ok_clicked()));
    connect(m_Ui->cancelButton,SIGNAL(clicked()),this,SLOT(cancel_clicked()));
    connect(m_Ui->chooseCachePathButton,SIGNAL(clicked()),this,SLOT(updateLineEditString()));

    connect(m_Ui->batchLdrFormatComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(batchTmFormatSelector(int)));

    connect(m_Ui->user_qual_comboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(user_qual_comboBox_currentIndexChanged(int)));
    connect(m_Ui->med_passes_spinBox,SIGNAL(valueChanged(int)),this,SLOT(med_passes_spinBox_valueChanged(int)));
    connect(m_Ui->wb_method_comboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(wb_method_comboBox_currentIndexChanged(int)));
    connect(m_Ui->TK_spinBox,SIGNAL(valueChanged(int)),this,SLOT(TK_spinBox_valueChanged(int)));
    connect(m_Ui->highlights_comboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(highlights_comboBox_currentIndexChanged(int)));
    connect(m_Ui->level_spinBox,SIGNAL(valueChanged(int)),this,SLOT(level_spinBox_valueChanged(int)));
    connect(m_Ui->user_black_spinBox,SIGNAL(valueChanged(int)),this,SLOT(user_black_spinBox_valueChanged(int)));
    connect(m_Ui->user_sat_spinBox,SIGNAL(valueChanged(int)),this,SLOT(user_sat_spinBox_valueChanged(int)));
    connect(m_Ui->threshold_spinBox,SIGNAL(valueChanged(int)),this,SLOT(threshold_spinBox_valueChanged(int)));
    connect(m_Ui->use_black_CB,SIGNAL(stateChanged(int)),this,SLOT(use_black_CB_stateChanged(int)));
    connect(m_Ui->use_sat_CB,SIGNAL(stateChanged(int)),this,SLOT(use_sat_CB_stateChanged(int)));
    connect(m_Ui->use_noise_CB,SIGNAL(stateChanged(int)),this,SLOT(use_noise_CB_stateChanged(int)));
    connect(m_Ui->use_chroma_CB,SIGNAL(stateChanged(int)),this,SLOT(use_chroma_CB_stateChanged(int)));
    connect(m_Ui->brightness_horizontalSlider,SIGNAL(valueChanged(int)),this,SLOT(brightness_horizontalSlider_valueChanged(int)));
    connect(m_Ui->brightness_doubleSpinBox,SIGNAL(valueChanged(double)),this,SLOT(brightness_doubleSpinBox_valueChanged(double)));
    connect(m_Ui->red_horizontalSlider,SIGNAL(valueChanged(int)),this,SLOT(red_horizontalSlider_valueChanged(int)));
    connect(m_Ui->red_doubleSpinBox,SIGNAL(valueChanged(double)),this,SLOT(red_doubleSpinBox_valueChanged(double)));
    connect(m_Ui->blue_horizontalSlider,SIGNAL(valueChanged(int)),this,SLOT(blue_horizontalSlider_valueChanged(int)));
    connect(m_Ui->blue_doubleSpinBox,SIGNAL(valueChanged(double)),this,SLOT(blue_doubleSpinBox_valueChanged(double)));
    connect(m_Ui->green_horizontalSlider,SIGNAL(valueChanged(int)),this,SLOT(green_horizontalSlider_valueChanged(int)));
    connect(m_Ui->green_doubleSpinBox,SIGNAL(valueChanged(double)),this,SLOT(green_doubleSpinBox_valueChanged(double)));
	
    connect(m_Ui->user_qual_toolButton,SIGNAL(clicked()),this,SLOT(user_qual_toolButton_clicked()));
    connect(m_Ui->med_passes_toolButton,SIGNAL(clicked()),this,SLOT(med_passes_toolButton_clicked()));
    connect(m_Ui->wb_method_toolButton,SIGNAL(clicked()),this,SLOT(wb_method_toolButton_clicked()));
    connect(m_Ui->TK_toolButton,SIGNAL(clicked()),this,SLOT(TK_toolButton_clicked()));
    connect(m_Ui->highlights_toolButton,SIGNAL(clicked()),this,SLOT(highlights_toolButton_clicked()));
    connect(m_Ui->level_toolButton,SIGNAL(clicked()),this,SLOT(level_toolButton_clicked()));
    connect(m_Ui->brightness_toolButton,SIGNAL(clicked()),this,SLOT(brightness_toolButton_clicked()));
    connect(m_Ui->user_black_toolButton,SIGNAL(clicked()),this,SLOT(user_black_toolButton_clicked()));
    connect(m_Ui->user_sat_toolButton,SIGNAL(clicked()),this,SLOT(user_sat_toolButton_clicked()));
    connect(m_Ui->threshold_toolButton,SIGNAL(clicked()),this,SLOT(threshold_toolButton_clicked()));
    connect(m_Ui->red_toolButton,SIGNAL(clicked()),this,SLOT(red_toolButton_clicked()));
    connect(m_Ui->blue_toolButton,SIGNAL(clicked()),this,SLOT(blue_toolButton_clicked()));
    connect(m_Ui->green_toolButton,SIGNAL(clicked()),this,SLOT(green_toolButton_clicked()));

    connect(m_Ui->toolButtonInterface,SIGNAL(clicked()),this,SLOT(toolButtonInterface_clicked()));
    connect(m_Ui->toolButtonHDR,SIGNAL(clicked()),this,SLOT(toolButtonHDR_clicked()));
    connect(m_Ui->toolButtonTM,SIGNAL(clicked()),this,SLOT(toolButtonTM_clicked()));
    connect(m_Ui->toolButtonRAW,SIGNAL(clicked()),this,SLOT(toolButtonRAW_clicked()));
    connect(m_Ui->toolButtonExtTool,SIGNAL(clicked()),this,SLOT(toolButtonExtTool_clicked()));
}

void PreferencesDialog::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange)
		 m_Ui->retranslateUi(this);
	QWidget::changeEvent(event);
}

void PreferencesDialog::negative_clicked()
{
    negcolor = QColorDialog::getColor(negcolor, this);
    change_color_of(*m_Ui->negativeColorButton, negcolor);
}

void PreferencesDialog::infnan_clicked()
{
    infnancolor = QColorDialog::getColor(infnancolor, this);
    change_color_of(*m_Ui->ifnanColorButton, infnancolor);
}

void PreferencesDialog::ok_clicked()
{
    LuminanceOptions luminance_options;

    if (luminance_options.getGuiLang() != fromGuiIndexToIso639[m_Ui->languageComboBox->currentIndex()])
    {
        luminance_options.setGuiLang( fromGuiIndexToIso639[m_Ui->languageComboBox->currentIndex()] );
        installTranslators(true);
    }

    // UI
    luminance_options.setViewerNegColor( negcolor.rgba() );
    luminance_options.setViewerNanInfColor( infnancolor.rgba() );

    luminance_options.setTempDir( m_Ui->lineEditTempPath->text() );

    luminance_options.setPreviewWidth( m_Ui->previewsWidthSpinBox->value() );
    luminance_options.setShowFirstPageWizard( m_Ui->checkBoxWizardShowFirstPage->isChecked() );

    // --- Batch TM
    luminance_options.setBatchTmLdrFormat( m_Ui->batchLdrFormatComboBox->currentText() );
    luminance_options.setBatchTmNumThreads( m_Ui->numThreadspinBox->value() );
    luminance_options.setBatchTmDefaultOutputQuality( m_Ui->batchTmOutputQualitySlider->value() );

    // --- Other Parameters
    luminance_options.setAlignImageStackOptions( sanitizeAISparams( m_Ui->aisParamsLineEdit->text() ) );
    luminance_options.setSaveLogLuvTiff( m_Ui->logLuvRadioButton->isChecked() );

    // --- RAW parameters
    luminance_options.setRawFourColorRGB( m_Ui->four_color_rgb_CB->isChecked() );
    luminance_options.setRawDoNotUseFujiRotate( m_Ui->do_not_use_fuji_rotate_CB->isChecked() );
    luminance_options.setRawUserQuality( m_Ui->user_qual_comboBox->currentIndex() );
    luminance_options.setRawMedPasses( m_Ui->med_passes_spinBox->value() );
    luminance_options.setRawWhiteBalanceMethod( m_Ui->wb_method_comboBox->currentIndex() );
    luminance_options.setRawTemperatureKelvin( m_Ui->TK_spinBox->value() );
    luminance_options.setRawGreen( m_Ui->green_doubleSpinBox->value() );
    luminance_options.setRawHighlightsMode( m_Ui->highlights_comboBox->currentIndex() );
    luminance_options.setRawLevel( m_Ui->level_spinBox->value() );
    luminance_options.setRawAutoBrightness( m_Ui->auto_bright_CB->isChecked() );
    luminance_options.setRawBrightness( m_Ui->brightness_doubleSpinBox->value() );
    luminance_options.setRawUseBlack( m_Ui->use_black_CB->isChecked() );
    luminance_options.setRawUseSaturation( m_Ui->use_sat_CB->isChecked() );
    luminance_options.setRawUseNoiseReduction( m_Ui->use_noise_CB->isChecked() );
    luminance_options.setRawUseChroma( m_Ui->use_chroma_CB->isChecked() );
    luminance_options.setRawNoiseReductionThreshold( m_Ui->threshold_spinBox->value() );
    luminance_options.setRawUserBlack( m_Ui->user_black_spinBox->value() );
    luminance_options.setRawUserSaturation( m_Ui->user_sat_spinBox->value() );
    luminance_options.setRawAber0( m_Ui->red_doubleSpinBox->value() );
    luminance_options.setRawAber2( m_Ui->blue_doubleSpinBox->value() );

    // ---- temporary... this rubbish must go away!
    luminance_options.setValue(KEY_USER_QUAL_TOOLBUTTON, m_Ui->user_qual_toolButton->isEnabled());
    luminance_options.setValue(KEY_MED_PASSES_TOOLBUTTON, m_Ui->med_passes_toolButton->isEnabled());
    luminance_options.setValue(KEY_WB_METHOD_TOOLBUTTON, m_Ui->wb_method_toolButton->isEnabled());
    luminance_options.setValue(KEY_TK_TOOLBUTTON, m_Ui->TK_toolButton->isEnabled());
    luminance_options.setValue(KEY_HIGHLIGHTS_TOOLBUTTON, m_Ui->highlights_toolButton->isEnabled());
    luminance_options.setValue(KEY_LEVEL_TOOLBUTTON, m_Ui->level_toolButton->isEnabled());
    luminance_options.setValue(KEY_BRIGHTNESS_TOOLBUTTON, m_Ui->brightness_toolButton->isEnabled());
    luminance_options.setValue(KEY_USER_BLACK_TOOLBUTTON, m_Ui->user_black_toolButton->isEnabled());
    luminance_options.setValue(KEY_USER_SAT_TOOLBUTTON, m_Ui->user_sat_toolButton->isEnabled());
    luminance_options.setValue(KEY_THRESHOLD_TOOLBUTTON, m_Ui->threshold_toolButton->isEnabled());
    luminance_options.setValue(KEY_RED_TOOLBUTTON, m_Ui->red_toolButton->isEnabled());
    luminance_options.setValue(KEY_BLUE_TOOLBUTTON, m_Ui->blue_toolButton->isEnabled());
    luminance_options.setValue(KEY_GREEN_TOOLBUTTON, m_Ui->green_toolButton->isEnabled());

    accept();
}

void PreferencesDialog::cancel_clicked()
{
	reject();
}

void PreferencesDialog::batchTmFormatSelector(int index)
{
    switch (index)
    {
    case 0: // JPEG
        m_Ui->batchTmOutputQualitySlider->setEnabled(true);
        m_Ui->batchTmOutputQualitySpinBox->setEnabled(true);
        break;
    case 1: // PNG
        m_Ui->batchTmOutputQualitySlider->setEnabled(true);
        m_Ui->batchTmOutputQualitySpinBox->setEnabled(true);
        break;
    case 2: // PPM
        m_Ui->batchTmOutputQualitySlider->setEnabled(false);
        m_Ui->batchTmOutputQualitySpinBox->setEnabled(false);
        break;
    case 3: // PBM
        m_Ui->batchTmOutputQualitySlider->setEnabled(false);
        m_Ui->batchTmOutputQualitySpinBox->setEnabled(false);
        break;
    case 4: // BMP
        m_Ui->batchTmOutputQualitySlider->setEnabled(false);
        m_Ui->batchTmOutputQualitySpinBox->setEnabled(false);
        break;
    case 5:
        m_Ui->batchTmOutputQualitySlider->setEnabled(false);
        m_Ui->batchTmOutputQualitySpinBox->setEnabled(false);
        break;
    default:
        break;
    }
}

void PreferencesDialog::user_qual_comboBox_currentIndexChanged(int i)
{
	if (i == 0)
        m_Ui->user_qual_toolButton->setEnabled(false);
	else
        m_Ui->user_qual_toolButton->setEnabled(true);
}

void PreferencesDialog::med_passes_spinBox_valueChanged(int value)
{
	if (value == 0)
        m_Ui->med_passes_toolButton->setEnabled(false);
	else
        m_Ui->med_passes_toolButton->setEnabled(true);
}

void PreferencesDialog::wb_method_comboBox_currentIndexChanged(int i)
{
    if (i == 3)	// Manual method
    {
        m_Ui->TK_label->setEnabled(true);
        m_Ui->TK_horizontalSlider->setEnabled(true);
        m_Ui->TK_spinBox->setEnabled(true);
        m_Ui->green_label->setEnabled(true);
        m_Ui->green_horizontalSlider->setEnabled(true);
        m_Ui->green_doubleSpinBox->setEnabled(true);
    }
    else {
        m_Ui->TK_label->setEnabled(false);
        m_Ui->TK_horizontalSlider->setEnabled(false);
        m_Ui->TK_spinBox->setEnabled(false);
        m_Ui->green_label->setEnabled(false);
        m_Ui->green_horizontalSlider->setEnabled(false);
        m_Ui->green_doubleSpinBox->setEnabled(false);
    }
    if (i == 1)
        m_Ui->wb_method_toolButton->setEnabled(false);
    else
        m_Ui->wb_method_toolButton->setEnabled(true);
}

void PreferencesDialog::TK_spinBox_valueChanged(int value)
{
    if (value == 6500)
        m_Ui->TK_toolButton->setEnabled(false);
    else
        m_Ui->TK_toolButton->setEnabled(true);
}

void PreferencesDialog::highlights_comboBox_currentIndexChanged(int i)
{
	if (i == 0)
        m_Ui->highlights_toolButton->setEnabled(false);
	else
        m_Ui->highlights_toolButton->setEnabled(true);
}

void PreferencesDialog::level_spinBox_valueChanged(int value)
{
	if (value == 0)
        m_Ui->level_toolButton->setEnabled(false);
	else
        m_Ui->level_toolButton->setEnabled(true);
}

void PreferencesDialog::user_black_spinBox_valueChanged(int value)
{
	if (value == 0)
        m_Ui->user_black_toolButton->setEnabled(false);
	else
        m_Ui->user_black_toolButton->setEnabled(true);
}

void PreferencesDialog::user_sat_spinBox_valueChanged(int value)
{
	if (value == 20000)
        m_Ui->user_sat_toolButton->setEnabled(false);
	else
        m_Ui->user_sat_toolButton->setEnabled(true);
}

void PreferencesDialog::threshold_spinBox_valueChanged(int value)
{
	if (value == 100)
        m_Ui->threshold_toolButton->setEnabled(false);
	else
        m_Ui->threshold_toolButton->setEnabled(true);
}

void PreferencesDialog::use_black_CB_stateChanged(int)
{
    if (m_Ui->use_black_CB->isChecked())
    {
        m_Ui->user_black_horizontalSlider->setEnabled(true);
        m_Ui->user_black_spinBox->setEnabled(true);
	}
	else {
        m_Ui->user_black_horizontalSlider->setEnabled(false);
        m_Ui->user_black_spinBox->setEnabled(false);
	}
}

void PreferencesDialog::use_sat_CB_stateChanged(int) {
    if (m_Ui->use_sat_CB->isChecked()) {
        m_Ui->user_sat_horizontalSlider->setEnabled(true);
        m_Ui->user_sat_spinBox->setEnabled(true);
	}
	else {
        m_Ui->user_sat_horizontalSlider->setEnabled(false);
        m_Ui->user_sat_spinBox->setEnabled(false);
	}
}

void PreferencesDialog::use_noise_CB_stateChanged(int)
{
    if (m_Ui->use_noise_CB->isChecked()) {
        m_Ui->threshold_label->setEnabled(true);
        m_Ui->threshold_horizontalSlider->setEnabled(true);
        m_Ui->threshold_spinBox->setEnabled(true);
	}
	else {
        m_Ui->threshold_label->setEnabled(false);
        m_Ui->threshold_horizontalSlider->setEnabled(false);
        m_Ui->threshold_spinBox->setEnabled(false);
	}
}

void PreferencesDialog::use_chroma_CB_stateChanged(int)
{
    if (m_Ui->use_chroma_CB->isChecked())
    {
        m_Ui->red_label->setEnabled(true);
        m_Ui->red_horizontalSlider->setEnabled(true);
        m_Ui->red_doubleSpinBox->setEnabled(true);
        m_Ui->blue_label->setEnabled(true);
        m_Ui->blue_horizontalSlider->setEnabled(true);
        m_Ui->blue_doubleSpinBox->setEnabled(true);
	}
	else {
        m_Ui->red_label->setEnabled(false);
        m_Ui->red_horizontalSlider->setEnabled(false);
        m_Ui->red_doubleSpinBox->setEnabled(false);
        m_Ui->blue_label->setEnabled(false);
        m_Ui->blue_horizontalSlider->setEnabled(false);
        m_Ui->blue_doubleSpinBox->setEnabled(false);
	}
}

void PreferencesDialog::brightness_horizontalSlider_valueChanged(int value)
{
    m_Ui->brightness_doubleSpinBox->setValue(((double) value)/m_Ui->brightness_doubleSpinBox->maximum());
}

void PreferencesDialog::brightness_doubleSpinBox_valueChanged(double value)
{
    m_Ui->brightness_horizontalSlider->setValue((int) (value*m_Ui->brightness_doubleSpinBox->maximum()));
	if (fabs(value - 1.0) < 1e-4)
        m_Ui->brightness_toolButton->setEnabled(false);
	else
        m_Ui->brightness_toolButton->setEnabled(true);
}

void PreferencesDialog::red_horizontalSlider_valueChanged(int pos)
{
    int minpos = m_Ui->red_horizontalSlider->minimum();
    int maxpos = m_Ui->red_horizontalSlider->maximum();
    double minv = m_Ui->red_doubleSpinBox->minimum();
    double maxv = m_Ui->red_doubleSpinBox->maximum();

    m_Ui->red_doubleSpinBox->setValue( pos2value(pos, minpos, maxpos, minv, maxv) );
}

void PreferencesDialog::red_doubleSpinBox_valueChanged(double value)
{
    int minpos = m_Ui->red_horizontalSlider->minimum();
    int maxpos = m_Ui->red_horizontalSlider->maximum();
    double minv = m_Ui->red_doubleSpinBox->minimum();
    double maxv = m_Ui->red_doubleSpinBox->maximum();

    m_Ui->red_horizontalSlider->setValue( value2pos(value, minpos, maxpos, minv, maxv) );
	if (fabs(value - 1.0) < 1e-4)
        m_Ui->red_toolButton->setEnabled(false);
	else
        m_Ui->red_toolButton->setEnabled(true);
}

void PreferencesDialog::blue_horizontalSlider_valueChanged(int pos)
{
    int minpos = m_Ui->blue_horizontalSlider->minimum();
    int maxpos = m_Ui->blue_horizontalSlider->maximum();
    double minv = m_Ui->blue_doubleSpinBox->minimum();
    double maxv = m_Ui->blue_doubleSpinBox->maximum();

    m_Ui->blue_doubleSpinBox->setValue( pos2value(pos, minpos, maxpos, minv, maxv) );
}

void PreferencesDialog::blue_doubleSpinBox_valueChanged(double value)
{
    int minpos = m_Ui->blue_horizontalSlider->minimum();
    int maxpos = m_Ui->blue_horizontalSlider->maximum();
    double minv = m_Ui->blue_doubleSpinBox->minimum();
    double maxv = m_Ui->blue_doubleSpinBox->maximum();

    m_Ui->blue_horizontalSlider->setValue( value2pos(value, minpos, maxpos, minv, maxv) );
	if (fabs(value - 1.0) < 1e-4)
        m_Ui->blue_toolButton->setEnabled(false);
	else
        m_Ui->blue_toolButton->setEnabled(true);
}

void PreferencesDialog::green_horizontalSlider_valueChanged( int pos)
{
    int minpos = m_Ui->green_horizontalSlider->minimum();
    int maxpos = m_Ui->green_horizontalSlider->maximum();
    double minv = m_Ui->green_doubleSpinBox->minimum();
    double maxv = m_Ui->green_doubleSpinBox->maximum();

    m_Ui->green_doubleSpinBox->setValue( pos2value(pos, minpos, maxpos, minv, maxv) );
}

void PreferencesDialog::green_doubleSpinBox_valueChanged( double value)
{
    int minpos = m_Ui->green_horizontalSlider->minimum();
    int maxpos = m_Ui->green_horizontalSlider->maximum();
    double minv = m_Ui->green_doubleSpinBox->minimum();
    double maxv = m_Ui->green_doubleSpinBox->maximum();

    m_Ui->blue_horizontalSlider->setValue( value2pos(value, minpos, maxpos, minv, maxv) );
	if (fabs(value - 1.0) < 1e-4)
        m_Ui->green_toolButton->setEnabled(false);
	else
        m_Ui->green_toolButton->setEnabled(true);
}

void PreferencesDialog::user_qual_toolButton_clicked()
{
    m_Ui->user_qual_comboBox->setCurrentIndex(0);
    m_Ui->user_qual_toolButton->setEnabled(false);
}

void PreferencesDialog::med_passes_toolButton_clicked()
{
    m_Ui->med_passes_horizontalSlider->setValue(0);
    m_Ui->med_passes_spinBox->setValue(0);
    m_Ui->med_passes_toolButton->setEnabled(false);
}

void PreferencesDialog::wb_method_toolButton_clicked()
{
    m_Ui->wb_method_comboBox->setCurrentIndex(1);
    m_Ui->wb_method_toolButton->setEnabled(false);
}

void PreferencesDialog::TK_toolButton_clicked()
{
    m_Ui->TK_horizontalSlider->setValue(6500);
    m_Ui->TK_spinBox->setValue(6500);
    m_Ui->TK_toolButton->setEnabled(false);
}

void PreferencesDialog::highlights_toolButton_clicked()
{
    m_Ui->highlights_comboBox->setCurrentIndex(0);
    m_Ui->highlights_toolButton->setEnabled(false);
}

void PreferencesDialog::level_toolButton_clicked()
{
    m_Ui->level_horizontalSlider->setValue(0);
    m_Ui->level_spinBox->setValue(0);
    m_Ui->level_toolButton->setEnabled(false);
}

void PreferencesDialog::brightness_toolButton_clicked()
{
    m_Ui->brightness_horizontalSlider->setValue(10);
    m_Ui->brightness_doubleSpinBox->setValue(1.0);
    m_Ui->brightness_toolButton->setEnabled(false);
}

void PreferencesDialog::user_black_toolButton_clicked()
{
    m_Ui->user_black_horizontalSlider->setValue(0);
    m_Ui->user_black_spinBox->setValue(0);
    m_Ui->user_black_toolButton->setEnabled(false);
}

void PreferencesDialog::user_sat_toolButton_clicked()
{
    m_Ui->user_sat_horizontalSlider->setValue(20000);
    m_Ui->user_sat_spinBox->setValue(20000);
    m_Ui->user_sat_toolButton->setEnabled(false);
}

void PreferencesDialog::threshold_toolButton_clicked()
{
    m_Ui->use_noise_CB->setChecked(true);
    m_Ui->threshold_horizontalSlider->setValue(100);
    m_Ui->threshold_spinBox->setValue(100);
    m_Ui->threshold_toolButton->setEnabled(false);
}

void PreferencesDialog::red_toolButton_clicked()
{
    int minpos = m_Ui->red_horizontalSlider->minimum();
    int maxpos = m_Ui->red_horizontalSlider->maximum();
    double minv = m_Ui->red_doubleSpinBox->minimum();
    double maxv = m_Ui->red_doubleSpinBox->maximum();

    m_Ui->red_horizontalSlider->setValue( value2pos(1.0, minpos, maxpos, minv, maxv) );
    m_Ui->red_doubleSpinBox->setValue(1.0);
    m_Ui->red_toolButton->setEnabled(false);
}

void PreferencesDialog::blue_toolButton_clicked()
{
    int minpos = m_Ui->blue_horizontalSlider->minimum();
    int maxpos = m_Ui->blue_horizontalSlider->maximum();
    double minv = m_Ui->blue_doubleSpinBox->minimum();
    double maxv = m_Ui->blue_doubleSpinBox->maximum();

    m_Ui->blue_horizontalSlider->setValue( value2pos(1.0, minpos, maxpos, minv, maxv) );
    m_Ui->blue_doubleSpinBox->setValue(1.0);
    m_Ui->blue_toolButton->setEnabled(false);
}

void PreferencesDialog::green_toolButton_clicked()
{
    int minpos = m_Ui->green_horizontalSlider->minimum();
    int maxpos = m_Ui->green_horizontalSlider->maximum();
    double minv = m_Ui->green_doubleSpinBox->minimum();
    double maxv = m_Ui->green_doubleSpinBox->maximum();

    m_Ui->green_horizontalSlider->setValue( value2pos(1.0, minpos, maxpos, minv, maxv) );
    m_Ui->green_doubleSpinBox->setValue(1.0);
    m_Ui->green_toolButton->setEnabled(false);
}

void PreferencesDialog::toolButtonInterface_clicked()
{
    m_Ui->stackedPagesWidget->setCurrentIndex(0);
}

void PreferencesDialog::toolButtonHDR_clicked()
{
    m_Ui->stackedPagesWidget->setCurrentIndex(1);
}

void PreferencesDialog::toolButtonTM_clicked()
{
    m_Ui->stackedPagesWidget->setCurrentIndex(2);
}

void PreferencesDialog::toolButtonRAW_clicked()
{
    m_Ui->stackedPagesWidget->setCurrentIndex(3);
}

void PreferencesDialog::toolButtonExtTool_clicked()
{
    m_Ui->stackedPagesWidget->setCurrentIndex(4);
}

void PreferencesDialog::from_options_to_gui()
{
    LuminanceOptions luminance_options;

    //language: if by any chance luminance_options.gui_lang does NOT contain one of the valid 2 chars
    //codes which are key for the fromIso639ToGuiIndex QMap, provide the default "en"
    if (!fromIso639ToGuiIndex.contains(luminance_options.getGuiLang()))
    {
        luminance_options.setGuiLang("en");
    }
    m_Ui->languageComboBox->setCurrentIndex(fromIso639ToGuiIndex.value(luminance_options.getGuiLang()));

    // Temp directory
    m_Ui->lineEditTempPath->setText(luminance_options.getTempDir());

    // Batch TM output format
    int current_batch_tm_output_type = mappingBatchTmQStringToInt( luminance_options.getBatchTmLdrFormat() );
    m_Ui->batchLdrFormatComboBox->setCurrentIndex( current_batch_tm_output_type );
    batchTmFormatSelector( current_batch_tm_output_type );

    m_Ui->batchTmOutputQualitySlider->setValue( luminance_options.getBatchTmDefaultOutputQuality() );
    m_Ui->batchTmOutputQualitySpinBox->setValue( luminance_options.getBatchTmDefaultOutputQuality() );

    m_Ui->numThreadspinBox->setValue( luminance_options.getBatchTmNumThreads() );

    m_Ui->aisParamsLineEdit->setText( luminance_options.getAlignImageStackOptions().join(" ") );
    m_Ui->logLuvRadioButton->setChecked(luminance_options.isSaveLogLuvTiff());
    m_Ui->floatTiffRadioButton->setChecked(!luminance_options.isSaveLogLuvTiff());

    change_color_of(*m_Ui->negativeColorButton, negcolor);
    change_color_of(*m_Ui->ifnanColorButton, infnancolor);

    m_Ui->previewsWidthSpinBox->setValue( luminance_options.getPreviewWidth() );

    m_Ui->checkBoxTMOWindowsPreviewPanel->setChecked(luminance_options.isPreviewPanelActive());
    m_Ui->checkBoxWizardShowFirstPage->setChecked(luminance_options.isShowFirstPageWizard());

    // RAW Processing
    m_Ui->four_color_rgb_CB->setChecked(luminance_options.isRawFourColorRGB());
    m_Ui->do_not_use_fuji_rotate_CB->setChecked(luminance_options.isRawDoNotUseFujiRotate());
    m_Ui->user_qual_comboBox->setCurrentIndex(luminance_options.getRawUserQuality());
    m_Ui->med_passes_horizontalSlider->setValue(luminance_options.getRawMedPasses());
    m_Ui->med_passes_spinBox->setValue(luminance_options.getRawMedPasses());
    m_Ui->wb_method_comboBox->setCurrentIndex(luminance_options.getRawWhiteBalanceMethod());
    if (luminance_options.getRawWhiteBalanceMethod() < 3)
    {
        //TODO
        m_Ui->TK_label->setEnabled(false);
        m_Ui->TK_horizontalSlider->setEnabled(false);
        m_Ui->TK_spinBox->setEnabled(false);
        m_Ui->green_label->setEnabled(false);
        m_Ui->green_horizontalSlider->setEnabled(false);
        m_Ui->green_doubleSpinBox->setEnabled(false);
    }
    else {
        m_Ui->TK_label->setEnabled(true);
        m_Ui->TK_horizontalSlider->setEnabled(true);
        m_Ui->TK_spinBox->setEnabled(true);
        m_Ui->green_label->setEnabled(true);
        m_Ui->green_horizontalSlider->setEnabled(true);
        m_Ui->green_doubleSpinBox->setEnabled(true);
    }
    m_Ui->TK_horizontalSlider->setValue(luminance_options.getRawTemperatureKelvin());
    m_Ui->TK_spinBox->setValue(luminance_options.getRawTemperatureKelvin());
    m_Ui->highlights_comboBox->setCurrentIndex(luminance_options.getRawHighlightsMode());
    m_Ui->level_horizontalSlider->setValue(luminance_options.getRawLevel());
    m_Ui->level_spinBox->setValue(luminance_options.getRawLevel());
    //m_Ui->false_colors_CB->setChecked(luminance_options.false_colors);
    m_Ui->auto_bright_CB->setChecked(luminance_options.isRawAutoBrightness());
    m_Ui->brightness_horizontalSlider->setValue((int) 10.0*luminance_options.getRawBrightness());
    m_Ui->brightness_doubleSpinBox->setValue(luminance_options.getRawBrightness());
    m_Ui->use_black_CB->setChecked(luminance_options.isRawUseBlack());
    if ( luminance_options.isRawUseBlack() ) {
        m_Ui->user_black_horizontalSlider->setEnabled(true);
        m_Ui->user_black_spinBox->setEnabled(true);
    }
    else {
        m_Ui->user_black_horizontalSlider->setEnabled(false);
        m_Ui->user_black_spinBox->setEnabled(false);
    }
    m_Ui->user_black_horizontalSlider->setValue(luminance_options.getRawUserBlack());
    m_Ui->user_black_spinBox->setValue(luminance_options.getRawUserBlack());
    m_Ui->use_sat_CB->setChecked(luminance_options.isRawUseSaturation());
    if (luminance_options.isRawUseSaturation()) {
        m_Ui->user_sat_horizontalSlider->setEnabled(true);
        m_Ui->user_sat_spinBox->setEnabled(true);
    }
    else {
        m_Ui->user_sat_horizontalSlider->setEnabled(false);
        m_Ui->user_sat_spinBox->setEnabled(false);
    }
    m_Ui->user_sat_horizontalSlider->setValue(luminance_options.getRawUserSaturation());
    m_Ui->user_sat_spinBox->setValue(luminance_options.getRawUserSaturation());
    m_Ui->use_noise_CB->setChecked(luminance_options.isRawUseNoiseReduction());
    if ( luminance_options.isRawUseNoiseReduction() ) {
        m_Ui->threshold_horizontalSlider->setEnabled(true);
        m_Ui->threshold_spinBox->setEnabled(true);
    }
    else {
        m_Ui->threshold_horizontalSlider->setEnabled(false);
        m_Ui->threshold_spinBox->setEnabled(false);
    }
    m_Ui->threshold_horizontalSlider->setValue( luminance_options.getRawNoiseReductionThreshold() );
    m_Ui->threshold_spinBox->setValue( luminance_options.getRawNoiseReductionThreshold() );
    m_Ui->use_chroma_CB->setChecked( luminance_options.isRawUseChroma() );
    if ( luminance_options.isRawUseChroma() ) {
        m_Ui->red_horizontalSlider->setEnabled(true);
        m_Ui->red_doubleSpinBox->setEnabled(true);
        m_Ui->blue_horizontalSlider->setEnabled(true);
        m_Ui->blue_doubleSpinBox->setEnabled(true);
    }
    else {
        m_Ui->red_horizontalSlider->setEnabled(false);
        m_Ui->red_doubleSpinBox->setEnabled(false);
        m_Ui->blue_horizontalSlider->setEnabled(false);
        m_Ui->blue_doubleSpinBox->setEnabled(false);
    }
    double r_minv = m_Ui->red_doubleSpinBox->minimum();
    double r_maxv = m_Ui->red_doubleSpinBox->maximum();
    double r_minpos = m_Ui->red_horizontalSlider->minimum();
    double r_maxpos = m_Ui->red_horizontalSlider->maximum();
    double b_minv = m_Ui->blue_doubleSpinBox->minimum();
    double b_maxv = m_Ui->blue_doubleSpinBox->maximum();
    double b_minpos = m_Ui->blue_horizontalSlider->minimum();
    double b_maxpos = m_Ui->blue_horizontalSlider->maximum();
    double g_minv = m_Ui->green_doubleSpinBox->minimum();
    double g_maxv = m_Ui->green_doubleSpinBox->maximum();
    double g_minpos = m_Ui->green_horizontalSlider->minimum();
    double g_maxpos = m_Ui->green_horizontalSlider->maximum();

    m_Ui->red_horizontalSlider->setValue(value2pos(luminance_options.getRawAber0(), r_minpos, r_maxpos, r_minv, r_maxv));
    m_Ui->red_doubleSpinBox->setValue(luminance_options.getRawAber0());
    m_Ui->blue_horizontalSlider->setValue(value2pos(luminance_options.getRawAber2(), b_minpos, b_maxpos, b_minv, b_maxv));
    m_Ui->blue_doubleSpinBox->setValue(luminance_options.getRawAber2());
    m_Ui->green_horizontalSlider->setValue(value2pos(luminance_options.getRawGreen(), g_minpos, g_maxpos, g_minv, g_maxv));
    m_Ui->green_doubleSpinBox->setValue(luminance_options.getRawGreen());

    m_Ui->user_qual_toolButton->setEnabled( luminance_options.value(KEY_USER_QUAL_TOOLBUTTON).toBool());
    m_Ui->med_passes_toolButton->setEnabled( luminance_options.value(KEY_MED_PASSES_TOOLBUTTON).toBool());
    m_Ui->wb_method_toolButton->setEnabled( luminance_options.value(KEY_WB_METHOD_TOOLBUTTON).toBool());
    m_Ui->TK_toolButton->setEnabled( luminance_options.value(KEY_TK_TOOLBUTTON).toBool());
    m_Ui->highlights_toolButton->setEnabled( luminance_options.value(KEY_HIGHLIGHTS_TOOLBUTTON).toBool());
    m_Ui->level_toolButton->setEnabled( luminance_options.value(KEY_LEVEL_TOOLBUTTON).toBool());
    m_Ui->brightness_toolButton->setEnabled( luminance_options.value(KEY_BRIGHTNESS_TOOLBUTTON).toBool());
    m_Ui->user_black_toolButton->setEnabled( luminance_options.value(KEY_USER_BLACK_TOOLBUTTON).toBool());
    m_Ui->user_sat_toolButton->setEnabled( luminance_options.value(KEY_USER_SAT_TOOLBUTTON).toBool());
    m_Ui->threshold_toolButton->setEnabled( luminance_options.value(KEY_THRESHOLD_TOOLBUTTON).toBool());
    m_Ui->red_toolButton->setEnabled( luminance_options.value(KEY_RED_TOOLBUTTON).toBool());
    m_Ui->blue_toolButton->setEnabled( luminance_options.value(KEY_BLUE_TOOLBUTTON).toBool());
    m_Ui->green_toolButton->setEnabled( luminance_options.value(KEY_GREEN_TOOLBUTTON).toBool());
}

PreferencesDialog::~PreferencesDialog() {}

void PreferencesDialog::updateLineEditString()
{
    QString dir = QFileDialog::getExistingDirectory(this,
                                                    tr("Choose a directory"),
                                                    QDir::currentPath(),
                                                    QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty())
    {
        m_Ui->lineEditTempPath->setText(dir);
    }
}

void PreferencesDialog::enterWhatsThis()
{
	QWhatsThis::enterWhatsThisMode();
}

