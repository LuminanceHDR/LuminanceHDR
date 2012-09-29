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

#include <QApplication>
#include <QString>
#include <QLocale>
#include <QFile>
#include <QDebug>

#include "Common/LuminanceOptions.h"
#include "Common/config.h"

bool LuminanceOptions::isCurrentPortableMode = false;

LuminanceOptions::LuminanceOptions():
    QObject()
{
    initSettings();
}

LuminanceOptions::~LuminanceOptions()
{
    delete m_settingHolder;
}

void LuminanceOptions::conditionallyDoUpgrade()
{
    LuminanceOptions options;
    int currentVersion = options.value("LuminanceOptionsVersion", 0).toInt();

    // check if update needed
    if (currentVersion < LUMINANCEVERSION_NUM)
    {
        if (currentVersion < 2030099)
        {
            options.setRawWhiteBalanceMethod(1);
#ifdef DEMOSAICING_GPL3
            options.setRawUserQuality(10); // AMaZE 
#endif
        }

        options.setValue("LuminanceOptionsVersion", LUMINANCEVERSION_NUM);
    }
}

void LuminanceOptions::setPortableMode(bool isPortable)
{
    if (LuminanceOptions::isCurrentPortableMode != isPortable)
    {
        QSettings* oldSettings = m_settingHolder;
        LuminanceOptions::isCurrentPortableMode = isPortable;
        initSettings();
        foreach (QString key, oldSettings->allKeys())
        {
            m_settingHolder->setValue(key, oldSettings->value(key));
        }
        delete oldSettings;

        QString filePath = QDir(QApplication::applicationDirPath()).relativeFilePath("PortableMode.txt");
        QFile file(filePath);
        if (isPortable && !file.exists()) 
        {
            if (file.open(QIODevice::WriteOnly))
                file.close();
        }
        else if (!isPortable && file.exists())
        {
            file.remove();
        }
    }
}

void LuminanceOptions::initSettings()
{
    if (LuminanceOptions::isCurrentPortableMode)
        m_settingHolder = new QSettings("settings.ini", QSettings::IniFormat);
    else
        m_settingHolder = new QSettings();
}

void LuminanceOptions::setValue(const QString& key, const QVariant& value)
{
    m_settingHolder->setValue(key, value);
}

QVariant LuminanceOptions::value(const QString & key, const QVariant& defaultValue) const
{
    return m_settingHolder->value(key, defaultValue);
}

QString LuminanceOptions::getDatabaseFileName()
{
    QString filename;
    if (LuminanceOptions::isCurrentPortableMode)
    {
        filename = QDir::currentPath();
    }
    else
    {	
        filename = QDir(QDir::homePath()).absolutePath();
#ifdef WIN32
        filename += "/LuminanceHDR";
#else
        filename += "/.LuminanceHDR";
#endif
    }
    filename += "/saved_parameters.db";

    return filename;
}


// write system default language the first time around (discard "_country")
QString LuminanceOptions::getGuiLang()
{
    return m_settingHolder->value(KEY_GUI_LANG, QLocale::system().name().left(2)).toString();
}

void LuminanceOptions::setGuiLang(QString s)
{
    m_settingHolder->setValue(KEY_GUI_LANG, s);
}

bool LuminanceOptions::isRawFourColorRGB()
{
    return m_settingHolder->value(KEY_FOUR_COLOR_RGB, false).toBool();
}

void LuminanceOptions::setRawFourColorRGB(bool b)
{
    m_settingHolder->setValue(KEY_FOUR_COLOR_RGB, b);
}

bool LuminanceOptions::isRawDoNotUseFujiRotate()
{
    return m_settingHolder->value(KEY_DO_NOT_USE_FUJI_ROTATE, false).toBool();
}

void LuminanceOptions::setRawDoNotUseFujiRotate(bool b)
{
    m_settingHolder->setValue(KEY_DO_NOT_USE_FUJI_ROTATE, b);
}

double LuminanceOptions::getRawAber0()
{
    return m_settingHolder->value(KEY_ABER_0, 1.0).toDouble();
}

void LuminanceOptions::setRawAber0(double v)
{
    m_settingHolder->setValue(KEY_ABER_0, v);
}

double LuminanceOptions::getRawAber1()
{
    return m_settingHolder->value(KEY_ABER_1, 1.0).toDouble();
}

void LuminanceOptions::setRawAber1(double v)
{
    m_settingHolder->setValue(KEY_ABER_1, v);
}

double LuminanceOptions::getRawAber2()
{
    return m_settingHolder->value(KEY_ABER_2).toDouble();
}

void LuminanceOptions::setRawAber2(double v)
{
    m_settingHolder->setValue(KEY_ABER_2, v);
}

double LuminanceOptions::getRawAber3()
{
    return m_settingHolder->value(KEY_ABER_3).toDouble();
}

void LuminanceOptions::setRawAber3(double v)
{
    m_settingHolder->setValue(KEY_ABER_3, v);
}

double LuminanceOptions::getRawGamm0()
{
    return m_settingHolder->value(KEY_GAMM_0, 1.0/2.4).toDouble();
}

void LuminanceOptions::setRawGamm0(double v)
{
    m_settingHolder->setValue(KEY_GAMM_0, v);
}

double LuminanceOptions::getRawGamm1()
{
    return m_settingHolder->value(KEY_GAMM_1, 12.92).toDouble();
}

void LuminanceOptions::setRawGamm1(double v)
{
    m_settingHolder->setValue(KEY_GAMM_1, v);
}

int LuminanceOptions::getRawTemperatureKelvin()
{
    return m_settingHolder->value(KEY_TK, 6500).toInt();
}

void LuminanceOptions::setRawTemperatureKelvin(int v)
{
    m_settingHolder->setValue(KEY_TK, v);
}

float LuminanceOptions::getRawGreen()
{
    return m_settingHolder->value(KEY_GREEN, 1.0f).toFloat();
}

void LuminanceOptions::setRawGreen(float v)
{
    m_settingHolder->setValue(KEY_GREEN, v);
}

float LuminanceOptions::getRawUserMul0()
{
    return m_settingHolder->value(KEY_USER_MUL_0, 1.0f).toFloat();
}

void LuminanceOptions::setRawUserMul0(float v)
{
    m_settingHolder->setValue(KEY_USER_MUL_0, v);
}

float LuminanceOptions::getRawUserMul1()
{
    return m_settingHolder->value(KEY_USER_MUL_1, 1.0f).toFloat();
}

void LuminanceOptions::setRawUserMul1(float v)
{
    m_settingHolder->setValue(KEY_USER_MUL_1, v);
}

float LuminanceOptions::getRawUserMul2()
{
    return m_settingHolder->value(KEY_USER_MUL_2, 1.0f).toFloat();
}

void LuminanceOptions::setRawUserMul2(float v)
{
    m_settingHolder->setValue(KEY_USER_MUL_2, v);
}

float LuminanceOptions::getRawUserMul3()
{
    return m_settingHolder->value(KEY_USER_MUL_3, 1.0f).toFloat();
}

void LuminanceOptions::setRawUserMul3(float v)
{
    m_settingHolder->setValue(KEY_USER_MUL_3, v);
}

bool LuminanceOptions::isRawAutoBrightness()
{
    return m_settingHolder->value(KEY_AUTO_BRIGHT, false).toBool();
}

void LuminanceOptions::setRawAutoBrightness(bool b)
{
    m_settingHolder->setValue(KEY_AUTO_BRIGHT, b);
}

float LuminanceOptions::getRawBrightness()
{
    return m_settingHolder->value(KEY_BRIGHTNESS, 1.0f).toFloat();
}

void LuminanceOptions::setRawBrightness(float f)
{
    m_settingHolder->setValue(KEY_BRIGHTNESS, f);
}

float LuminanceOptions::getRawNoiseReductionThreshold()
{
    return m_settingHolder->value(KEY_THRESHOLD, 100.0f).toFloat();
}

void LuminanceOptions::setRawNoiseReductionThreshold(float v)
{
    m_settingHolder->setValue(KEY_THRESHOLD, v);
}

int LuminanceOptions::getRawHalfSize()
{
    return m_settingHolder->value(KEY_HALF_SIZE, 0).toInt();
}

void LuminanceOptions::setRawHalfSize(int v)
{
    m_settingHolder->setValue(KEY_HALF_SIZE, v);
}

int LuminanceOptions::getRawWhiteBalanceMethod()
{
    return m_settingHolder->value(KEY_WB_METHOD, 1).toInt();
}

void LuminanceOptions::setRawWhiteBalanceMethod(int v)
{
    m_settingHolder->setValue(KEY_WB_METHOD, v);
}

int LuminanceOptions::getRawOutputColor()
{
    return m_settingHolder->value(KEY_OUTPUT_COLOR, 1).toInt();
}

// double check those!
QString LuminanceOptions::getRawOutputProfile()
{
    //QFile::encodeName(this->value(KEY_OUTPUT_PROFILE).toString()).constData();
    return QFile::encodeName(m_settingHolder->value(KEY_OUTPUT_PROFILE, "").toString());
}

void LuminanceOptions::setRawOutputProfile(QString v)
{
    m_settingHolder->setValue(KEY_OUTPUT_PROFILE, v);
}

QString LuminanceOptions::getRawCameraProfile()
{
    // QFile::encodeName(this->value(KEY_CAMERA_PROFILE).toString()).constData();
    return QFile::encodeName(m_settingHolder->value(KEY_CAMERA_PROFILE, "").toString());
}

void LuminanceOptions::setRawCameraProfile(QString v)
{
    m_settingHolder->setValue(KEY_CAMERA_PROFILE, v);
}

int LuminanceOptions::getRawUserFlip()
{
    return m_settingHolder->value(KEY_USER_FLIP, 0).toInt();
}

int LuminanceOptions::getRawUserQuality()
{
#ifdef DEMOSAICING_GPL2
    int defaultUserQuality = 5; // using AHDv2
#else
    int defaultUserQuality = 3; // using AHD
#endif
#ifdef DEMOSAICING_GPL3
    defaultUserQuality = 10; // using  AMaZE interpolation
#endif
    return m_settingHolder->value(KEY_USER_QUAL, defaultUserQuality).toInt();
}

void LuminanceOptions::setRawUserQuality(int v)
{
    m_settingHolder->setValue(KEY_USER_QUAL, v);
}

int LuminanceOptions::getRawUserSaturation()
{
    return m_settingHolder->value(KEY_USER_SAT, 20000).toInt();
}

void LuminanceOptions::setRawUserSaturation(int v)
{
    m_settingHolder->setValue(KEY_USER_SAT, v);
}

int LuminanceOptions::getRawMedPasses()
{
    return m_settingHolder->value(KEY_MED_PASSES, 0).toInt();
}

void LuminanceOptions::setRawMedPasses(int v)
{
    m_settingHolder->setValue(KEY_MED_PASSES, v);
}

int LuminanceOptions::getRawHighlightsMode()
{
    return m_settingHolder->value(KEY_HIGHLIGHTS, 0).toInt();
}

void LuminanceOptions::setRawHighlightsMode(int v)
{
    m_settingHolder->setValue(KEY_HIGHLIGHTS, v);
}

int LuminanceOptions::getRawLevel()
{
    return m_settingHolder->value(KEY_LEVEL, 0).toInt();
}

void LuminanceOptions::setRawLevel(int v)
{
    m_settingHolder->setValue(KEY_LEVEL, v);
}

float LuminanceOptions::getRawBrightnessThreshold()
{
    return m_settingHolder->value(KEY_AUTO_BRIGHT_THR, 0.0f).toFloat();
}

void LuminanceOptions::setRawBrightnessThreshold(float v)
{
    m_settingHolder->setValue(KEY_AUTO_BRIGHT_THR, v);
}

float LuminanceOptions::getRawMaximumThreshold()
{
    return m_settingHolder->value(KEY_ADJUST_MAXIMUM_THR, 0.0f).toFloat();
}

void LuminanceOptions::setRawMaximumThreshold(float v)
{
    m_settingHolder->setValue(KEY_ADJUST_MAXIMUM_THR, v);
}

bool LuminanceOptions::isRawUseBlack()
{
    return m_settingHolder->value(KEY_USE_BLACK, false).toBool();
}

void LuminanceOptions::setRawUseBlack(bool b)
{
    m_settingHolder->setValue(KEY_USE_BLACK, b);
}

int LuminanceOptions::getRawUserBlack()
{
    return m_settingHolder->value(KEY_USER_BLACK, 0).toInt();
}

void LuminanceOptions::setRawUserBlack(int v)
{
    m_settingHolder->setValue(KEY_USER_BLACK, v);
}

bool LuminanceOptions::isRawUseSaturation()
{
    return m_settingHolder->value(KEY_USE_SAT, false).toBool();
}

void LuminanceOptions::setRawUseSaturation(bool b)
{
    m_settingHolder->setValue(KEY_USE_SAT, b);
}

bool LuminanceOptions::isRawUseNoiseReduction()
{
    return m_settingHolder->value(KEY_USE_NOISE, true).toBool();
}

void LuminanceOptions::setRawUseNoiseReduction(bool b)
{
    m_settingHolder->setValue(KEY_USE_NOISE, b);
}

bool LuminanceOptions::isRawUseChroma()
{
    return m_settingHolder->value(KEY_USE_CHROMA, false).toBool();
}

void LuminanceOptions::setRawUseChroma(bool b)
{
    m_settingHolder->setValue(KEY_USE_CHROMA, b);
}

QString LuminanceOptions::getBatchHdrPathInput(QString defaultPath)
{
    return m_settingHolder->value(KEY_BATCH_HDR_PATH_INPUT, defaultPath).toString();
}

void LuminanceOptions::setBatchHdrPathInput(QString qstr)
{
    m_settingHolder->setValue(KEY_BATCH_HDR_PATH_INPUT, qstr);
}

QString LuminanceOptions::getBatchHdrPathOutput(QString defaultPath)
{
    return m_settingHolder->value(KEY_BATCH_HDR_PATH_OUTPUT, defaultPath).toString();
}

void LuminanceOptions::setBatchHdrPathOutput(QString qstr)
{
    m_settingHolder->setValue(KEY_BATCH_HDR_PATH_OUTPUT, qstr);
}

bool LuminanceOptions::isSaveLogLuvTiff()
{
    return m_settingHolder->value(KEY_SAVE_LOGLUV, true).toBool();
}

void LuminanceOptions::setSaveLogLuvTiff(bool b)
{
    m_settingHolder->setValue(KEY_SAVE_LOGLUV, b);
}

QString LuminanceOptions::getBatchTmPathHdrInput()
{
    return m_settingHolder->value(KEY_BATCH_TM_PATH_INPUT, QDir::currentPath()).toString();
}

void LuminanceOptions::setBatchTmPathHdrInput(QString s)
{
    m_settingHolder->setValue(KEY_BATCH_TM_PATH_INPUT, s);
}

QString LuminanceOptions::getBatchTmPathTmoSettings()
{
    return m_settingHolder->value(KEY_BATCH_TM_PATH_TMO_SETTINGS, QDir::currentPath()).toString();
}

void LuminanceOptions::setBatchTmPathTmoSettings(QString s)
{
    m_settingHolder->setValue(KEY_BATCH_TM_PATH_TMO_SETTINGS, s);
}

QString LuminanceOptions::getBatchTmPathLdrOutput()
{
    return m_settingHolder->value(KEY_BATCH_TM_PATH_OUTPUT, QDir::currentPath()).toString();
}

void LuminanceOptions::setBatchTmPathLdrOutput(QString s)
{
    m_settingHolder->setValue(KEY_BATCH_TM_PATH_OUTPUT, s);
}

int LuminanceOptions::getBatchTmNumThreads()
{
    return m_settingHolder->value(KEY_BATCH_TM_NUM_THREADS, 1).toInt();
}

void LuminanceOptions::setBatchTmNumThreads(int v)
{
    m_settingHolder->setValue(KEY_BATCH_TM_NUM_THREADS, v);
}

namespace
{
#ifdef QT_DEBUG
struct PrintTempDir
{
    PrintTempDir(QString& str):
        str_(str)
    {}

    ~PrintTempDir()
    {
        qDebug() << "Temporary directory: " << str_;
    }

private:
    QString& str_;
};
#endif // QT_DEBUG

}


QString LuminanceOptions::getTempDir()
{
    QString os_temp_dir_name = QDir::temp().absolutePath();
    QString temp_dir_name = m_settingHolder->value(KEY_TEMP_RESULT_PATH,
                                  QDir::temp().absolutePath()).toString();
#ifdef QT_DEBUG
    PrintTempDir print_temp_dir(temp_dir_name);
#endif
    if ( temp_dir_name == os_temp_dir_name )
    {
        // temporary directory is equal to the OS's
        return temp_dir_name;
    }

    QDir temp_dir(temp_dir_name);
    if ( !temp_dir.exists() )
    {
        // directory doesn't exist!
        qDebug() << "Candidate temporary directory does not exist";
        // reset to OS temporary directory;
        temp_dir_name = os_temp_dir_name;
        remove(KEY_TEMP_RESULT_PATH);
        return temp_dir_name;
    }

    // directory exists...
    // let's check whether I can create a file or not!
    QFile file(temp_dir.filePath("test_write.txt"));
    if ( !file.open(QIODevice::ReadWrite) )
    {
        // directory is not writtable
        qDebug() << "Candidate temporary directory is not writtable";
        // reset to OS temporary directory;
        temp_dir_name = os_temp_dir_name;
        remove(KEY_TEMP_RESULT_PATH);
        // return temp_dir_name;
    }

    return temp_dir_name;
}

void LuminanceOptions::setTempDir(QString path)
{
    m_settingHolder->setValue(KEY_TEMP_RESULT_PATH, path);
}

QString LuminanceOptions::getDefaultPathHdrInOut()
{
    return m_settingHolder->value(KEY_RECENT_PATH_LOAD_SAVE_HDR,QDir::currentPath()).toString();
}

void LuminanceOptions::setDefaultPathHdrInOut(QString path)
{
    m_settingHolder->setValue(KEY_RECENT_PATH_LOAD_SAVE_HDR, path);
}

QString LuminanceOptions::getDefaultPathLdrIn()
{
    return m_settingHolder->value(KEY_RECENT_PATH_LOAD_LDRs_FOR_HDR, QDir::currentPath()).toString();
}

void LuminanceOptions::setDefaultPathLdrIn(QString path)
{
    m_settingHolder->setValue(KEY_RECENT_PATH_LOAD_LDRs_FOR_HDR, path);
}

QString LuminanceOptions::getDefaultPathLdrOut()
{
    return m_settingHolder->value(KEY_RECENT_PATH_SAVE_LDR, QDir::currentPath()).toString();
}

void LuminanceOptions::setDefaultPathLdrOut(QString path)
{
    m_settingHolder->setValue(KEY_RECENT_PATH_SAVE_LDR, path);
}

QString LuminanceOptions::getDefaultPathTmoSettings()
{
    return m_settingHolder->value(KEY_RECENT_PATH_LOAD_SAVE_TMO_SETTINGS, QDir::currentPath()).toString();
}

void LuminanceOptions::setDefaultPathTmoSettings(QString path)
{
    m_settingHolder->setValue(KEY_RECENT_PATH_LOAD_SAVE_TMO_SETTINGS, path);
}



/*
settings->beginGroup(GROUP_EXTERNALTOOLS);
//bug 2001032, remove spurious default QString "-a aligned_" value set by ver 1.9.2
if (!settings->contains(KEY_EXTERNAL_AIS_OPTIONS) || settings->value(KEY_EXTERNAL_AIS_OPTIONS).toString()=="-v -a aligned_")
    settings->m_settingHolder->setValue(KEY_EXTERNAL_AIS_OPTIONS, QStringList() << "-v" << "-a" << "aligned_");
align_image_stack_options=settings->value(KEY_EXTERNAL_AIS_OPTIONS).toStringList();
settings->endGroup();
*/
QStringList LuminanceOptions::getAlignImageStackOptions()
{
    return m_settingHolder->value(KEY_EXTERNAL_AIS_OPTIONS, QStringList() << "-v" << "-a" << "aligned_").toStringList();
}

void LuminanceOptions::setAlignImageStackOptions(QStringList qstrlist)
{
    m_settingHolder->setValue(KEY_EXTERNAL_AIS_OPTIONS, qstrlist);
}

bool LuminanceOptions::isShowFirstPageWizard()
{
    return m_settingHolder->value(KEY_WIZARD_SHOWFIRSTPAGE,true).toBool();
}

void LuminanceOptions::setShowFirstPageWizard(bool b)
{
    m_settingHolder->setValue(KEY_WIZARD_SHOWFIRSTPAGE, b);
}

bool LuminanceOptions::isShowFattalWarning()
{
    return m_settingHolder->value(KEY_TMOWARNING_FATTALSMALL,true).toBool();
}

void LuminanceOptions::setShowFattalWarning(bool b)
{
    m_settingHolder->setValue(KEY_TMOWARNING_FATTALSMALL, b);
}

int LuminanceOptions::getMainWindowToolBarMode()
{
    return m_settingHolder->value(KEY_TOOLBAR_MODE, Qt::ToolButtonTextUnderIcon).toInt();
}

void LuminanceOptions::setMainWindowToolBarMode(int mode)
{
    m_settingHolder->setValue(KEY_TOOLBAR_MODE, mode);
}

// Viewer
unsigned int LuminanceOptions::getViewerNanInfColor()
{
    return m_settingHolder->value(KEY_NANINFCOLOR,0xFF000000).toUInt();
}

void LuminanceOptions::setViewerNanInfColor(unsigned int color)
{
    m_settingHolder->setValue(KEY_NANINFCOLOR, color);
}

unsigned int LuminanceOptions::getViewerNegColor()
{
    return m_settingHolder->value(KEY_NEGCOLOR,0xFF000000).toUInt();
}

void LuminanceOptions::setViewerNegColor(unsigned int color)
{
    m_settingHolder->setValue(KEY_NEGCOLOR, color);
}

bool LuminanceOptions::isPreviewPanelActive()
{
    return m_settingHolder->value(KEY_TMOWINDOW_SHOWPREVIEWPANEL, true).toBool();
}

void LuminanceOptions::setPreviewPanelActive(bool status)
{
    m_settingHolder->setValue(KEY_TMOWINDOW_SHOWPREVIEWPANEL, status);
}

bool LuminanceOptions::isRealtimePreviewsActive()
{
    return m_settingHolder->value(KEY_TMOWINDOW_REALTIMEPREVIEWS_ACTIVE, true).toBool();
}

void LuminanceOptions::setRealtimePreviewsActive(bool status)
{
    m_settingHolder->setValue(KEY_TMOWINDOW_REALTIMEPREVIEWS_ACTIVE, status);
}


int  LuminanceOptions::getPreviewWidth()
{
    return m_settingHolder->value(KEY_TMOWINDOW_PREVIEWS_WIDTH, 400).toInt();
}

void LuminanceOptions::setPreviewWidth(int v)
{
    m_settingHolder->setValue(KEY_TMOWINDOW_PREVIEWS_WIDTH, v);
}

QString LuminanceOptions::getCameraProfileFileName()
{
	return m_settingHolder->value(KEY_COLOR_CAMERA_PROFILE_FILENAME, "").toString();
}

void LuminanceOptions::setCameraProfileFileName(QString fname)
{
	m_settingHolder->setValue(KEY_COLOR_CAMERA_PROFILE_FILENAME, fname);
}

QString LuminanceOptions::getMonitorProfileFileName()
{
	return m_settingHolder->value(KEY_COLOR_MONITOR_PROFILE_FILENAME).toString();
}

void LuminanceOptions::setMonitorProfileFileName(QString fname)
{
	m_settingHolder->setValue(KEY_COLOR_MONITOR_PROFILE_FILENAME, fname);
}

QString LuminanceOptions::getPrinterProfileFileName()
{
	return m_settingHolder->value(KEY_COLOR_PRINTER_PROFILE_FILENAME).toString();
}

void LuminanceOptions::setPrinterProfileFileName(QString fname)
{
	m_settingHolder->setValue(KEY_COLOR_PRINTER_PROFILE_FILENAME, fname);
}

int LuminanceOptions::getPreviewPanelMode() // 0 means on the right, 1 on the bottom
{
	return m_settingHolder->value(KEY_PREVIEW_PANEL_MODE).toInt();
}

void LuminanceOptions::setPreviewPanelMode(int mode) 
{
	m_settingHolder->setValue(KEY_PREVIEW_PANEL_MODE, mode);
}
