/**
 * @file pfstmo_fattal02.cpp
 * @brief Tone map XYZ channels using Fattal02 model
 *
 * Gradient Domain High Dynamic Range Compression
 * R. Fattal, D. Lischinski, and M. Werman
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
 * $Id: pfstmo_fattal02.cpp,v 1.3 2008/09/04 12:46:49 julians37 Exp $
 */

#include "tmo_fattal02.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>

#include "Libpfs/colorspace/colorspace.h"
#include "Libpfs/exception.h"
#include "Libpfs/frame.h"
#include "Libpfs/progress.h"
#include "../../opthelper.h"
#include "../../sleef.c"
#define pow_F(a,b) (xexpf(b*xlogf(a)))

namespace {
const float epsilon = 1e-4f;
}

void pfstmo_fattal02(pfs::Frame &frame, float opt_alpha, float opt_beta,
                     float opt_saturation, float opt_noise, bool newfattal,
                     bool fftsolver, int detail_level, pfs::Progress &ph) {

    if (fftsolver) {
        // opt_alpha = 1.f;
        newfattal = true;  // let's make sure, prudence is never enough!
    }

    if (opt_noise <= 0.0f) {
        opt_noise = opt_alpha * 0.01f;
    }
#ifndef NDEBUG
    std::stringstream ss;
    ss << "pfstmo_fattal02 (";
    ss << "alpha: " << opt_alpha;
    ss << ", beta: " << opt_beta;
    ss << ". saturation: " << opt_saturation;
    ss << ", noise: " << opt_noise;
    ss << ", fftsolver: " << fftsolver << ")";
    std::cout << ss.str() << std::endl;
#endif

    ph.setValue(0);

    // Store RGB data temporarily in XYZ channels
    pfs::Channel *R, *G, *B;
    frame.getXYZChannels(R, G, B);
    //---

    if (!R || !G || !B) {
        throw pfs::Exception("Missing X, Y, Z channels in the PFS stream");
    }

    frame.getTags().setTag("LUMINANCE", "RELATIVE");
    // tone mapping
    const int w = frame.getWidth();
    const int h = frame.getHeight();

    pfs::Array2Df Yr(w, h);
    pfs::Array2Df L(w, h);

    pfs::transformRGB2Y(R, G, B, &Yr);

    try {
        tmo_fattal02(w, h, Yr, L, opt_alpha, opt_beta, opt_noise, newfattal,
                     fftsolver, detail_level, ph);
    } catch (...) {
        throw pfs::Exception("Tonemapping Failed!");
    }

    if (!ph.canceled()) {
        pfs::Array2Df &arrayRed = *R;
        pfs::Array2Df &arrayGreen = *G;
        pfs::Array2Df &arrayBlue = *B;

#ifdef __SSE2__
        const vfloat epsilonv = F2V(epsilon);
        const vfloat opt_saturationv = F2V(opt_saturation);
#endif
        #pragma omp parallel for
        for (int i = 0; i < h; ++i) {
            int j = 0;
#ifdef __SSE2__
            for (; j < w - 3; j += 4) {
                vfloat yv = vmaxf(LVFU(Yr(j, i)), epsilonv);
                vfloat lv = vmaxf(LVFU(L(j, i)), epsilonv);
                STVFU(arrayRed(j, i), pow_F(vmaxf(LVFU(arrayRed(j, i)) / yv, ZEROV), opt_saturationv) * lv);
                STVFU(arrayGreen(j, i), pow_F(vmaxf(LVFU(arrayGreen(j, i)) / yv, ZEROV), opt_saturationv) * lv);
                STVFU(arrayBlue(j, i), pow_F(vmaxf(LVFU(arrayBlue(j, i)) / yv, ZEROV), opt_saturationv) * lv);
            }
#endif
            for (; j < w; ++j) {
                float y = std::max(Yr(j, i), epsilon);
                float l = std::max(L(j, i), epsilon);
                arrayRed(j, i) = pow_F(std::max(arrayRed(j, i) / y, 0.f), opt_saturation) * l;
                arrayGreen(j, i) = pow_F(std::max(arrayGreen(j, i) / y, 0.f), opt_saturation) * l;
                arrayBlue(j, i) = pow_F(std::max(arrayBlue(j, i) / y, 0.f), opt_saturation) * l;
            }
        }

        ph.setValue(100);
    }
}
