/**
 * @file pfstmo_reinhard05.cpp
 * @brief Tone map XYZ channels using Reinhard05 model
 *
 * Dynamic Range Reduction Inspired by Photoreceptor Physiology.
 * E. Reinhard and K. Devlin.
 * In IEEE Transactions on Visualization and Computer Graphics, 2005.
 *
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
 * @author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
 *
 * $Id: pfstmo_reinhard05.cpp,v 1.2 2008/09/04 12:46:49 julians37 Exp $
 */

#include "tmo_reinhard05.h"

#include <cmath>
#include <iostream>
#include <sstream>

#include "Libpfs/colorspace/colorspace.h"
#include "Libpfs/exception.h"
#include "Libpfs/frame.h"
#include "Libpfs/progress.h"

void pfstmo_reinhard05(pfs::Frame &frame, float brightness,
                       float chromaticadaptation, float lightadaptation,
                       pfs::Progress &ph) {
//--- default tone mapping parameters;
// float brightness = 0.0f;
// float chromaticadaptation = 0.5f;
// float lightadaptation = 0.75f;

#ifndef NDEBUG
    std::stringstream ss;

    ss << "pfstmo_reinhard05 (";
    ss << "brightness: " << brightness << ", ";
    ss << "chromatic adaptation: " << chromaticadaptation << ", ";
    ss << "light adaptation: " << lightadaptation << ") " << std::endl;

    std::cout << ss.str();
#endif

    pfs::Channel *R, *G, *B;
    frame.getXYZChannels(R, G, B);
    //---

    if (!R || !G || !B) {
        throw pfs::Exception("Missing X, Y, Z channels in the PFS stream");
    }

    frame.getTags().setTag("LUMINANCE", "RELATIVE");
    // tone mapping
    const unsigned int width = frame.getWidth();
    const unsigned int height = frame.getHeight();

    // is there a way to remove this copy as well?
    // I am pretty sure there is!
    pfs::Array2Df Y(width, height);
    pfs::transformRGB2Y(R, G, B, &Y);
    try {
        tmo_reinhard05(
            width, height, R->data(), G->data(), B->data(), Y.data(),
            Reinhard05Params(brightness, chromaticadaptation, lightadaptation),
            ph);
    } catch (...) {
        throw pfs::Exception("Tonemapping Failed!");
    }

    if (!ph.canceled()) {
        ph.setValue(100);
    }
}
