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

#include "tmo_reinhard05.h"
#include "Libpfs/progress.h"
#include "TonemappingOperators/pfstmo.h"

#include <assert.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <numeric>

using namespace std;

namespace {
class LuminanceEqualization {
   private:
    static const float DISPL_F;

   public:
    LuminanceEqualization()
        : min_lum_(numeric_limits<float>::max()),
          max_lum_(-numeric_limits<float>::max()),
          avg_lum_(0.0f),
          adapted_lum_(0.0f) {}

    void operator()(const float &value) {
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

void computeAverage(const float *samples, size_t numSamples, float &average) {
    float summation = accumulate(samples, samples + numSamples, 0.0f);
    average = summation / numSamples;
}

class Normalizer {
   private:
    const float &min_;
    const float &max_;

   public:
    Normalizer(const float &min, const float &max) : min_(min), max_(max) {}

    void operator()(float &value) { value = (value - min_) / (max_ - min_); }
};

struct LuminanceProperties {
    float max;
    float min;
    float adaptedAverage;
    float average;
    float imageKey;
    float imageContrast;
    float imageBrightness;
};

void computeLuminanceProperties(const float *samples, size_t numSamples,
                                LuminanceProperties &luminanceProperties,
                                const Reinhard05Params &params) {
    // equalization parameters for the Luminance Channel
    LuminanceEqualization lum_eq =
        for_each(samples, samples + numSamples, LuminanceEqualization());

    luminanceProperties.max = std::log(lum_eq.max_lum_);
    luminanceProperties.min = std::log(lum_eq.min_lum_);
    luminanceProperties.adaptedAverage = lum_eq.adapted_lum_ / numSamples;
    luminanceProperties.average = lum_eq.avg_lum_ / numSamples;

    // image key (k)
    luminanceProperties.imageKey =
        (luminanceProperties.max - luminanceProperties.adaptedAverage) /
        (luminanceProperties.max - luminanceProperties.min);
    // image contrast based on key value (f)
    luminanceProperties.imageContrast =
        0.3f + 0.7f * std::pow(luminanceProperties.imageKey, 1.4f);
    // image brightness (m?)
    luminanceProperties.imageBrightness = std::exp(-params.m_brightness);
}

class ChannelTransformation {
   public:
    ChannelTransformation(float &min_sample, float &max_sample,
                          const float &channelAverage,
                          const Reinhard05Params &params,
                          const LuminanceProperties &lumProps)
        : min_sample_(min_sample),
          max_sample_(max_sample),
          channelAverage_(channelAverage),
          params_(params),
          lumProps_(lumProps) {}

    float operator()(float ch_sample, const float y_sample) {
        if (y_sample != 0.0f && ch_sample != 0.0f) {
            // local light adaptation
            float Il = (params_.m_chromaticAdaptation * ch_sample) +
                       ((1.f - params_.m_chromaticAdaptation) * y_sample);
            // global light adaptation
            float Ig =
                (params_.m_chromaticAdaptation * channelAverage_) +
                ((1.f - params_.m_chromaticAdaptation) * lumProps_.average);
            // interpolated light adaptation
            float Ia = (params_.m_lightAdaptation * Il) +
                       ((1.f - params_.m_lightAdaptation) * Ig);
            // photoreceptor equation
            ch_sample /= ch_sample + std::pow(lumProps_.imageBrightness * Ia,
                                              lumProps_.imageContrast);

            max_sample_ = std::max(ch_sample, max_sample_);
            min_sample_ = std::min(ch_sample, min_sample_);
        }
        return ch_sample;
    }

   private:
    // output
    float &min_sample_;
    float &max_sample_;

    const float &channelAverage_;
    const Reinhard05Params &params_;
    const LuminanceProperties &lumProps_;
};

inline void transformChannel(const float *samplesChannel,
                             const float *samplesLuminance,
                             float *outputSamples, size_t numSamples,
                             float channelAverage,
                             const Reinhard05Params &params,
                             const LuminanceProperties &lumProps,
                             float &minSample, float &maxSample) {
    transform(samplesChannel, samplesChannel + numSamples, samplesLuminance,
              outputSamples,
              ChannelTransformation(minSample, maxSample, channelAverage,
                                    params, lumProps));
}

void normalizeChannel(float *samples, size_t numSamples, float min, float max) {
    for_each(samples, samples + numSamples, Normalizer(min, max));
}
}

void tmo_reinhard05(size_t width, size_t height, float *nR, float *nG,
                    float *nB, const float *nY, const Reinhard05Params &params,
                    pfs::Progress &ph) {
    float Cav[] = {0.0f, 0.0f, 0.0f};

    const size_t imSize = width * height;

    computeAverage(nR, imSize, Cav[0]);
    computeAverage(nG, imSize, Cav[1]);
    computeAverage(nB, imSize, Cav[2]);

    LuminanceProperties luminanceProperties;
    computeLuminanceProperties(nY, imSize, luminanceProperties, params);

    // output
    float max_col = std::numeric_limits<float>::min();
    float min_col = std::numeric_limits<float>::max();

    // transform Red Channel
    transformChannel(nR, nY, nR, imSize, Cav[0], params, luminanceProperties,
                     min_col, max_col);
    ph.setValue(22);

    // transform Green Channel
    transformChannel(nG, nY, nG, imSize, Cav[1], params, luminanceProperties,
                     min_col, max_col);
    ph.setValue(44);

    // transform Blue Channel
    transformChannel(nB, nY, nB, imSize, Cav[2], params, luminanceProperties,
                     min_col, max_col);
    ph.setValue(66);

    if (!ph.canceled()) {
        //--- normalize intensities
        // normalize RED channel
        normalizeChannel(nR, imSize, min_col, max_col);
        ph.setValue(77);  // done!

        // normalize GREEN channel
        normalizeChannel(nG, imSize, min_col, max_col);
        ph.setValue(88);

        // normalize BLUE channel
        normalizeChannel(nB, imSize, min_col, max_col);
        ph.setValue(99);
    }
}
