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

#ifndef PFS_RGBREMAPPER_H
#define PFS_RGBREMAPPER_H

//! \file RGBRemapper.h
//! \brief This file creates common routines for mapping RGB values into
//! different
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! \since Luminance HDR 2.3.0-beta1

#include <stdint.h>
#include <array>

#include <Libpfs/colorspace/convert.h>
#include <Libpfs/colorspace/rgbremapper_fwd.h>
#include <Libpfs/utils/transform.h>

// takes as template parameter a TypeOut, so that integer optimization can be
// performed if TypeOut is an uint8_t or uint16_t
class RemapperBase {
   protected:
    static float toLinear(float sample);
    static float toGamma14(float sample);
    static float toGamma18(float sample);
    static float toGamma22(float sample);
    static float toGamma26(float sample);
    static float toLog(float sample);

    typedef float (*MappingFunc)(float);

    static const MappingFunc s_callbacks[];
};

template <typename TypeOut>
class Remapper : public RemapperBase {
   public:
    Remapper(RGBMappingType mappingMethod = MAP_LINEAR)
        : m_mappingMethod(mappingMethod),
          m_callback(s_callbacks[mappingMethod]) {
        assert(mappingMethod >= 0);
        assert(mappingMethod < 6);
    }

    RGBMappingType getMappingMethod() const { return m_mappingMethod; }

    TypeOut operator()(float sample) const {
        assert(sample >= 0.f);
        assert(sample <= 1.f);

        using namespace pfs::colorspace;

        return convertSample<TypeOut>(m_callback(sample));
    }

    void operator()(float i1, float i2, float i3, TypeOut &o1, TypeOut &o2,
                    TypeOut &o3) const {
        o1 = (*this)(i1);
        o2 = (*this)(i2);
        o3 = (*this)(i3);
    }

   private:
    RGBMappingType m_mappingMethod;
    MappingFunc m_callback;
};

template <>
class Remapper<uint8_t> : public RemapperBase {
   public:
    Remapper(RGBMappingType mappingMethod = MAP_LINEAR);

    RGBMappingType getMappingMethod() const { return m_mappingMethod; }

    uint8_t operator()(float sample) const {
        assert(sample >= 0.f);
        assert(sample <= 1.f);

        using namespace pfs::colorspace;

        return m_lut[convertSample<uint8_t>(sample)];
    }

    void operator()(float i1, float i2, float i3, uint8_t &o1, uint8_t &o2,
                    uint8_t &o3) const {
        o1 = (*this)(i1);
        o2 = (*this)(i2);
        o3 = (*this)(i3);
    }

   private:
    RGBMappingType m_mappingMethod;
    std::array<uint8_t, 256> m_lut;  // LUT of 256 bins...
};

#endif  // PFS_RGBREMAPPER_H
