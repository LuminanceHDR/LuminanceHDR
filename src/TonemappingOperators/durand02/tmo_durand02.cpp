/**
 * @file tmo_bilateral.cpp
 * @brief Local tone mapping operator based on bilateral filtering.
 * Durand et al. 2002
 *
 * Fast Bilateral Filtering for the Display of High-Dynamic-Range Images.
 * F. Durand and J. Dorsey.
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
 * $Id: tmo_durand02.cpp,v 1.6 2009/02/23 19:09:41 rafm Exp $
 */

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

#include "Libpfs/array2d.h"
#include "Libpfs/progress.h"
#include "TonemappingOperators/pfstmo.h"

//#undef HAVE_FFTW3F

#ifdef HAVE_FFTW3F
#include "fastbilateral.h"
#else
#include "bilateral.h"
#endif

namespace {
/**
 * @brief Find minimum and maximum value skipping the extreems
 *
 */
inline void findMaxMinPercentile(pfs::Array2Df *I, float minPrct, float maxPrct,
                                 float &minLum, float &maxLum) {
    int size = I->getRows() * I->getCols();
    std::vector<float> vI;

    for (int i = 0; i < size; i++) {
        if ((*I)(i) != 0.0f) vI.push_back((*I)(i));
    }

    std::sort(vI.begin(), vI.end());

    minLum = vI.at(int(minPrct * vI.size()));
    maxLum = vI.at(int(maxPrct * vI.size()));
}

template <typename T>
inline T decode(const T &value) {
    if (value <= 0.0031308f) {
        return (value * 12.92f);
    }
    return (1.055f * std::pow(value, 1.f / 2.4f) - 0.055f);
}
}

/*

From Durand's webpage:
<http://graphics.lcs.mit.edu/~fredo/PUBLI/Siggraph2002/>

Here is the high-level set of operation that you need to do in order
to perform contrast reduction

input intensity= 1/61*(R*20+G*40+B)
r=R/(input intensity), g=G/input intensity, B=B/input intensity
log(base)=Bilateral(log(input intensity))
log(detail)=log(input intensity)-log(base)
log (output intensity)=log(base)*compressionfactor+log(detail)
R output = r*exp(log(output intensity)), etc.

*/

void tmo_durand02(pfs::Array2Df &R, pfs::Array2Df &G, pfs::Array2Df &B,
                  float sigma_s, float sigma_r, float baseContrast,
                  int downsample, bool color_correction, pfs::Progress &ph) {
    int w = R.getCols();
    int h = R.getRows();
    int size = w * h;

    pfs::Array2Df I(w, h);       // intensities
    pfs::Array2Df BASE(w, h);    // base layer
    pfs::Array2Df DETAIL(w, h);  // detail layer

    float min_pos = 1e10f;  // minimum positive value (to avoid log(0))
    for (int i = 0; i < size; i++) {
        I(i) = 1.0f / 61.0f * (20.0f * R(i) + 40.0f * G(i) + B(i));
        if (I(i) < min_pos && I(i) > 0.0f) {
            min_pos = I(i);
        }
    }

    for (int i = 0; i < size; i++) {
        float L = I(i);
        if (L <= 0.0f) {
            L = min_pos;
        }

        R(i) /= L;
        G(i) /= L;
        B(i) /= L;

        I(i) = std::log(L);
    }

#ifdef HAVE_FFTW3F
    fastBilateralFilter(I, BASE, sigma_s, sigma_r, downsample, ph);
#else
    bilateralFilter(&I, &BASE, sigma_s, sigma_r, ph);
#endif

    //!! FIX: find minimum and maximum luminance, but skip 1% of outliers
    float maxB;
    float minB;
    findMaxMinPercentile(&BASE, 0.01f, 0.99f, minB, maxB);

    float compressionfactor = baseContrast / (maxB - minB);

    // Color correction factor
    const float k1 = 1.48f;
    const float k2 = 0.82f;
    const float s = ((1 + k1) * pow(compressionfactor, k2)) /
                    (1 + k1 * pow(compressionfactor, k2));

    for (int i = 0; i < size; i++) {
        DETAIL(i) = I(i) - BASE(i);
        I(i) = BASE(i) * compressionfactor + DETAIL(i);

        //!! FIX: this to keep the output in normalized range 0.01 - 1.0
        // intensitites are related only to minimum luminance because I
        // would say this is more stable over time than using maximum
        // luminance and is also robust against random peaks of very high
        // luminance
        I(i) -= 4.3f + minB * compressionfactor;

        if (color_correction) {
            R(i) = decode(std::pow(R(i), s) * std::exp(I(i)));
            G(i) = decode(std::pow(G(i), s) * std::exp(I(i)));
            B(i) = decode(std::pow(B(i), s) * std::exp(I(i)));
        } else {
            R(i) *= decode(std::exp(I(i)));
            G(i) *= decode(std::exp(I(i)));
            B(i) *= decode(std::exp(I(i)));
        }
    }

    if (!ph.canceled()) {
        ph.setValue(100);
    }
}
