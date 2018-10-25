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

#ifndef TONEMAPPINGOPTIONS_H
#define TONEMAPPINGOPTIONS_H

#include <QObject>
#include <QString>

//----------------- DO NOT CHANGE ENUMERATION ORDER -----------------------
// all is used by SavedParametersDialog to select comments from all operators
enum TMOperator : unsigned short {
    mantiuk06 = 0,
    mantiuk08 = 1,
    fattal = 2,
    ferradans = 3,
    drago = 4,
    durand = 5,
    reinhard02 = 6,
    reinhard05 = 7,
    ashikhmin = 8,
    pattanaik = 9,
    mai = 10,
    ferwerda = 11,
    kimkautz = 12,
    vanhateren = 13
};

class TonemappingOptions {
   public:
    int origxsize;      // this parameter should be coming from the UI
    int xsize_percent;  // this parameter should be coming from the UI
    int xsize;          // this parameter should be coming from the frame
    int quality;
    float pregamma;
    float postgamma;
    float postsaturation;
    bool tonemapSelection;  // we should let do this thing to the tonemapping
                            // thread
    TMOperator tmoperator;
    struct {
        struct {
            bool simple;
            bool eq2;  // false means eq4
            float lct;
        } ashikhminoptions;
        struct {
            float bias;
        } dragooptions;
        struct {
            float spatial;
            float range;
            float base;
        } durandoptions;
        struct {
            float alpha;
            float beta;
            float color;
            float noiseredux;
            bool newfattal;
            bool fftsolver;
        } fattaloptions;
        struct {
            float rho;
            float inv_alpha;
        } ferradansoptions;
        struct {
            bool autolum;
            bool local;
            float cone;
            float rod;
            float multiplier;
        } pattanaikoptions;
        struct {
            bool scales;
            float key;
            float phi;
            int range;
            int lower;
            int upper;
        } reinhard02options;
        struct {
            float brightness;
            float chromaticAdaptation;
            float lightAdaptation;
        } reinhard05options;
        struct {
            float contrastfactor;
            float saturationfactor;
            float detailfactor;
            bool contrastequalization;
        } mantiuk06options;
        struct {
            float colorsaturation;
            float contrastenhancement;
            float luminancelevel;
            bool setluminance;
        } mantiuk08options;
        struct {
            float multiplier;
            float adaptationluminance;
        } ferwerdaoptions;
        struct {
            float c1;
            float c2;
        } kimkautzoptions;
        struct {
            float pupil_area;
        } vanhaterenoptions;
    } operator_options;

    // Davide Anastasia <davideanastasia@users.sourceforge.net>
    // Adding the coordinates of the crop inside this structure will allow
    // TMOThread
    // to crop itself the region of interest, keeping the code tight and simple
    // and avoiding useless copy in memory of the frame to be processed.
    int selection_x_up_left;
    int selection_y_up_left;
    int selection_x_bottom_right;
    int selection_y_bottom_right;

    // default constructor
    TonemappingOptions() { setDefaultParameters(); }
    const QString getPostfix();

    /** returns the translated description of the TMO operator**/
    const QString getCaption(bool pregamma = true,
                             QString separator = QStringLiteral(" ~ "));
    void setDefaultTonemapParameters();
    void setDefaultParameters();

    char getRatingForOperator();
};

/*
 * TODO: Davide Anastasia 2011.11.19
 *
 * This class depends strongly on the TonemappingOptions
 * However, most of these functionality can be brought inside TonemappingOptions
 * and get rid of this class
 */

class TMOptionsOperations {
   public:
    TMOptionsOperations(const TonemappingOptions *opts);
    static TonemappingOptions *parseFile(const QString &file);
    static TonemappingOptions *getDefaultTMOptions();
    QString getExifComment();

   private:
    const TonemappingOptions *opts;
};

#endif  // TONEMAPPINGOPTIONS_H
