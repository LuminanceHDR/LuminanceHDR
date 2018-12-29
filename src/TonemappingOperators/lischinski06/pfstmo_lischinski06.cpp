/*
 * @brief Lischinski Tone Mapping Operator:
 *    "Interactive Local Adjustment of Tonal Values"
 *     by Dani Lischinski, Zeev Farbman, Matt Uyttendaele, Richard Szeliski
 *     in Proceedings of SIGGRAPH 2006
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
 * Code adapted for Luminance HDR from:
 *
 *  PICCANTE
 *  The hottest HDR imaging library!
 *  http://vcg.isti.cnr.it/piccante
 *  Copyright (C) 2014
 *  Visual Computing Laboratory - ISTI CNR
 *  http://vcg.isti.cnr.it
 *  First author: Francesco Banterle
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

#include "tmo_lischinski06.h"
#include "../../opthelper.h"
#include "../../sleef.c"
#define BENCHMARK
#include "StopWatch.h"
using namespace pfs;

void pfstmo_lischinski06(Frame &frame, float alpha_mul,
                         Progress &ph) {

#ifndef NDEBUG
    //--- default tone mapping parameters;
    std::cout << "pfstmo_lischinski06 (";
    std::cout << "alpha_mul: " << alpha_mul << ")" << std::endl;
#endif
BENCHFUN
    ph.setValue(0);

    Channel *inX, *inY, *inZ;
    frame.getXYZChannels(inX, inY, inZ);
    assert(inX != NULL);
    assert(inY != NULL);
    assert(inZ != NULL);
    if (!inX || !inY || !inZ) {
        throw Exception("Missing X, Y, Z channels in the PFS stream");
    }

    const int w = inX->getCols();
    const int h = inY->getRows();

    Array2Df L(w, h);

    transformRGB2Y(inX, inY, inZ, &L);

    try {
            tmo_lischinski06(L, *inX, *inY, *inZ, alpha_mul, ph);
    } catch (...) {
        throw Exception("Tonemapping Failed!");
    }

    frame.getTags().setTag("LUMINANCE", "DISPLAY");

    ph.setValue(100);
}
