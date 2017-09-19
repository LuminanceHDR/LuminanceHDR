/**
 * @brief Contrast mapping TMO
 *
 * From:
 *
 * Rafal Mantiuk, Karol Myszkowski, Hans-Peter Seidel.
 * A Perceptual Framework for Contrast Processing of High Dynamic Range Images
 * In: ACM Transactions on Applied Perception 3 (3), pp. 286-308, 2006
 * http://www.mpi-inf.mpg.de/~mantiuk/contrast_domain/
 *
 * This file is a part of LuminanceHDR package, based on pfstmo.
 * ----------------------------------------------------------------------
 * Copyright (C) 2007 Grzegorz Krawczyk
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
 * @author Radoslaw Mantiuk, <radoslaw.mantiuk@gmail.com>
 * @author Rafal Mantiuk, <mantiuk@gmail.com>
 * Updated 2007/12/17 by Ed Brambley <E.J.Brambley@damtp.cam.ac.uk>
 *
 * $Id: pfstmo_mantiuk06.cpp,v 1.9 2008/06/16 22:09:33 rafm Exp $
 */

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>

#include "contrast_domain.h"

#include "Libpfs/colorspace/colorspace.h"
#include "Libpfs/frame.h"
#include "Libpfs/pfs.h"
#include "Libpfs/progress.h"

//--- default tone mapping parameters;
// float scaleFactor = 0.1f;
// float saturationFactor = 0.8f;
// bool cont_map = false, cont_eq = false
// bool cont_map = false;

namespace {
const int itmax = 200;
const float tol = 5e-3f;
}

void pfstmo_mantiuk06(pfs::Frame &frame, float scaleFactor,
                      float saturationFactor, float detailFactor, bool cont_eq,
                      pfs::Progress &ph) {
#ifndef NDEBUG
    std::stringstream ss;

    ss << "pfstmo_mantiuk06 (";
    if (!cont_eq) {
        ss << "Mode: Contrast Mapping, ";
    } else {
        // scaleFactor = -scaleFactor; this was only executed in release mode!!!
        ss << "Mode: Contrast Equalization, ";
    }

    ss << "scaleFactor: " << scaleFactor;
    ss << ", saturationFactor: " << saturationFactor;
    ss << ", detailFactor: " << detailFactor << ")" << std::endl;

    std::cout << ss.str();
#endif
    ph.setValue(0);

    if (cont_eq) scaleFactor = -scaleFactor;

    pfs::Channel *inRed, *inGreen, *inBlue;
    frame.getXYZChannels(inRed, inGreen, inBlue);

    if (!inRed || !inGreen || !inBlue) {
        throw pfs::Exception("Missing X, Y, Z channels in the PFS stream");
    }

    const int cols = frame.getWidth();
    const int rows = frame.getHeight();

    pfs::Array2Df inY(cols, rows);
    pfs::transformRGB2Y(inRed, inGreen, inBlue, &inY);

    try {
        tmo_mantiuk06_contmap(*inRed, *inGreen, *inBlue, inY, scaleFactor,
                              saturationFactor, detailFactor, itmax, tol, ph);
    } catch (...) {
        throw pfs::Exception("Tonemapping Failed!");
    }

    frame.getTags().setTag("LUMINANCE", "RELATIVE");
    if (!ph.canceled()) ph.setValue(100);
}
