/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
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
 */

#ifndef PFS_COLORSPACE_XYZ_HXX
#define PFS_COLORSPACE_XYZ_HXX

//! \brief XYZ conversion functions
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#include <Libpfs/colorspace/convert.h>
#include <Libpfs/colorspace/rgb.h>
#include <Libpfs/colorspace/xyz.h>

namespace pfs {
namespace colorspace {

template <typename TypeIn, typename TypeOut>
void ConvertRGB2Y::operator()(TypeIn i1, TypeIn i2, TypeIn i3,
                              TypeOut &o) const {
    o = convertSample<TypeOut>(rgb2xyzD65Mat[1][0] * convertSample<float>(i1) +
                               rgb2xyzD65Mat[1][1] * convertSample<float>(i2) +
                               rgb2xyzD65Mat[1][2] * convertSample<float>(i3));
}

inline void ConvertSRGB2XYZ::operator()(float i1, float i2, float i3, float &o1,
                                        float &o2, float &o3) const {
    colorspace::ConvertSRGB2RGB()(i1, i2, i3, i1, i2, i3);
    colorspace::ConvertRGB2XYZ()(i1, i2, i3, o1, o2, o3);
}

inline void ConvertXYZ2SRGB::operator()(float i1, float i2, float i3, float &o1,
                                        float &o2, float &o3) const {
    colorspace::ConvertXYZ2RGB()(i1, i2, i3, i1, i2, i3);
    colorspace::ConvertRGB2SRGB()(i1, i2, i3, o1, o2, o3);
}

inline void ConvertSRGB2Y::operator()(float i1, float i2, float i3,
                                      float &o) const {
    colorspace::ConvertSRGB2RGB()(i1, i2, i3, i1, i2, i3);
    colorspace::ConvertRGB2Y()(i1, i2, i3, o);
}

}  // namespace
}  // pfs

#endif  // PFS_COLORSPACE_XYZ_H
