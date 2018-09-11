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
 * Implement class deriving from QSettings (override Singleton pattern with
 * QSettings functionalities)
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *
 */

#include <QtGlobal>
#include <QApplication>
#include <QDate>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QLocale>
#include <QMessageBox>
#include <QString>
#include <QStyleFactory>

#include "Common/LuminanceOptions.h"
#include "Common/config.h"

#if defined(Q_OS_WIN)
const QString LuminanceOptions::LUMINANCE_HDR_HOME_FOLDER = "LuminanceHDR";
#elif defined(Q_OS_MACOS)
const QString LuminanceOptions::LUMINANCE_HDR_HOME_FOLDER =
    ".config/.LuminanceHDR";
#else
const QString LuminanceOptions::LUMINANCE_HDR_HOME_FOLDER =
    QStringLiteral(".config/luminance-hdr");
#endif

const static QString MAC_THEME = QStringLiteral("Macintosh");

bool LuminanceOptions::isCurrentPortableMode = false;

void LuminanceOptions::checkHomeFolder() {
    if (isCurrentPortableMode) {
        return;
    }

    QDir dir(QDir::homePath());
    if (!dir.exists(LUMINANCE_HDR_HOME_FOLDER)) {
        dir.mkdir(LUMINANCE_HDR_HOME_FOLDER);
    }
}

LuminanceOptions::LuminanceOptions() : QObject() { initSettings(); }

LuminanceOptions::~LuminanceOptions() { delete m_settingHolder; }

void LuminanceOptions::conditionallyDoUpgrade() {
    LuminanceOptions options;
    int currentVersion = options.value(KEY_OPTIONS_VERSION, 0).toInt();

    // check if update needed
    if (currentVersion < LUMINANCEVERSION_NUM) {
        if (currentVersion < 2030099) {
            options.setRawWhiteBalanceMethod(1);
#ifdef DEMOSAICING_GPL3
            options.setRawUserQuality(10);  // AMaZE
#endif
        }
        if (currentVersion < 2040099) {
            options.setAlignImageStackOptions(
                sanitizeAISparams(options.getAlignImageStackOptions()));
        }

        options.setValue(KEY_WARNING_WOW64,
                         0);  // remind the user again with a new version
        options.setValue(KEY_OPTIONS_VERSION, LUMINANCEVERSION_NUM);
    }
}

bool LuminanceOptions::doShowWindowsOnWindows64Message() {
    int currentTimes = value(KEY_WARNING_WOW64, 0).toInt() + 1;
    bool result = currentTimes <= 3;
    if (result) setValue(KEY_WARNING_WOW64, currentTimes);

    return result;
}

void LuminanceOptions::setPortableMode(bool isPortable) {
    if (LuminanceOptions::isCurrentPortableMode != isPortable) {
        QSettings *oldSettings = m_settingHolder;
        LuminanceOptions::isCurrentPortableMode = isPortable;
        initSettings();
        foreach (const QString &key, oldSettings->allKeys()) {
            m_settingHolder->setValue(key, oldSettings->value(key));
        }
        delete oldSettings;

#ifdef Q_OS_MACOS
        QString settingsDirName =
            QGuiApplication::applicationDirPath() + QStringLiteral("/../../../.LuminanceHDR");
        QFile settingsDir(settingsDirName);
        if (!settingsDir.exists()) {
            QDir settingsPath(QGuiApplication::applicationDirPath() + QStringLiteral("/../../../"));
            settingsPath.mkpath(settingsDirName);
        }
#endif
        QString filePath =
#ifdef Q_OS_MACOS
            QGuiApplication::applicationDirPath() + QStringLiteral("/../../../.LuminanceHDR/PortableMode.txt");
#else
            QGuiApplication::applicationDirPath() + QStringLiteral("/PortableMode.txt");
#endif
        QFile file(filePath);
        if (isPortable && !file.exists()) {
            if (file.open(QIODevice::WriteOnly)) file.close();
        } else if (!isPortable && file.exists()) {
            file.remove();
        }
    }
}

bool LuminanceOptions::checkForUpdate() {
    QDate date = value(KEY_UPDATE_CHECKED_ON, QDate(1, 1, 1)).toDate();
    return date < QDate::currentDate();
}

void LuminanceOptions::setUpdateChecked() {
    setValue(KEY_UPDATE_CHECKED_ON, QDate::currentDate());
}

void LuminanceOptions::initSettings() {
    if (LuminanceOptions::isCurrentPortableMode) {
        QString iniFile =
#ifdef Q_OS_MACOS
            QGuiApplication::applicationDirPath() + QStringLiteral("/../../../.LuminanceHDR/settings.ini");
        iniFile = QDir::cleanPath(iniFile);
#else
            QGuiApplication::applicationDirPath() + QStringLiteral("/settings.ini");
#endif
        m_settingHolder =
            new QSettings( iniFile, QSettings::IniFormat);
    }
    else {
        m_settingHolder = new QSettings();
    }
}

void LuminanceOptions::setValue(const QString &key, const QVariant &value) {
    m_settingHolder->setValue(key, value);
}

QVariant LuminanceOptions::value(const QString &key,
                                 const QVariant &defaultValue) const {
    return m_settingHolder->value(key, defaultValue);
}

QString LuminanceOptions::getDatabaseFileName() {
    QString filename;
    if (LuminanceOptions::isCurrentPortableMode) {
        filename = QGuiApplication::applicationDirPath();
#ifdef Q_OS_MACOS
        filename += QLatin1String("/../../../.LuminanceHDR");
        filename = QDir::cleanPath(filename);
#endif
    } else {
        filename = QDir(QDir::homePath()).absolutePath() + "/" +
                   LUMINANCE_HDR_HOME_FOLDER;
    }
    filename += QLatin1String("/saved_parameters.db");

    return filename;
}

QString LuminanceOptions::getFftwWisdomFileName() {
    QString filename;
    if (LuminanceOptions::isCurrentPortableMode) {
        filename = QGuiApplication::applicationDirPath();
    } else {
        filename = QDir(QDir::homePath()).absolutePath() + "/" +
                   LUMINANCE_HDR_HOME_FOLDER;
    }
    filename += QLatin1String("/lhdrwisdom.fftw");

    return filename;
}

QString LuminanceOptions::getGuiTheme() {
#ifdef Q_OS_MACOS
    return m_settingHolder->value(KEY_GUI_THEME, MAC_THEME).toString();
#else
    return m_settingHolder->value(KEY_GUI_THEME, "Fusion").toString();
#endif
}

void LuminanceOptions::setGuiTheme(const QString &s) {
    if (s == MAC_THEME) {
        setGuiDarkMode(false);
    }

    m_settingHolder->setValue(KEY_GUI_THEME, s);
}

bool LuminanceOptions::isGuiDarkMode() const {
    return m_settingHolder->value(KEY_GUI_DARKMODE, false).toBool();
}

void LuminanceOptions::setGuiDarkMode(bool b) {
    m_settingHolder->setValue(KEY_GUI_DARKMODE, b);
}

void LuminanceOptions::applyTheme(bool /*init*/) {
    QString theme = LuminanceOptions().getGuiTheme();
    if (theme.compare(QLatin1String("Macintosh")) != 0 && isGuiDarkMode()) {
        // QPalette darkPalette;
        QPalette darkPalette = QApplication::palette();

        darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::WindowText, Qt::white);
        darkPalette.setColor(QPalette::Disabled, QPalette::WindowText,
                             Qt::lightGray);
        darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
        darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
        darkPalette.setColor(QPalette::ToolTipText, Qt::black);
        darkPalette.setColor(QPalette::Text, Qt::white);
        darkPalette.setColor(QPalette::Disabled, QPalette::Text, Qt::lightGray);
        darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::ButtonText, Qt::white);
        darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText,
                             Qt::lightGray);
        darkPalette.setColor(QPalette::BrightText, Qt::red);
        // darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::Link, QColor(0, 120, 220));

        // darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::Highlight, QColor(0, 120, 220));
        darkPalette.setColor(QPalette::HighlightedText, Qt::black);

        QApplication::setPalette(darkPalette);

        // QApplication::setStyleSheet("QToolTip { color: #ffffff;
        // background-color:
        // #2a82da; border: 1px solid white; }");
    } else {
        QApplication::setPalette(QApplication::palette());
    }

    QApplication::setStyle(
        QStyleFactory::create(LuminanceOptions().getGuiTheme()));
}

// write system default language the first time around (discard "_country")
QString LuminanceOptions::getGuiLang() {
    return m_settingHolder
        ->value(KEY_GUI_LANG, QLocale::system().name().left(2))
        .toString();
}

void LuminanceOptions::setGuiLang(const QString &s) {
    m_settingHolder->setValue(KEY_GUI_LANG, s);
}

bool LuminanceOptions::isRawFourColorRGB() const {
    return m_settingHolder->value(KEY_FOUR_COLOR_RGB, false).toBool();
}

void LuminanceOptions::setRawFourColorRGB(bool b) {
    m_settingHolder->setValue(KEY_FOUR_COLOR_RGB, b);
}

bool LuminanceOptions::isRawDoNotUseFujiRotate() const {
    return m_settingHolder->value(KEY_DO_NOT_USE_FUJI_ROTATE, false).toBool();
}

void LuminanceOptions::setRawDoNotUseFujiRotate(bool b) {
    m_settingHolder->setValue(KEY_DO_NOT_USE_FUJI_ROTATE, b);
}

double LuminanceOptions::getRawAber0() const {
    return m_settingHolder->value(KEY_ABER_0, 1.0).toDouble();
}

void LuminanceOptions::setRawAber0(double v) {
    m_settingHolder->setValue(KEY_ABER_0, v);
}

double LuminanceOptions::getRawAber1() const {
    return m_settingHolder->value(KEY_ABER_1, 1.0).toDouble();
}

void LuminanceOptions::setRawAber1(double v) {
    m_settingHolder->setValue(KEY_ABER_1, v);
}

double LuminanceOptions::getRawAber2() const {
    return m_settingHolder->value(KEY_ABER_2, 1.0).toDouble();
}

void LuminanceOptions::setRawAber2(double v) {
    m_settingHolder->setValue(KEY_ABER_2, v);
}

double LuminanceOptions::getRawAber3() const {
    return m_settingHolder->value(KEY_ABER_3, 1.0).toDouble();
}

void LuminanceOptions::setRawAber3(double v) {
    m_settingHolder->setValue(KEY_ABER_3, v);
}

double LuminanceOptions::getRawGamm0() const {
    return m_settingHolder->value(KEY_GAMM_0, 1.0 / 2.4).toDouble();
}

void LuminanceOptions::setRawGamm0(double v) {
    m_settingHolder->setValue(KEY_GAMM_0, v);
}

double LuminanceOptions::getRawGamm1() const {
    return m_settingHolder->value(KEY_GAMM_1, 12.92).toDouble();
}

void LuminanceOptions::setRawGamm1(double v) {
    m_settingHolder->setValue(KEY_GAMM_1, v);
}

int LuminanceOptions::getRawTemperatureKelvin() const {
    return m_settingHolder->value(KEY_TK, 6500).toInt();
}

void LuminanceOptions::setRawTemperatureKelvin(int v) {
    m_settingHolder->setValue(KEY_TK, v);
}

float LuminanceOptions::getRawGreen() const {
    return m_settingHolder->value(KEY_GREEN, 1.0f).toFloat();
}

void LuminanceOptions::setRawGreen(float v) {
    m_settingHolder->setValue(KEY_GREEN, v);
}

float LuminanceOptions::getRawUserMul0() const {
    return m_settingHolder->value(KEY_USER_MUL_0, 1.0f).toFloat();
}

void LuminanceOptions::setRawUserMul0(float v) {
    m_settingHolder->setValue(KEY_USER_MUL_0, v);
}

float LuminanceOptions::getRawUserMul1() const {
    return m_settingHolder->value(KEY_USER_MUL_1, 1.0f).toFloat();
}

void LuminanceOptions::setRawUserMul1(float v) {
    m_settingHolder->setValue(KEY_USER_MUL_1, v);
}

float LuminanceOptions::getRawUserMul2() const {
    return m_settingHolder->value(KEY_USER_MUL_2, 1.0f).toFloat();
}

void LuminanceOptions::setRawUserMul2(float v) {
    m_settingHolder->setValue(KEY_USER_MUL_2, v);
}

float LuminanceOptions::getRawUserMul3() const {
    return m_settingHolder->value(KEY_USER_MUL_3, 1.0f).toFloat();
}

void LuminanceOptions::setRawUserMul3(float v) {
    m_settingHolder->setValue(KEY_USER_MUL_3, v);
}

bool LuminanceOptions::isRawAutoBrightness() const {
    return m_settingHolder->value(KEY_AUTO_BRIGHT, false).toBool();
}

void LuminanceOptions::setRawAutoBrightness(bool b) {
    m_settingHolder->setValue(KEY_AUTO_BRIGHT, b);
}

float LuminanceOptions::getRawBrightness() const {
    return m_settingHolder->value(KEY_BRIGHTNESS, 1.0f).toFloat();
}

void LuminanceOptions::setRawBrightness(float f) {
    m_settingHolder->setValue(KEY_BRIGHTNESS, f);
}

float LuminanceOptions::getRawNoiseReductionThreshold() const {
    return m_settingHolder->value(KEY_THRESHOLD, 100.0f).toFloat();
}

void LuminanceOptions::setRawNoiseReductionThreshold(float v) {
    m_settingHolder->setValue(KEY_THRESHOLD, v);
}

int LuminanceOptions::getRawHalfSize() const {
    return m_settingHolder->value(KEY_HALF_SIZE, 0).toInt();
}

void LuminanceOptions::setRawHalfSize(int v) {
    m_settingHolder->setValue(KEY_HALF_SIZE, v);
}

int LuminanceOptions::getRawWhiteBalanceMethod() const {
    return m_settingHolder->value(KEY_WB_METHOD, 1).toInt();
}

void LuminanceOptions::setRawWhiteBalanceMethod(int v) {
    m_settingHolder->setValue(KEY_WB_METHOD, v);
}

int LuminanceOptions::getRawOutputColor() const {
    return m_settingHolder->value(KEY_OUTPUT_COLOR, 1).toInt();
}

// double check those!
QString LuminanceOptions::getRawOutputProfile() const {
    // QFile::encodeName(this->value(KEY_OUTPUT_PROFILE).toString()).constData();
    return QFile::encodeName(
        m_settingHolder->value(KEY_OUTPUT_PROFILE).toString());
}

void LuminanceOptions::setRawOutputProfile(const QString &v) {
    m_settingHolder->setValue(KEY_OUTPUT_PROFILE, v);
}

QString LuminanceOptions::getRawCameraProfile() const {
    // QFile::encodeName(this->value(KEY_CAMERA_PROFILE).toString()).constData();
    return QFile::encodeName(
        m_settingHolder->value(KEY_CAMERA_PROFILE).toString());
}

void LuminanceOptions::setRawCameraProfile(const QString &v) {
    m_settingHolder->setValue(KEY_CAMERA_PROFILE, v);
}

int LuminanceOptions::getRawUserFlip() const {
    return m_settingHolder->value(KEY_USER_FLIP, 0).toInt();
}

#ifdef DEMOSAICING_GPL3
#define USER_QUALITY 10  // using  AMaZE interpolation
#elif DEMOSAICING_GPL2
#define USER_QUALITY 5  // using AHDv2
#else
#define USER_QUALITY 3  // using AHD
#endif

int LuminanceOptions::getRawUserQuality() const {
    return m_settingHolder->value(KEY_USER_QUAL, USER_QUALITY).toInt();
}

void LuminanceOptions::setRawUserQuality(int v) {
    m_settingHolder->setValue(KEY_USER_QUAL, v);
}

int LuminanceOptions::getRawUserSaturation() const {
    return m_settingHolder->value(KEY_USER_SAT, 20000).toInt();
}

void LuminanceOptions::setRawUserSaturation(int v) {
    m_settingHolder->setValue(KEY_USER_SAT, v);
}

int LuminanceOptions::getRawMedPasses() const {
    return m_settingHolder->value(KEY_MED_PASSES, 0).toInt();
}

void LuminanceOptions::setRawMedPasses(int v) {
    m_settingHolder->setValue(KEY_MED_PASSES, v);
}

int LuminanceOptions::getRawHighlightsMode() const {
    return m_settingHolder->value(KEY_HIGHLIGHTS, 0).toInt();
}

void LuminanceOptions::setRawHighlightsMode(int v) {
    m_settingHolder->setValue(KEY_HIGHLIGHTS, v);
}

int LuminanceOptions::getRawLevel() const {
    return m_settingHolder->value(KEY_LEVEL, 0).toInt();
}

void LuminanceOptions::setRawLevel(int v) {
    m_settingHolder->setValue(KEY_LEVEL, v);
}

float LuminanceOptions::getRawAutoBrightnessThreshold() const {
    return m_settingHolder->value(KEY_AUTO_BRIGHT_THR, 0.001f).toFloat();
}

void LuminanceOptions::setRawAutoBrightnessThreshold(float v) {
    m_settingHolder->setValue(KEY_AUTO_BRIGHT_THR, v);
}

float LuminanceOptions::getRawMaximumThreshold() const {
    return m_settingHolder->value(KEY_ADJUST_MAXIMUM_THR, 0.0f).toFloat();
}

void LuminanceOptions::setRawMaximumThreshold(float v) {
    m_settingHolder->setValue(KEY_ADJUST_MAXIMUM_THR, v);
}

bool LuminanceOptions::isRawUseBlack() const {
    return m_settingHolder->value(KEY_USE_BLACK, false).toBool();
}

void LuminanceOptions::setRawUseBlack(bool b) {
    m_settingHolder->setValue(KEY_USE_BLACK, b);
}

int LuminanceOptions::getRawUserBlack() const {
    return m_settingHolder->value(KEY_USER_BLACK, 0).toInt();
}

void LuminanceOptions::setRawUserBlack(int v) {
    m_settingHolder->setValue(KEY_USER_BLACK, v);
}

bool LuminanceOptions::isRawUseSaturation() const {
    return m_settingHolder->value(KEY_USE_SAT, false).toBool();
}

void LuminanceOptions::setRawUseSaturation(bool b) {
    m_settingHolder->setValue(KEY_USE_SAT, b);
}

bool LuminanceOptions::isRawUseNoiseReduction() const {
    return m_settingHolder->value(KEY_USE_NOISE, true).toBool();
}

void LuminanceOptions::setRawUseNoiseReduction(bool b) {
    m_settingHolder->setValue(KEY_USE_NOISE, b);
}

bool LuminanceOptions::isRawUseChromaAber() const {
    return m_settingHolder->value(KEY_USE_CHROMA, false).toBool();
}

void LuminanceOptions::setRawUseChromaAber(bool b) {
    m_settingHolder->setValue(KEY_USE_CHROMA, b);
}

QString LuminanceOptions::getBatchHdrPathInput(const QString &defaultPath) {
    return m_settingHolder->value(KEY_BATCH_HDR_PATH_INPUT, defaultPath)
        .toString();
}

void LuminanceOptions::setBatchHdrPathInput(const QString &qstr) {
    m_settingHolder->setValue(KEY_BATCH_HDR_PATH_INPUT, qstr);
}

QString LuminanceOptions::getBatchHdrPathOutput(const QString &defaultPath) {
    return m_settingHolder->value(KEY_BATCH_HDR_PATH_OUTPUT, defaultPath)
        .toString();
}

void LuminanceOptions::setBatchHdrPathOutput(const QString &qstr) {
    m_settingHolder->setValue(KEY_BATCH_HDR_PATH_OUTPUT, qstr);
}

QString LuminanceOptions::getBatchTmPathHdrInput() {
    return m_settingHolder->value(KEY_BATCH_TM_PATH_INPUT, QDir::currentPath())
        .toString();
}

void LuminanceOptions::setBatchTmPathHdrInput(const QString &s) {
    m_settingHolder->setValue(KEY_BATCH_TM_PATH_INPUT, s);
}

QString LuminanceOptions::getBatchTmPathTmoSettings() {
    return m_settingHolder
        ->value(KEY_BATCH_TM_PATH_TMO_SETTINGS, QDir::currentPath())
        .toString();
}

void LuminanceOptions::setBatchTmPathTmoSettings(const QString &s) {
    m_settingHolder->setValue(KEY_BATCH_TM_PATH_TMO_SETTINGS, s);
}

QString LuminanceOptions::getBatchTmPathLdrOutput() {
    return m_settingHolder->value(KEY_BATCH_TM_PATH_OUTPUT, QDir::currentPath())
        .toString();
}

void LuminanceOptions::setBatchTmPathLdrOutput(const QString &s) {
    m_settingHolder->setValue(KEY_BATCH_TM_PATH_OUTPUT, s);
}

int LuminanceOptions::getBatchTmNumThreads() {
    return m_settingHolder->value(KEY_BATCH_TM_NUM_THREADS, 1).toInt();
}

void LuminanceOptions::setBatchTmNumThreads(int v) {
    m_settingHolder->setValue(KEY_BATCH_TM_NUM_THREADS, v);
}

namespace {
#ifdef QT_DEBUG
struct PrintTempDir {
    explicit PrintTempDir(const QString &str) : str_(str) {}

    ~PrintTempDir() { qDebug() << "Temporary directory: " << str_; }

   private:
    const QString &str_;
};
#endif  // QT_DEBUG
}

QString LuminanceOptions::getTempDir() {
    QString os_temp_dir_name = QDir::temp().absolutePath();
    QString temp_dir_name =
        m_settingHolder
            ->value(KEY_TEMP_RESULT_PATH, QDir::temp().absolutePath())
            .toString();
#ifdef QT_DEBUG
    PrintTempDir print_temp_dir(temp_dir_name);
#endif
    if (temp_dir_name == os_temp_dir_name) {
        // temporary directory is equal to the OS's
        return temp_dir_name;
    }

    QDir temp_dir(temp_dir_name);
    if (!temp_dir.exists()) {
        // directory doesn't exist!
        qDebug() << "Candidate temporary directory does not exist";
        // reset to OS temporary directory;
        temp_dir_name = os_temp_dir_name;
        remove(KEY_TEMP_RESULT_PATH);
        return temp_dir_name;
    }

    // directory exists...
    // let's check whether I can create a file or not!
    QFile file(temp_dir.filePath(QStringLiteral("test_write.txt")));
    if (!file.open(QIODevice::ReadWrite)) {
        // directory is not writtable
        qDebug() << "Candidate temporary directory is not writtable";
        // reset to OS temporary directory;
        temp_dir_name = os_temp_dir_name;
        remove(KEY_TEMP_RESULT_PATH);
        // return temp_dir_name;
    }

    return temp_dir_name;
}

void LuminanceOptions::setTempDir(const QString &path) {
    m_settingHolder->setValue(KEY_TEMP_RESULT_PATH, path);
}

QString LuminanceOptions::getDefaultPathHdrIn() {
    return m_settingHolder->value(KEY_RECENT_PATH_LOAD_HDR, QDir::currentPath())
        .toString();
}

void LuminanceOptions::setDefaultPathHdrIn(const QString &path) {
    m_settingHolder->setValue(KEY_RECENT_PATH_LOAD_HDR, path);
}

QString LuminanceOptions::getDefaultPathHdrOut() {
    return m_settingHolder->value(KEY_RECENT_PATH_SAVE_HDR, QDir::currentPath())
        .toString();
}

void LuminanceOptions::setDefaultPathHdrOut(const QString &path) {
    m_settingHolder->setValue(KEY_RECENT_PATH_SAVE_HDR, path);
}

QString LuminanceOptions::getDefaultPathLdrIn() {
    return m_settingHolder->value(KEY_RECENT_PATH_LOAD_LDR, QDir::currentPath())
        .toString();
}

void LuminanceOptions::setDefaultPathLdrIn(const QString &path) {
    m_settingHolder->setValue(KEY_RECENT_PATH_LOAD_LDR, path);
}

QString LuminanceOptions::getDefaultPathLdrOut() {
    return m_settingHolder
        ->value(KEY_RECENT_PATH_SAVE_LDR, getDefaultPathHdrOut())
        .toString();
}

void LuminanceOptions::setDefaultPathLdrOut(const QString &path) {
    m_settingHolder->setValue(KEY_RECENT_PATH_SAVE_LDR, path);
}

bool LuminanceOptions::isShowMissingEVsWarning() {
    return m_settingHolder->value(KEY_WIZARD_SHOW_MISSING_EVS_WARNING, true)
        .toBool();
}

void LuminanceOptions::setShowMissingEVsWarning(const bool b) {
    m_settingHolder->setValue(KEY_WIZARD_SHOW_MISSING_EVS_WARNING, b);
}

QString LuminanceOptions::getDefaultPathTmoSettings() {
    return m_settingHolder
        ->value(KEY_RECENT_PATH_LOAD_SAVE_TMO_SETTINGS, QDir::currentPath())
        .toString();
}

void LuminanceOptions::setDefaultPathTmoSettings(const QString &path) {
    m_settingHolder->setValue(KEY_RECENT_PATH_LOAD_SAVE_TMO_SETTINGS, path);
}

/*
settings->beginGroup(GROUP_EXTERNALTOOLS);
//bug 2001032, remove spurious default QString "-a aligned_" value set by ver
1.9.2
if (!settings->contains(KEY_EXTERNAL_AIS_OPTIONS) ||
settings->value(KEY_EXTERNAL_AIS_OPTIONS).toString()=="-v -a aligned_")
    settings->m_settingHolder->setValue(KEY_EXTERNAL_AIS_OPTIONS, QStringList()
<< "-v" << "-a" << "aligned_");
align_image_stack_options=settings->value(KEY_EXTERNAL_AIS_OPTIONS).toStringList();
settings->endGroup();
*/
QStringList LuminanceOptions::getAlignImageStackOptions() {
    // return m_settingHolder->value(KEY_EXTERNAL_AIS_OPTIONS,
    //                              QStringList() << "-v" <<
    //                              "aligned_").toStringList();
    return m_settingHolder
        ->value(KEY_EXTERNAL_AIS_OPTIONS, QStringList() << QStringLiteral("-v"))
        .toStringList();
}

QStringList LuminanceOptions::sanitizeAISparams(QStringList temp_ais_options,
                                                bool verbose) {
    bool align_opt_was_ok = true;

    // check that we don't have '-a "aligned_"'
    int idx_a = temp_ais_options.indexOf(QStringLiteral("-a"));

    if (idx_a != -1) {
        if (idx_a != temp_ais_options.size() - 1 &&
            !temp_ais_options.at(idx_a + 1).startsWith(QLatin1String("-"))) {
            temp_ais_options.removeAt(idx_a + 1);
        }
        temp_ais_options.removeAt(idx_a);

        align_opt_was_ok = false;
    }

    // check if we have '-v'
    if (temp_ais_options.indexOf(QStringLiteral("-v")) < 0) {
        temp_ais_options.insert(0, QStringLiteral("-v"));
        align_opt_was_ok = false;
    }

    if (verbose && !align_opt_was_ok) {
        QMessageBox::information(
            0, QObject::tr("Option -v -a..."),
            QObject::tr(
                "LuminanceHDR requires align_image_stack to be executed "
                "with the \"-v\" and without the \"-a\" options. Command "
                "line options have been corrected."));
    }
    return temp_ais_options;
}

void LuminanceOptions::setAlignImageStackOptions(const QStringList &qstrlist,
                                                 bool verbose) {
    m_settingHolder->setValue(KEY_EXTERNAL_AIS_OPTIONS,
                              sanitizeAISparams(qstrlist, verbose));
}

bool LuminanceOptions::isShowFattalWarning() {
    return m_settingHolder->value(KEY_TMOWARNING_FATTALSMALL, true).toBool();
}

void LuminanceOptions::setShowFattalWarning(const bool b) {
    m_settingHolder->setValue(KEY_TMOWARNING_FATTALSMALL, b);
}

int LuminanceOptions::getMainWindowToolBarMode() {
    return m_settingHolder->value(KEY_TOOLBAR_MODE, Qt::ToolButtonTextUnderIcon)
        .toInt();
}

void LuminanceOptions::setMainWindowToolBarMode(int mode) {
    m_settingHolder->setValue(KEY_TOOLBAR_MODE, mode);
}

bool LuminanceOptions::isPreviewPanelActive() {
    return m_settingHolder->value(KEY_TMOWINDOW_SHOWPREVIEWPANEL, true)
        .toBool();
}

void LuminanceOptions::setPreviewPanelActive(bool status) {
    m_settingHolder->setValue(KEY_TMOWINDOW_SHOWPREVIEWPANEL, status);
}

bool LuminanceOptions::isRealtimePreviewsActive() {
    return m_settingHolder->value(KEY_TMOWINDOW_REALTIMEPREVIEWS_ACTIVE, false)
        .toBool();
}

void LuminanceOptions::setRealtimePreviewsActive(bool status) {
    m_settingHolder->setValue(KEY_TMOWINDOW_REALTIMEPREVIEWS_ACTIVE, status);
}

int LuminanceOptions::getPreviewWidth() {
    return m_settingHolder->value(KEY_TMOWINDOW_PREVIEWS_WIDTH, 400).toInt();
}

void LuminanceOptions::setPreviewWidth(int v) {
    m_settingHolder->setValue(KEY_TMOWINDOW_PREVIEWS_WIDTH, v);
}

QString LuminanceOptions::getCameraProfileFileName() {
    return m_settingHolder->value(KEY_COLOR_CAMERA_PROFILE_FILENAME, "")
        .toString();
}

void LuminanceOptions::setCameraProfileFileName(const QString &fname) {
    m_settingHolder->setValue(KEY_COLOR_CAMERA_PROFILE_FILENAME, fname);
}

QString LuminanceOptions::getMonitorProfileFileName() {
    return m_settingHolder->value(KEY_COLOR_MONITOR_PROFILE_FILENAME)
        .toString();
}

void LuminanceOptions::setMonitorProfileFileName(const QString &fname) {
    m_settingHolder->setValue(KEY_COLOR_MONITOR_PROFILE_FILENAME, fname);
}

QString LuminanceOptions::getPrinterProfileFileName() {
    return m_settingHolder->value(KEY_COLOR_PRINTER_PROFILE_FILENAME)
        .toString();
}

void LuminanceOptions::setPrinterProfileFileName(const QString &fname) {
    m_settingHolder->setValue(KEY_COLOR_PRINTER_PROFILE_FILENAME, fname);
}

int LuminanceOptions::getPreviewPanelMode()  // 0 means on the right, 1 on the
                                             // bottom
{
    return m_settingHolder->value(KEY_PREVIEW_PANEL_MODE).toInt();
}

void LuminanceOptions::setPreviewPanelMode(int mode) {
    m_settingHolder->setValue(KEY_PREVIEW_PANEL_MODE, mode);
}

void LuminanceOptions::setExportDir(const QString &dir) {
    m_settingHolder->setValue(KEY_EXPORT_FILE_PATH, dir);
}

QString LuminanceOptions::getExportDir() {
    QString path = m_settingHolder->value(KEY_EXPORT_FILE_PATH).toString();
    if (path.isEmpty()) {
        path = QDir::homePath();
    } else {
        QDir dir(path);
        if (!dir.exists()) {
            path = QDir::homePath();
        }
    }
    return path;
}
