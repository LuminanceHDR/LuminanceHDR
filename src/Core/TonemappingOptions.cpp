/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
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
 * This class hold the Tonemapping Options
 *
 * Original Work
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 * New Design based on class rather then struct
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *
 */

#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <climits>

#include <Common/config.h>
#include <Core/TonemappingOptions.h>
#include <TonemappingOperators/pfstmdefaultparams.h>

void TonemappingOptions::setDefaultTonemapParameters() {
    // Mantiuk06
    operator_options.mantiuk06options.contrastfactor =
        MANTIUK06_CONTRAST_FACTOR;
    operator_options.mantiuk06options.saturationfactor =
        MANTIUK06_SATURATION_FACTOR;
    operator_options.mantiuk06options.detailfactor = MANTIUK06_DETAIL_FACTOR;
    operator_options.mantiuk06options.contrastequalization =
        MANTIUK06_CONTRAST_EQUALIZATION;

    // Mantiuk08
    operator_options.mantiuk08options.colorsaturation =
        MANTIUK08_COLOR_SATURATION;
    operator_options.mantiuk08options.contrastenhancement =
        MANTIUK08_CONTRAST_ENHANCEMENT;
    operator_options.mantiuk08options.luminancelevel =
        MANTIUK08_LUMINANCE_LEVEL;
    operator_options.mantiuk08options.setluminance = MANTIUK08_SET_LUMINANCE;

    // Fattal
    operator_options.fattaloptions.alpha = FATTAL02_ALPHA;
    operator_options.fattaloptions.beta = FATTAL02_BETA;
    operator_options.fattaloptions.color = FATTAL02_COLOR;
    operator_options.fattaloptions.noiseredux = FATTAL02_NOISE_REDUX;
    operator_options.fattaloptions.newfattal = FATTAL02_NEWFATTAL;
    operator_options.fattaloptions.fftsolver = true;

    // Ferradans
    operator_options.ferradansoptions.rho = FERRADANS11_RHO;
    operator_options.ferradansoptions.inv_alpha = FERRADANS11_INV_ALPHA;

    // Ferwerda
    operator_options.ferwerdaoptions.multiplier = FERWERDA96_MULTIPLIER;
    operator_options.ferwerdaoptions.adaptationluminance = FERWERDA96_ADAPTATION_LUMINANCE;

    // KimKautz
    operator_options.kimkautzoptions.c1 = KIMKAUTZ08_C1;
    operator_options.kimkautzoptions.c2 = KIMKAUTZ08_C2;

    // Drago
    operator_options.dragooptions.bias = DRAGO03_BIAS;

    // Durand
    operator_options.durandoptions.spatial = DURAND02_SPATIAL;
    operator_options.durandoptions.range = DURAND02_RANGE;
    operator_options.durandoptions.base = DURAND02_BASE;

    // Reinhard 02
    operator_options.reinhard02options.scales = REINHARD02_SCALES;
    operator_options.reinhard02options.key = REINHARD02_KEY;
    operator_options.reinhard02options.phi = REINHARD02_PHI;
    operator_options.reinhard02options.range = REINHARD02_RANGE;
    operator_options.reinhard02options.lower = REINHARD02_LOWER;
    operator_options.reinhard02options.upper = REINHARD02_UPPER;

    // Reinhard 05
    operator_options.reinhard05options.brightness = REINHARD05_BRIGHTNESS;
    operator_options.reinhard05options.chromaticAdaptation =
        REINHARD05_CHROMATIC_ADAPTATION;
    operator_options.reinhard05options.lightAdaptation =
        REINHARD05_LIGHT_ADAPTATION;

    // Ashikhmin
    operator_options.ashikhminoptions.simple = ASHIKHMIN_SIMPLE;
    operator_options.ashikhminoptions.eq2 = ASHIKHMIN_EQ2;
    operator_options.ashikhminoptions.lct = ASHIKHMIN_LCT;

    // Pattanaik
    operator_options.pattanaikoptions.autolum = PATTANAIK00_AUTOLUM;
    operator_options.pattanaikoptions.local = PATTANAIK00_LOCAL;
    operator_options.pattanaikoptions.cone = PATTANAIK00_CONE;
    operator_options.pattanaikoptions.rod = PATTANAIK00_ROD;
    operator_options.pattanaikoptions.multiplier = PATTANAIK00_MULTIPLIER;

    // VanHateren
    operator_options.vanhaterenoptions.pupil_area =  VANHATEREN06_PUPIL_AREA;
}

void TonemappingOptions::setDefaultParameters() {
    // TM Defaults
    setDefaultTonemapParameters();

    origxsize = INT_MAX;
    xsize = INT_MAX;
    xsize_percent = 100;
    quality = 100;
    pregamma = 1.0f;
    postgamma = 1.0f;
    postsaturation = 1.0f;
    tonemapSelection = false;
    tmoperator = mantiuk06;

    selection_x_up_left = 0;
    selection_y_up_left = 0;
    selection_x_bottom_right = INT_MAX;
    selection_y_bottom_right = INT_MAX;
}

char TonemappingOptions::getRatingForOperator() {
    switch (tmoperator) {
        case ashikhmin:
            return 'H';
        case drago:
            return 'G';
        case durand:
            return 'F';
        case fattal:
            return 'B';
        case mantiuk06:
            return 'A';
        case mantiuk08:
            return 'C';
        case pattanaik:
            return 'I';
        case reinhard02:
            return 'E';
        case reinhard05:
            return 'D';
        case ferradans:
            return 'J';
        case mai:
            return 'K';
        case ferwerda:
            return 'L';
        case kimkautz:
            return 'M';
        case vanhateren:
            return 'N';
    }
    return ' ';
}

const QString TonemappingOptions::getPostfix() {
    QString postfix = QStringLiteral("pregamma_%1_").arg(pregamma);
    switch (tmoperator) {
        case mantiuk06: {
            postfix += QLatin1String("mantiuk06_");
            float contrastfactor =
                operator_options.mantiuk06options.contrastfactor;
            float saturationfactor =
                operator_options.mantiuk06options.saturationfactor;
            float detailfactor = operator_options.mantiuk06options.detailfactor;
            bool contrast_eq =
                operator_options.mantiuk06options.contrastequalization;
            if (contrast_eq) {
                postfix += QStringLiteral("contrast_equalization_%1_")
                               .arg(contrastfactor);
            } else {
                postfix +=
                    QStringLiteral("contrast_mapping_%1_").arg(contrastfactor);
            }
            postfix +=
                QStringLiteral("saturation_factor_%1_").arg(saturationfactor);
            postfix += QStringLiteral("detail_factor_%1").arg(detailfactor);
        } break;
        case mantiuk08: {
            postfix += QLatin1String("mantiuk08_");
            float colorsaturation =
                operator_options.mantiuk08options.colorsaturation;
            float contrastenhancement =
                operator_options.mantiuk08options.contrastenhancement;
            float luminancelevel =
                operator_options.mantiuk08options.luminancelevel;
            bool setluminance = operator_options.mantiuk08options.setluminance;
            if (setluminance) {
                postfix +=
                    QStringLiteral("luminancelevel_%1_").arg(luminancelevel);
            } else {
                postfix += QStringLiteral("auto_luminance");
            }
            postfix +=
                QStringLiteral("colorsaturation_%1_").arg(colorsaturation);
            postfix += QStringLiteral("contrastenhancement_%1")
                           .arg(contrastenhancement);
        } break;
        case fattal: {
            if (!operator_options.fattaloptions.newfattal)
                postfix += QLatin1String("v1_");
            postfix += QLatin1String("fattal_");
            float alpha = operator_options.fattaloptions.alpha;
            float beta = operator_options.fattaloptions.beta;
            float saturation2 = operator_options.fattaloptions.color;
            float noiseredux = operator_options.fattaloptions.noiseredux;
            bool fftsolver = operator_options.fattaloptions.fftsolver;
            postfix += QStringLiteral("alpha_%1_").arg(alpha);
            postfix += QStringLiteral("beta_%1_").arg(beta);
            postfix += QStringLiteral("saturation_%1_").arg(saturation2);
            postfix += QStringLiteral("noiseredux_%1_").arg(noiseredux);
            postfix += QStringLiteral("fftsolver_%1").arg(fftsolver);
        } break;
        case ferradans: {
            postfix += QLatin1String("ferradans_");
            float rho = operator_options.ferradansoptions.rho;
            float inv_alpha = operator_options.ferradansoptions.inv_alpha;
            postfix += QStringLiteral("rho_%1_").arg(rho);
            postfix += QStringLiteral("inv_alpha_%1").arg(inv_alpha);
        } break;
        case ferwerda: {
            postfix += QLatin1String("ferwerda_");
            float maxlum = operator_options.ferwerdaoptions.multiplier;
            float adaptlum = operator_options.ferwerdaoptions.adaptationluminance;
            postfix += QStringLiteral("max_luminance_%1_").arg(maxlum);
            postfix += QStringLiteral("adaptation_luminance_%1").arg(adaptlum);
        } break;
        case kimkautz: {
            postfix += QLatin1String("kimkautz_");
            float c1 = operator_options.kimkautzoptions.c1;
            float c2 = operator_options.kimkautzoptions.c2;
            postfix += QStringLiteral("c1_%1").arg(c1);
            postfix += QStringLiteral("c2_%1").arg(c2);
        } break;
        case mai: {
            postfix += QLatin1String("mai_");
        } break;
        case ashikhmin: {
            postfix += QLatin1String("ashikhmin_");
            if (operator_options.ashikhminoptions.simple) {
                postfix += QLatin1String("-simple");
            } else {
                if (operator_options.ashikhminoptions.eq2) {
                    postfix += QLatin1String("-eq2_");
                } else {
                    postfix += QLatin1String("-eq4_");
                }
                postfix += QStringLiteral("local_%1")
                               .arg(operator_options.ashikhminoptions.lct);
            }
        } break;
        case drago: {
            postfix += QLatin1String("drago_");
            postfix += QStringLiteral("bias_%1").arg(
                operator_options.dragooptions.bias);
        } break;
        case durand: {
            float spatial = operator_options.durandoptions.spatial;
            float range = operator_options.durandoptions.range;
            float base = operator_options.durandoptions.base;
            postfix += QLatin1String("durand_");
            postfix += QStringLiteral("spatial_%1_").arg(spatial);
            postfix += QStringLiteral("range_%1_").arg(range);
            postfix += QStringLiteral("base_%1").arg(base);
        } break;
        case pattanaik: {
            float multiplier = operator_options.pattanaikoptions.multiplier;
            float cone = operator_options.pattanaikoptions.cone;
            float rod = operator_options.pattanaikoptions.rod;
            postfix += QLatin1String("pattanaik00_");
            postfix += QStringLiteral("mul_%1_").arg(multiplier);
            if (operator_options.pattanaikoptions.local) {
                postfix += QLatin1String("local");
            } else if (operator_options.pattanaikoptions.autolum) {
                postfix += QLatin1String("autolum");
            } else {
                postfix += QStringLiteral("cone_%1_").arg(cone);
                postfix += QStringLiteral("rod_%1_").arg(rod);
            }
        } break;
        case reinhard02: {
            float key = operator_options.reinhard02options.key;
            float phi = operator_options.reinhard02options.phi;
            int range = operator_options.reinhard02options.range;
            int lower = operator_options.reinhard02options.lower;
            int upper = operator_options.reinhard02options.upper;
            postfix += QLatin1String("reinhard02_");
            postfix += QStringLiteral("key_%1_").arg(key);
            postfix += QStringLiteral("phi_%1").arg(phi);
            if (operator_options.reinhard02options.scales) {
                postfix += QStringLiteral("_scales_");
                postfix += QStringLiteral("range_%1_").arg(range);
                postfix += QStringLiteral("lower_%1_").arg(lower);
                postfix += QStringLiteral("upper_%1").arg(upper);
            }
        } break;
        case reinhard05: {
            float brightness = operator_options.reinhard05options.brightness;
            float chromaticAdaptation =
                operator_options.reinhard05options.chromaticAdaptation;
            float lightAdaptation =
                operator_options.reinhard05options.lightAdaptation;
            postfix += QLatin1String("reinhard05_");
            postfix += QStringLiteral("brightness_%1_").arg(brightness);
            postfix += QStringLiteral("chromatic_adaptation_%1_")
                           .arg(chromaticAdaptation);
            postfix +=
                QStringLiteral("light_adaptation_%1").arg(lightAdaptation);
        } break;
        case vanhateren: {
            postfix += QLatin1String("vanhateren_");
            float pupil_area = operator_options.vanhaterenoptions.pupil_area;
            postfix += QStringLiteral("pupil_area_%1").arg(pupil_area);
        } break;
    }
    postfix += QStringLiteral("_postsaturation_%1").arg(postsaturation);
    postfix += QStringLiteral("_postgamma_%1").arg(postgamma);
    return postfix;
}

const QString TonemappingOptions::getCaption(bool includePregamma,
                                             QString separator) {
    QString caption =
        includePregamma
            ? QString(QObject::tr("PreGamma=%1")).arg(pregamma) + separator
            : QString();
    switch (tmoperator) {
        case mantiuk06: {
            caption += QLatin1String("Mantiuk06:");
            caption += separator;
            float contrastfactor =
                operator_options.mantiuk06options.contrastfactor;
            float saturationfactor =
                operator_options.mantiuk06options.saturationfactor;
            float detailfactor = operator_options.mantiuk06options.detailfactor;
            bool contrast_eq =
                operator_options.mantiuk06options.contrastequalization;
            if (contrast_eq) {
                caption += QString(QObject::tr("Contrast Equalization") + "=%1")
                               .arg(contrastfactor);
            } else {
                caption += QString(QObject::tr("Contrast") + "=%1")
                               .arg(contrastfactor);
            }
            caption += separator;
            caption += QString(QObject::tr("Saturation") + "=%1")
                           .arg(saturationfactor);
            caption += separator;
            caption += QString(QObject::tr("Detail") + "=%1").arg(detailfactor);
        } break;
        case mantiuk08: {
            caption += "Mantiuk08:" + separator;
            float colorsaturation =
                operator_options.mantiuk08options.colorsaturation;
            float contrastenhancement =
                operator_options.mantiuk08options.contrastenhancement;
            float luminancelevel =
                operator_options.mantiuk08options.luminancelevel;
            bool setluminance = operator_options.mantiuk08options.setluminance;
            if (setluminance) {
                caption += QString(QObject::tr("Luminance Level") + "=%1")
                               .arg(luminancelevel);
            } else {
                caption += QString(QObject::tr("Luminance Level=Auto"));
            }
            caption += separator;
            caption += QString(QObject::tr("Color Saturation") + "=%1")
                           .arg(colorsaturation) +
                       separator;
            caption += QString(QObject::tr("Contrast Enhancement") + "=%1")
                           .arg(contrastenhancement);
        } break;
        case fattal: {
            if (!operator_options.fattaloptions.newfattal)
                caption += QLatin1String("V1_");
            float alpha = operator_options.fattaloptions.alpha;
            float beta = operator_options.fattaloptions.beta;
            float saturation2 = operator_options.fattaloptions.color;
            float noiseredux = operator_options.fattaloptions.noiseredux;
            bool fftsolver = operator_options.fattaloptions.fftsolver;
            caption += "Fattal:" + separator;
            caption +=
                QString(QObject::tr("Alpha") + "=%1").arg(alpha) + separator;
            caption +=
                QString(QObject::tr("Beta") + "=%1").arg(beta) + separator;
            caption +=
                QString(QObject::tr("Saturation") + "=%1").arg(saturation2) +
                separator;
            caption +=
                QString(QObject::tr("NoiseRedux") + "=%1").arg(noiseredux) +
                separator;
            caption += QString(QObject::tr("FFTSolver") + "=%1").arg(fftsolver);
        } break;
        case ferradans: {
            float rho = operator_options.ferradansoptions.rho;
            float inv_alpha = operator_options.ferradansoptions.inv_alpha;
            caption += "Ferrands:" + separator;
            caption += QString(QObject::tr("Rho") + "=%1").arg(rho) + separator;
            caption += QString(QObject::tr("InvAlpha") + "=%1").arg(inv_alpha);
        } break;
        case ferwerda: {
            float maxlum = operator_options.ferwerdaoptions.multiplier;
            float adaptlum = operator_options.ferwerdaoptions.adaptationluminance;
            caption += "Ferwerda:" + separator;
            caption += QString(QObject::tr("MaxLuminance") + "=%1").arg(maxlum) + separator;
            caption += QString(QObject::tr("AdaptationLuminance") + "=%1").arg(adaptlum);
        } break;
        case kimkautz: {
            float c1 = operator_options.kimkautzoptions.c1;
            float c2 = operator_options.kimkautzoptions.c2;
            caption += "KimKautz:" + separator;
            caption += QString(QObject::tr("C1") + "=%1").arg(c1) + separator;
            caption += QString(QObject::tr("C2") + "=%1").arg(c2) + separator;
        } break;
        case mai: {
            caption += "Mai:" + separator;
        } break;
        case ashikhmin: {
            caption += "Ashikhmin:" + separator;
            if (operator_options.ashikhminoptions.simple) {
                caption += QObject::tr("simple");
            } else {
                if (operator_options.ashikhminoptions.eq2) {
                    caption += QObject::tr("Equation 2");
                } else {
                    caption += QObject::tr("Equation 4");
                }
                caption += separator;
                caption += QString(QObject::tr("Local") + "=%1")
                               .arg(operator_options.ashikhminoptions.lct);
            }
        } break;
        case drago: {
            caption += "Drago:" + separator;
            caption += QString(QObject::tr("Bias") + "=%1")
                           .arg(operator_options.dragooptions.bias);
        } break;
        case durand: {
            float spatial = operator_options.durandoptions.spatial;
            float range = operator_options.durandoptions.range;
            float base = operator_options.durandoptions.base;
            caption += "Durand:" + separator;
            caption += QString(QObject::tr("Spatial") + "=%1").arg(spatial) +
                       separator;
            caption +=
                QString(QObject::tr("Range") + "=%1").arg(range) + separator;
            caption += QString(QObject::tr("Base") + "=%1").arg(base);
        } break;
        case pattanaik: {
            float multiplier = operator_options.pattanaikoptions.multiplier;
            float cone = operator_options.pattanaikoptions.cone;
            float rod = operator_options.pattanaikoptions.rod;
            caption += "Pattanaik00:" + separator;
            caption +=
                QString(QObject::tr("Multiplier") + "=%1").arg(multiplier) +
                separator;
            if (operator_options.pattanaikoptions.local) {
                caption += QObject::tr("Local");
            } else if (operator_options.pattanaikoptions.autolum) {
                caption += QObject::tr("AutoLuminance");
            } else {
                caption +=
                    QString(QObject::tr("Cone") + "=%1").arg(cone) + separator;
                caption += QString(QObject::tr("Rod") + "=%1").arg(rod);
            }
        } break;
        case reinhard02: {
            float key = operator_options.reinhard02options.key;
            float phi = operator_options.reinhard02options.phi;
            int range = operator_options.reinhard02options.range;
            int lower = operator_options.reinhard02options.lower;
            int upper = operator_options.reinhard02options.upper;
            caption += "Reinhard02:" + separator;
            caption += QString(QObject::tr("Key") + "=%1").arg(key) + separator;
            caption += QString(QObject::tr("Phi") + "=%1").arg(phi);
            if (operator_options.reinhard02options.scales) {
                caption +=
                    separator + QString(QObject::tr("Scales:")) + separator;
                caption += QString(QObject::tr("Range") + "=%1").arg(range) +
                           separator;
                caption += QString(QObject::tr("Lower") + "=%1").arg(lower) +
                           separator;
                caption += QString(QObject::tr("Upper") + "=%1").arg(upper);
            }
        } break;
        case reinhard05: {
            float brightness = operator_options.reinhard05options.brightness;
            float chromaticAdaptation =
                operator_options.reinhard05options.chromaticAdaptation;
            float lightAdaptation =
                operator_options.reinhard05options.lightAdaptation;
            caption += "Reinhard05:" + separator;
            caption +=
                QString(QObject::tr("Brightness") + "=%1").arg(brightness) +
                separator;
            caption += QString(QObject::tr("Chromatic Adaptation") + "=%1")
                           .arg(chromaticAdaptation) +
                       separator;
            caption += QString(QObject::tr("Light Adaptation") + "=%1")
                           .arg(lightAdaptation);
        } break;
        case vanhateren: {
            float pupil_area = operator_options.vanhaterenoptions.pupil_area;
            caption += "VanHateren:" + separator;
            caption += QString(QObject::tr("Pupil Area") + "=%1").arg(pupil_area) + separator;
        } break;
    }
    caption += includePregamma
            ? separator + QString(QObject::tr("PostSaturation=%1")).arg(postsaturation) +
              separator + QString(QObject::tr("PostGamma=%1")).arg(postgamma)
            : QString();
    return caption;
}

TonemappingOptions *TMOptionsOperations::parseFile(const QString &fname) {
    QFile file(fname);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text) || file.size() == 0) {
        throw(
            QApplication::tr("ERROR: cannot load Tone Mapping Setting file: ") +
            fname);
        // return NULL;
    }

    TonemappingOptions *toreturn = new TonemappingOptions;
    // memset(toreturn, 0, sizeof *toreturn);

    QTextStream in(&file);
    QString field, value;

    QString tmo;  // Hack, same parameter "RANGE" in durand and reinhard02

    while (!in.atEnd()) {
        QString line = in.readLine();
        // skip comments
        if (line.startsWith('#')) continue;

        field = line.section('=', 0, 0);  // get the field
        value = line.section('=', 1, 1);  // get the value
        if (field == QLatin1String("TMOSETTINGSVERSION")) {
            if (value != TMOSETTINGSVERSION) {
                delete toreturn;
                throw(QApplication::tr(
                          "ERROR: File too old, cannot parse Tone Mapping "
                          "Setting file: ") +
                      fname);
                //                 return NULL;
            }
        } else if (field == QLatin1String("XSIZE")) {
            toreturn->xsize = value.toInt();
        } else if (field == QLatin1String("QUALITY")) {
            toreturn->quality = value.toInt();
        } else if (field == QLatin1String("TMO")) {
            if (value == QLatin1String("Ashikhmin02")) {
                toreturn->tmoperator = ashikhmin;
                tmo = QStringLiteral("Ashikhmin02");
            } else if (value == QLatin1String("Drago03")) {
                toreturn->tmoperator = drago;
                tmo = QStringLiteral("Drago03");
            } else if (value == QLatin1String("Durand02")) {
                toreturn->tmoperator = durand;
                tmo = QStringLiteral("Durand02");
            } else if (value == QLatin1String("Fattal02")) {
                toreturn->tmoperator = fattal;
                tmo = QStringLiteral("Fattal02");
            } else if (value == QLatin1String("Ferradans11")) {
                toreturn->tmoperator = ferradans;
                tmo = QStringLiteral("Ferradans11");
            } else if (value == QLatin1String("Ferwerda96")) {
                toreturn->tmoperator = ferwerda;
                tmo = QStringLiteral("Ferwerda96");
            } else if (value == QLatin1String("KimKautz08")) {
                toreturn->tmoperator = kimkautz;
                tmo = QStringLiteral("KimKautz08");
            } else if (value == QLatin1String("Mai11")) {
                toreturn->tmoperator = mai;
                tmo = QStringLiteral("Mai11");
            } else if (value == QLatin1String("Pattanaik00")) {
                toreturn->tmoperator = pattanaik;
                tmo = QStringLiteral("Pattanaik00");
            } else if (value == QLatin1String("Reinhard02")) {
                toreturn->tmoperator = reinhard02;
                tmo = QStringLiteral("Reinhard02");
            } else if (value == QLatin1String("Reinhard05")) {
                toreturn->tmoperator = reinhard05;
                tmo = QStringLiteral("Reinhard05");
            } else if (value == QLatin1String("Mantiuk06")) {
                toreturn->tmoperator = mantiuk06;
                tmo = QStringLiteral("Mantiuk06");
            } else if (value == QLatin1String("Mantiuk08")) {
                toreturn->tmoperator = mantiuk08;
                tmo = QStringLiteral("Mantiuk08");
            } else if (value == QLatin1String("VanHateren06")) {
                toreturn->tmoperator = vanhateren;
                tmo = QStringLiteral("VanHateren06");
            }
        } else if (field == QLatin1String("CONTRASTFACTOR")) {
            toreturn->operator_options.mantiuk06options.contrastfactor =
                value.toFloat();
        } else if (field == QLatin1String("SATURATIONFACTOR")) {
            toreturn->operator_options.mantiuk06options.saturationfactor =
                value.toFloat();
        } else if (field == QLatin1String("DETAILFACTOR")) {
            toreturn->operator_options.mantiuk06options.detailfactor =
                value.toFloat();
        } else if (field == QLatin1String("CONTRASTEQUALIZATION")) {
            toreturn->operator_options.mantiuk06options.contrastequalization =
                (value == QLatin1String("YES"));
        } else if (field == QLatin1String("COLORSATURATION")) {
            toreturn->operator_options.mantiuk08options.colorsaturation =
                value.toFloat();
        } else if (field == QLatin1String("CONTRASTENHANCEMENT")) {
            toreturn->operator_options.mantiuk08options.contrastenhancement =
                value.toFloat();
        } else if (field == QLatin1String("LUMINANCELEVEL")) {
            toreturn->operator_options.mantiuk08options.luminancelevel =
                value.toFloat();
        } else if (field == QLatin1String("SETLUMINANCE")) {
            toreturn->operator_options.mantiuk08options.setluminance =
                (value == QLatin1String("YES"));
        } else if (field == QLatin1String("SIMPLE")) {
            toreturn->operator_options.ashikhminoptions.simple =
                (value == QLatin1String("YES")) ? true : false;
        } else if (field == QLatin1String("EQUATION")) {
            toreturn->operator_options.ashikhminoptions.eq2 =
                (value == QLatin1String("2")) ? true : false;
        } else if (field == QLatin1String("CONTRAST")) {
            toreturn->operator_options.ashikhminoptions.lct = value.toFloat();
        } else if (field == QLatin1String("BIAS")) {
            toreturn->operator_options.dragooptions.bias = value.toFloat();
        } else if (field == QLatin1String("SPATIAL")) {
            toreturn->operator_options.durandoptions.spatial = value.toFloat();
        } else if (field == QLatin1String("RANGE")) {
            if (tmo == QLatin1String("Durand02"))
                toreturn->operator_options.durandoptions.range =
                    value.toFloat();
            else
                toreturn->operator_options.reinhard02options.range =
                    value.toInt();
        } else if (field == QLatin1String("BASE")) {
            toreturn->operator_options.durandoptions.base = value.toFloat();
        } else if (field == QLatin1String("ALPHA")) {
            toreturn->operator_options.fattaloptions.alpha = value.toFloat();
        } else if (field == QLatin1String("BETA")) {
            toreturn->operator_options.fattaloptions.beta = value.toFloat();
        } else if (field == QLatin1String("COLOR")) {
            toreturn->operator_options.fattaloptions.color = value.toFloat();
        } else if (field == QLatin1String("NOISE")) {
            toreturn->operator_options.fattaloptions.noiseredux =
                value.toFloat();
        } else if (field == QLatin1String("OLDFATTAL")) {
            toreturn->operator_options.fattaloptions.newfattal =
                true;  // This is the new version of fattal pre FFT (always yes)
            toreturn->operator_options.fattaloptions.fftsolver =
                (value == QLatin1String("NO"));
        } else if (field == QLatin1String("RHO")) {
            toreturn->operator_options.ferradansoptions.rho = value.toFloat();
        } else if (field == QLatin1String("INV_ALPHA")) {
            toreturn->operator_options.ferradansoptions.inv_alpha =
                value.toFloat();
        } else if (field == QLatin1String("MAX_LUMINANCE")) {
            toreturn->operator_options.ferwerdaoptions.multiplier = value.toFloat();
        } else if (field == QLatin1String("ADAPTATION_LUMINANCE")) {
            toreturn->operator_options.ferwerdaoptions.adaptationluminance =
                value.toFloat();
        } else if (field == QLatin1String("KK_C1")) {
            toreturn->operator_options.kimkautzoptions.c1 = value.toFloat();
        } else if (field == QLatin1String("KK_C2")) {
            toreturn->operator_options.kimkautzoptions.c2 = value.toFloat();
        } else if (field == QLatin1String("MULTIPLIER")) {
            toreturn->operator_options.pattanaikoptions.multiplier =
                value.toFloat();
        } else if (field == QLatin1String("LOCAL")) {
            toreturn->operator_options.pattanaikoptions.local =
                (value == QLatin1String("YES"));
        } else if (field == QLatin1String("AUTOLUMINANCE")) {
            toreturn->operator_options.pattanaikoptions.autolum =
                (value == QLatin1String("YES"));
        } else if (field == QLatin1String("CONE")) {
            toreturn->operator_options.pattanaikoptions.cone = value.toFloat();
        } else if (field == QLatin1String("ROD")) {
            toreturn->operator_options.pattanaikoptions.rod = value.toFloat();
        } else if (field == QLatin1String("KEY")) {
            toreturn->operator_options.reinhard02options.key = value.toFloat();
        } else if (field == QLatin1String("PHI")) {
            toreturn->operator_options.reinhard02options.phi = value.toFloat();
        } else if (field == QLatin1String("SCALES")) {
            toreturn->operator_options.reinhard02options.scales =
                (value == QLatin1String("YES")) ? true : false;
        } else if (field == QLatin1String("LOWER")) {
            toreturn->operator_options.reinhard02options.lower = value.toInt();
        } else if (field == QLatin1String("UPPER")) {
            toreturn->operator_options.reinhard02options.upper = value.toInt();
        } else if (field == QLatin1String("BRIGHTNESS")) {
            toreturn->operator_options.reinhard05options.brightness =
                value.toFloat();
        } else if (field == QLatin1String("CHROMATICADAPTATION")) {
            toreturn->operator_options.reinhard05options.chromaticAdaptation =
                value.toFloat();
        } else if (field == QLatin1String("LIGHTADAPTATION")) {
            toreturn->operator_options.reinhard05options.lightAdaptation =
                value.toFloat();
        } else if (field == QLatin1String("PUPIL_AREA")) {
            toreturn->operator_options.vanhaterenoptions.pupil_area =
                value.toFloat();
        } else if (field == QLatin1String("PREGAMMA")) {
            toreturn->pregamma = value.toFloat();
        } else if (field == QLatin1String("POSTGAMMA")) {
            toreturn->postgamma = value.toFloat();
        } else if (field == QLatin1String("POSTSATURATION")) {
            toreturn->postsaturation = value.toFloat();
        } else {
            delete toreturn;
            throw(QApplication::tr(
                      "ERROR: cannot parse Tone Mapping Setting file: ") +
                  fname);
            //             return NULL;
        }
    }
    return toreturn;
}

TonemappingOptions *TMOptionsOperations::getDefaultTMOptions() {
    TonemappingOptions *toreturn = new TonemappingOptions;
    // TODO when instantiating the tonemapperThread, check this value: if -2 =>
    // create thread with originalsize=-2 (to skip resize the step as we did
    // with
    // the batch tone mapping), else (the user wants to resize) create thread
    // with
    // true originalxsize
    toreturn->xsize = -2;
    return toreturn;
}

TMOptionsOperations::TMOptionsOperations(const TonemappingOptions *opts)
    : opts(opts) {}

QString TMOptionsOperations::getExifComment() {
    QString exif_comment =
        "Luminance HDR " LUMINANCEVERSION "\n\nTonemapping parameters:\n";
    exif_comment += QLatin1String("Operator: ");
    switch (opts->tmoperator) {
        case mantiuk06: {
            float contrastfactor =
                opts->operator_options.mantiuk06options.contrastfactor;
            float saturationfactor =
                opts->operator_options.mantiuk06options.saturationfactor;
            float detailfactor =
                opts->operator_options.mantiuk06options.detailfactor;
            bool contrast_eq =
                opts->operator_options.mantiuk06options.contrastequalization;
            exif_comment += QLatin1String("Mantiuk06\nParameters:\n");
            if (contrast_eq) {
                exif_comment +=
                    QStringLiteral("Contrast Equalization factor: %1\n")
                        .arg(contrastfactor);
            } else {
                exif_comment += QStringLiteral("Contrast Mapping factor: %1\n")
                                    .arg(contrastfactor);
            }
            exif_comment += QStringLiteral("Saturation Factor: %1 \n")
                                .arg(saturationfactor);
            exif_comment +=
                QStringLiteral("Detail Factor: %1 \n").arg(detailfactor);
        } break;
        case mantiuk08: {
            float colorsaturation =
                opts->operator_options.mantiuk08options.colorsaturation;
            float contrastenhancement =
                opts->operator_options.mantiuk08options.contrastenhancement;
            float luminancelevel =
                opts->operator_options.mantiuk08options.luminancelevel;
            bool setluminance =
                opts->operator_options.mantiuk08options.setluminance;
            exif_comment += QLatin1String("Mantiuk08\nParameters:\n");
            if (setluminance) {
                exif_comment += QStringLiteral("Luminance Level: %1 \n")
                                    .arg(luminancelevel);
            } else {
                exif_comment += QStringLiteral("Luminance Level: Auto \n");
            }
            exif_comment +=
                QStringLiteral("Color Saturation: %1 \n").arg(colorsaturation);
            exif_comment += QStringLiteral("Contrast Enhancement: %1 \n")
                                .arg(contrastenhancement);
        } break;
        case fattal: {
            float alpha = opts->operator_options.fattaloptions.alpha;
            float beta = opts->operator_options.fattaloptions.beta;
            float saturation2 = opts->operator_options.fattaloptions.color;
            float noiseredux = opts->operator_options.fattaloptions.noiseredux;
            if (!opts->operator_options.fattaloptions.newfattal) {
                exif_comment += QLatin1String("V1_");
            }
            exif_comment += QLatin1String("Fattal\nParameters:\n");
            exif_comment += QStringLiteral("Alpha: %1\n").arg(alpha);
            exif_comment += QStringLiteral("Beta: %1\n").arg(beta);
            exif_comment +=
                QStringLiteral("Color Saturation: %1 \n").arg(saturation2);
            exif_comment +=
                QStringLiteral("Noise Reduction: %1 \n").arg(noiseredux);
        } break;
        case ferradans: {
            float rho = opts->operator_options.ferradansoptions.rho;
            float inv_alpha = opts->operator_options.ferradansoptions.inv_alpha;
            exif_comment += QLatin1String("Ferrands\nParameters:\n");
            exif_comment += QStringLiteral("Rho: %1\n").arg(rho);
            exif_comment += QStringLiteral("InvAlpha: %1\n").arg(inv_alpha);
        } break;
        case ferwerda: {
            float maxlum = opts->operator_options.ferwerdaoptions.multiplier;
            float adaptlum = opts->operator_options.ferwerdaoptions.adaptationluminance;
            exif_comment += QLatin1String("Ferwerda\nParameters:\n");
            exif_comment += QStringLiteral("MaxLuminance: %1\n").arg(maxlum);
            exif_comment += QStringLiteral("AdaptationLuminance: %1\n").arg(adaptlum);
        } break;
        case kimkautz: {
            float c1 = opts->operator_options.kimkautzoptions.c1;
            float c2 = opts->operator_options.kimkautzoptions.c2;
            exif_comment += QLatin1String("KimKautz\nParameters:\n");
            exif_comment += QStringLiteral("C1: %1\n").arg(c1);
            exif_comment += QStringLiteral("C2: %1\n").arg(c2);
        } break;
        case mai: {
            exif_comment += QLatin1String(
                "Mai\nParameters:\nThis operator has no parameters\n");
        } break;
        case ashikhmin: {
            exif_comment += QLatin1String("Ashikhmin\nParameters:\n");
            if (opts->operator_options.ashikhminoptions.simple) {
                exif_comment += QLatin1String("Simple\n");
            } else {
                if (opts->operator_options.ashikhminoptions.eq2) {
                    exif_comment += QLatin1String("Equation 2\n");
                } else {
                    exif_comment += QLatin1String("Equation 4\n");
                }
                exif_comment +=
                    QStringLiteral("Local Contrast value: %1\n")
                        .arg(opts->operator_options.ashikhminoptions.lct);
            }
        } break;
        case drago: {
            exif_comment += QLatin1String("Drago\nParameters:\n");
            exif_comment += QStringLiteral("Bias: %1\n")
                                .arg(opts->operator_options.dragooptions.bias);
        } break;
        case durand: {
            float spatial = opts->operator_options.durandoptions.spatial;
            float range = opts->operator_options.durandoptions.range;
            float base = opts->operator_options.durandoptions.base;
            exif_comment += QLatin1String("Durand\nParameters:\n");
            exif_comment +=
                QStringLiteral("Spatial Kernel Sigma: %1\n").arg(spatial);
            exif_comment +=
                QStringLiteral("Range Kernel Sigma: %1\n").arg(range);
            exif_comment += QStringLiteral("Base Contrast: %1\n").arg(base);
        } break;
        case pattanaik: {
            float multiplier =
                opts->operator_options.pattanaikoptions.multiplier;
            float cone = opts->operator_options.pattanaikoptions.cone;
            float rod = opts->operator_options.pattanaikoptions.rod;
            exif_comment += QLatin1String("Pattanaik\nParameters:\n");
            exif_comment += QStringLiteral("Multiplier: %1\n").arg(multiplier);
            if (opts->operator_options.pattanaikoptions.local) {
                exif_comment += QLatin1String("Local Tone Mapping\n");
            } else if (opts->operator_options.pattanaikoptions.autolum) {
                exif_comment +=
                    QLatin1String("Con and Rod based on image luminance\n");
            } else {
                exif_comment += QStringLiteral("Cone Level: %1\n").arg(cone);
                exif_comment += QStringLiteral("Rod Level: %1\n").arg(rod);
            }
        } break;
        case reinhard02: {
            float key = opts->operator_options.reinhard02options.key;
            float phi = opts->operator_options.reinhard02options.phi;
            int range = opts->operator_options.reinhard02options.range;
            int lower = opts->operator_options.reinhard02options.lower;
            int upper = opts->operator_options.reinhard02options.upper;
            exif_comment += QLatin1String("Reinhard02\nParameters:\n");
            exif_comment += QStringLiteral("Key: %1\n").arg(key);
            exif_comment += QStringLiteral("Phi: %1\n").arg(phi);
            if (opts->operator_options.reinhard02options.scales) {
                exif_comment += QStringLiteral("Scales\n");
                exif_comment += QStringLiteral("Range: %1\n").arg(range);
                exif_comment += QStringLiteral("Lower: %1\n").arg(lower);
                exif_comment += QStringLiteral("Upper: %1\n").arg(upper);
            }
        } break;
        case reinhard05: {
            float brightness =
                opts->operator_options.reinhard05options.brightness;
            float chromaticAdaptation =
                opts->operator_options.reinhard05options.chromaticAdaptation;
            float lightAdaptation =
                opts->operator_options.reinhard05options.lightAdaptation;
            exif_comment += QLatin1String("Reinhard05\nParameters:\n");
            exif_comment += QStringLiteral("Brightness: %1\n").arg(brightness);
            exif_comment += QStringLiteral("Chromatic Adaptation: %1\n")
                                .arg(chromaticAdaptation);
            exif_comment +=
                QStringLiteral("Light Adaptation: %1\n").arg(lightAdaptation);
        } break;
        case vanhateren: {
            float pupil_area =
                opts->operator_options.vanhaterenoptions.pupil_area;
            exif_comment += QLatin1String("VanHateren06\nParameters:\n");
            exif_comment += QStringLiteral("Pupil Area: %1\n").arg(pupil_area);
        } break;
    }
    exif_comment +=
        QStringLiteral("------\nPreGamma: %1\n").arg(opts->pregamma);
    return exif_comment;
}
