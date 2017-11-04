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


#include <boost/compute/core.hpp>
#include <boost/compute/algorithm/transform.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/functional/math.hpp>
#include <boost/compute/functional/bind.hpp>
#include <boost/compute/lambda/functional.hpp>
#include <boost/compute/lambda/placeholders.hpp>
#include <boost/compute/types/struct.hpp>

#include "HdrCreation/debevec.h"
#include <Libpfs/colorspace/normalizer.h>
#include <Libpfs/utils/msec_timer.h>
#include <Libpfs/utils/numeric.h>

#include <limits>
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

namespace compute = boost::compute;
using boost::compute::lambda::_1;

namespace libhdr {
namespace fusion {

void DebevecOperator::computeFusion(ResponseCurve &response,
                                    WeightFunction &weight,
                                    const vector<FrameEnhanced> &images,
                                    pfs::Frame &frame) {
#define TIMER_PROFILING
#ifdef TIMER_PROFILING
    msec_timer f_timer;
    f_timer.start();
#endif
    assert(images.size() != 0);

    compute::device device = compute::system::default_device();
    compute::context context(device);
    compute::command_queue queue(context, device);

    std::cout << " (platform: " << device.platform().name() << ")" << std::endl;

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

    compute::vector<float> resultCh_c[channels] { compute::vector<float>(W*H, context),
                                                  compute::vector<float>(W*H, context),
                                                  compute::vector<float>(W*H, context) };
    for (int c = 0; c < channels; c++) {
        compute::fill(resultCh_c[c].begin(), resultCh_c[c].end(), 0.f, queue);
    }
    compute::vector<float> weight_sum_c(W*H, context);
    compute::fill(weight_sum_c.begin(), weight_sum_c.end(), 0.f, queue);

    int length = images.size();
    for (int i = 0; i < length; i++) {
        Channel *Ch[channels];
        images[i].frame()->getXYZChannels(Ch[0], Ch[1], Ch[2]);
        Array2Df *imagesCh[channels] = {Ch[0], Ch[1], Ch[2]};
        compute::vector<float> imagesCh_c[channels] = {compute::vector<float>(W*H, context),
                                                       compute::vector<float>(W*H, context),
                                                       compute::vector<float>(W*H, context)};

        for (int c = 0; c < channels; c++) {
            compute::copy(imagesCh[c]->begin(), imagesCh[c]->end(), imagesCh_c[c].begin(), queue);
        }

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

        compute::vector<float> temp_array_c(W*H, context);
        Array2Df splitted[channels] = {Array2Df(W, H), Array2Df(W, H),
                                       Array2Df(W, H)};
        compute::vector<float> splitted_c[channels] = {compute::vector<float>(W*H, context),
                                                       compute::vector<float>(W*H, context),
                                                       compute::vector<float>(W*H, context)};
        compute::vector<float> response_img_c[channels] = {compute::vector<float>(W*H, context),
                                                       compute::vector<float>(W*H, context),
                                                       compute::vector<float>(W*H, context)};
        compute::vector<float> w_c(W*H, context);
        compute::fill(w_c.begin(), w_c.end(), 0.f, queue);

        for (int c = 0; c < channels; c++) {
            for (size_t k = 0; k < size; k++) {
                splitted[c](k) = weight((*imagesCh[c])(k));
            }
        }
        for (int c = 0; c < channels; c++) {
            compute::copy(splitted[c].begin(), splitted[c].end(), splitted_c[c].begin(), queue);
            compute::transform(w_c.begin(), w_c.end(), splitted_c[c].begin(), w_c.begin(), compute::plus<float>(), queue);
        }

        compute::transform(w_c.begin(), w_c.end(), w_c.begin(), _1 / 3.f, queue);

        vector<float> tmp[channels] = {vector<float>(size), vector<float>(size), vector<float>(size)};
        for (int c = 0; c < channels; c++) {
            for (size_t k = 0; k < size; k++) {
                tmp[c][k] = logf(response((*imagesCh[c])(k)));
            }
        }
        for (int c = 0; c < channels; c++) {
            compute::copy(tmp[c].begin(), tmp[c].end(), response_img_c[c].begin(), queue);
        }
        for (int c = 0; c < channels; c++) {
            float l = -logf(times.at((int)i));
            compute::transform(response_img_c[c].begin(), response_img_c[c].end(),
                    response_img_c[c].begin(), _1 + l, queue);
            compute::transform(response_img_c[c].begin(), response_img_c[c].end(), w_c.begin(), temp_array_c.begin(),
                    compute::multiplies<float>(), queue);
            compute::transform(resultCh_c[c].begin(), resultCh_c[c].end(), temp_array_c.begin(), resultCh_c[c].begin(),
                    compute::plus<float>(), queue);
        }
        compute::transform(weight_sum_c.begin(), weight_sum_c.end(), w_c.begin(), weight_sum_c.begin(),
                    compute::plus<float>(), queue);
    }
    compute::transform(weight_sum_c.begin(), weight_sum_c.end(), weight_sum_c.begin(), 1.f / _1, queue);

    for (int c = 0; c < channels; c++) {
        compute::transform(weight_sum_c.begin(), weight_sum_c.end(), resultCh_c[c].begin(), resultCh_c[c].begin(),
                    compute::multiplies<float>(), queue);
    }
    for (int c = 0; c < channels; c++) {
        compute::transform(
                        resultCh_c[c].begin(),
                        resultCh_c[c].end(),
                        resultCh_c[c].begin(),
                        compute::exp<float>(),
                        queue
                      );
        compute::copy(
                resultCh_c[c].begin(), resultCh_c[c].end(), resultCh[c]->begin(), queue
                );
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
