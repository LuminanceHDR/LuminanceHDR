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

//! \brief Sample convert functions
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#ifndef PFS_COLORSPACE_CONVERT_H
#define PFS_COLORSPACE_CONVERT_H

#include <stdint.h>
#include <cmath>
#include <iostream>

namespace pfs {
namespace colorspace {

template <typename TypeOut, typename TypeIn>
struct ConvertSample;

template <typename Type>
struct ConvertSample<Type, Type> {
    Type operator()(Type vIn) { return vIn; }
};

template <>
struct ConvertSample<float, uint16_t> {
    float operator()(uint16_t vIn) { return ((float)vIn) / 65535.f; }
};

template <>
struct ConvertSample<float, uint8_t> {
    float operator()(uint8_t vIn) { return ((float)vIn) / 255.f; }
};

template <>
struct ConvertSample<uint8_t, float> {
    uint8_t operator()(float vIn) {
        return static_cast<uint8_t>(vIn * 255.f + 0.5f);
    }
};

template <>
struct ConvertSample<uint16_t, float> {
    uint16_t operator()(float vIn) {
        return static_cast<uint16_t>(vIn * 65535.f + 0.5f);
    }
};

template <typename TypeOut, typename TypeIn>
TypeOut convertSample(TypeIn vIn) {
    return ConvertSample<TypeOut, TypeIn>()(vIn);
}

}  // colorspace
}  // pfs

#endif  // PFS_COLORSPACE_CONVERT_H
