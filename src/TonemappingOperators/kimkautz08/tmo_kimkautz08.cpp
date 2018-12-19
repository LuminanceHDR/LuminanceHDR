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
#include "tmo_kimkautz08.h"
#include "sleef.c"
#include "opthelper.h"

using namespace std;
using namespace pfs;
using namespace pfs::colorspace;

int tmo_kimkautz08(Array2Df &L,
                    float KK_c1, float KK_c2,
                    Progress &ph) {
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif

    ph.setValue(5);

    int w = L.getCols();
    int h = L.getRows();

    Array2Df L_log(w, h);

    float sum = 0.f;
    float minVal = std::numeric_limits<float>::max();
    float maxVal = std::numeric_limits<float>::min();
#ifdef _OPENMP
    #pragma omp parallel
#endif
    {
        float sumThr = 0.f;
        float minValThr = std::numeric_limits<float>::max();
        float maxValThr = std::numeric_limits<float>::min();
#ifdef __SSE2__
        const vfloat c1em6v = F2V(1e-6f);
        vfloat sumThrv = ZEROV;
        vfloat minValv = F2V(minValThr);
        vfloat maxValv = F2V(maxValThr);
#endif
#ifdef _OPENMP
        #pragma omp for schedule(dynamic, 16) nowait
#endif
        for (int i = 0; i < h; i++) {
            int j = 0;
#ifdef __SSE2__
            for (; j < w - 3; j += 4) {
                const vfloat valv = xlogf(LVFU(L(j, i)) + c1em6v);
                sumThrv += valv;
                minValv = vminf(minValv, valv);
                maxValv = vmaxf(maxValv, valv);
                STVFU(L_log(j, i), valv);
            }
#endif
            for (; j < w; ++j) {
                const float val = xlogf(L(j, i) + 1e-6f);
                sumThr += val;
                minValThr = std::min(minValThr, val);
                maxValThr = std::max(maxValThr, val);
                L_log(j, i) = val;
            }
        }
#ifdef _OPENMP
        #pragma omp critical
#endif
        {
            sum += sumThr;
            minVal = std::min(minVal, minValThr);
            maxVal = std::max(maxVal, maxValThr);
#ifdef __SSE2__
            sum += vhadd(sumThrv);
            minVal = std::min(minVal, vhmin(minValv));
            maxVal = std::max(maxVal, vhmax(maxValv));
#endif
        }
    }

    ph.setValue(25);
    if (ph.canceled()) return 0;

    const float mu = sum / (w * h);

    const float maxLd = logf(300.f);
    const float minLd = logf(0.3f);

    const float k1 = (maxLd - minLd) / (maxVal - minVal);
    const float d0 = maxVal - minVal;
    const float sigma = d0 / KK_c1;
    const float sigma_sq_2 = sigma*sigma * 2;

#ifdef _OPENMP
    #pragma omp parallel
#endif
    {
#ifdef __SSE2__
        const vfloat sigma_sq_2v = F2V(sigma_sq_2);
        const vfloat muv = F2V(mu);
        const vfloat k1v = F2V(k1);
        const vfloat onevmk1v = F2V(1.f - k1);

#endif
#ifdef _OPENMP
        #pragma omp for schedule(dynamic, 16)
#endif
        for (int i = 0; i < h; i++) {
            int j = 0;
#ifdef __SSE2__
            for (; j < w - 3; j += 4) {
                const vfloat Lv = LVFU(L(j, i));
                STVFU(L(j, i), onevmk1v * xexpf(-SQRV(Lv - muv) / sigma_sq_2v) + k1v);
            }
#endif
            for (; j < w; ++j) {
                L(j, i) = (1 - k1) * xexpf(-(L(j, i) - mu)*(L(j, i) - mu) / sigma_sq_2) + k1;
            }
        }
    }

    ph.setValue(50);
    if (ph.canceled()) return 0;

#ifdef _OPENMP
    #pragma omp parallel
#endif
    {
#ifdef __SSE2__
        const vfloat KK_c2v = F2V(KK_c2);
        const vfloat muv = F2V(mu);
#endif
#ifdef _OPENMP
        #pragma omp for schedule(dynamic, 16)
#endif
        for (int i = 0; i < h; i++) {
            int j = 0;
#ifdef __SSE2__
            for (; j < w - 3; j += 4) {
                STVFU(L(j, i), xexpf(KK_c2v * LVFU(L(j, i)) * (LVFU(L_log(j, i)) - muv) + muv));
            }
#endif
            for (; j < w; ++j) {
                L(j, i) = xexpf(KK_c2 * L(j, i) * (L_log(j, i) - mu) + mu);
            }
        }
    }

    ph.setValue(75);
    if (ph.canceled()) return 0;

    //Percentile clamping
    lhdrengine::findMinMaxPercentile(L.data(), L.getCols() * L.getRows(), 0.01f, minVal, 0.99f, maxVal, true);

    const float range = maxVal - minVal;
#ifdef _OPENMP
    #pragma omp parallel
#endif
    {
#ifdef __SSE2__
        const vfloat minValv = F2V(minVal);
        const vfloat maxValv = F2V(maxVal);
        const vfloat rangev = F2V(range);
#endif
#ifdef _OPENMP
        #pragma omp for schedule(dynamic, 16)
#endif
        for (int i = 0; i < h; i++) {
            int j = 0;
#ifdef __SSE2__
            for (; j < w - 3; j += 4) {
                STVFU(L(j, i), (LIMV(LVFU(L(j, i)), minValv, maxValv) - minValv) / rangev);
            }
#endif
            for (; j < w; ++j) {
                L(j, i) = (lhdrengine::LIM(L(j, i), minVal, maxVal) - minVal) / range;
            }
        }
    }

    ph.setValue(99);

#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    cout << endl;
    cout << "tmo_kimkautz08 = " << stop_watch.get_time() << " msec" << endl;
#endif

    return 0;
}
