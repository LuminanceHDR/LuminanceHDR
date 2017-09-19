/**
 * @file pfstmo_durand02.cpp
 * @brief Tone map XYZ channels using Durand02 model
 *
 * Fast Bilateral Filtering for the Display of High-Dynamic-Range Images.
 * F. Durand and J. Dorsey.
 * In ACM Transactions on Graphics, 2002.
 *
 *
 * This file is a part of LuminanceHDR package, based on pfstmo.
 * ----------------------------------------------------------------------
 * Copyright (C) 2003,2004 Grzegorz Krawczyk
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
 * @author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
 *
 * $Id: pfstmo_durand02.cpp,v 1.5 2009/02/23 19:09:41 rafm Exp $
 */

#include <cmath>
#include <iostream>
#include <sstream>

#include "Libpfs/colorspace/colorspace.h"
#include "Libpfs/exception.h"
#include "Libpfs/frame.h"
#include "Libpfs/progress.h"
#include "tmo_durand02.h"

namespace {
const int downsample = 1;
const bool original_algorithm = false;
}

//--- default tone mapping parameters;
//#ifdef HAVE_FFTW3F
//  float sigma_s = 40.0f;
//#else
// float sigma_s = 8.0f;
//#endif
// float sigma_r = 0.4f;
// float baseContrast = 5.0f;

void pfstmo_durand02(pfs::Frame &frame, float sigma_s, float sigma_r,
                     float baseContrast, pfs::Progress &ph) {
#ifndef NDEBUG
    std::stringstream ss;

    ss << "pfstmo_durand02 (";
#ifdef HAVE_FFTW3F
    ss << "fftw3f ON";
#else
    ss << "fftw3f OFF";
#endif
    ss << ", sigma_s: " << sigma_s;
    ss << ", sigma_r: " << sigma_r;
    ss << ", base contrast: " << baseContrast << ")";

    std::cout << ss.str() << std::endl;
#endif

    pfs::Channel *X, *Y, *Z;

    frame.getXYZChannels(X, Y, Z);
    frame.getTags().setTag("LUMINANCE", "RELATIVE");
    //---

    if (Y == NULL || X == NULL || Z == NULL) {
        throw pfs::Exception("Missing X, Y, Z channels in the PFS stream");
    }

    try {
        tmo_durand02(*X, *Y, *Z, sigma_s, sigma_r, baseContrast, downsample,
                     !original_algorithm, ph);
    } catch (...) {
        throw pfs::Exception("Tonemapping Failed!");
    }

    if (!ph.canceled()) ph.setValue(100);
}
