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

#include <assert.h>
#include <math.h>
#include <algorithm>
#include <numeric>
#include <iostream>

#include "Libpfs/array2d.h"
#include "Libpfs/frame.h"
#include "Libpfs/progress.h"
#include "Libpfs/rt_algo.h"
#include <Libpfs/colorspace/normalizer.h>
#include "Libpfs/utils/msec_timer.h"
#include "tmo_lischinski06.h"
#include "lischinski_minimization.h"
#include "sleef.c"
#include "opthelper.h"


#ifndef CLAMP
    #define CLAMP(x, a)         (x >= a ? (a - 1) : (x < 0 ? 0 : x))
#endif

using namespace std;
using namespace pfs;
using namespace pfs::colorspace;

float LogMeanVal(const Array2Df &L) {
    const size_t h = L.getRows();
    const size_t w = L.getCols();
    const size_t size = w*h;
    double ret = 0.0;
    for(size_t j = 0; j < h; j++) {
        for(size_t i = 0; i < w; i++) {
            ret += log2(L(i, j) + 1e-6);
        }
    }
    return expf(ret/size);
}

int tmo_lischinski06(Array2Df &L,Array2Df &inX, Array2Df &inY, Array2Df &inZ,
                     const float alpha_mul, const float whitePoint_mul,
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

    float Lav = LogMeanVal(L);
    float maxL_log = log2f(maxL + 1e-6f);
    float minL_log = log2f(minL + 1e-6f);

    float log2Average = log2(Lav + 1e-9f);
    float alpha = 0.18f * powf(4.0f, (2.0f * log2Average - minL_log - maxL_log)/( maxL_log - minL_log));
    float whitePoint = 1.5f * powf(2.0f, maxL_log - minL_log - 5.0f);

    alpha *= alpha_mul;
    whitePoint *= whitePoint_mul/100.f;

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

    for(int i = 0; i < height; i++) {
        for(int j = 0; j < width; j++) {
            float Lum = L(j,i);
            float Lum_log = log2f(Lum + 1e-6f);

            int zone = CLAMP(int(ceilf(Lum_log - minL_log)), Z);

            zones[zone].push_back(Lum);
        }
    }

    ph.setValue(25);

    for(int i = 0; i < Z; i++) {
        int n = int(zones[i].size());
        if(n > 0) {
            std::sort(zones[i].begin(), zones[i].end());
            Rz[i] = zones[i][n / 2];
            if(Rz[i] > 0.0f) {
                //photographic operator
                float Rz2 = Rz[i] * alpha / Lav;
                float f = (Rz2 * (1 + Rz2 / whitePoint_sq) ) / (1.0f + Rz2);
                fstop[i] = log2f(f / Rz[i] + 1e-6f);
            }
        }
    }

    //create the fstop map
    Array2Df L_log(width, height);
    transform(L.begin(), L.end(), L_log.begin(),
            [](float l) { return log2f(l + 1e-6f); } );

    Array2Df fstopMap(width, height);

    for(int i = 0; i < height; i++) {
        for(int j = 0; j < width; j++) {
            float l_log = L_log(j,i);

            int zone = CLAMP(int(ceilf(l_log - minL_log)), Z);

            fstopMap(j,i) = fstop[zone];
        }
    }

    ph.setValue(50);

    //Lischinski minimization
    Array2Df tmp(width, height);
    tmp.fill(0.007f);
    Array2Df fstopMap_min(width, height);
    LischinskiMinimization(L, fstopMap, tmp, fstopMap_min);

    for(int i = 0; i < height; i++) {
        for(int j = 0; j < width; j++) {
            float exposure = powf(2.0f, fstopMap_min(j, i));

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
