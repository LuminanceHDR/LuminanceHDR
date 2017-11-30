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

#include <Libpfs/colorspace/rgb.h>
#include <cmath>
#include "../../sleef.c"
#define pow_F(a,b) (xexpf(b*xlogf(a)))

namespace pfs {
namespace colorspace {

float ConvertSRGB2RGB::operator()(float sample) const {
    if (sample > 0.04045f) {
        return pow_F((sample + 0.055f) * (1.f / 1.055f), 2.4f);
    }
    if (sample >= -0.04045f) {
        return sample * (1.f / 12.92f);
    }
    return -pow_F((0.055f - sample) * (1.f / 1.055f), 2.4f);
}

float ConvertRGB2SRGB::operator()(float sample) const {
    if (sample > 0.0031308f) {
        return ((1.055f * pow_F(sample, 1.f / 2.4f)) - 0.055f);
    }
    if (sample >= -0.0031308f) {
        return (sample * 12.92f);
    }
    return ((0.055f - 1.f) * pow_F(-sample, 1.f / 2.4f) - 0.055f);
}
}
}
