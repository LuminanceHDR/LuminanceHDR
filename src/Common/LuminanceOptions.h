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
 * Implement class deriving from QSettings (override Singleton pattern with
 * QSettings functionalities)
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *
 */

#ifndef LUMINANCEOPTIONS_H
#define LUMINANCEOPTIONS_H

#include <QDir>
#include <QSettings>
#include <QString>
#include <QStringList>

class LuminanceOptions : public QObject {
    Q_OBJECT
   public:
    static const QString LUMINANCE_HDR_HOME_FOLDER;
    static bool isCurrentPortableMode;
    static void checkHomeFolder();

    explicit LuminanceOptions();
    ~LuminanceOptions();

    void setValue(const QString &key, const QVariant &value);
    QVariant value(const QString &key,
                   const QVariant &defaultValue = QVariant()) const;

    static void conditionallyDoUpgrade();

    bool doShowWindowsOnWindows64Message();

    QString getDatabaseFileName();
    void setPortableMode(bool isPortable);

    bool checkForUpdate();
    void setUpdateChecked();

   public Q_SLOTS:
    // RAW settings
    bool isRawFourColorRGB() const;
    void setRawFourColorRGB(bool);
    bool isRawDoNotUseFujiRotate() const;
    void setRawDoNotUseFujiRotate(bool);

    // Chromatic Aberation
    bool isRawUseChromaAber() const;
    void setRawUseChromaAber(bool);
    double getRawAber0() const;
    void setRawAber0(double);
    double getRawAber1() const;
    void setRawAber1(double);
    double getRawAber2() const;
    void setRawAber2(double);
    double getRawAber3() const;
    void setRawAber3(double);
    double getRawGamm0() const;
    void setRawGamm0(double);
    double getRawGamm1() const;
    void setRawGamm1(double);
    // ---
    // White Balance
    int getRawWhiteBalanceMethod() const;
    void setRawWhiteBalanceMethod(int);
    int getRawTemperatureKelvin() const;
    void setRawTemperatureKelvin(int);
    float getRawGreen() const;
    void setRawGreen(float);
    float getRawUserMul0() const;
    void setRawUserMul0(float);
    float getRawUserMul1() const;
    void setRawUserMul1(float);
    float getRawUserMul2() const;
    void setRawUserMul2(float);
    float getRawUserMul3() const;
    void setRawUserMul3(float);
    // ----
    // Brightness
    bool isRawAutoBrightness() const;
    void setRawAutoBrightness(bool);
    float getRawAutoBrightnessThreshold() const;
    void setRawAutoBrightnessThreshold(float);
    float getRawBrightness() const;
    void setRawBrightness(float f);
    // ---
    // Noise Reduction
    // ---
    int getRawHalfSize() const;
    void setRawHalfSize(int);
    int getRawOutputColor() const;
    // void    setRawOutputColor(int);
    QString getRawOutputProfile() const;
    void setRawOutputProfile(const QString &);
    QString getRawCameraProfile() const;
    void setRawCameraProfile(const QString &);
    int getRawUserFlip() const;
    // void    setRawUserFlip(int);
    int getRawUserQuality() const;
    void setRawUserQuality(int);
    int getRawMedPasses() const;
    void setRawMedPasses(int);
    // Highlights
    int getRawHighlightsMode() const;
    void setRawHighlightsMode(int);
    int getRawLevel() const;  // reconstruction level
    void setRawLevel(int);
    // ---
    float getRawMaximumThreshold() const;
    void setRawMaximumThreshold(float);

    // Black/White Point
    bool isRawUseBlack() const;
    void setRawUseBlack(bool);
    int getRawUserBlack() const;
    void setRawUserBlack(int);
    bool isRawUseSaturation() const;
    void setRawUseSaturation(bool);
    int getRawUserSaturation() const;
    void setRawUserSaturation(int);
    // ---
    // Noise Reduction
    bool isRawUseNoiseReduction() const;
    void setRawUseNoiseReduction(bool);
    float getRawNoiseReductionThreshold() const;
    void setRawNoiseReductionThreshold(float);
    // ---

    QString getGuiTheme();
    void setGuiTheme(const QString &);
    bool isGuiDarkMode() const;
    void setGuiDarkMode(bool);
    void applyTheme(bool init);

    // Language
    // 2-chars ISO 639 language code for Luminance's user interface
    QString getGuiLang();
    void setGuiLang(const QString &);

    // Batch HDR
    QString getBatchHdrPathInput(
        const QString &defaultPath = QDir::currentPath());
    QString getBatchHdrPathOutput(
        const QString &defaultPath = QDir::currentPath());
    void setBatchHdrPathInput(const QString &);
    void setBatchHdrPathOutput(const QString &);

    // Batch TM
    QString getBatchTmPathHdrInput();
    QString getBatchTmPathTmoSettings();
    QString getBatchTmPathLdrOutput();
    int getBatchTmNumThreads();

    void setBatchTmPathHdrInput(const QString &);
    void setBatchTmPathTmoSettings(const QString &);
    void setBatchTmPathLdrOutput(const QString &);
    void setBatchTmNumThreads(int);

    int getNumThreads() { return getBatchTmNumThreads(); }
    void setNumThreads(int i) { setBatchTmNumThreads(i); }

    // Default Paths
    // Path to save temporary cached files
    QString getTempDir();
    QString getDefaultPathHdrIn();   // MainWindow
    QString getDefaultPathHdrOut();  // MainWindow
    QString getDefaultPathLdrIn();   // HdrWizard
    QString getDefaultPathLdrOut();  // MainWindow
    QString getDefaultPathTmoSettings();

    void setTempDir(const QString &);
    void setDefaultPathHdrIn(const QString &);
    void setDefaultPathHdrOut(const QString &);
    void setDefaultPathLdrIn(const QString &);  // HdrWizard
    void setDefaultPathLdrOut(const QString &);
    void setDefaultPathTmoSettings(const QString &);

    // HdrWizard
    // commandline options for align_image_stack
    QStringList getAlignImageStackOptions();
    void setAlignImageStackOptions(const QStringList &, bool verbose = false);

    bool isShowFattalWarning();
    void setShowFattalWarning(const bool b);

    bool isShowMissingEVsWarning();
    void setShowMissingEVsWarning(const bool b);

    // MainWindow
    int getMainWindowToolBarMode();
    void setMainWindowToolBarMode(int);

    // Preview Panel
    bool isPreviewPanelActive();
    void setPreviewPanelActive(bool);

    int getPreviewWidth();
    void setPreviewWidth(int);

    // Realtime Previews
    bool isRealtimePreviewsActive();
    void setRealtimePreviewsActive(bool);

    // Color Management
    QString getCameraProfileFileName();
    void setCameraProfileFileName(const QString &);

    QString getMonitorProfileFileName();
    void setMonitorProfileFileName(const QString &);

    QString getPrinterProfileFileName();
    void setPrinterProfileFileName(const QString &);

    int getPreviewPanelMode();
    void setPreviewPanelMode(int);

    // Queue
    QString getExportDir();
    void setExportDir(const QString &dir);

   private:
    void initSettings();
    QSettings *m_settingHolder;

    static QStringList sanitizeAISparams(QStringList temp_ais_options,
                                         bool verbose = false);
};

#endif
