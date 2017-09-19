/**
 * @brief Adaptive logarithmic tone mapping
 *
 * Adaptive logarithmic mapping for displaying high contrast
 * scenes.
 * F. Drago, K. Myszkowski, T. Annen, and N. Chiba. In Eurographics 2003.
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
 * @author Grzegorz Krawczyk, <krawczyk@mpi-inf.mpg.de>
 *
 * $Id: pfstmo_drago03.cpp,v 1.3 2008/09/04 12:46:48 julians37 Exp $
 */

#include <cmath>
#include <iostream>

#include <boost/math/special_functions/fpclassify.hpp>

#include "Libpfs/exception.h"
#include "Libpfs/frame.h"
#include "Libpfs/progress.h"
#include "tmo_drago03.h"

void pfstmo_drago03(pfs::Frame &frame, float opt_biasValue, pfs::Progress &ph) {
#ifndef NDEBUG
    std::stringstream ss;
    ss << "pfstmo_drago03 (";
    ss << "bias: " << opt_biasValue;
    ss << ")";
    std::cout << ss.str() << std::endl;
#endif

    pfs::Channel *X, *Y, *Z;
    frame.getXYZChannels(X, Y, Z);

    if (!X || !Y || !Z) {
        throw pfs::Exception("Missing X, Y, Z channels in the PFS stream");
    }

    frame.getTags().setTag("LUMINANCE", "RELATIVE");
    //---

    pfs::Array2Df &Xr = *X;
    pfs::Array2Df &Yr = *Y;
    pfs::Array2Df &Zr = *Z;

    int w = Yr.getCols();
    int h = Yr.getRows();

    float maxLum;
    float avLum;
    calculateLuminance(w, h, Yr.data(), avLum, maxLum);

    pfs::Array2Df L(w, h);
    try {
        tmo_drago03(Yr, L, maxLum, avLum, opt_biasValue, ph);
    } catch (...) {
        throw pfs::Exception("Tonemapping Failed!");
    }

    for (int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++) {
            float yr = Yr(x, y);
            float scale = 0.f;
            if (yr != 0.f) {
                scale = L(x, y) / yr;
            }

            assert(!boost::math::isnan(scale));

            Yr(x, y) = Yr(x, y) * scale;
            Xr(x, y) = Xr(x, y) * scale;
            Zr(x, y) = Zr(x, y) * scale;
        }
    }

    if (!ph.canceled()) {
        ph.setValue(100);
    }
}
