/*
 * @brief VanHateren Tone Mapping Operator:
 *    "Encoding of High Dynamic Range Video with a Model of Human Cones"
 * 	  by J. Hans Van Hateren
 *    in ACM Transaction on Graphics 2006
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
#include "Libpfs/manip/gamma.h"
#include "Libpfs/frame.h"
#include "Libpfs/progress.h"

#include "tmo_vanhateren06.h"
#include "../../sleef.c"
#include "../../opthelper.h"

using namespace pfs;

void pfstmo_vanhateren06(Frame &frame, float pupil_area, Progress &ph) {

#ifndef NDEBUG
    //--- default tone mapping parameters;
    std::cout << "pfstmo_vanhateren06 (";
    std::cout << "pupil area: " << pupil_area << ")" << std::endl;
#endif

    ph.setValue(0);

    applyGamma(&frame, 1.8f);

    Channel *inX, *inY, *inZ;
    frame.getXYZChannels(inY, inX, inZ);
    assert(inX != NULL);
    assert(inY != NULL);
    assert(inZ != NULL);
    if (!inX || !inY || !inZ) {
        throw Exception("Missing X, Y, Z channels in the PFS stream");
    }

    int w = inX->getCols();
    int h = inX->getRows();

    Array2Df L(w, h);
    Array2Df Lold(w, h);
    transformRGB2Y(inX, inY, inZ, &L);
    copy(L.begin(), L.end(), Lold.begin());

    try {
            tmo_vanhateren06(L, pupil_area, ph);
    } catch (...) {
        throw Exception("Tonemapping Failed!");
    }

    if (!ph.canceled()) {
        Array2Df &arrayRed = *inX;
        Array2Df &arrayGreen = *inY;
        Array2Df &arrayBlue = *inZ;

        #pragma omp parallel for
        for (int i = 0; i < h; ++i) {
            int j = 0;
#ifdef __SSE2__
            for (; j < w - 3; j += 4) {
                vfloat scalev = LVFU(L(j, i)) / LVFU(Lold(j, i));
                STVFU(arrayRed(j, i), LVFU(arrayRed(j, i)) * scalev);
                STVFU(arrayGreen(j, i), LVFU(arrayGreen(j, i)) * scalev);
                STVFU(arrayBlue(j, i), LVFU(arrayBlue(j, i)) * scalev);
            }
#endif
            for (; j < w; ++j) {
                float scale = L(j, i) / Lold(j, i);
                arrayRed(j, i) = arrayRed(j, i) * scale;
                arrayGreen(j, i) = arrayGreen(j, i) * scale;
                arrayBlue(j, i) = arrayBlue(j, i) * scale;
            }
        }
    }

    frame.getTags().setTag("LUMINANCE", "DISPLAY");

    ph.setValue(100);
}
