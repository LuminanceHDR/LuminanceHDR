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

#ifndef PFS_COLORSPACE_XYZ_H
#define PFS_COLORSPACE_XYZ_H

//! \brief XYZ conversion functions
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

namespace pfs {
namespace colorspace {

extern const float rgb2xyzD65Mat[3][3];
extern const float xyz2rgbD65Mat[3][3];

struct ConvertRGB2XYZ {
    void operator()(float i1, float i2, float i3, float &o1, float &o2,
                    float &o3) const;
};

struct ConvertSRGB2XYZ {
    void operator()(float i1, float i2, float i3, float &o1, float &o2,
                    float &o3) const;
};

struct ConvertRGB2Y {
    template <typename TypeIn, typename TypeOut>
    void operator()(TypeIn i1, TypeIn i2, TypeIn i3, TypeOut &o) const;
};

struct ConvertSRGB2Y {
    void operator()(float i1, float i2, float i3, float &o) const;
};

struct ConvertXYZ2RGB {
    void operator()(float i1, float i2, float i3, float &o1, float &o2,
                    float &o3) const;
};

struct ConvertXYZ2SRGB {
    void operator()(float i1, float i2, float i3, float &o1, float &o2,
                    float &o3) const;
};
}
}

#include <Libpfs/colorspace/xyz.hxx>
#endif  // PFS_COLORSPACE_XYZ_H
