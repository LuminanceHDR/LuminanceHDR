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
 *  but WITHo ANY WARRANTY; witho even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ----------------------------------------------------------------------
 */

#include <Libpfs/colorspace/rgb.h>
#include <Libpfs/colorspace/xyz.h>

namespace pfs {
namespace colorspace {

//! \brief Basic matrices for the SRGB <-> XYZ conversion
//! \ref http://www.brucelidbloom.com/Eqn_RGB_XYZ_Matrix.html
const float rgb2xyzD65Mat[3][3] = {{0.4124564f, 0.3575761f, 0.1804375f},
                                   {0.2126729f, 0.7151522f, 0.0721750f},
                                   {0.0193339f, 0.1191920f, 0.9503041f}};

const float xyz2rgbD65Mat[3][3] = {{3.2404542f, -1.5371385f, -0.4985314f},
                                   {-0.9692660f, 1.8760108f, 0.0415560f},
                                   {0.0556434f, -0.2040259f, 1.0572252f}};

void ConvertRGB2XYZ::operator()(float i1, float i2, float i3, float &o1,
                                float &o2, float &o3) const {
    o1 = rgb2xyzD65Mat[0][0] * i1 + rgb2xyzD65Mat[0][1] * i2 +
         rgb2xyzD65Mat[0][2] * i3;
    o2 = rgb2xyzD65Mat[1][0] * i1 + rgb2xyzD65Mat[1][1] * i2 +
         rgb2xyzD65Mat[1][2] * i3;
    o3 = rgb2xyzD65Mat[2][0] * i1 + rgb2xyzD65Mat[2][1] * i2 +
         rgb2xyzD65Mat[2][2] * i3;
}

void ConvertXYZ2RGB::operator()(float i1, float i2, float i3, float &o1,
                                float &o2, float &o3) const {
    o1 = xyz2rgbD65Mat[0][0] * i1 + xyz2rgbD65Mat[0][1] * i2 +
         xyz2rgbD65Mat[0][2] * i3;
    o2 = xyz2rgbD65Mat[1][0] * i1 + xyz2rgbD65Mat[1][1] * i2 +
         xyz2rgbD65Mat[1][2] * i3;
    o3 = xyz2rgbD65Mat[2][0] * i1 + xyz2rgbD65Mat[2][1] * i2 +
         xyz2rgbD65Mat[2][2] * i3;
}

}  // colorspace
}  // pfs
