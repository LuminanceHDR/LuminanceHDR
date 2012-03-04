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

#include "TonemappingOperators/pfstmo.h"
#include "tmo_reinhard05.h"

#include <assert.h>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <cmath>

using namespace std;

namespace
{
class LuminanceEqualization : public std::unary_function<const float&, void>
{
private:
    static float DISPL_F;
public:
    LuminanceEqualization():
        min_lum_( std::numeric_limits<float>::max() ),
        max_lum_( -std::numeric_limits<float>::max() ),
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
// set class static member
float LuminanceEqualization::DISPL_F = 2.3e-5f;


class ChannelTransformation
{
public:
    ChannelTransformation(float& min_sample, float& max_sample,
                          const float& ch_average, const float& lum_average,
                          float brightness,
                          float contrast,
                          float chromatic_adaptation,
                          float light_adaptation):
        min_sample_(min_sample),
        max_sample_(max_sample),
        channel_average_(ch_average),
        luminance_average_(lum_average),
        brightness_(brightness),
        contrast_(contrast),
        chromatic_adaptation_(chromatic_adaptation),
        light_adaptation_(light_adaptation)
    {}

    float operator()(float ch_sample, const float y_sample)
    {
        if ( y_sample != 0.0f && ch_sample != 0.0f )
        {
            // local light adaptation
            float Il = chromatic_adaptation_ * ch_sample + (1-chromatic_adaptation_)*y_sample;
            // global light adaptation
            float Ig = chromatic_adaptation_*channel_average_ + (1-chromatic_adaptation_)*luminance_average_;
            // interpolated light adaptation
            float Ia = light_adaptation_*Il + (1-light_adaptation_)*Ig;
            // photoreceptor equation
            ch_sample /= ch_sample + pow(brightness_*Ia, contrast_);

            max_sample_ = std::max(ch_sample, max_sample_);
            min_sample_ = std::min(ch_sample, min_sample_);
        }
        return ch_sample;
    }

private:
    // output
    float& min_sample_;
    float& max_sample_;
    // input
    const float& channel_average_;
    const float& luminance_average_;
    // parameters
    float brightness_;
    float contrast_;
    float chromatic_adaptation_;
    float light_adaptation_;
};

class Normalizer : public std::unary_function<const float&, void>
{
private:
    const float& min_;
    const float& max_;

public:
    Normalizer(const float& min, const float& max):
        min_(min),
        max_(max)
    {}

    void operator() (float& value)
    {
        value = (value - min_)/(max_ - min_);
    }
};


}

void tmo_reinhard05(unsigned int width, unsigned int height,
                    float* nR, float* nG, float* nB,
                    const float* nY,
                    const float br,
                    const float ca,
                    const float la,
                    ProgressHelper *ph)
{
    float Cav[] = {0.0f, 0.0f, 0.0f};

    const int im_width = width;
    const int im_height = height;
    const int im_size = im_width * im_height;

    // Average RED Channel
    Cav[0] = accumulate(nR, nR + im_size, 0.0f);
    Cav[0] /= im_size;
    // Average RED Channel
    Cav[1] = accumulate(nG, nG + im_size, 0.0f);
    Cav[1] /= im_size;
    // Average RED Channel
    Cav[1] = accumulate(nB, nB + im_size, 0.0f);
    Cav[1] /= im_size;

    // equalization parameters for the Luminance Channel
    LuminanceEqualization lum_eq = for_each(nY, nY + im_size, LuminanceEqualization());

    float max_lum = logf(lum_eq.max_lum_);
    float min_lum = logf(lum_eq.min_lum_);
    float world_lum = lum_eq.adapted_lum_/im_size;
    float Lav = lum_eq.avg_lum_/im_size;

    // image key
    const float k = (max_lum - world_lum) / (max_lum - min_lum);
    // image contrast based on key value
    const float m = 0.3f + 0.7f*powf(k, 1.4f);
    // image brightness
    const float f = exp(-br);

    // output
    float max_col = 0.0f;
    float min_col = 1.0f;

    int xTotal = 0;

    float* temp_red = nR;
    float* temp_green = nG;
    float* temp_blue = nB;
    const float* temp_lum = nY;

    // this loop cannot easily parallelizabile with OpenMP, it will be parallelizabile in LibHDR
    for (int i = 0; i < im_height; ++i)
    {
        // transform Red Channel
        temp_red = transform(temp_red, temp_red + im_width, temp_lum, temp_red, ChannelTransformation(min_col, max_col, Cav[0], Lav, f, m, ca, la) );
        // transform Green Channel
        temp_green = transform(temp_green, temp_green + im_width, temp_lum, temp_green, ChannelTransformation(min_col, max_col, Cav[1], Lav, f, m, ca, la) );
        // transform Blue Channel
        temp_blue = transform(temp_blue, temp_blue + im_width, temp_lum, temp_blue, ChannelTransformation(min_col, max_col, Cav[2], Lav, f, m, ca, la) );

        temp_lum += im_width;
        ph->newValue(66 * (xTotal++) / im_height);
    }


    if (!ph->isTerminationRequested())
    {
        //--- normalize intensities
        // normalize RED channel
        for_each(nR, nR + im_size, Normalizer(min_col, max_col));
        ph->newValue(77);  // done!

        // normalize GREEN channel
        for_each(nG, nG + im_size, Normalizer(min_col, max_col));
        ph->newValue(88);

        // normalize BLUE channel
        for_each(nB, nB + im_size, Normalizer(min_col, max_col));
        ph->newValue(99);
    }
}
