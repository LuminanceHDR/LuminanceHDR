/**
 * This file is a part of Luminance HDR package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2006,2007 Giuseppe Rota
 * Copyright (C) 2011 Davide Anastasia
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

#ifndef LUMINANCEOPTIONS_H
#define LUMINANCEOPTIONS_H

#include <QSettings>
#include <QString>
#include <QStringList>

#include "Common/config.h"

class LuminanceOptions: public QSettings
{
    Q_OBJECT
public:
    LuminanceOptions():
        QSettings(LUMINANCEORGANIZATION, LUMINANCEAPPLICATION) { }

public Q_SLOTS:
    // RAW settings
    bool    isRawFourColorRGB();
    void    setRawFourColorRGB(bool);
    bool    isRawDoNotUseFujiRotate();
    void    setRawDoNotUseFujiRotate(bool);
    double  getRawAber0();
    void    setRawAber0(double);
    double  getRawAber1();
    void    setRawAber1(double);
    double  getRawAber2();
    void    setRawAber2(double);
    double  getRawAber3();
    void    setRawAber3(double);
    double  getRawGamm0();
    void    setRawGamm0(double);
    double  getRawGamm1();
    void    setRawGamm1(double);
    int     getRawTemperatureKelvin();
    void    setRawTemperatureKelvin(int);
    float   getRawGreen();
    void    setRawGreen(float);
    float   getRawUserMul0();
    void    setRawUserMul0(float);
    float   getRawUserMul1();
    void    setRawUserMul1(float);
    float   getRawUserMul2();
    void    setRawUserMul2(float);
    float   getRawUserMul3();
    void    setRawUserMul3(float);
    bool    isRawAutoBrightness();
    void    setRawAutoBrightness(bool);
    float   getRawBrightness();
    void    setRawBrightness(float f);
    int     getRawHalfSize();
    void    setRawHalfSize(int);
    int     getRawWhiteBalanceMethod();
    void    setRawWhiteBalanceMethod(int);
    int     getRawOutputColor();
    //void    setRawOutputColor(int);
    QString getRawOutputProfile();
    void    setRawOutputProfile(QString);
    QString getRawCameraProfile();
    void    setRawCameraProfile(QString);
    int     getRawUserFlip();
    //void    setRawUserFlip(int);
    int     getRawUserQuality();
    void    setRawUserQuality(int);
    int     getRawMedPasses();
    void    setRawMedPasses(int);
    int     getRawHighlightsMode();
    void    setRawHighlightsMode(int);
    int     getRawLevel();
    void    setRawLevel(int);
    float   getRawBrightnessThreshold();
    void    setRawBrightnessThreshold(float);
    float   getRawMaximumThreshold();
    void    setRawMaximumThreshold(float);
    bool    isRawUseBlack();
    void    setRawUseBlack(bool);
    int     getRawUserBlack();
    void    setRawUserBlack(int);
    bool    isRawUseSaturation();
    void    setRawUseSaturation(bool);
    int     getRawUserSaturation();
    void    setRawUserSaturation(int);
    bool    isRawUseNoiseReduction();
    void    setRawUseNoiseReduction(bool);
    float   getRawNoiseReductionThreshold();
    void    setRawNoiseReductionThreshold(float);
    bool    isRawUseChroma();
    void    setRawUseChroma(bool);

    // Language
    // 2-chars ISO 639 language code for Luminance's user interface
    QString getGuiLang();
    void    setGuiLang(QString);

    // Batch HDR
    QString getBatchHdrPathInput();
    QString getBatchHdrPathOutput();
    void    setBatchHdrPathInput(QString);
    void    setBatchHdrPathOutput(QString);

    // Batch TM
    QString getBatchTmPathHdrInput();
    QString getBatchTmPathTmoSettings();
    QString getBatchTmPathLdrOutput();
    int     getBatchTmNumThreads();
    QString getBatchTmLdrFormat();

    void    setBatchTmPathHdrInput(QString);
    void    setBatchTmPathTmoSettings(QString);
    void    setBatchTmPathLdrOutput(QString);
    void    setBatchTmNumThreads(int);
    void    setBatchTmLdrFormat(QString);

    int     getNumThreads() { return getBatchTmNumThreads(); }
    void    setNumThreads(int i) { setBatchTmNumThreads(i); }

    // TIFF mode
    // if true, we save a logluv tiff (if false a uncompressed 32 bit tiff)
    bool    isSaveLogLuvTiff();
    void    setSaveLogLuvTiff(bool);

    // Default Paths
    // Path to save temporary cached files
    QString getTempDir();
    QString getDefaultPathHdrInOut();
    QString getDefaultPathLdrIn(); // HdrWizard
    QString getDefaultPathLdrOut();
    QString getDefaultPathTmoSettings();

    void    setTempDir(QString path);
    void    setDefaultPathHdrInOut(QString);
    void    setDefaultPathLdrIn(QString); // HdrWizard
    void    setDefaultPathLdrOut(QString);
    void    setDefaultPathTmoSettings(QString);

    // HdrWizard
    // commandline options for align_image_stack
    QStringList getAlignImageStackOptions();
    void        setAlignImageStackOptions(QStringList);
    // if true always show first page of hdr wizard
    bool    isShowFirstPageWizard();
    void    setShowFirstPageWizard(bool b);

    bool    isShowFattalWarning();
    void    setShowFattalWarning(bool b);

    // MainWindow
    int     getMainWindowToolBarMode();
    void    setMainWindowToolBarMode(int);

    // Viewer
    unsigned int getViewerNanInfColor();
    unsigned int getViewerNegColor();
    void setViewerNanInfColor(unsigned int);
    void setViewerNegColor(unsigned int);

    // Preview Panel
    bool    isPreviewPanelActive();
    void    setPreviewPanelActive(bool);

    int     getPreviewWidth();
    void    setPreviewWidth(int);
};

#endif
