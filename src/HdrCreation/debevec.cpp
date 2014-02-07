/*
 * This file is a part of Luminance HDR package
 * ----------------------------------------------------------------------
 * Copyright (C) 2006,2007 Giuseppe Rota
 * Copyright (C) 2013 Davide Anastasia
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

#include <cassert>
#include <iostream>
#include <vector>
#include <boost/numeric/conversion/bounds.hpp>
#include <boost/limits.hpp>

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

namespace libhdr {
namespace fusion {

struct ColorData {
    ColorData()
        : numerator_(0.f)
        , denominator_(0.f)
        , blackValue_(0.f)
        , whiteValue_(1.f)
    {}

    float value() {
        return ((denominator_ != 0.0f) ? numerator_/denominator_ : 0.0f);
    }

    float numerator_;
    float denominator_;

    float blackValue_;
    float whiteValue_;
};

void DebevecOperator::computeFusion(ResponseCurve& response,
                                    const vector<FrameEnhanced> &frames,
                                    pfs::Frame &frame) const
{
    assert(frames.size());

    size_t numExposures = frames.size();
    Frame tempFrame ( frames[0].frame()->getWidth(), frames[0].frame()->getHeight() );

    Channel* outputRed;
    Channel* outputGreen;
    Channel* outputBlue;
    tempFrame.createXYZChannels(outputRed, outputGreen, outputBlue);

    DataList redChannels(numExposures);
    DataList greenChannels(numExposures);
    DataList blueChannels(numExposures);

    fillDataLists(frames, redChannels, greenChannels, blueChannels);

    size_t saturatedPixels = 0;
    float maxAllowedValue = maxTrustedValue();
    float minAllowedValue = minTrustedValue();

#pragma omp parallel for
    for (int idx = 0; idx < tempFrame.size(); ++idx)
    {
        // data...
        ColorData redData;
        ColorData greenData;
        ColorData blueData;

        float maxAvgLum = boost::numeric::bounds<float>::lowest();
        float minAvgLum = boost::numeric::bounds<float>::highest();

        // for all exposures
        for (size_t exp = 0; exp < numExposures; ++exp)
        {
            // average luminance for this exposure
            float avgLum    = frames[exp].averageLuminance();
            // pick the 3 channel values
            float red       = redChannels[exp][idx];
            float green     = greenChannels[exp][idx];
            float blue      = blueChannels[exp][idx];

            float w_red     = weight(red);
            float w_green   = weight(green);
            float w_blue    = weight(blue);

            // if at least one of the color channel's values are in the bright
            // "untrusted zone" and we have min exposure time
            // check red channel
            if ( (avgLum < minAvgLum) &&
                 ((red > maxAllowedValue) || (green > maxAllowedValue) || (blue > maxAllowedValue)) )
            {
                minAvgLum               = avgLum;
                redData.blackValue_     = red;
                greenData.blackValue_   = green;
                blueData.blackValue_    = blue;
            }

            // if at least one of the color channel's values are in the dim
            // "not-trusted zone" and we have max exposure time
            if ( (avgLum > maxAvgLum) &&
                 ((red < minAllowedValue) || (green < minAllowedValue) || (blue < minAllowedValue)) )
            {
                maxAvgLum               = avgLum;
                redData.whiteValue_     = red;
                greenData.whiteValue_   = green;
                blueData.whiteValue_    = blue;
            }

            float w_average = (w_red + w_green + w_blue)/3.0f;
            redData.numerator_      += (w_average * response(red))/avgLum;
            redData.denominator_    += w_average;
            greenData.numerator_    += (w_average * response(green))/avgLum;
            greenData.denominator_  += w_average;
            blueData.numerator_     += (w_average * response(blue))/avgLum;
            blueData.denominator_   += w_average;
        }
        // END for all the exposures

        if ( (redData.denominator_ == 0.f) || (greenData.denominator_ == 0.f) || (blueData.denominator_ == 0.f) ) {
            ++saturatedPixels;

            if ( maxAvgLum > boost::numeric::bounds<float>::lowest() )
            {
                redData.numerator_      = response(redData.blackValue_) / maxAvgLum;
                greenData.numerator_    = response(greenData.blackValue_) / maxAvgLum;
                blueData.numerator_     = response(blueData.blackValue_) / maxAvgLum;

                redData.denominator_    = 1.f;
                greenData.denominator_  = 1.f;
                blueData.denominator_   = 1.f;
            }

            if ( minAvgLum < boost::numeric::bounds<float>::highest() )
            {
                redData.numerator_      = response(redData.whiteValue_) / minAvgLum;
                greenData.numerator_    = response(greenData.whiteValue_) / minAvgLum;
                blueData.numerator_     = response(blueData.whiteValue_) / minAvgLum;

                redData.denominator_    = 1.f;
                greenData.denominator_  = 1.f;
                blueData.denominator_   = 1.f;
            }
        }

        (*outputRed)(idx)   = redData.value();
        (*outputGreen)(idx) = greenData.value();
        (*outputBlue)(idx)  = blueData.value();
    }

    PRINT_DEBUG("Saturated pixels: " << saturatedPixels);

    frame.swap( tempFrame );
}

}   // libhdr
}   // fusion
