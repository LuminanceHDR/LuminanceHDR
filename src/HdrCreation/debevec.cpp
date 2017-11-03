/*
 * This file is a part of Luminance HDR package
 * ----------------------------------------------------------------------
 * Copyright (C) 2006,2007 Giuseppe Rota
 * Copyright (C) 2013 Davide Anastasia
 * Copyright (C) 2017 Franco Comida
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

#include <limits>
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/numeric/conversion/bounds.hpp>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <functional>
#include <iostream>
#include <vector>

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
#pragma omp parallel for
    for (int c = 0; c < channels; c++) {
        resultCh[c]->fill(0.f);
    }
    Array2Df weight_sum(W, H);
    weight_sum.fill(0.f);

    int length = images.size();
#pragma omp parallel for
    for (int i = 0; i < length; i++) {
        Channel *Ch[channels];
        images[i].frame()->getXYZChannels(Ch[0], Ch[1], Ch[2]);
        Array2Df *imagesCh[channels] = {Ch[0], Ch[1], Ch[2]};

        float cmax[3];
        float cmin[3];
        for (int c = 0; c < channels; c++) {
            cmax[c] = *max_element(Ch[c]->begin(), Ch[c]->end());
            cmin[c] = *min_element(Ch[c]->begin(), Ch[c]->end());
        }
        float Max = max(cmax[0], max(cmax[1], cmax[2]));
        float Min = min(cmin[0], min(cmin[1], cmin[2]));

        for (int c = 0; c < channels; c++) {
            transform(Ch[c]->begin(), Ch[c]->end(), Ch[c]->begin(),
                      Normalizer(Min, Max));
        }

        Array2Df temp_array(W, H);
        Array2Df splitted[channels] = {Array2Df(W, H), Array2Df(W, H),
                                       Array2Df(W, H)};
        Array2Df response_img[channels] = {Array2Df(W, H), Array2Df(W, H),
                                           Array2Df(W, H)};
        Array2Df w(W, H);
        for (int c = 0; c < channels; c++) {
            for (size_t k = 0; k < size; k++) {
                splitted[c](k) = weight((*imagesCh[c])(k));
            }
            vadd(&w, &splitted[c], &w, size);
        }
        vmul_scalar(&w, 1.f / channels, &w, size);
        for (int c = 0; c < channels; c++) {
            for (size_t k = 0; k < size; k++) {
                (response_img[c])(k) = logf(response((*imagesCh[c])(k)));
            }
        }
        for (int c = 0; c < channels; c++) {
            vsum_scalar(&response_img[c], -logf(times.at((int)i)),
                        &response_img[c], size);
            vmul(&w, &response_img[c], &temp_array, size);
            vadd(resultCh[c], &temp_array, resultCh[c], size);
        }
        vadd(&weight_sum, &w, &weight_sum, size);
    }
    vdiv_scalar(&weight_sum, 1.0f, &weight_sum, size);
#pragma omp parallel for
    for (int c = 0; c < channels; c++) {
        vmul(resultCh[c], &weight_sum, resultCh[c], size);
    }
#pragma omp parallel for
    for (int c = 0; c < channels; c++) {
        transform(resultCh[c]->begin(), resultCh[c]->end(),
                  resultCh[c]->begin(), expf);
    }
#pragma omp parallel for
    for (int c = 0; c < channels; c++) {
#ifdef WIN32
        replace_if(resultCh[c]->begin(), resultCh[c]->end(),
                   not1(ref(isnormal<float>)), numeric_limits<float>::min());
#else
        replace_if(resultCh[c]->begin(), resultCh[c]->end(),
                   [](float f) { return !isnormal(f); }, numeric_limits<float>::min());
#endif
    }
    float cmax[3];
#pragma omp parallel for
    for (int c = 0; c < channels; c++) {
        cmax[c] = *max_element(resultCh[c]->begin(), resultCh[c]->end());
    }
    float Max = max(cmax[0], max(cmax[1], cmax[2]));

#pragma omp parallel for
    for (int c = 0; c < channels; c++) {
        replace(resultCh[c]->begin(), resultCh[c]->end(),
                   numeric_limits<float>::min(), Max);
    }

#ifdef TIMER_PROFILING
    f_timer.stop_and_update();
    cout << "MergeDebevec = " << f_timer.get_time() << " msec"
              << endl;
#endif
}


}  // libhdr
}  // fusion
