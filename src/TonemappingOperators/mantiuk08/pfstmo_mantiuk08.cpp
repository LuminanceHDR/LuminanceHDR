/**
 * @brief Display Adaptive TMO
 *
 * From:
 * Rafal Mantiuk, Scott Daly, Louis Kerofsky.
 * Display Adaptive Tone Mapping.
 * To appear in: ACM Transactions on Graphics (Proc. of SIGGRAPH'08) 27 (3)
 * http://www.mpi-inf.mpg.de/resources/hdr/datmo/
 *
 * This file is a part of LuminanceHDR package, based on pfstmo.
 * ----------------------------------------------------------------------
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
 * @author Rafal Mantiuk, <mantiuk@gmail.com>
 *
 * $Id: pfstmo_mantiuk08.cpp,v 1.12 2009/02/23 18:46:36 rafm Exp $
 */

#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <memory>

#include "Libpfs/colorspace/colorspace.h"
#include "Libpfs/frame.h"
#include "Libpfs/progress.h"
#include "display_adaptive_tmo.h"

using namespace std;

void pfstmo_mantiuk08(pfs::Frame &frame, float saturation_factor,
                      float contrast_enhance_factor, float white_y,
                      bool setluminance, pfs::Progress &ph) {
    //--- default tone mapping parameters;
    // float contrast_enhance_factor = 1.f;
    // float saturation_factor = 1.f;
    // float white_y = -2.f;
    // int temporal_filter = 0;
    float fps = 25;
    datmoVisualModel visual_model = vm_full;
    double scene_l_adapt = 1000;

    if (!setluminance) white_y = -2.f;

    if (contrast_enhance_factor <= 0.0f)
        throw pfs::Exception(
            "incorrect contrast enhancement factor, accepted "
            "value is any positive "
            "number");

    if (saturation_factor < 0.0f || saturation_factor > 2.0f)
        throw pfs::Exception(
            "incorrect saturation factor, accepted range is (0..2)");

#ifndef NDEBUG
    std::cout << "pfstmo_mantiuk08 (";
    std::cout << "saturation factor: " << saturation_factor;
    std::cout << ", contrast enhancement factor: " << contrast_enhance_factor;
    std::cout << ", white_y: " << white_y;
    std::cout << ", setluminance: " << setluminance << ")" << std::endl;
#endif

    DisplayFunction *df = NULL;
    DisplaySize *ds = NULL;

    if (df == NULL)  // As of now df is not selected by users but hardcoded here
        df = new DisplayFunctionGGBA("lcd");

    if (ds == NULL)  // As of now ds is not selected by users but hardcoded here
        ds = new DisplaySize(30.f, 0.5f);

#ifndef NDEBUG
    df->print(stderr);
    ds->print(stderr);
#endif

    pfs::Channel *inX, *inY, *inZ;
    frame.getXYZChannels(inX, inY, inZ);

    if (!inX || !inY || !inZ) {
        if (df != NULL) delete df;
        throw pfs::Exception("Missing X, Y, Z channels in the PFS stream");
    }

    const int cols = frame.getWidth();
    const int rows = frame.getHeight();

    pfs::Array2Df R(cols, rows);
    pfs::transformColorSpace(pfs::CS_XYZ, inX, inY, inZ, pfs::CS_RGB, inX, &R,
                             inZ);

    if (white_y == -2.f) {
        std::string white_y_str = frame.getTags().getTag("WHITE_Y");
        if (!white_y_str.empty())  // TODO check this
        {
            white_y = strtod(white_y_str.c_str(), NULL);
            if (white_y == 0) {
                white_y = -1;
                fprintf(stderr, "warning - wrong WHITE_Y in the input image");
            }
        }
    }

    /* no need for flushing stdout with this stuff
      fprintf( stderr, "Luminance factor of the reference white: " );
      if( white_y < 0 )
        fprintf( stderr, "not specified\n" );
      else
        fprintf( stderr, "%g\n", white_y );

      std::string lum_data = frame.getTags().getTag("LUMINANCE");
      if( !lum_data.empty() && lum_data != "DISPLAY" ) {
        fprintf( stderr, "warning: input image should be in linear (not gamma
      corrected) luminance factor units. Use '--linear' option with pfsin*
      commands.\n" );
      }
    */

    std::unique_ptr<datmoConditionalDensity> C =
        datmo_compute_conditional_density(cols, rows, inY->data(), ph);
    if (C.get() == NULL) {
        delete df;
        delete ds;
        throw pfs::Exception("failed to analyse the image");
    }

    datmoTCFilter rc_filter(fps, log10(df->display(0)), log10(df->display(1)));

    // datmoToneCurve tc;
    datmoToneCurve *tc = rc_filter.getToneCurvePtr();

    int res;
    res = datmo_compute_tone_curve(tc, C.get(), df, ds, contrast_enhance_factor,
                                   white_y, visual_model, scene_l_adapt, ph);
    if (res != PFSTMO_OK) {
        delete df;
        delete ds;
        throw pfs::Exception("failed to compute the tone-curve");
    }

    datmoToneCurve *tc_filt = rc_filter.filterToneCurve();

    res = datmo_apply_tone_curve_cc(
        inX->data(), R.data(), inZ->data(), cols, rows, inX->data(), R.data(),
        inZ->data(), inY->data(), tc_filt, df, saturation_factor);
    if (res != PFSTMO_OK) {
        delete df;
        delete ds;
        throw pfs::Exception("failed to tone-map the image");
    }

    pfs::transformColorSpace(pfs::CS_RGB, inX, &R, inZ, pfs::CS_XYZ, inX, inY,
                             inZ);
    frame.getTags().setTag("LUMINANCE", "DISPLAY");

    if (!ph.canceled()) ph.setValue(100);

    delete df;
    delete ds;
}
