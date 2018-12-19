/**
 * @brief Ashikhmin Tone Mapping Operator: tone reproduction for displaying high
 * contrast scenes
 * Michael Ashikhmin 2002
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
 * @author Akiko Yoshida, <yoshida@mpi-sb.mpg.de>
 * @author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
 *
 * $Id: pfstmo_ashikhmin02.cpp,v 1.5 2004/12/15 10:11:03 krawczyk Exp $
 */

#include <stdlib.h>
#include <cassert>
#include <cmath>
#include <iostream>

#include "Libpfs/colorspace/colorspace.h"
#include "Libpfs/frame.h"
#include "Libpfs/progress.h"

#include "tmo_ashikhmin02.h"
#include "../../sleef.c"
#include "../../opthelper.h"

namespace {
void calculateLuminance(pfs::Array2Df *Y, float &avLum, float &maxLum,
                        float &minLum) {

    avLum = 0.0f;
    maxLum = 0.0f;
    minLum = 0.0f;

#ifdef _OPENMP
#pragma omp parallel
#endif
{
    float maxLumThr = 0.f;
    float minLumThr = 0.f;
    float avLumThr = 0.f;

#ifdef __SSE2__
    vfloat maxLumThrv = ZEROV;
    vfloat minLumThrv = ZEROV;
    vfloat avLumThrv = ZEROV;
    vfloat c1v = F2V(1e-4f);
#endif
#ifdef _OPENMP
    #pragma omp for nowait
#endif
    for (size_t y = 0; y < Y->getRows(); ++y) {
        size_t x = 0;
#ifdef __SSE2__
        for (; x < Y->getCols() - 3; x+=4) {
            vfloat Yv = LVFU((*Y)(x, y));
            avLumThrv += xlogf(Yv + c1v);
            maxLumThrv = vmaxf(Yv, maxLumThrv);
            minLumThrv = vminf(Yv, minLumThrv);
        }
#endif
        for (; x < Y->getCols(); ++x) {
            avLumThr += xlogf((*Y)(x, y) + 1e-4f);
            maxLumThr = std::max((*Y)(x, y), maxLumThr);
            minLumThr = std::min((*Y)(x, y), minLumThr);
        }
    }
#ifdef _OPENMP
#pragma omp critical
#endif
{
    avLum += avLumThr;
    maxLum = std::max(maxLum, maxLumThr);
    minLum = std::max(minLum, minLumThr);
#ifdef __SSE2__
    avLum += vhadd(avLumThrv);
    maxLum = std::max(maxLum, vhmax(maxLumThrv));
    minLum = std::max(minLum, vhmin(minLumThrv));
#endif
}
}
    avLum = exp(avLum / (Y->getRows() * Y->getCols()));
}
}

void pfstmo_ashikhmin02(pfs::Frame &frame, bool simple_flag, float lc_value,
                        int eq, pfs::Progress &ph) {

#ifndef NDEBUG
    //--- default tone mapping parameters;
    std::cout << "pfstmo_ashikhmin02 (";
    std::cout << "simple: " << simple_flag;
    std::cout << ", lc_value: " << lc_value;
    std::cout << ", eq: " << eq << ")" << std::endl;
#endif

    ph.setValue(0);

    pfs::Channel *Xr, *Yr, *Zr;
    frame.getXYZChannels(Xr, Yr, Zr);
    assert(Xr != NULL);
    assert(Yr != NULL);
    assert(Zr != NULL);
    if (!Xr || !Yr || !Zr) {
        throw pfs::Exception("Missing X, Y, Z channels in the PFS stream");
    }

    pfs::transformColorSpace(pfs::CS_RGB, Xr, Yr, Zr, pfs::CS_XYZ, Xr, Yr, Zr);
    float maxLum, avLum, minLum;
    calculateLuminance(Yr, avLum, maxLum, minLum);

    int w = Yr->getCols();
    int h = Yr->getRows();

    pfs::Array2Df L(w, h);
    try {
        tmo_ashikhmin02(Yr, &L, maxLum, minLum, avLum, simple_flag, lc_value,
                        eq, ph);
    } catch (...) {
        throw pfs::Exception("Tonemapping Failed!");
    }

    // TODO: this section can be rewritten using SSE Function
#ifdef _OPENMP
    #pragma omp parallel for
#endif
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float scale = L(x, y) / (*Yr)(x, y);
            (*Yr)(x, y) = (*Yr)(x, y) * scale;
            (*Xr)(x, y) = (*Xr)(x, y) * scale;
            (*Zr)(x, y) = (*Zr)(x, y) * scale;
        }
    }

    pfs::transformColorSpace(pfs::CS_XYZ, Xr, Yr, Zr, pfs::CS_RGB, Xr, Yr, Zr);

    ph.setValue(100);
}
