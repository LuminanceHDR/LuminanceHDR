/*
 * @brief KimKautz Tone Mapping Operator:
 *    "Consistent Tone Reproduction"
 *	  by Min H. Kim, Jan Kautz
 *    in CGIM '08 Proceedings of the Tenth IASTED
 *    International Conference on Computer Graphics and Imaging  2008
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
#include "Libpfs/manip/copy.h"
#include "Libpfs/frame.h"
#include "Libpfs/progress.h"

#include "tmo_kimkautz08.h"
#include "../../opthelper.h"
#include "../../sleef.c"

using namespace pfs;

void pfstmo_kimkautz08(Frame &frame, float KK_c1, float KK_c2,
                        Progress &ph) {

#ifndef NDEBUG
    //--- default tone mapping parameters;
    std::cout << "pfstmo_kimkautz08 (";
    std::cout << ", KK_c2: " << KK_c2 << ")" << std::endl;
#endif

    ph.setValue(0);

    Channel *inX, *inY, *inZ;
    frame.getXYZChannels(inX, inY, inZ);
    assert(inX != NULL);
    assert(inY != NULL);
    assert(inZ != NULL);
    if (!inX || !inY || !inZ) {
        throw Exception("Missing X, Y, Z channels in the PFS stream");
    }

    int w = inX->getCols();
    int h = inY->getRows();

    Array2Df L(w, h);
    Array2Df Lold(w, h);

    transformRGB2Y(inX, inY, inZ, &L);
    copy(L.begin(), L.end(), Lold.begin());

    try {
            tmo_kimkautz08(L, KK_c1, KK_c2, ph);
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

        frame.getTags().setTag("LUMINANCE", "DISPLAY");

        ph.setValue(100);
    }
}
