/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2006,2007 Giuseppe Rota
 * Copytight (C) 2011 Davide Anastasia
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
 * Original Work
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * Improvements, bugfixing
 * @author Franco Comida <fcomida@users.sourceforge.net>
 * Implement class deriving from QSettings (override Singleton pattern with QSettings functionalities)
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *
 */

//#include <QTextStream>
//#include <QApplication>
#include <QString>
#include <QLocale>
#include <QFile>
#include <QDir>

#include "Common/LuminanceOptions.h"
#include "Common/config.h"

LuminanceOptions::LuminanceOptions():
    QSettings(LUMINANCEORGANIZATION, LUMINANCEAPPLICATION)
{ }

// write system default language the first time around (discard "_country")
QString LuminanceOptions::getGuiLang()
{
    return value(KEY_GUI_LANG, QLocale::system().name().left(2)).toString();
}

void LuminanceOptions::setGuiLang(QString s)
{
    setValue(KEY_GUI_LANG, s);
}

bool LuminanceOptions::isRawFourColorRGB()
{
    return value(KEY_FOUR_COLOR_RGB, false).toBool();
}

void LuminanceOptions::setRawFourColorRGB(bool b)
{
    setValue(KEY_FOUR_COLOR_RGB, b);
}

bool LuminanceOptions::isRawDoNotUseFujiRotate()
{
    return value(KEY_DO_NOT_USE_FUJI_ROTATE, false).toBool();
}

void LuminanceOptions::setRawDoNotUseFujiRotate(bool b)
{
    setValue(KEY_DO_NOT_USE_FUJI_ROTATE, b);
}

double LuminanceOptions::getRawAber0()
{
    return this->value(KEY_ABER_0, 1.0).toDouble();
}

void LuminanceOptions::setRawAber0(double v)
{
    setValue(KEY_ABER_0, v);
}

double LuminanceOptions::getRawAber1()
{
    return this->value(KEY_ABER_1, 1.0).toDouble();
}

void LuminanceOptions::setRawAber1(double v)
{
    setValue(KEY_ABER_1, v);
}

double LuminanceOptions::getRawAber2()
{
    return this->value(KEY_ABER_2).toDouble();
}

void LuminanceOptions::setRawAber2(double v)
{
    setValue(KEY_ABER_2, v);
}

double LuminanceOptions::getRawAber3()
{
    return this->value(KEY_ABER_3).toDouble();
}

void LuminanceOptions::setRawAber3(double v)
{
    setValue(KEY_ABER_3, v);
}

double LuminanceOptions::getRawGamm0()
{
    return value(KEY_GAMM_0, 1.0/2.4).toDouble();
}

void LuminanceOptions::setRawGamm0(double v)
{
    setValue(KEY_GAMM_0, v);
}

double LuminanceOptions::getRawGamm1()
{
    return this->value(KEY_GAMM_1, 12.92).toDouble();
}

void LuminanceOptions::setRawGamm1(double v)
{
    setValue(KEY_GAMM_1, v);
}

int LuminanceOptions::getRawTemperatureKelvin()
{
    return value(KEY_TK, 6500).toInt();
}

void LuminanceOptions::setRawTemperatureKelvin(int v)
{
    setValue(KEY_TK, v);
}

float LuminanceOptions::getRawGreen()
{
    return value(KEY_GREEN, 1.0f).toFloat();
}

void LuminanceOptions::setRawGreen(float v)
{
    setValue(KEY_GREEN, v);
}

float LuminanceOptions::getRawUserMul0()
{
    return value(KEY_USER_MUL_0, 1.0f).toFloat();
}

void LuminanceOptions::setRawUserMul0(float v)
{
    setValue(KEY_USER_MUL_0, v);
}

float LuminanceOptions::getRawUserMul1()
{
    return value(KEY_USER_MUL_1, 1.0f).toFloat();
}

void LuminanceOptions::setRawUserMul1(float v)
{
    setValue(KEY_USER_MUL_1, v);
}

float LuminanceOptions::getRawUserMul2()
{
    return value(KEY_USER_MUL_2, 1.0f).toFloat();
}

void LuminanceOptions::setRawUserMul2(float v)
{
    setValue(KEY_USER_MUL_2, v);
}

float LuminanceOptions::getRawUserMul3()
{
    return value(KEY_USER_MUL_3, 1.0f).toFloat();
}

void LuminanceOptions::setRawUserMul3(float v)
{
    setValue(KEY_USER_MUL_3, v);
}

bool LuminanceOptions::isRawAutoBrightness()
{
    return value(KEY_AUTO_BRIGHT, false).toBool();
}

void LuminanceOptions::setRawAutoBrightness(bool b)
{
    setValue(KEY_AUTO_BRIGHT, b);
}

float LuminanceOptions::getRawBrightness()
{
    return value(KEY_BRIGHTNESS, 1.0f).toFloat();
}

void LuminanceOptions::setRawBrightness(float f)
{
    setValue(KEY_BRIGHTNESS, f);
}

float LuminanceOptions::getRawNoiseReductionThreshold()
{
    return value(KEY_THRESHOLD, 100.0f).toFloat();
}

void LuminanceOptions::setRawNoiseReductionThreshold(float v)
{
    setValue(KEY_THRESHOLD, v);
}

int LuminanceOptions::getRawHalfSize()
{
    return value(KEY_HALF_SIZE, 0).toInt();
}

void LuminanceOptions::setRawHalfSize(int v)
{
    setValue(KEY_HALF_SIZE, v);
}

int LuminanceOptions::getRawWhiteBalanceMethod()
{
    return value(KEY_WB_METHOD, 0).toInt();
}

void LuminanceOptions::setRawWhiteBalanceMethod(int v)
{
    setValue(KEY_WB_METHOD, v);
}

int LuminanceOptions::getRawOutputColor()
{
    return value(KEY_OUTPUT_COLOR, 1).toInt();
}

// double check those!
QString LuminanceOptions::getRawOutputProfile()
{
    //QFile::encodeName(this->value(KEY_OUTPUT_PROFILE).toString()).constData();
    return QFile::encodeName(value(KEY_OUTPUT_PROFILE, "").toString());
}

void LuminanceOptions::setRawOutputProfile(QString v)
{
    setValue(KEY_OUTPUT_PROFILE, v);
}

QString LuminanceOptions::getRawCameraProfile()
{
    // QFile::encodeName(this->value(KEY_CAMERA_PROFILE).toString()).constData();
    return QFile::encodeName(value(KEY_CAMERA_PROFILE, "").toString());
}

void LuminanceOptions::setRawCameraProfile(QString v)
{
    setValue(KEY_CAMERA_PROFILE, v);
}

int LuminanceOptions::getRawUserFlip()
{
    return value(KEY_USER_FLIP, 0).toInt();
}

int LuminanceOptions::getRawUserQuality()
{
    return value(KEY_USER_QUAL, 0).toInt();
}

void LuminanceOptions::setRawUserQuality(int v)
{
    setValue(KEY_USER_QUAL, v);
}

int LuminanceOptions::getRawUserSaturation()
{
    return value(KEY_USER_SAT, 20000).toInt();
}

void LuminanceOptions::setRawUserSaturation(int v)
{
    setValue(KEY_USER_SAT, v);
}

int LuminanceOptions::getRawMedPasses()
{
    return value(KEY_MED_PASSES, 0).toInt();
}

void LuminanceOptions::setRawMedPasses(int v)
{
    setValue(KEY_MED_PASSES, v);
}

int LuminanceOptions::getRawHighlightsMode()
{
    return value(KEY_HIGHLIGHTS, 0).toInt();
}

void LuminanceOptions::setRawHighlightsMode(int v)
{
    setValue(KEY_HIGHLIGHTS, v);
}

int LuminanceOptions::getRawLevel()
{
    return value(KEY_LEVEL, 0).toInt();
}

void LuminanceOptions::setRawLevel(int v)
{
    setValue(KEY_LEVEL, v);
}

float LuminanceOptions::getRawBrightnessThreshold()
{
    return value(KEY_AUTO_BRIGHT_THR, 0.0f).toFloat();
}

void LuminanceOptions::setRawBrightnessThreshold(float v)
{
    setValue(KEY_AUTO_BRIGHT_THR, v);
}

float LuminanceOptions::getRawMaximumThreshold()
{
    return value(KEY_ADJUST_MAXIMUM_THR, 0.0f).toFloat();
}

void LuminanceOptions::setRawMaximumThreshold(float v)
{
    setValue(KEY_ADJUST_MAXIMUM_THR, v);
}

bool LuminanceOptions::isRawUseBlack()
{
    return value(KEY_USE_BLACK, false).toBool();
}

void LuminanceOptions::setRawUseBlack(bool b)
{
    setValue(KEY_USE_BLACK, b);
}

int LuminanceOptions::getRawUserBlack()
{
    return value(KEY_USER_BLACK, 0).toInt();
}

void LuminanceOptions::setRawUserBlack(int v)
{
    setValue(KEY_USER_BLACK, v);
}

bool LuminanceOptions::isRawUseSaturation()
{
    return value(KEY_USE_SAT, false).toBool();
}

void LuminanceOptions::setRawUseSaturation(bool b)
{
    setValue(KEY_USE_SAT, b);
}

bool LuminanceOptions::isRawUseNoiseReduction()
{
    return value(KEY_USE_NOISE, true).toBool();
}

void LuminanceOptions::setRawUseNoiseReduction(bool b)
{
    setValue(KEY_USE_NOISE, b);
}

bool LuminanceOptions::isRawUseChroma()
{
    return value(KEY_USE_CHROMA, false).toBool();
}

void LuminanceOptions::setRawUseChroma(bool b)
{
    setValue(KEY_USE_CHROMA, b);
}

QString LuminanceOptions::getBatchHdrPathInput()
{
    return value(KEY_BATCH_HDR_PATH_INPUT, QDir::currentPath()).toString();
}

void LuminanceOptions::setBatchHdrPathInput(QString qstr)
{
    setValue(KEY_BATCH_HDR_PATH_INPUT, qstr);
}

QString LuminanceOptions::getBatchHdrPathOutput()
{
    return value(KEY_BATCH_HDR_PATH_OUTPUT, QDir::currentPath()).toString();
}

void LuminanceOptions::setBatchHdrPathOutput(QString qstr)
{
    setValue(KEY_BATCH_HDR_PATH_OUTPUT, qstr);
}

bool LuminanceOptions::isSaveLogLuvTiff()
{
    return value(KEY_SAVE_LOGLUV, true).toBool();
}

void LuminanceOptions::setSaveLogLuvTiff(bool b)
{
    setValue(KEY_SAVE_LOGLUV, b);
}

QString LuminanceOptions::getBatchTmPathHdrInput()
{
    return value(KEY_BATCH_TM_PATH_INPUT, QDir::currentPath()).toString();
}

void LuminanceOptions::setBatchTmPathHdrInput(QString s)
{
    setValue(KEY_BATCH_TM_PATH_INPUT, s);
}

QString LuminanceOptions::getBatchTmPathTmoSettings()
{
    return value(KEY_BATCH_TM_PATH_TMO_SETTINGS, QDir::currentPath()).toString();
}

void LuminanceOptions::setBatchTmPathTmoSettings(QString s)
{
    setValue(KEY_BATCH_TM_PATH_TMO_SETTINGS, s);
}

QString LuminanceOptions::getBatchTmPathLdrOutput()
{
    return value(KEY_BATCH_TM_PATH_OUTPUT, QDir::currentPath()).toString();
}

void LuminanceOptions::setBatchTmPathLdrOutput(QString s)
{
    setValue(KEY_BATCH_TM_PATH_OUTPUT, s);
}

int LuminanceOptions::getBatchTmNumThreads()
{
    return value(KEY_BATCH_TM_NUM_THREADS, 1).toInt();
}

void LuminanceOptions::setBatchTmNumThreads(int v)
{
    setValue(KEY_BATCH_TM_NUM_THREADS, v);
}

QString LuminanceOptions::getBatchTmLdrFormat()
{
    return value(KEY_BATCH_TM_LDR_FORMAT, "JPEG").toString();
}

void LuminanceOptions::setBatchTmLdrFormat(QString s)
{
    setValue(KEY_BATCH_TM_LDR_FORMAT, s);
}

QString LuminanceOptions::getTempDir()
{
    QString temp_dir_name = value(KEY_TEMP_RESULT_PATH, QDir::temp().absolutePath()).toString();
    QDir dir(temp_dir_name);
    QFileInfo test_temp_dir_name(dir.absoluteFilePath("file"));
    if ( dir.exists() &&
         test_temp_dir_name.isWritable() )
    {
        // directory choosen by the user (or the default one) is usable
        return temp_dir_name;
    }
    else
    {
        // return default temporary directory
        return QDir::temp().absolutePath();
    }
}

void LuminanceOptions::setTempDir(QString path)
{
    setValue(KEY_TEMP_RESULT_PATH, path);
}

QString LuminanceOptions::getDefaultPathHdrInOut()
{
    return value(KEY_RECENT_PATH_LOAD_SAVE_HDR,QDir::currentPath()).toString();
}

void LuminanceOptions::setDefaultPathHdrInOut(QString path)
{
    setValue(KEY_RECENT_PATH_LOAD_SAVE_HDR, path);
}

QString LuminanceOptions::getDefaultPathLdrIn()
{
    return value(KEY_RECENT_PATH_LOAD_LDRs_FOR_HDR, QDir::currentPath()).toString();
}

void LuminanceOptions::setDefaultPathLdrIn(QString path)
{
    setValue(KEY_RECENT_PATH_LOAD_LDRs_FOR_HDR, path);
}

QString LuminanceOptions::getDefaultPathLdrOut()
{
    return value(KEY_RECENT_PATH_SAVE_LDR, QDir::currentPath()).toString();
}

void LuminanceOptions::setDefaultPathLdrOut(QString path)
{
    setValue(KEY_RECENT_PATH_SAVE_LDR, path);
}

QString LuminanceOptions::getDefaultPathTmoSettings()
{
    return value(KEY_RECENT_PATH_LOAD_SAVE_TMO_SETTINGS, QDir::currentPath()).toString();
}

void LuminanceOptions::setDefaultPathTmoSettings(QString path)
{
    setValue(KEY_RECENT_PATH_LOAD_SAVE_TMO_SETTINGS, path);
}



/*
settings->beginGroup(GROUP_EXTERNALTOOLS);
//bug 2001032, remove spurious default QString "-a aligned_" value set by ver 1.9.2
if (!settings->contains(KEY_EXTERNAL_AIS_OPTIONS) || settings->value(KEY_EXTERNAL_AIS_OPTIONS).toString()=="-v -a aligned_")
    settings->setValue(KEY_EXTERNAL_AIS_OPTIONS, QStringList() << "-v" << "-a" << "aligned_");
align_image_stack_options=settings->value(KEY_EXTERNAL_AIS_OPTIONS).toStringList();
settings->endGroup();
*/
QStringList LuminanceOptions::getAlignImageStackOptions()
{
    return value(KEY_EXTERNAL_AIS_OPTIONS, QStringList() << "-v" << "-a" << "aligned_").toStringList();
}

void LuminanceOptions::setAlignImageStackOptions(QStringList qstrlist)
{
    setValue(KEY_EXTERNAL_AIS_OPTIONS, qstrlist);
}

bool LuminanceOptions::isShowFirstPageWizard()
{
    return value(KEY_WIZARD_SHOWFIRSTPAGE,true).toBool();
}

void LuminanceOptions::setShowFirstPageWizard(bool b)
{
    setValue(KEY_WIZARD_SHOWFIRSTPAGE, b);
}

bool LuminanceOptions::isShowFattalWarning()
{
    return value(KEY_TMOWARNING_FATTALSMALL,true).toBool();
}

void LuminanceOptions::setShowFattalWarning(bool b)
{
    setValue(KEY_TMOWARNING_FATTALSMALL, b);
}

int LuminanceOptions::getMainWindowToolBarMode()
{
    return value(KEY_TOOLBAR_MODE, Qt::ToolButtonTextUnderIcon).toInt();
}

void LuminanceOptions::setMainWindowToolBarMode(int mode)
{
    setValue(KEY_TOOLBAR_MODE, mode);
}

// Viewer
unsigned int LuminanceOptions::getViewerNanInfColor()
{
    return value(KEY_NANINFCOLOR,0xFF000000).toUInt();
}

void LuminanceOptions::setViewerNanInfColor(unsigned int color)
{
    setValue(KEY_NANINFCOLOR, color);
}

unsigned int LuminanceOptions::getViewerNegColor()
{
    return value(KEY_NEGCOLOR,0xFF000000).toUInt();
}

void LuminanceOptions::setViewerNegColor(unsigned int color)
{
    setValue(KEY_NEGCOLOR, color);
}

bool LuminanceOptions::isPreviewPanelActive()
{
    return value(KEY_TMOWINDOW_SHOWPREVIEWPANEL, true).toBool();
}

void LuminanceOptions::setPreviewPanelActive(bool status)
{
    setValue(KEY_TMOWINDOW_SHOWPREVIEWPANEL, status);
}

int  LuminanceOptions::getPreviewWidth()
{
    return value(KEY_TMOWINDOW_PREVIEWS_WIDTH, 400).toInt();
}

void LuminanceOptions::setPreviewWidth(int v)
{
    setValue(KEY_TMOWINDOW_PREVIEWS_WIDTH, v);
}

int LuminanceOptions::getCameraProfile()
{
	return value(KEY_COLOR_CAMERA_PROFILE, 0).toInt();
}

void LuminanceOptions::setCameraProfile(int index)
{
	setValue(KEY_COLOR_CAMERA_PROFILE, index);
}

QString LuminanceOptions::getCameraProfileFileName()
{
	return value(KEY_COLOR_CAMERA_PROFILE_FILENAME).toString();
}

void LuminanceOptions::setCameraProfileFileName(QString fname)
{
	setValue(KEY_COLOR_CAMERA_PROFILE_FILENAME, fname);
}

QString LuminanceOptions::getMonitorProfileFileName()
{
	return value(KEY_COLOR_MONITOR_PROFILE_FILENAME).toString();
}

void LuminanceOptions::setMonitorProfileFileName(QString fname)
{
	setValue(KEY_COLOR_MONITOR_PROFILE_FILENAME, fname);
}

QString LuminanceOptions::getPrinterProfileFileName()
{
	return value(KEY_COLOR_PRINTER_PROFILE_FILENAME).toString();
}

void LuminanceOptions::setPrinterProfileFileName(QString fname)
{
	setValue(KEY_COLOR_PRINTER_PROFILE_FILENAME, fname);
}
