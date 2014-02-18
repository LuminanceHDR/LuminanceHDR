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
#include <QRgb>

#include <Libpfs/colorspace/rgbremapper_fwd.h>
#include <Libpfs/colorspace/convert.h>
#include <Libpfs/utils/transform.h>

// takes as template parameter a TypeOut, so that integer optimization can be
// performed if TypeOut is an uint8_t or uint16_t
class RemapperBase
{
protected:
    static float toLinear(float sample);
    static float toGamma14(float sample);
    static float toGamma18(float sample);
    static float toGamma22(float sample);
    static float toGamma26(float sample);
    static float toLog(float sample);
};

template <typename TypeOut>
class Remapper : public RemapperBase
{
private:
    typedef float (*MappingFunc)(float);

#ifdef LHDR_CXX11_ENABLED
    static constexpr MappingFunc s_callbacks[] =
    { &toLinear, &toGamma14, &toGamma18, &toGamma22, &toGamma26, &toLog };
#else
    static const MappingFunc s_callbacks[] =
    { &toLinear, &toGamma14, &toGamma18, &toGamma22, &toGamma26, &toLog };
#endif

public:
    Remapper(RGBMappingType mappingMethod = MAP_LINEAR)
        : m_mappingMethod(mappingMethod)
        , m_callback(s_callbacks[mappingMethod])
    {
        assert(mappingMethod >= 0);
        assert(mappingMethod < 6);
    }

    RGBMappingType getMappingMethod() const
    { return m_mappingMethod; }

    TypeOut operator()(float sample) const
    {
        assert(sample >= 0.f);
        assert(sample <= 1.f);

        using namespace pfs::colorspace;

        return convertSample<TypeOut>(
                    m_callback(sample));
    }

private:
    RGBMappingType m_mappingMethod;
    MappingFunc m_callback;
};

class RGBRemapper
{
public:
    // ctor
    RGBRemapper(float minValue = 0.0f, float maxValue = 1.0f,
                RGBMappingType mappingMethod = MAP_LINEAR);

    ~RGBRemapper();

    // non-const functions
    void setMinMax(float min, float max);
    void setMappingMethod(RGBMappingType method);

    // const functions
    //float getMinLuminance() const;
    //float getMaxLuminance() const;
    RGBMappingType getMappingType() const { return m_MappingMethod; }

    void toQRgb(float r, float g, float b, QRgb& qrgb) const;
    void operator()(float r, float g, float b, QRgb& qrgb) const {
        this->toQRgb(r,g,b,qrgb);
    }

    void toUint8(float rI, float gI, float bI,
                 uint8_t& rO, uint8_t& gO, uint8_t& bO) const;
    void operator()(float rI, float gI, float bI,
                    uint8_t& rO, uint8_t& gO, uint8_t& bO) const {
        this->toUint8(rI, gI, bI, rO, gO, bO);
    }

    void toUint16(float r, float g, float b,
                  uint16_t& red, uint16_t& green, uint16_t& blue) const;
    void operator()(float rI, float gI, float bI,
                    uint16_t& rO, uint16_t& gO, uint16_t& bO) const {
        this->toUint16(rI, gI, bI, rO, gO, bO);
    }

    void toFloat(float rI, float gI, float bI,
                 float& rO, float& gO, float& bO) const;
    void operator()(float rI, float gI, float bI,
                    float& rO, float& gO, float& bO) const {
        this->toFloat(rI, gI, bI, rO, gO, bO);
    }

private:
    struct RgbF3 {
        RgbF3(float r, float g, float b)
            : red(r) , green(g), blue(b)
        {}

        float red;
        float green;
        float blue;
    };

    // pointer to function
    typedef RgbF3 (RGBRemapper::*MappingFunc)(float, float, float) const;

    RGBMappingType m_MappingMethod;
    MappingFunc m_MappingFunc;

    float m_MinValue;
    float m_MaxValue;
    float m_Range;
    float m_LogRange;

    RgbF3 get(float r, float g, float b) const;

    // private stuff!
    RgbF3 buildRgb(float r, float g, float b) const;
    RgbF3 mappingLinear(float r, float g, float b) const;
    RgbF3 mappingGamma14(float r, float g, float b) const;
    RgbF3 mappingGamma18(float r, float g, float b) const;
    RgbF3 mappingGamma22(float r, float g, float b) const;
    RgbF3 mappingGamma26(float r, float g, float b) const;
    RgbF3 mappingLog(float r, float g, float b) const;
};

#endif // PFS_RGBREMAPPER_H
