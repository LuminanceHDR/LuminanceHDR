/**
 * @file pfstmo_reinhard02.cpp
 * @brief Tone map XYZ channels using Reinhard02 model
 *
 * Photographic Tone Reproduction for Digital Images.
 * E. Reinhard, M. Stark, P. Shirley, and J. Ferwerda.
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
 * $Id: pfstmo_reinhard02.cpp,v 1.3 2008/09/04 12:46:49 julians37 Exp $
 */

#include <math.h>
#include <cstddef>
#include <iostream>

#include "Libpfs/exception.h"
#include "Libpfs/frame.h"
#include "Libpfs/progress.h"
#include "tmo_reinhard02.h"
#include "../../opthelper.h"
#define BENCHMARK
#include "../../StopWatch.h"

void pfstmo_reinhard02(pfs::Frame &frame, float key, float phi, int num,
                       int low, int high, bool use_scales, pfs::Progress &ph) {
    BENCHFUN
    //--- default tone mapping parameters;
    // float key = 0.18;
    // float phi = 1.0;
    // int num = 8;
    // int low = 1;
    // int high = 43;
    // bool use_scales = false;
    bool temporal_coherent = false;
#ifndef NDEBUG
    std::cout << "pfstmo_reinhard02 (";
    std::cout << "key: " << key;
    std::cout << ", phi: " << phi;
    std::cout << ", range: " << num;
    std::cout << ", lower scale: " << low;
    std::cout << ", upper scale: " << high;
    std::cout << ", use scales: " << use_scales << ")" << std::endl;
#endif
    pfs::Channel *X, *Y, *Z;
    frame.getXYZChannels(X, Y, Z);
    //---

    if (Y == NULL || X == NULL || Z == NULL) {
        throw pfs::Exception("Missing X, Y, Z channels in the PFS stream");
    }

    frame.getTags().setTag("LUMINANCE", "RELATIVE");
    // tone mapping
    size_t w = Y->getWidth();
    size_t h = Y->getHeight();
    pfs::Array2Df L(w, h);

    Reinhard02 tmoperator(Y, &L, use_scales, key, phi, num, low, high,
                          temporal_coherent, ph);

    try {
        tmoperator.tmo_reinhard02();
    } catch (...) {
        throw pfs::Exception("Tonemapping Failed!");
    }

    // TODO: this section can be rewritten using SSE Function
    // DONE

    #pragma omp parallel for
    for (size_t y = 0; y < h; y++) {
        size_t x = 0;
#ifdef __SSE2__
        for (; x < w - 3; x+=4) {
            vfloat yrv = LVFU((*Y)(x, y));
            vmask selmask = vmaskf_eq(yrv, ZEROV);
            vfloat scalev = vselfnotzero(selmask, LVFU(L(x, y)) / yrv);

            STVFU((*Y)(x, y), yrv * scalev);
            STVFU((*X)(x, y), LVFU((*X)(x, y)) * scalev);
            STVFU((*Z)(x, y), LVFU((*Z)(x, y)) * scalev);
        }
#endif
        for (; x < w; x++) {
            float yr = (*Y)(x, y);
            float scale = yr != 0.f ? L(x, y) / yr : 0.f;

            (*Y)(x, y) *= scale;
            (*X)(x, y) *= scale;
            (*Z)(x, y) *= scale;
        }
    }

    if (!ph.canceled()) {
        ph.setValue(100);
    }
}
