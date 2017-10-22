/**
 * @file tmo_reinhard05.cpp
 * @brief Tone map XYZ channels using Reinhard05 model
 *
 * Dynamic Range Reduction Inspired by Photoreceptor Physiology.
 * E. Reinhard and K. Devlin.
 * In IEEE Transactions on Visualization and Computer Graphics, 2005.
 *
 *
 * This file is a part of LuminanceHDR package, based on pfstmo.
 * ----------------------------------------------------------------------
 * Copyright (C) 2007 Grzegorz Krawczyk
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
 * @author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 */

#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/compute/core.hpp>
#include <boost/compute/closure.hpp>
#include <boost/compute/algorithm/transform.hpp>
#include <boost/compute/algorithm/accumulate.hpp>
#include <boost/compute/algorithm/replace.hpp>
#include <boost/compute/algorithm/minmax_element.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/functional/math.hpp>
#include <boost/compute/lambda/functional.hpp>
#include <boost/compute/lambda/placeholders.hpp>

#include "tmo_reinhard05.h"
#include "Libpfs/progress.h"
#include "TonemappingOperators/pfstmo.h"
#include <Libpfs/utils/msec_timer.h>

#include <assert.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <numeric>

using namespace std;
namespace compute = boost::compute;
using boost::compute::lambda::_1;
using boost::compute::lambda::_2;

namespace {
class LuminanceEqualization
{
private:
    static const float DISPL_F;

public:
    LuminanceEqualization():
        min_lum_( numeric_limits<float>::max() ),
        max_lum_( -numeric_limits<float>::max() ),
        avg_lum_( 0.0f ),
        adapted_lum_( 0.0f )
    {}

    void operator() (const float& value)
    {
        min_lum_ = std::min(min_lum_, value);
        max_lum_ = std::max(max_lum_, value);
        avg_lum_ += value;
        adapted_lum_ += logf(DISPL_F + value);
    }

    float min_lum_;
    float max_lum_;
    float avg_lum_;
    float adapted_lum_;
};

const float LuminanceEqualization::DISPL_F = 2.3e-5f;

void computeAverage(const float* samples, size_t numSamples, float& average)
{
    float summation = accumulate(samples, samples + numSamples, 0.0f);
    average = summation/numSamples;
}

struct LuminanceProperties {
    float max;
    float min;
    float adaptedAverage;
    float average;
    float imageKey;
    float imageContrast;
    float imageBrightness;
};

void computeLuminanceProperties(const float* samples,
                                size_t numSamples,
                                LuminanceProperties& luminanceProperties,
                                const Reinhard05Params& params)
{
    // equalization parameters for the Luminance Channel
    LuminanceEqualization lum_eq = for_each(samples, samples + numSamples, LuminanceEqualization());

    luminanceProperties.max = std::log(lum_eq.max_lum_);
    luminanceProperties.min = std::log(lum_eq.min_lum_);
    luminanceProperties.adaptedAverage = lum_eq.adapted_lum_/numSamples;
    luminanceProperties.average = lum_eq.avg_lum_/numSamples;


    // image key (k)
    luminanceProperties.imageKey = (luminanceProperties.max - luminanceProperties.adaptedAverage) / (luminanceProperties.max - luminanceProperties.min);
    // image contrast based on key value (f)
    luminanceProperties.imageContrast = 0.3f + 0.7f*std::pow(luminanceProperties.imageKey, 1.4f);
    // image brightness (m?)
    luminanceProperties.imageBrightness = std::exp(-params.m_brightness);
}

inline void transformChannel_c(compute::vector<float> &samplesChannel,
                             const compute::vector<float> &samplesLuminance,
                             compute::vector<float> &outputSamples, size_t numSamples,
                             float channelAverage,
                             const Reinhard05Params &params,
                             const LuminanceProperties &lumProps,
                             compute::context &context,
                             compute::command_queue &queue) {
    float chromaticAdaptation_c = params.m_chromaticAdaptation;
    float channelAverage_c = channelAverage;
    float average_c = lumProps.average;
    float lightAdaptation_c = params.m_lightAdaptation;
    float imageBrightness_c = lumProps.imageBrightness;
    float imageContrast_c = lumProps.imageContrast;

    float compl_chromaticAdaptation_c = 1.f - chromaticAdaptation_c;
    float compl_lightAdaptation_c = 1.f - lightAdaptation_c;

    float Ig = (chromaticAdaptation_c * channelAverage_c) + (compl_chromaticAdaptation_c * average_c);
    compute::vector<float> Il(numSamples, context);
    compute::replace(samplesChannel.begin(), samplesChannel.end(), 0.0f, 0.01f, queue);
    compute::transform(samplesChannel.begin(), samplesChannel.end(), samplesLuminance.begin(), Il.begin(),
            lightAdaptation_c * ((chromaticAdaptation_c * _1) + (compl_chromaticAdaptation_c * _2)) +
            (compl_lightAdaptation_c * Ig) , queue);
    compute::transform(samplesChannel.begin(), samplesChannel.end(), Il.begin(), outputSamples.begin(),
            _1 /(_1 + pow(imageBrightness_c * _2, imageContrast_c)), queue);
}

inline void normalizeChannel_c(compute::vector<float> &samples, float min, float max, compute::command_queue &queue) {
    float range = max - min;
    compute::transform(samples.begin(), samples.end(), samples.begin(), (_1 - min)/range, queue);
}
}

#define TIMER_PROFILING
void tmo_reinhard05(size_t width, size_t height, float *nR, float *nG,
                    float *nB, const float *nY, const Reinhard05Params &params,
                    pfs::Progress &ph) {
#ifdef TIMER_PROFILING
    msec_timer f_timer;
    f_timer.start();
#endif

    compute::device device = compute::system::default_device();
    compute::context context(device);
    compute::command_queue queue(context, device);

    std::cout << " (platform: " << device.platform().name() << ")" << std::endl;

    float Cav[] = {0.0f, 0.0f, 0.0f};

    const size_t imSize = width * height;

    compute::vector<float> nR_c(imSize, context);
    compute::vector<float> nG_c(imSize, context);
    compute::vector<float> nB_c(imSize, context);
    compute::vector<float> nY_c(imSize, context);
    compute::copy(nR, nR + imSize, nR_c.begin(), queue);
    compute::copy(nG, nG + imSize, nG_c.begin(), queue);
    compute::copy(nB, nB + imSize, nB_c.begin(), queue);
    compute::copy(nY, nY + imSize, nY_c.begin(), queue);

    computeAverage(nR, imSize, Cav[0]);
    computeAverage(nG, imSize, Cav[1]);
    computeAverage(nB, imSize, Cav[2]);

    LuminanceProperties luminanceProperties;
    computeLuminanceProperties(nY, imSize, luminanceProperties, params);

    // output
    float max_col;
    float min_col;

    // transform Red Channel
    transformChannel_c(nR_c, nY_c, nR_c, imSize, Cav[0], params, luminanceProperties,
                       context, queue);

    // transform Green Channel
    transformChannel_c(nG_c, nY_c, nG_c, imSize, Cav[1], params, luminanceProperties,
                       context, queue);

    // transform Blue Channel
    transformChannel_c(nB_c, nY_c, nB_c, imSize, Cav[2], params, luminanceProperties,
                       context, queue);

    auto minmaxR = compute::minmax_element(nR_c.begin(), nR_c.end(), queue);
    auto minmaxG = compute::minmax_element(nG_c.begin(), nG_c.end(), queue);
    auto minmaxB = compute::minmax_element(nB_c.begin(), nB_c.end(), queue);

    float minR = *minmaxR.first;
    float minG = *minmaxG.first;
    float minB = *minmaxB.first;

    float maxR = *minmaxR.second;
    float maxG = *minmaxG.second;
    float maxB = *minmaxB.second;

    min_col = std::min(minR, std::min(minG, minB));
    max_col = std::max(maxR, std::max(maxG, maxB));

    //--- normalize intensities
    // normalize RED channel
    normalizeChannel_c(nR_c, min_col, max_col, queue);

    // normalize GREEN channel
    normalizeChannel_c(nG_c, min_col, max_col, queue);

    // normalize BLUE channel
    normalizeChannel_c(nB_c, min_col, max_col, queue);

    compute::copy(nR_c.begin(), nR_c.end(), nR, queue);
    compute::copy(nG_c.begin(), nG_c.end(), nG, queue);
    compute::copy(nB_c.begin(), nB_c.end(), nB, queue);

#ifdef TIMER_PROFILING
    f_timer.stop_and_update();
    cout << "reinhard05 = " << f_timer.get_time() << " msec"
              << endl;
#endif
}
