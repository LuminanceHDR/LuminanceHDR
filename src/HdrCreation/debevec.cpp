/*
 * This file is a part of Luminance HDR package
 * ----------------------------------------------------------------------
 * Copyright (C) 2006,2007 Giuseppe Rota
 * Copyright (C) 2013 Davide Anastasia
 * Copyright (C) 2017 Franco Comida
 * Optimized speed and memory usage 2017 Ingo Weyrich
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
 */

//! \author Giuseppe Rota <grota@users.sourceforge.net>
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#include "HdrCreation/debevec.h"
#include <Libpfs/colorspace/normalizer.h>
#include <Libpfs/utils/msec_timer.h>
#include <Libpfs/utils/numeric.h>

#include <QtGlobal>
#include <limits>
#include <boost/numeric/conversion/bounds.hpp>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <functional>
#include <iostream>
#include <vector>
#include "../sleef.c"
#include "../opthelper.h"
#ifdef _OPENMP
#include <omp.h>
#endif

#include "Libpfs/array2d.h"

#ifndef NDEBUG
#define PRINT_DEBUG(str) std::cerr << "Debevec: " << str << std::endl
#else
#define PRINT_DEBUG(str)
#endif

using namespace pfs;
using namespace std;
using namespace utils;
using namespace colorspace;
using namespace boost::math;

namespace libhdr {
namespace fusion {

void DebevecOperator::computeFusion(ResponseCurve &response,
                                    WeightFunction &weight,
                                    const vector<FrameEnhanced> &images,
                                    pfs::Frame &frame) {

#ifdef TIMER_PROFILING
    msec_timer f_timer;
    f_timer.start();
#endif
    assert(images.size() != 0);

    vector<float> times;

    const int W = images[0].frame()->getWidth();
    const int H = images[0].frame()->getHeight();

    for (size_t idx = 0; idx < images.size(); ++idx) {
        times.push_back(images[idx].averageLuminance());
    }

    const int channels = 3;
    const size_t size = W * H;

    vector<float> exp_values(times);

    transform(exp_values.begin(), exp_values.end(), exp_values.begin(), logf);

    frame.resize(W, H);
    Channel *Ch[3];
    frame.createXYZChannels(Ch[0], Ch[1], Ch[2]);
    Array2Df *resultCh[channels] = {Ch[0], Ch[1], Ch[2]};

#ifdef _OPENMP
    #pragma omp parallel for
#endif
    for (int c = 0; c < channels; c++) {
        resultCh[c]->fill(0.f);
    }
    Array2Df weight_sum(W, H);
    weight_sum.fill(0.f);

    int length = images.size();

#ifdef _OPENMP
    int numthreads = omp_get_max_threads();
    int nestedthreads = std::max(numthreads / length, 1);
    bool oldNested = omp_get_nested();
    if(nestedthreads > 1) {
        omp_set_nested(true);
    }
    #pragma omp parallel for
#endif
    for (int i = 0; i < length; i++) {
        Channel *Ch[channels];
        images[i].frame()->getXYZChannels(Ch[0], Ch[1], Ch[2]);
        Array2Df *imagesCh[channels] = {Ch[0], Ch[1], Ch[2]};

        float cmax[3];
        float cmin[3];
#ifdef _OPENMP
        #pragma omp parallel for num_threads(nestedthreads) if (nestedthreads>1)
#endif
        for (int c = 0; c < channels; c++) {
            float minval = numeric_limits<float>::max();
            float maxval = numeric_limits<float>::min();
            for (size_t k = 0; k < size; k++) {
                minval = std::min(minval, (*imagesCh[c])(k));
                maxval = std::max(maxval, (*imagesCh[c])(k));
            }
            cmax[c] = maxval;
            cmin[c] = minval;
        }

        float Max = max(cmax[0], max(cmax[1], cmax[2]));
        float Min = min(cmin[0], min(cmin[1], cmin[2]));

#ifdef _OPENMP
        #pragma omp parallel for num_threads(nestedthreads) if (nestedthreads>1)
#endif
        for (int c = 0; c < channels; c++) {
            transform(Ch[c]->begin(), Ch[c]->end(), Ch[c]->begin(),
                      Normalizer(Min, Max));
        }

        Array2Df response_img(W, H);
        Array2Df w(W, H);

        float cmul = 1.f / channels;
#ifdef _OPENMP
        #pragma omp parallel for num_threads(nestedthreads) if (nestedthreads>1)
#endif
        for (size_t k = 0; k < size; k++) {
            w(k) = cmul * (weight((*imagesCh[0])(k)) + weight((*imagesCh[1])(k)) + weight((*imagesCh[2])(k)));
        }
        float cadd = -logf(times.at((int)i));
        for (int c = 0; c < channels; c++) {
            #pragma omp parallel for num_threads(nestedthreads) if (nestedthreads>1)
            for (size_t k = 0; k < size; k++) {
                (response_img)(k) = response((*imagesCh[c])(k));
            }
#ifdef __SSE2__
            vfloat caddv = F2V(cadd);
#ifdef _OPENMP
            #pragma omp parallel for num_threads(nestedthreads) if (nestedthreads>1)
#endif
            for (size_t k = 0; k < size - 3; k+=4) {
                STVFU((response_img)(k), (xlogf(LVFU((response_img)(k))) + caddv) * LVFU(w(k)));
            }
            for (size_t k = size - (size % 4); k < size; k++) {
                (response_img)(k) = (xlogf((response_img)(k)) + cadd) * w(k);
            }
#else
#ifdef _OPENMP
            #pragma omp parallel for num_threads(nestedthreads) if (nestedthreads>1)
#endif
            for (size_t k = 0; k < size; k++) {
                (response_img)(k) = (xlogf((response_img)(k)) + cadd) * w(k);
            }
#endif

#ifdef _OPENMP
            #pragma omp critical
#endif
            vadd(resultCh[c], &response_img, resultCh[c], size);
        }

#ifdef _OPENMP
        #pragma omp critical
#endif
        vadd(&weight_sum, &w, &weight_sum, size);
    }
#ifdef _OPENMP
    omp_set_nested(oldNested);
#endif

    for (int c = 0; c < channels; c++) {
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for(int y = 0; y < H; ++y) {
            int x = 0;
#ifdef __SSE2__
            for(; x < W - 3; x += 4) {
                STVFU((*resultCh[c])(x, y), xexpf(LVFU((*resultCh[c])(x,y)) / LVFU(weight_sum(x,y))));
            }
#endif
            for(; x < W; ++x) {
                (*resultCh[c])(x, y) = xexpf((*resultCh[c])(x,y) / weight_sum(x,y));
            }
        }
    }
    float cmax[3];
    for (int c = 0; c < channels; c++) {
        float max = numeric_limits<float>::min();
#ifdef _OPENMP
    #pragma omp parallel for reduction(max:max)
#endif
        for (size_t k = 0; k < size; k++) {
            float val = (*resultCh[c])(k);
            if(std::isnormal(val)) {
                max = std::max(max, val);
            }
        }
        cmax[c] = max;
    }

    float Max = max(cmax[0], max(cmax[1], cmax[2]));

    for (int c = 0; c < channels; c++) {
#ifdef _OPENMP
    #pragma omp parallel for
#endif
        for (size_t k = 0; k < size; k++) {
            float val = (*resultCh[c])(k);
            if(!std::isnormal(val)) {
                (*resultCh[c])(k) = Max;
            }
        }
    }

    // TODO: Investigate why scaling hdr yields better result
    for (int c = 0; c < channels; c++) {
#ifdef _OPENMP
    #pragma omp parallel for
#endif
        for (size_t k = 0; k < size; k++) {
            (*resultCh[c])(k) *= 0.1f;
        }
    }

#ifdef TIMER_PROFILING
    f_timer.stop_and_update();
    cout << "MergeDebevec = " << f_timer.get_time() << " msec"
              << endl;
#endif
}

}  // libhdr
}  // fusion
