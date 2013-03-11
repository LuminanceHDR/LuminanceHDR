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

    void toUint8(float rI, float gI, float bI,
                 uint8_t& rO, uint8_t& gO, uint8_t& bO) const;
    void toUint16(float r, float g, float b,
                   quint16& red, quint16& green, quint16& blue) const;
    void toFloat(float rI, float gI, float bI,
                 float& rO, float& gO, float& bO) const;

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
