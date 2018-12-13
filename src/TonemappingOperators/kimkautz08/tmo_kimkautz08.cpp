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

    int w = L.getCols();
    int h = L.getRows();

    Array2Df L_log(w, h);
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for (int i = 0; i < h; i++) {
            for (int j = 0; j < w; ++j) {
                L_log(j, i) = xlogf(L(j, i) + 1e-6);
            }
        }

    ph.setValue(2);
    if (ph.canceled()) return 0;

    float mu = accumulate(L_log.begin(), L_log.end(), 0.f)/(w*h);

    float maxL = *max_element(L_log.begin(), L_log.end());
    float minL = *min_element(L_log.begin(), L_log.end());

    float maxLd = logf(300.f);
    float minLd = logf(0.3f);

    float k1 = (maxLd - minLd) / (maxL - minL);
    float d0 = maxL - minL;
    float sigma = d0 / KK_c1;
    float sigma_sq_2 = sigma*sigma * 2;

#ifdef _OPENMP
    #pragma omp parallel for
#endif
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; ++j) {
            L(j, i) = xexpf(-(L(j, i) - mu)*(L(j, i) - mu) / sigma_sq_2);
        }
    }

    Array2Df &W = L;

    ph.setValue(25);
    if (ph.canceled()) return 0;

    Array2Df &K2 = L;
    transform(W.begin(), W.end(), K2.begin(),
            [k1](float pix) { return (1 - k1) * pix + k1;} );

    ph.setValue(50);
    if (ph.canceled()) return 0;

#ifdef _OPENMP
    #pragma omp parallel for
#endif
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; ++j) {
            L(j, i) = xexpf(KK_c2 * K2(j, i) * (L_log(j, i) -mu) + mu);
        }
    }

    Array2Df &Ld = L;


    ph.setValue(75);
    if (ph.canceled()) return 0;

    //Percentile clamping
    lhdrengine::findMinMaxPercentile(Ld.data(), Ld.getCols() * Ld.getRows(), 0.01f, minLd, 0.99f, maxLd, true);

    replace_if(Ld.begin(), Ld.end(), [maxLd](float pix) { return pix > maxLd; }, maxLd);
    replace_if(Ld.begin(), Ld.end(), [minLd](float pix) { return pix < minLd; }, minLd);

    transform(Ld.begin(), Ld.end(), Ld.begin(),
                      Normalizer(minLd, maxLd));

    ph.setValue(99);

#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    cout << endl;
    cout << "tmo_kimkautz08 = " << stop_watch.get_time() << " msec" << endl;
#endif

    return 0;
}
