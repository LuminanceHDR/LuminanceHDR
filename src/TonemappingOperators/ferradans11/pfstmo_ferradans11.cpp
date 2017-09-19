/**
 * @file pfstmo_ferradans11.cpp
 * @brief Tone map RGB channels using Ferradans11 model
 *
 * An Analysis of Visual Adaptation and Contrast Perception for Tone Mapping
 * S. Ferradans, M. Bertalmio, E. Provenzi, V. Caselles
 * In IEEE Trans. Pattern Analysis and Machine Intelligence 2011
 *
 *
 * This file is a part of LuminanceHDR package, based on pfstmo.
 * ----------------------------------------------------------------------
 * Copyright (C) 2013 Sira Ferradans
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
 * @author Sira Ferradans,
 *
 */
#include <sstream>

#include "Libpfs/exception.h"
#include "Libpfs/frame.h"
#include "Libpfs/manip/gamma.h"
#include "Libpfs/progress.h"
#include "tmo_ferradans11.h"

void pfstmo_ferradans11(pfs::Frame &frame, float opt_rho, float opt_inv_alpha,
                        pfs::Progress &ph) {
//--- default tone mapping parameters;
// float rho = -2;
// float inv_alpha = 5;

#ifndef NDEBUG
    std::stringstream ss;
    ss << "pfstmo_ferradans11 (";
    ss << "rho: " << opt_rho;
    ss << ", inv_alpha: " << opt_inv_alpha << ")";
    std::cout << ss.str() << std::endl;
#endif

    pfs::Channel *inR, *inG, *inB;
    frame.getXYZChannels(inR, inG, inB);
    //---

    if (inR == NULL || inG == NULL || inB == NULL)
        throw pfs::Exception("Missing X, Y, Z channels in the PFS stream");

    frame.getTags().setTag("LUMINANCE", "RELATIVE");
    // TODO Check why gamma is 1/4 of gamma in pfstools
    // pfs::applyGamma(&frame, 0.25f);

    // tone mapping
    try {
        tmo_ferradans11(*inR, *inG, *inB, opt_rho, opt_inv_alpha, ph);
    } catch (...) {
        throw pfs::Exception("Tonemapping Failed!");
    }

    if (!ph.canceled()) ph.setValue(100);
}
