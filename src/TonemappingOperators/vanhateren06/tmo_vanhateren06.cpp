/*
 * @brief VanHateren Tone Mapping Operator:
 *    "Encoding of High Dynamic Range Video with a Model of Human Cones"
 *     by J. Hans Van Hateren
 *     in ACM Transaction on Graphics 2006
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
#include <iostream>
#include <gsl/gsl_poly.h>

#include "Libpfs/array2d.h"
#include "Libpfs/frame.h"
#include "Libpfs/progress.h"
#include "Libpfs/utils/msec_timer.h"
#include "Libpfs/utils/clamp.h"
#include <Libpfs/colorspace/normalizer.h>
#include "rt_math.h"
#include "tmo_vanhateren06.h"

using namespace pfs;
using namespace pfs::colorspace;
using namespace std;

int tmo_vanhateren06(Array2Df &L, float pupil_area, Progress &ph) {
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif

    if(pupil_area <= 0.0f)
        pupil_area = 10.f; //fixed pupil area 10 mm^2

    size_t w = L.getCols();
    size_t h = L.getRows();
    size_t size = w*h;

    float k_beta = 1.6e-4; // td/ms
    float a_C = 9e-2;
    float C_beta = 2.8e-3; // 1/ms

    //Calculate Ios,max
    double polIosMax[] = {-1.0/C_beta, 0.0, 0.0, 0.0, 1.0, a_C};
    gsl_poly_complex_workspace * ws = gsl_poly_complex_workspace_alloc(6);
    double roots[10];
    double real_roots[5];
    gsl_poly_complex_solve (polIosMax, 6, ws, roots);
    gsl_poly_complex_workspace_free (ws);

    for (int i = 0; i < 10 ; i += 2) {
        real_roots[i>>1] = roots[i];
    }

    float maxIos = (float) *max_element(real_roots, real_roots + 5);

    //conversion from cd/m^2 to trolands (tr)
    transform(L.begin(), L.end(), L.begin(),
            [pupil_area](float lori) { return lori * pupil_area; } );

    //Range reduction
    transform(L.begin(), L.end(), L.begin(),
            [C_beta, k_beta](float l) { return -1.0f / (C_beta + k_beta * l); } );


    float minVal = std::numeric_limits<float>::max();
    float maxVal = std::numeric_limits<float>::min();
    for (size_t i = 0; i < size; ++i) {
        minVal = std::min(minVal, L(i));
        maxVal = std::max(maxVal, L(i));
    }
    const float scale = 65535.f / (maxVal - minVal);

    float lookup[65536];

    const float lutscale = (maxVal - minVal) / 65536.f;
    #pragma omp parallel
    {
        gsl_poly_complex_workspace* wsp = gsl_poly_complex_workspace_alloc(6);
        double base[] = {0.0, 0.0, 0.0, 0.0, 1.0, a_C};
        double roots[10];
        #pragma omp for nowait
        for (size_t i = 0; i < 65536; ++i) {
            base[0] = minVal + lutscale * i;
            gsl_poly_complex_solve (base, 6, wsp, roots);
            double maxRoot = roots[0];
            for (int k = 2; k < 10; k += 2) {
                maxRoot = std::max(maxRoot, roots[k]);
            }
            lookup[i] = maxRoot;
        }
        gsl_poly_complex_workspace_free (wsp);
    }

    #pragma omp parallel for schedule(dynamic, w * 16)
    for (size_t i = 0; i < size; i++) {
        L(i) = lookup[lhdrengine::LIM(static_cast<int>((L(i) - minVal) * scale), 0, 65535)];
    }

    if (ph.canceled())
        return 0;

    transform(L.begin(), L.end(), L.begin(),
            [maxIos](float i_os) { return (1.0f - i_os/maxIos); } );

    ph.setValue(100);

#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    cout << endl;
    cout << "tmo_vanhateren06 = " << stop_watch.get_time() << " msec" << endl;
#endif

    return 0;
}
