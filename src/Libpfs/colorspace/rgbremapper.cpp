/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2012 Davide Anastasia
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

//! \file RGBRemapper.cpp
//! \brief This file creates common routines for mapping float RGB values into
//! 8-bits or 16-bits integer RGB (QRgb or quint16)
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! \since Luminance HDR 2.3.0-beta1

#include <Libpfs/colorspace/rgbremapper.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include "arch/math.h"

namespace {
const float GAMMA_1_4 = 1.0f / 1.4f;
const float GAMMA_1_8 = 1.0f / 1.8f;
const float GAMMA_2_2 = 1.0f / 2.2f;
const float GAMMA_2_6 = 1.0f / 2.6f;
}

float RemapperBase::toLinear(float sample) { return sample; }

float RemapperBase::toGamma14(float sample) {
    return std::pow(sample, GAMMA_1_4);
}

float RemapperBase::toGamma18(float sample) {
    return std::pow(sample, GAMMA_1_8);
}

float RemapperBase::toGamma22(float sample) {
    return std::pow(sample, GAMMA_2_2);
}

float RemapperBase::toGamma26(float sample) {
    return std::pow(sample, GAMMA_2_6);
}

float RemapperBase::toLog(float sample) {
    return std::pow(sample, 1.f / GAMMA_2_2);
}

const RemapperBase::MappingFunc RemapperBase::s_callbacks[] = {
    &toLinear, &toGamma14, &toGamma18, &toGamma22, &toGamma26, &toLog};

Remapper<uint8_t>::Remapper(RGBMappingType mappingMethod)
    : m_mappingMethod(mappingMethod) {
    assert(mappingMethod >= 0);
    assert(mappingMethod < 6);

    MappingFunc callback(s_callbacks[mappingMethod]);

    for (int idx = 0; idx < 256; ++idx) {
        m_lut[idx] = 255.f * callback(float(idx) / 255.f);
    }
}
