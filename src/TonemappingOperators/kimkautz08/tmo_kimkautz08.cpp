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
#include <Libpfs/colorspace/normalizer.h>
#include "Libpfs/utils/msec_timer.h"
#include "tmo_kimkautz08.h"

using namespace std;
using namespace pfs;
using namespace pfs::colorspace;

namespace {
    float maxQuart(const Array2Df &X, float percentile)
    {
        if(percentile > 1.0f)
            percentile = 1.0f;
        else if(percentile < 0.0f)
            percentile = 0.0f;

        int w = X.getCols();
        int h = X.getRows();

        Array2Df L(w, h);
        L = X;
        std::sort(L.begin(), L.end());
        int index = round(w*h * percentile);
        index = max(index, 1);
        return L(index);
    }
}

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
    transform(L.begin(), L.end(), L_log.begin(), [](float pix) { return logf(pix + 1e-6); } );

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

    Array2Df W(w, h);
    transform(L_log.begin(), L_log.end(), W.begin(),
            [mu, sigma_sq_2](float pix) { return expf(-(pix - mu)*(pix - mu) / sigma_sq_2); } );

    ph.setValue(10);
    if (ph.canceled()) return 0;

    Array2Df K2(w, h);
    transform(W.begin(), W.end(), K2.begin(),
            [k1](float pix) { return (1 - k1) * pix + k1;} );

    ph.setValue(25);
    if (ph.canceled()) return 0;

    Array2Df Ld(w, h);
    transform(K2.begin(), K2.end(), L_log.begin(), Ld.begin(),
            [KK_c2, mu](float pix1, float pix2) { return expf(KK_c2 * pix1 * (pix2 -mu) + mu); } );

    ph.setValue(40);
    if (ph.canceled()) return 0;

    //Percentile clamping
    maxLd = maxQuart(Ld, 0.99f);
    minLd = maxQuart(Ld, 0.01f);

    replace_if(Ld.begin(), Ld.end(), [maxLd](float pix) { return pix > maxLd; }, maxLd);
    replace_if(Ld.begin(), Ld.end(), [minLd](float pix) { return pix < minLd; }, minLd);

    transform(Ld.begin(), Ld.end(), Ld.begin(),
                      Normalizer(minLd, maxLd));

    ph.setValue(80);
    if (ph.canceled()) return 0;

    copy(Ld.begin(), Ld.end(), L.begin());

    ph.setValue(99);
    if (ph.canceled()) return 0;

#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    cout << endl;
    cout << "tmo_kimkautz08 = " << stop_watch.get_time() << " msec" << endl;
#endif

    return 0;
}
