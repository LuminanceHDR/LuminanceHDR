/*
 * @brief Ferwerda Tone Mapping Operator:
 *    "A Model of Visual Adaptation for Realistic Image Synthesis"
 *     by James A. Ferwerda, Sumanta N. Pattanaik, Peter Shirley, Donald P. Greenberg
 *
 *
 * This file is a part of LuminanceHDR package
 * ----------------------------------------------------------------------
 * Copyright (C) 2018 Franco Comida
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
 * @author Franco Comida, <fcomida@users.sourceforge.net>
 *
 */

#include <stdlib.h>
#include <cassert>
#include <cmath>
#include <iostream>

#include "Libpfs/colorspace/colorspace.h"
#include "Libpfs/frame.h"
#include "Libpfs/progress.h"

#include "tmo_ferwerda96.h"
#include "../../sleef.c"
#include "../../opthelper.h"

void pfstmo_ferwerda96(pfs::Frame &frame, float Ld_Max,
                        float L_da, pfs::Progress &ph) {

#ifndef NDEBUG
    //--- default tone mapping parameters;
    std::cout << "pfstmo_ferwerda96 (";
    std::cout << "max luminance: " << Ld_Max;
    std::cout << ", adaptation luminance: " << L_da << ")" << std::endl;
#endif

    ph.setValue(0);

    pfs::Channel *inX, *inY, *inZ;
    frame.getXYZChannels(inY, inX, inZ);
    assert(inX != NULL);
    assert(inY != NULL);
    assert(inZ != NULL);
    if (!inX || !inY || !inZ) {
        throw pfs::Exception("Missing X, Y, Z channels in the PFS stream");
    }

    int w = inX->getCols();
    int h = inX->getRows();

    pfs::Array2Df L(w, h);
    transformRGB2Y(inX, inY, inZ, &L);

    try {
            tmo_ferwerda96(inX, inY, inZ, &L,
                           Ld_Max, L_da,
                           ph);
    } catch (...) {
        throw pfs::Exception("Tonemapping Failed!");
    }

    frame.getTags().setTag("LUMINANCE", "DISPLAY");

    if (!ph.canceled()) {
        ph.setValue(100);
    }
}
