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

#include "fastbilateral.h"

#define BENCHMARK
#include "../../StopWatch.h"
#include "../../sleef.c"
#include "../../opthelper.h"
#define pow_F(a,b) (xexpf(b*xlogf(a)))
#include "LUT.h"

namespace {
/**
 * @brief Find minimum and maximum value skipping the extreems
 *
 */
inline void findMaxMinPercentile(pfs::Array2Df *I, float minPrct, float maxPrct,
                                 float &minLum, float &maxLum) {

    int size = I->getRows() * I->getCols();
    const float* data = I->data();

    float minval = data[0];
    float maxval = data[0];
#ifdef _OPENMP
    #pragma omp parallel for reduction(min:minval) reduction(max:maxval)
#endif
    for (int i = 1; i < size; ++i) {
        minval = std::min(minval, data[i]);
        maxval = std::max(maxval, data[i]);
    }

    float offset = minval < 0.f ? -minval : 0.f;
    float scale = 65535.f / (maxval + offset);

    // we need to find the (minPrct*size) smallest value and the (maxPrct*size) smallest value in I
    // We use a histogram based search for speed and to reduce memory usage
    // memory usage of this method is 65536 * sizeof(float) * (t + 1) byte, where t is the number of threads

    // We need one global histogram
    LUTu histo (65536, LUT_CLIP_BELOW | LUT_CLIP_ABOVE);
    histo.clear();
#ifdef _OPENMP
    #pragma omp parallel
#endif
    {
        // We need one histogram per thread
        LUTu histothr (65536, LUT_CLIP_BELOW | LUT_CLIP_ABOVE);
        histothr.clear();

#ifdef _OPENMP
        #pragma omp for nowait
#endif

        for (int i = 0; i < size; ++i) {
            // values are in [0;1] range, so we have to multiply with 65535 to get the histogram index
            histothr[ (unsigned int) (scale * (data[i] + offset))]++;
        }

#ifdef _OPENMP
        #pragma omp critical
#endif
        // add per thread histogram to global histogram
        histo += histothr;
    }

    int k = 0;
    int count = 0;

    // find (minPrct*size) smallest value
    while (count < minPrct * size) {
        count += histo[k++];
    }

    if (k > 0) { // interpolate
        int count_ = count - histo[k - 1];
        float c0 = count - minPrct * size;
        float c1 = minPrct * size - count_;
        minLum = (c1 * k + c0 * (k - 1)) / ((c0 + c1) * scale);
    } else {
        minLum = k / scale;
    }
    minLum -= offset;
    // find (maxPrct*size) smallest value
    while (count < maxPrct * size) {
        count += histo[k++];
    }

    if (k > 0) { // interpolate
        int count_ = count - histo[k - 1];
        float c0 = count - maxPrct * size;
        float c1 = maxPrct * size - count_;
        maxLum = (c1 * k + c0 * (k - 1)) / ((c0 + c1) * scale);
    } else {
        maxLum = k / scale;
    }
    maxLum -= offset;
}

template <typename T>
inline T decode(const T &value) {
    if (value <= 0.0031308f) {
        return (value * 12.92f);
    }
    return (1.055f * pow_F(value, 1.f / 2.4f) - 0.055f);
}

template <typename T>
inline T fastDecode(const T &value) {
    if (value <= -5.766466716f) {
        return (xexpf(value) * 12.92f);
    }
    return (1.055f * xexpf(1.f / 2.4f * value) - 0.055f);
}
#ifdef __SSE2__
inline vfloat fastDecode(const vfloat &valuev, const vfloat &c0, const vfloat &c1, const vfloat &c2, const vfloat &c3, const vfloat &c4) {
    vmask selmask = vmaskf_le(valuev, c0);
    vfloat tempv = vself(selmask, valuev, valuev * c1);
    tempv = xexpf(tempv);
    return vself(selmask, tempv * c2, tempv * c3 - c4);
}
#endif
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
BENCHFUN
    int w = R.getCols();
    int h = R.getRows();
    int size = w * h;

    pfs::Array2Df I(w, h);       // intensities
    pfs::Array2Df BASE(w, h);    // base layer

    float min_pos = 1e10f;  // minimum positive value (to avoid log(0))
    vfloat min_posv = F2V(1e10f);

#ifdef _OPENMP
#pragma omp parallel
#endif
{
    float min_posthr = 1e10f;
#ifdef __SSE2__
    vfloat min_posthrv = F2V(min_posthr);
    vfloat onev = F2V(1.f);
    vfloat c61v = F2V(61.f);
    vfloat c20v = F2V(20.f);
    vfloat c40v = F2V(40.f);
    vfloat cmaxv = F2V(1e10f);
#endif

#ifdef _OPENMP
    #pragma omp for nowait
#endif
    for (int i = 0; i < h; i++) {
        int j = 0;
#ifdef __SSE2__
        for (; j < w - 3; j+=4) {
            vfloat Iv = onev / c61v * (c20v * LVFU(R(j, i)) + c40v * LVFU(G(j, i)) + LVFU(B(j,i)));
            vfloat tminv = vself(vmaskf_gt(Iv, ZEROV), Iv, cmaxv);
            min_posthrv = vminf(min_posthrv, tminv);
            STVFU(I(j,i), Iv);
        }
#endif
        for (; j < w; j++) {
            I(j, i) = 1.0f / 61.0f * (20.0f * R(j, i) + 40.0f * G(j, i) + B(j, i));
            if (I(j, i) < min_posthr && I(j, i) > 0.0f) {
                min_posthr = I(j, i);
            }
        }
    }
#ifdef _OPENMP
    #pragma omp critical
#endif
{
    min_pos = std::min(min_pos, min_posthr);
    min_posv = vminf(min_posv, min_posthrv);
}
}
    min_pos = std::min(min_pos, vhmin(min_posv));

#ifdef _OPENMP
#pragma omp parallel
#endif
{
#ifdef __SSE2__
    vfloat min_posv = F2V(min_pos);
#endif
#ifdef _OPENMP
    #pragma omp for
#endif
    for (int i = 0; i < h; i++) {
        int j = 0;
#ifdef __SSE2__
        for (; j < w-3; j+=4) {
            vfloat Lv = LVFU(I(j, i));
            Lv = vmaxf(Lv, min_posv);

            STVFU(R(j, i), LVFU(R(j, i)) / Lv);
            STVFU(G(j, i), LVFU(G(j, i)) / Lv);
            STVFU(B(j, i), LVFU(B(j, i)) / Lv);

            STVFU(I(j, i), xlogf(Lv));
        }
#endif
        for (; j < w; j++) {
            float L = I(j, i);
            L = std::max(L, min_pos);

            R(j, i) /= L;
            G(j, i) /= L;
            B(j, i) /= L;

            I(j, i) = xlogf(L);
        }
    }
}

    fastBilateralFilter(I, BASE, sigma_s, sigma_r, downsample, ph);

    //!! FIX: find minimum and maximum luminance, but skip 1% of outliers
    float maxB;
    float minB;
    findMaxMinPercentile(&BASE, 0.01f, 0.99f, minB, maxB);

    float compressionfactor = baseContrast / (maxB - minB);
    float compressionfactorm1 = compressionfactor - 1.f;

    // Color correction factor
    constexpr float k1 = 1.48f;
    constexpr float k2 = 0.82f;
    const float s = ((1 + k1) * pow(compressionfactor, k2)) /
                    (1 + k1 * pow(compressionfactor, k2));

    if (color_correction) {

#ifdef _OPENMP
#pragma omp parallel
#endif
{
#ifdef __SSE2__
        vfloat offsetv = F2V(4.3f + minB * compressionfactor);
        vfloat compressionfactorm1v = F2V(compressionfactorm1);
        vfloat sv = F2V(s);
        const vfloat c0 = F2V(-5.7664667f);
        const vfloat c1 = F2V(0.416666667f);
        const vfloat c2 = F2V(12.92f);
        const vfloat c3 = F2V(1.055f);
        const vfloat c4 = F2V(0.055f);
#endif
#ifdef _OPENMP
        #pragma omp for
#endif
        for (int i = 0; i < h; i++) {
            int j = 0;
#ifdef __SSE2__
            for (; j < w - 3; j+=4) {
                vfloat Iiv = LVFU(BASE(j, i)) * compressionfactorm1v + LVFU(I(j, i)) - offsetv;

                //!! FIX: this to keep the output in normalized range 0.01 - 1.0
                // intensitites are related only to minimum luminance because I
                // would say this is more stable over time than using maximum
                // luminance and is also robust against random peaks of very high
                // luminance
//                Iiv -= offsetv;

                STVFU(R(j, i), fastDecode(sv * xlogf(LVFU(R(j,i))) + Iiv, c0, c1, c2, c3, c4));
                STVFU(G(j, i), fastDecode(sv * xlogf(LVFU(G(j,i))) + Iiv, c0, c1, c2, c3, c4));
                STVFU(B(j, i), fastDecode(sv * xlogf(LVFU(B(j,i))) + Iiv, c0, c1, c2, c3, c4));
            }
#endif
            for (; j < w; j++) {
                float Ii = BASE(j, i) * compressionfactorm1 + I(j, i);

                //!! FIX: this to keep the output in normalized range 0.01 - 1.0
                // intensitites are related only to minimum luminance because I
                // would say this is more stable over time than using maximum
                // luminance and is also robust against random peaks of very high
                // luminance
                Ii -= 4.3f + minB * compressionfactor;

                R(j, i) = fastDecode(s * xlogf(R(j,i)) + Ii);
                G(j, i) = fastDecode(s * xlogf(G(j,i)) + Ii);
                B(j, i) = fastDecode(s * xlogf(B(j,i)) + Ii);
            }
        }
}
    } else {
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for (int i = 0; i < size; i++) {
            float Ii = BASE(i) * compressionfactorm1 + I(i);

            //!! FIX: this to keep the output in normalized range 0.01 - 1.0
            // intensitites are related only to minimum luminance because I
            // would say this is more stable over time than using maximum
            // luminance and is also robust against random peaks of very high
            // luminance
            Ii -= 4.3f + minB * compressionfactor;

            float expi = xexpf(Ii);
            R(i) *= decode(expi);
            G(i) *= decode(expi);
            B(i) *= decode(expi);
        }
    }

    if (!ph.canceled()) {
        ph.setValue(100);
    }
}
