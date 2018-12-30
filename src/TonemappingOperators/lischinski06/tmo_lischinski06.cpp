/*
 * @brief Lischinski Tone Mapping Operator:
 *    "Interactive Local Adjustment of Tonal Values"
 *     by Dani Lischinski, Zeev Farbman, Matt Uyttendaele, Richard Szeliski
 *     in Proceedings of SIGGRAPH 2006
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

#include <iostream>

#include "Libpfs/array2d.h"
#include "Libpfs/progress.h"
#include "Libpfs/rt_algo.h"
#include "Libpfs/utils/msec_timer.h"
#include "tmo_lischinski06.h"
#include "lischinski_minimization.h"
#include "sleef.c"
#include "opthelper.h"


#ifndef CLAMP
    #define CLAMP(x, a)         (x >= a ? (a - 1) : (x < 0 ? 0 : x))
#endif

#define pow_F(a,b) (xexpf(b*xlogf(a)))

using namespace std;
using namespace pfs;

float LogMeanVal(const Array2Df &L) {
    const size_t height = L.getRows();
    const size_t width = L.getCols();
    const size_t size = width*height;

    float avg_loglum = 0.f;
#ifdef _OPENMP
#pragma omp parallel
#endif
{
#ifdef __SSE2__
    vfloat avg_loglumv = ZEROV;
    vfloat epsv = F2V(1e-6f);
#endif
#ifdef _OPENMP
    #pragma omp for reduction(+:avg_loglum) nowait
#endif
    for (size_t j = 0; j < height; ++j) {
        size_t i = 0;
#ifdef __SSE2__
        for (; i < width - 3; i += 4) {
            vfloat value = xlogf(LVFU(L(i,j)) + epsv);
            avg_loglumv += value;
        }
#endif
        for (; i < width; ++i) {
            float value = xlogf(L(i,j) + 1e-6f);
            avg_loglum += value;
        }
    }
#ifdef _OPENMP
#pragma omp critical
#endif
{
#ifdef __SSE2__
    avg_loglum += vhadd(avg_loglumv);
#endif
}
}
    return xexpf(avg_loglum/size);
}

int tmo_lischinski06(Array2Df &L,Array2Df &inX, Array2Df &inY, Array2Df &inZ,
                     const float alpha_mul,
                     Progress &ph) {
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif

    ph.setValue(5);

    int width = L.getCols();
    int height = L.getRows();

    //Percentile clamping
    float maxL, minL;
    lhdrengine::findMinMaxPercentile(L.data(), L.getCols() * L.getRows(), 0.01f, minL, 0.99f, maxL, true);

    const float invLOG2 = 1.0f/xlogf(2.0f);
    float Lav = LogMeanVal(L);
    float maxL_log = xlogf(maxL + 1e-6f) * invLOG2;
    float minL_log = xlogf(minL + 1e-6f) * invLOG2;

    float log2Average = xlogf(Lav + 1e-6f) * invLOG2;
    float alpha = 0.18f * pow_F(4.0f, (2.0f * log2Average - minL_log - maxL_log)/( maxL_log - minL_log));
    float whitePoint = 1.5f * pow_F(2.0f, maxL_log - minL_log - 5.0f);

    alpha *= alpha_mul;

    int Z = int(ceilf(maxL_log - minL_log));

#ifndef NDEBUG
    cout << "maxL:       " << maxL << endl;
    cout << "minL:       " << minL << endl;
    cout << "maxL_log:   " << maxL_log << endl;
    cout << "minL_log:   " << minL_log << endl;
    cout << "Lav:        " << Lav << endl;
    cout << "whitePoint: " << whitePoint << endl;
    cout << "alpha:      " << alpha << endl;
#endif

    float whitePoint_sq = whitePoint * whitePoint;

    if(Z <= 0) {
        return 0;
    }

    //Choose the representative Rz for each zone
    std::vector<float> *zones = new std::vector<float>[Z];
    float *fstop = new float[Z];
    float *Rz = new float[Z];

    for(int i = 0; i < Z; i++) {
        Rz[i] = 0.0f;
        fstop[i] = 0.0f;
    }

    #pragma omp parallel
    {
        std::vector<float> zonesThr[Z];
#ifdef __SSE2__
        std::vector<float> logBuffer(width);
        const vfloat c1em6v = F2V(1e-6f);
        const vfloat invLOG2v = F2V(invLOG2);
        const vfloat minL_logv = F2V(minL_log);
#endif
        #pragma omp for nowait
        for(int i = 0; i < height; i++) {
#ifdef __SSE2__
            int j = 0;
            for(; j + 3 < width; j+=4) {
                STVFU(logBuffer[j], xlogf(LVFU(L(j,i)) + c1em6v) * invLOG2v - minL_logv);
            }
            for(; j < width; ++j) {
                logBuffer[j] = xlogf(L(j,i) + 1e-6f) * invLOG2 - minL_log;
            }
#endif
            for(int j = 0; j < width; j++) {
#ifdef __SSE2__
                const float Lum = L(j,i);
                const float Lum_log = logBuffer[j];
#else
                const float Lum = L(j,i);
                const float Lum_log = xlogf(Lum + 1e-6f) * invLOG2 - minL_log;
#endif
                const int zone = CLAMP(int(ceilf(Lum_log)), Z);

                zonesThr[zone].push_back(Lum);
            }
        }
        #pragma omp critical
        {
            for (int i = 0; i < Z; ++i) {
                for (int j = 0; j < int(zonesThr[i].size()); ++j) {
                    zones[i].push_back(zonesThr[i][j]);
                }
            }
        }
    }

    ph.setValue(25);
    if (ph.canceled()) return 0;

    for(int i = 0; i < Z; i++) {
        int n = int(zones[i].size());
        if(n > 0) {
            float maxL, minL;
            lhdrengine::findMinMaxPercentile(zones[i].data(), n, 0.5f, minL, 0.5f, maxL, true);
            Rz[i] = minL;
            if(Rz[i] > 0.0f) {
                //photographic operator
                float Rz2 = Rz[i] * alpha / Lav;
                float f = (Rz2 * (1 + Rz2 / whitePoint_sq) ) / (1.0f + Rz2);
                fstop[i] = xlogf(f / Rz[i] + 1e-6f) * invLOG2;
            }
        }
    }

    //create the fstop map
    Array2Df L_log(width, height);

#ifdef __SSE2__
    const vfloat epsv = F2V(1e-6f);
    const vfloat invLOG2v = F2V(invLOG2);
#endif
#ifdef _OPENMP
    #pragma omp parallel for
#endif
    for (int i = 0; i < height; ++i) {
        int j = 0;
#ifdef __SSE2__
        for (; j < width - 3; j += 4) {
            vfloat lv = xlogf(LVFU(L(j, i)) + epsv) * invLOG2v;
            STVFU(L_log(j, i), lv);
        }
#endif
        for (; j < width; ++j) {
            L_log(j, i) = xlogf(L(j, i) + 1e-6f) * invLOG2;
        }
    }

    Array2Df fstopMap(width, height);

#ifdef _OPENMP
    #pragma omp parallel for
#endif
    for(int i = 0; i < height; i++) {
        for(int j = 0; j < width; j++) {
            float l_log = L_log(j,i);

            int zone = CLAMP(int(ceilf(l_log - minL_log)), Z);

            fstopMap(j,i) = fstop[zone];
        }
    }

    ph.setValue(50);
    if (ph.canceled()) return 0;

    //Lischinski minimization
    Array2Df fstopMap_min(width, height);
    LischinskiMinimization(L, fstopMap, fstopMap_min);

    ph.setValue(85);
    if (ph.canceled()) return 0;

#ifdef _OPENMP
    #pragma omp parallel for
#endif
    for (int i = 0; i < height; ++i) {
        int j = 0;
#ifdef __SSE2__
        for (; j < width - 3; j += 4) {
            vfloat exposurev = pow_F(2.0f, LVFU(fstopMap_min(j, i)));
            STVFU(inX(j, i), LVFU(inX(j, i)) * exposurev);
            STVFU(inY(j, i), LVFU(inY(j, i)) * exposurev);
            STVFU(inZ(j, i), LVFU(inZ(j, i)) * exposurev);
        }
#endif
        for (; j < width; ++j) {
            float exposure = pow_F(2.0f, fstopMap_min(j, i));
            inX(j, i) *= exposure;
            inY(j, i) *= exposure;
            inZ(j, i) *= exposure;
        }
    }

    delete[] zones;
    delete[] Rz;
    delete[] fstop;

    ph.setValue(99);

#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    cout << endl;
    cout << "tmo_lischinski06 = " << stop_watch.get_time() << " msec" << endl;
#endif

    return 0;
}
