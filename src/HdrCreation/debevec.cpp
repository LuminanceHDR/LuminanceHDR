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
//! \author Franco Comida <fcomida@users.sourceforge.net>

#include "HdrCreation/debevec.h"
#include <Libpfs/colorspace/normalizer.h>
#include <Libpfs/utils/msec_timer.h>
#include <Libpfs/utils/numeric.h>

#include <QtGlobal>
#include <limits>
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

    vector<float> exp_times;

    const size_t W = images[0].frame()->getWidth();
    const size_t H = images[0].frame()->getHeight();
    const size_t num_images = images.size();

    for (size_t idx = 0; idx < num_images; ++idx) {
        exp_times.push_back(images[idx].averageLuminance());
    }

    const int channels = 3;
    const size_t numPixels = W * H;

    // find index of next darker frame for each frame
    const float *arrayofexptime = exp_times.data();
    vector<size_t> i_sorted((int)num_images);
    iota(i_sorted.begin(), i_sorted.end(), 0);
    sort(i_sorted.begin(), i_sorted.end(), [&arrayofexptime](const size_t &a, const size_t &b)
         { return arrayofexptime[a] < arrayofexptime[b];});

    vector<int> idx_darker((int)num_images);
    idx_darker[i_sorted[0]] = -1; // darkest frame has no darker predecessor
    for (int i = 1; i < (int)num_images; ++i) {
        if (arrayofexptime[i_sorted[i]] > arrayofexptime[i_sorted[i-1]]) {
            // frame is truly brighter than previous one
            idx_darker[i_sorted[i]] = i_sorted[i-1];
        }
        else {
            // same brightness as previous frame -> take next darker one
            idx_darker[i_sorted[i]] = idx_darker[i_sorted[i-1]];
        }
    }

    float maxAllowedValue = weight.maxTrustedValue();
    ResponseChannel resp_chan[channels] = {RESPONSE_CHANNEL_RED, RESPONSE_CHANNEL_GREEN, RESPONSE_CHANNEL_BLUE};
    vector<float> max_responses(channels);
    for (int c = 0; c < channels; c++) {
        max_responses[c] = response(maxAllowedValue, resp_chan[c]);
    }

    vector<float> exp_values(exp_times);

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

#ifdef _OPENMP
    int numthreads = omp_get_max_threads();
    int nestedthreads = std::max(numthreads / (int)num_images, 1);
    bool oldNested = omp_get_nested();
    if(nestedthreads > 1) {
        omp_set_nested(true);
    }
    #pragma omp parallel for
#endif
    for (size_t i = 0; i < num_images; i++) {
        Channel *Ch[channels];
        images[i].frame()->getXYZChannels(Ch[0], Ch[1], Ch[2]);
        Array2Df *imagesCh[channels] = {Ch[0], Ch[1], Ch[2]};

        int i_darker = idx_darker[i];
        Channel *Ch_dark[channels];
        Array2Df *imgCh_dark[channels];
        float brighter_ratio = 1.f;
        if (i_darker >= 0) {
            brighter_ratio = arrayofexptime[i] / arrayofexptime[i_darker];
            images[i_darker].frame()->getXYZChannels(Ch_dark[0], Ch_dark[1], Ch_dark[2]);
            imgCh_dark[0] = Ch_dark[0];
            imgCh_dark[1] = Ch_dark[1];
            imgCh_dark[2] = Ch_dark[2];
        }

        Array2Df response_img(W, H);
        Array2Df w(W, H);

        float cmul = 1.f / channels;
#ifdef _OPENMP
        #pragma omp parallel for num_threads(nestedthreads) if (nestedthreads>1)
#endif
        for (size_t k = 0; k < numPixels; k++) {
            w(k) = cmul * (weight((*imagesCh[0])(k)) + weight((*imagesCh[1])(k)) + weight((*imagesCh[2])(k)));

            // Robust anti-overexposure: suppress values when overexposure was
            // expected from next-darker frame.
            // Work with worst case across all channels, because Debevec only
            // uses a single weight for all channels. If one channel is bad,
            // discard all to avoid funky colors.
            if (i_darker < 0) {
                // always give nonzero weight to darkest frame
                if (w(k) < 1e-6) {
                    w(k) = 1e-6;
                }
            }
            else {
                // suppress value, when overexposure is expected.
                float suppress_w = 1.f;
                for (int c = 0; c < channels; c++) {
                    float m_darker = (*imgCh_dark[c])(k);
                    float r_darker = response(m_darker, resp_chan[c]);
                    float expected_brightness = brighter_ratio * r_darker / max_responses[c];
                    // this weighting function could be pre-computed in a LUT
                    float w_darker_ch = 1.f - powf(expected_brightness, 4);
                    suppress_w = std::min(suppress_w, w_darker_ch);
                }
                w(k) *= std::max(0.f, suppress_w);
            }
        }
        float cadd = -logf(exp_times.at((int)i));
        for (int c = 0; c < channels; c++) {
            #pragma omp parallel for num_threads(nestedthreads) if (nestedthreads>1)
            for (size_t k = 0; k < numPixels; k++) {
                (response_img)(k) = response((*imagesCh[c])(k));
            }
#ifdef __SSE2__
            vfloat caddv = F2V(cadd);
#ifdef _OPENMP
            #pragma omp parallel for num_threads(nestedthreads) if (nestedthreads>1)
#endif
            for (size_t k = 0; k < numPixels - 3; k+=4) {
                STVFU((response_img)(k), (xlogf(LVFU((response_img)(k))) + caddv) * LVFU(w(k)));
            }
            for (size_t k = numPixels - (numPixels % 4); k < numPixels; k++) {
                (response_img)(k) = (xlogf((response_img)(k)) + cadd) * w(k);
            }
#else
#ifdef _OPENMP
            #pragma omp parallel for num_threads(nestedthreads) if (nestedthreads>1)
#endif
            for (size_t k = 0; k < numPixels; k++) {
                (response_img)(k) = (xlogf((response_img)(k)) + cadd) * w(k);
            }
#endif

#ifdef _OPENMP
            #pragma omp critical
#endif
            vadd(resultCh[c]->data(), response_img.data(), resultCh[c]->data(), numPixels);
        }

#ifdef _OPENMP
        #pragma omp critical
#endif
        vadd(weight_sum.data(), w.data(), weight_sum.data(), numPixels);
    }
#ifdef _OPENMP
    omp_set_nested(oldNested);
#endif

    for (int c = 0; c < channels; c++) {
#ifdef _OPENMP
        #pragma omp parallel for
#endif
        for(size_t y = 0; y < H; ++y) {
            size_t x = 0;
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
    float cmin[3];
#ifdef _OPENMP
    #pragma omp parallel for num_threads(nestedthreads) if (nestedthreads>1)
#endif
    for (int c = 0; c < channels; c++) {
        float minval = numeric_limits<float>::max();
        float maxval = numeric_limits<float>::min();
        for (size_t k = 0; k < numPixels; k++) {
            minval = std::min(minval, (*Ch[c])(k));
            maxval = std::max(maxval, (*Ch[c])(k));
        }
        cmax[c] = maxval;
        cmin[c] = minval;
    }

    float Max = max(cmax[0], max(cmax[1], cmax[2]));
    float Min = min(cmin[0], min(cmin[1], cmin[2]));

#ifdef _OPENMP
    #pragma omp parallel for
#endif
    for (size_t k = 0; k < W*H; k++) {
        float r = (*Ch[0])(k);
        float g = (*Ch[1])(k);
        float b = (*Ch[2])(k);
        if(std::isnan(r) || std::isnan(g) || std::isnan(b)) {
            if(!std::isnan(r)) {
                (*Ch[1])(k) = r;
                (*Ch[2])(k) = r;
            }
            else if(!std::isnan(g)) {
                (*Ch[0])(k) = g;
                (*Ch[2])(k) = g;
            }
            else if(!std::isnan(b)) {
                (*Ch[0])(k) = b;
                (*Ch[1])(k) = b;
            }
            else {
                (*Ch[0])(k) = Max;
                (*Ch[1])(k) = Max;
                (*Ch[2])(k) = Max;
            }
        }
    }

#ifdef _OPENMP
    #pragma omp parallel for num_threads(nestedthreads) if (nestedthreads>1)
#endif
    for (int c = 0; c < channels; c++) {
        transform(Ch[c]->begin(), Ch[c]->end(), Ch[c]->begin(),
                  Normalizer(Min, Max));
    }

#ifdef _OPENMP
    #pragma omp parallel for
#endif
    for (size_t k = 0; k < W*H; k++) {
        float r = (*Ch[0])(k);
        float g = (*Ch[1])(k);
        float b = (*Ch[2])(k);
        if((r < 0.f) || (g < 0.f) || (b < 0.f)) {
            (*Ch[0])(k) = .0f;
            (*Ch[1])(k) = .0f;
            (*Ch[2])(k) = .0f;
        }
        if(std::isnan(r) || std::isnan(g) || std::isnan(b)) {
            if(!std::isnan(r)) {
                (*Ch[1])(k) = r;
                (*Ch[2])(k) = r;
            }
            else if(!std::isnan(g)) {
                (*Ch[0])(k) = g;
                (*Ch[2])(k) = g;
            }
            else if(!std::isnan(b)) {
                (*Ch[0])(k) = b;
                (*Ch[1])(k) = b;
            }
            else {
                (*Ch[0])(k) = response(1.0f);
                (*Ch[1])(k) = response(1.0f);
                (*Ch[2])(k) = response(1.0f);
            }
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
