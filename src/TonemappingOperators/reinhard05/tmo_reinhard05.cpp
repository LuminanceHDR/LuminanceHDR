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
#include "../../opthelper.h"
#include "../../sleef.c"
#define pow_F(a,b) (xexpf(b*xlogf(a)))

using namespace std;

namespace {

void computeAverage(const float *samples, size_t width, size_t height, float &average) {

    double summation = 0.0; // always use double precision for large summations
#ifdef _OPENMP
    #pragma omp parallel for reduction(+:summation)
#endif
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            summation += samples[y * width + x];
        }
    }
    average = summation / (width * height);
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

void computeLuminanceProperties(const float *samples, size_t width, size_t height,
                                LuminanceProperties &luminanceProperties,
                                const Reinhard05Params &params) {

    // equalization parameters for the Luminance Channel
    float min_lum = numeric_limits<float>::max();
    float max_lum = -numeric_limits<float>::max();
    float avg_lum = 0.f;
    float adapted_lum = 0.f;
#ifdef _OPENMP
#pragma omp parallel
#endif
{
    float min_lumthr = numeric_limits<float>::max();
    float max_lumthr = -numeric_limits<float>::max();
    float avg_lumthr = 0.f;
    float adapted_lumthr = 0.f;

#ifdef __SSE2__
    vfloat min_lumv = F2V(min_lum);
    vfloat max_lumv = F2V(max_lum);
    vfloat avg_lumv = ZEROV;
    vfloat adapted_lumv = ZEROV;
    vfloat c1v = F2V(2.3e-5f);
#endif
#ifdef _OPENMP
    #pragma omp for nowait
#endif
    for (size_t y = 0; y < height; ++y) {
        size_t x = 0;
#ifdef __SSE2__
        for (; x < width - 3; x += 4) {
            vfloat value = LVFU(samples[y * width + x]);
            min_lumv = vminf(min_lumv, value);
            max_lumv = vmaxf(max_lumv, value);
            avg_lumv += value;
            adapted_lumv += xlogf(c1v + value);
        }
#endif
        for (; x < width; ++x) {
            float value = samples[y * width + x];
            min_lum = std::min(min_lum, value);
            max_lum = std::max(max_lum, value);
            avg_lum += value;
            adapted_lum += xlogf(2.3e-5f + value);
        }
    }
#ifdef _OPENMP
#pragma omp critical
#endif
{
    min_lum = std::min(min_lum, min_lumthr);
    max_lum = std::max(max_lum, max_lumthr);
    avg_lum += avg_lumthr;
    adapted_lum += adapted_lumthr;
#ifdef __SSE2__
    min_lum = std::min(min_lum, vhmin(min_lumv));
    max_lum = std::max(max_lum, vhmax(max_lumv));
    avg_lum += vhadd(avg_lumv);
    adapted_lum += vhadd(adapted_lumv);
#endif
}
}

    luminanceProperties.max = xlogf(max_lum);
    luminanceProperties.min = xlogf(min_lum);
    luminanceProperties.adaptedAverage = adapted_lum / (width * height);
    luminanceProperties.average = avg_lum / (width * height);

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

void transformChannel(const float *samplesChannel,
                             const float *samplesLuminance,
                             float *outputSamples, size_t width, size_t height,
                             float channelAverage,
                             const Reinhard05Params &params,
                             const LuminanceProperties &lumProps,
                             float &minSample, float &maxSample) {

#ifdef _OPENMP
#pragma omp parallel
#endif
{
    float minSampleThr = minSample;
    float maxSampleThr = maxSample;
#ifdef __SSE2__
    vfloat onev = F2V(1.f);
    vfloat m_chromaticAdaptationv = F2V(params.m_chromaticAdaptation);
    vfloat m_lightAdaptationv = F2V(params.m_lightAdaptation);
    vfloat channelAveragev = F2V(channelAverage);
    vfloat laveragev = F2V(lumProps.average);
    vfloat limageBrightnessv = F2V(lumProps.imageBrightness);
    vfloat limageContrastv = F2V(lumProps.imageContrast);
    vfloat minSamplev = F2V(minSample);
    vfloat maxSamplev = F2V(maxSample);
#endif
#ifdef _OPENMP
    #pragma omp for
#endif
    for (size_t y = 0; y < height; ++y) {
        size_t x = 0;
#ifdef __SSE2__
        for (; x < width - 3; x+=4) {
            vfloat chval = LVFU(samplesChannel[y * width + x]);
            vfloat oldchval = chval;
            vfloat yval = LVFU(samplesLuminance[y * width + x]);
            vmask selmask = vandm(vmaskf_neq(chval, ZEROV), vmaskf_neq(yval, ZEROV));
            // local light adaptation
            vfloat Il = (m_chromaticAdaptationv * chval) +
                       ((onev - m_chromaticAdaptationv) * yval);
            // global light adaptation
            vfloat Ig =
                (m_chromaticAdaptationv * channelAveragev) +
                ((onev - m_chromaticAdaptationv) * laveragev);
            // interpolated light adaptation
            vfloat Ia = (m_lightAdaptationv * Il) +
                       ((onev - m_lightAdaptationv) * Ig);
            // photoreceptor equation
            chval /= chval + pow_F(limageBrightnessv * Ia, limageContrastv);

            maxSamplev = vself(selmask, vmaxf(chval, maxSamplev), maxSamplev);
            minSamplev = vself(selmask, vminf(chval, minSamplev), minSamplev);
            chval = vself(selmask, chval, oldchval);
            STVFU(outputSamples[y * width + x], chval);
        }
#endif
        for (; x < width; ++x) {
            float chval = samplesChannel[y * width + x];
            float yval = samplesLuminance[y * width + x];
            if (yval != 0.0f && chval != 0.0f) {
                // local light adaptation
                float Il = (params.m_chromaticAdaptation * chval) +
                           ((1.f - params.m_chromaticAdaptation) * yval);
                // global light adaptation
                float Ig =
                    (params.m_chromaticAdaptation * channelAverage) +
                    ((1.f - params.m_chromaticAdaptation) * lumProps.average);
                // interpolated light adaptation
                float Ia = (params.m_lightAdaptation * Il) +
                           ((1.f - params.m_lightAdaptation) * Ig);
                // photoreceptor equation
                chval /= chval + pow_F(lumProps.imageBrightness * Ia,
                                                  lumProps.imageContrast);

                maxSampleThr = std::max(chval, maxSampleThr);
                minSampleThr = std::min(chval, minSampleThr);
            }
            outputSamples[y * width + x] = chval;
        }
    }
#ifdef _OPENMP
#pragma omp critical
#endif
{
    minSample = std::min(minSampleThr, minSample);
    maxSample = std::max(maxSampleThr, maxSample);
#ifdef __SSE2__
    minSample = std::min(minSample, vhmin(minSamplev));
    maxSample = std::max(maxSample, vhmax(maxSamplev));
#endif
}
}
}

void normalizeChannel(float *samples, size_t width, size_t height, float min, float max) {

    float dividor = max - min;
#ifdef _OPENMP
    #pragma omp parallel for
#endif
    for (size_t y = 0; y < height; ++y) {
        // manual vectorization of this simple loop gives no speedup. Most likely compiler auto-vectorizes this well
        for (size_t x = 0; x < width; ++x) {
            samples[y * width + x] = (samples[y * width + x] - min) / (dividor);
        }
    }
}

}

void tmo_reinhard05(size_t width, size_t height, float *nR, float *nG,
                    float *nB, const float *nY, const Reinhard05Params &params,
                    pfs::Progress &ph) {

    float Cav[] = {0.0f, 0.0f, 0.0f};

    computeAverage(nR, width, height, Cav[0]);
    ph.setValue(2);

    computeAverage(nG, width, height, Cav[1]);
    ph.setValue(4);

    computeAverage(nB, width, height, Cav[2]);
    ph.setValue(6);

    LuminanceProperties luminanceProperties;
    computeLuminanceProperties(nY, width, height, luminanceProperties, params);
    ph.setValue(11);

    // output
    float max_col = std::numeric_limits<float>::min();
    float min_col = std::numeric_limits<float>::max();


    // transform Red Channel
    transformChannel(nR, nY, nR, width, height, Cav[0], params, luminanceProperties,
                     min_col, max_col);
    ph.setValue(38);

    // transform Green Channel
    transformChannel(nG, nY, nG, width, height, Cav[1], params, luminanceProperties,
                     min_col, max_col);
    ph.setValue(58);

    // transform Blue Channel
    transformChannel(nB, nY, nB, width, height, Cav[2], params, luminanceProperties,
                     min_col, max_col);
    ph.setValue(78);

    //--- normalize intensities
    // normalize RED channel
    normalizeChannel(nR, width, height, min_col, max_col);

    // normalize GREEN channel
    normalizeChannel(nG, width, height, min_col, max_col);

    // normalize BLUE channel
    normalizeChannel(nB, width, height, min_col, max_col);
}
