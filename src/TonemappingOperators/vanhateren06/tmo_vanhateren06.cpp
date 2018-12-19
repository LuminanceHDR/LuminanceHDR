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

    ph.setValue(5);

    pupil_area = std::max(pupil_area, 0.f);

    const size_t w = L.getCols();
    const size_t h = L.getRows();
    const size_t size = w*h;

    const float k_beta = 1.6e-4; // td/ms
    const float a_C = 9e-2;
    const float C_beta = 2.8e-3; // 1/ms

    //Calculate Ios,max
    double polIosMax[] = {-1.0/C_beta, 0.0, 0.0, 0.0, 1.0, a_C};
    gsl_poly_complex_workspace* ws = gsl_poly_complex_workspace_alloc(6);
    double roots[10];
    gsl_poly_complex_solve(polIosMax, 6, ws, roots);
    gsl_poly_complex_workspace_free(ws);

    for (int i = 0; i < 10 ; i += 2) {
        roots[i>>1] = roots[i];
    }

    const float maxIos = (float) *max_element(roots, roots + 5);

    float minVal = std::numeric_limits<float>::max();
    float maxVal = std::numeric_limits<float>::min();

#ifdef _OPENMP
    #pragma omp parallel for reduction(min:minVal) reduction(max:maxVal)
#endif
    for (size_t i = 0; i < size; ++i) {
        const float val = -1.f / (C_beta + k_beta * L(i) * pupil_area);
        minVal = std::min(minVal, val);
        maxVal = std::max(maxVal, val);
        L(i) = val;
    }

    ph.setValue(33);

    constexpr int lutSize = 65536;
    const float scale = (lutSize - 1) / (maxVal - minVal);

    float lookup[lutSize];

    const float lutscale = (maxVal - minVal) / (lutSize - 1);

#ifdef _OPENMP
    #pragma omp parallel
#endif
    {
        gsl_poly_complex_workspace* wsp = gsl_poly_complex_workspace_alloc(6);
        double base[] = {0.0, 0.0, 0.0, 0.0, 1.0, a_C};
        double roots[10];
#ifdef _OPENMP
        #pragma omp for nowait
#endif
        for (size_t i = 0; i < lutSize; ++i) {
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

    ph.setValue(66);

#ifdef _OPENMP
    #pragma omp parallel for schedule(dynamic, w * 16)
#endif
    for (size_t i = 0; i < size; i++) {
        const float index = lhdrengine::LIM((L(i) - minVal) * scale, 0.f, static_cast<float>(lutSize - 1));
        const int lowerIndex = index;
        const int upperIndex = std::min(lowerIndex + 1, lutSize - 1);
        L(i) = 1.f - lhdrengine::intp(index - lowerIndex, lookup[upperIndex], lookup[lowerIndex]) / maxIos;
    }

    ph.setValue(99);

#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    cout << endl;
    cout << "tmo_vanhateren06 = " << stop_watch.get_time() << " msec" << endl;
#endif

    return 0;
}
