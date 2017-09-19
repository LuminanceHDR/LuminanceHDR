/**
 * @brief PFS library - color space transformations
 *
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ----------------------------------------------------------------------
 *
 * @author Rafal Mantiuk, <mantiuk@mpi-sb.mpg.de>
 *
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net> (2010 10 13)
 *  Reimplementation of most of the functions (in particular the ones involving
 * RGB and XYZ)
 *
 * $Id: colorspace.cpp,v 1.6 2007/07/18 08:49:25 rafm Exp $
 */

#include "colorspace.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <map>

#include "Libpfs/array2d.h"
#include "Libpfs/pfs.h"
#include "Libpfs/utils/msec_timer.h"

#include "Libpfs/colorspace/rgb.h"
#include "Libpfs/colorspace/xyz.h"
#include "Libpfs/colorspace/yuv.h"
#include "Libpfs/utils/transform.h"

#include <boost/assign.hpp>

using namespace std;
using namespace boost::assign;

namespace pfs {

//-----------------------------------------------------------
// sRGB conversion functions
//-----------------------------------------------------------
void transformSRGB2XYZ(const Array2Df *inC1, const Array2Df *inC2,
                       const Array2Df *inC3, Array2Df *outC1, Array2Df *outC2,
                       Array2Df *outC3) {
#ifdef TIMER_PROFILING
    msec_timer f_timer;
    f_timer.start();
#endif

    utils::transform(inC1->begin(), inC1->end(), inC2->begin(), inC3->begin(),
                     outC1->begin(), outC2->begin(), outC3->begin(),
                     colorspace::ConvertSRGB2XYZ());

#ifdef TIMER_PROFILING
    f_timer.stop_and_update();
    std::cout << "transformSRGB2XYZ() = " << f_timer.get_time() << " msec"
              << std::endl;
#endif
}
void transformSRGB2Y(const Array2Df *inC1, const Array2Df *inC2,
                     const Array2Df *inC3, Array2Df *outC1) {
    utils::transform(inC1->begin(), inC1->end(), inC2->begin(), inC3->begin(),
                     outC1->begin(), colorspace::ConvertSRGB2Y());
}

//-----------------------------------------------------------
// RGB conversion functions
//-----------------------------------------------------------
void transformRGB2XYZ(const Array2Df *inC1, const Array2Df *inC2,
                      const Array2Df *inC3, Array2Df *outC1, Array2Df *outC2,
                      Array2Df *outC3) {
#ifdef TIMER_PROFILING
    msec_timer f_timer;
    f_timer.start();
#endif

    utils::transform(inC1->begin(), inC1->end(), inC2->begin(), inC3->begin(),
                     outC1->begin(), outC2->begin(), outC3->begin(),
                     colorspace::ConvertRGB2XYZ());

#ifdef TIMER_PROFILING
    f_timer.stop_and_update();
    std::cout << "transformRGB2XYZ() = " << f_timer.get_time() << " msec"
              << std::endl;
#endif
}

void transformRGB2Y(const Array2Df *inC1, const Array2Df *inC2,
                    const Array2Df *inC3, Array2Df *outC1) {
    utils::transform(inC1->begin(), inC1->end(), inC2->begin(), inC3->begin(),
                     outC1->begin(), colorspace::ConvertRGB2Y());
}

void transformRGB2Yuv(const Array2Df *inC1, const Array2Df *inC2,
                      const Array2Df *inC3, Array2Df *outC1, Array2Df *outC2,
                      Array2Df *outC3) {
#ifdef TIMER_PROFILING
    msec_timer f_timer;
    f_timer.start();
#endif

    utils::transform(inC1->begin(), inC1->end(), inC2->begin(), inC3->begin(),
                     outC1->begin(), outC2->begin(), outC3->begin(),
                     colorspace::ConvertRGB2YUV());

#ifdef TIMER_PROFILING
    f_timer.stop_and_update();
    std::cout << "transformRGB2Yuv() = " << f_timer.get_time() << " msec"
              << std::endl;
#endif
}

void transformXYZ2SRGB(const Array2Df *inC1, const Array2Df *inC2,
                       const Array2Df *inC3, Array2Df *outC1, Array2Df *outC2,
                       Array2Df *outC3) {
#ifdef TIMER_PROFILING
    msec_timer f_timer;
    f_timer.start();
#endif

    utils::transform(inC1->begin(), inC1->end(), inC2->begin(), inC3->begin(),
                     outC1->begin(), outC2->begin(), outC3->begin(),
                     colorspace::ConvertXYZ2SRGB());

#ifdef TIMER_PROFILING
    f_timer.stop_and_update();
    std::cout << "transformXYZ2SRGB() = " << f_timer.get_time() << " msec"
              << std::endl;
#endif
}

void transformXYZ2RGB(const Array2Df *inC1, const Array2Df *inC2,
                      const Array2Df *inC3, Array2Df *outC1, Array2Df *outC2,
                      Array2Df *outC3) {
#ifdef TIMER_PROFILING
    msec_timer f_timer;
    f_timer.start();
#endif

    utils::transform(inC1->begin(), inC1->end(), inC2->begin(), inC3->begin(),
                     outC1->begin(), outC2->begin(), outC3->begin(),
                     colorspace::ConvertXYZ2RGB());

#ifdef TIMER_PROFILING
    f_timer.stop_and_update();
    std::cout << "transformXYZ2RGB() = " << f_timer.get_time() << " msec"
              << std::endl;
#endif
}

void transformXYZ2Yuv(const Array2Df *inC1, const Array2Df *inC2,
                      const Array2Df *inC3, Array2Df *outC1, Array2Df *outC2,
                      Array2Df *outC3) {
    const int elems = inC1->getRows() * inC1->getCols();
    for (int idx = 0; idx < elems; idx++) {
        const float &X = (*inC1)(idx), Y = (*inC2)(idx), &Z = (*inC3)(idx);
        float &outY = (*outC1)(idx), &u = (*outC2)(idx), &v = (*outC3)(idx);

        float x = X / (X + Y + Z);
        float y = Y / (X + Y + Z);

        // assert((4.f*nx / (-2.f*nx + 12.f*ny + 3.f)) <= 0.62 );
        // assert( (9.f*ny / (-2.f*nx + 12.f*ny + 3.f)) <= 0.62 );

        u = 4.f * x / (-2.f * x + 12.f * y + 3.f);
        v = 9.f * y / (-2.f * x + 12.f * y + 3.f);
        outY = Y;
    }
}

void transformYuv2XYZ(const Array2Df *inC1, const Array2Df *inC2,
                      const Array2Df *inC3, Array2Df *outC1, Array2Df *outC2,
                      Array2Df *outC3) {
    const int elems = inC1->getRows() * inC1->getCols();
    for (int idx = 0; idx < elems; idx++) {
        const float Y = (*inC1)(idx), &u = (*inC2)(idx), &v = (*inC3)(idx);
        float &X = (*outC1)(idx), &outY = (*outC2)(idx), &Z = (*outC3)(idx);

        float x = 9.f * u / (6.f * u - 16.f * v + 12.f);
        float y = 4.f * v / (6.f * u - 16.f * v + 12.f);

        X = x / y * Y;
        Z = (1.f - x - y) / y * Y;
        outY = Y;
    }
}

void transformYuv2RGB(const Array2Df *inC1, const Array2Df *inC2,
                      const Array2Df *inC3, Array2Df *outC1, Array2Df *outC2,
                      Array2Df *outC3) {
#ifdef TIMER_PROFILING
    msec_timer f_timer;
    f_timer.start();
#endif

    utils::transform(inC1->begin(), inC1->end(), inC2->begin(), inC3->begin(),
                     outC1->begin(), outC2->begin(), outC3->begin(),
                     colorspace::ConvertYUV2RGB());

#ifdef TIMER_PROFILING
    f_timer.stop_and_update();
    std::cout << "transformYuv2RGB() = " << f_timer.get_time() << " msec"
              << std::endl;
#endif
}

void transformYxy2XYZ(const Array2Df *inC1, const Array2Df *inC2,
                      const Array2Df *inC3, Array2Df *outC1, Array2Df *outC2,
                      Array2Df *outC3) {
    const int elems = inC1->getRows() * inC1->getCols();
    for (int idx = 0; idx < elems; idx++) {
        const float Y = (*inC1)(idx), x = (*inC2)(idx), y = (*inC3)(idx);
        float &X = (*outC1)(idx), &outY = (*outC2)(idx), &Z = (*outC3)(idx);

        X = x / y * Y;
        Z = (1.f - x - y) / y * Y;
        outY = Y;
    }
}

void transformXYZ2Yxy(const Array2Df *inC1, const Array2Df *inC2,
                      const Array2Df *inC3, Array2Df *outC1, Array2Df *outC2,
                      Array2Df *outC3) {
    const int elems = inC1->getRows() * inC1->getCols();
    for (int idx = 0; idx < elems; idx++) {
        const float X = (*inC1)(idx), Y = (*inC2)(idx), Z = (*inC3)(idx);
        float &outY = (*outC1)(idx), &x = (*outC2)(idx), &y = (*outC3)(idx);

        x = X / (X + Y + Z);
        y = Y / (X + Y + Z);

        outY = Y;
    }
}

typedef void (*CSTransformFunc)(const Array2Df *inC1, const Array2Df *inC2,
                                const Array2Df *inC3, Array2Df *outC1,
                                Array2Df *outC2, Array2Df *outC3);
typedef std::pair<ColorSpace, ColorSpace> CSTransformProfile;
typedef std::map<CSTransformProfile, CSTransformFunc> CSTransformMap;

void transformColorSpace(ColorSpace inCS, const Array2Df *inC1,
                         const Array2Df *inC2, const Array2Df *inC3,
                         ColorSpace outCS, Array2Df *outC1, Array2Df *outC2,
                         Array2Df *outC3) {
    assert(inC1->getCols() == inC2->getCols() &&
           inC2->getCols() == inC3->getCols() &&
           inC3->getCols() == outC1->getCols() &&
           outC1->getCols() == outC2->getCols() &&
           outC2->getCols() == outC3->getCols());

    assert(inC1->getRows() == inC2->getRows() &&
           inC2->getRows() == inC3->getRows() &&
           inC3->getRows() == outC1->getRows() &&
           outC1->getRows() == outC2->getRows() &&
           outC2->getRows() == outC3->getRows());

    // static dictionary... I already know in advance the subscription I want
    // to perform, hence this approach is far easier than try to create
    // automatic subscription to the factory
    static CSTransformMap s_csTransformMap = map_list_of
        // XYZ -> *
        (CSTransformProfile(CS_XYZ, CS_SRGB), transformXYZ2SRGB)(
            CSTransformProfile(CS_XYZ, CS_RGB), transformXYZ2RGB)(
            CSTransformProfile(CS_XYZ, CS_YUV), transformXYZ2Yuv)(
            CSTransformProfile(CS_XYZ, CS_Yxy), transformXYZ2Yxy)
        // sRGB -> *
        (CSTransformProfile(CS_SRGB, CS_XYZ), transformSRGB2XYZ)
        // RGB -> *
        (CSTransformProfile(CS_RGB, CS_XYZ), transformRGB2XYZ)(
            CSTransformProfile(CS_RGB, CS_YUV), transformRGB2Yuv)
        // Yuv -> *
        (CSTransformProfile(CS_YUV, CS_XYZ), transformYuv2XYZ)(
            CSTransformProfile(CS_YUV, CS_RGB), transformYuv2RGB)
        // Yxy -> *
        (CSTransformProfile(CS_Yxy, CS_XYZ), transformYxy2XYZ);

    CSTransformMap::const_iterator itTransform =
        s_csTransformMap.find(CSTransformProfile(inCS, outCS));

    if (itTransform == s_csTransformMap.end()) {
        throw Exception("Unsupported color tranform");
    }
#ifndef NDEBUG
    std::cerr << __FUNCTION__
              << ": Found right match for colorspace conversion\n";
#endif
    // CSTransformFunc func =
    (itTransform->second)(inC1, inC2, inC3, outC1, outC2, outC3);
}
}  // namespace pfs
