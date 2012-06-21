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

//! \file FloatRgbToQRgb.cpp
//! \brief This file creates common routines for mapping float RGB values into
//! 255 levels QImage
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! \since Luminance HDR 2.3.0-beta1

#include "FloatRgbToQRgb.h"
#include "Libpfs/vex.h"

namespace
{
#ifdef LUMINANCE_USE_SSE
const v4sf GAMMA_1_4 = _mm_set1_ps(1.0f/1.4f);
const v4sf GAMMA_1_8 = _mm_set1_ps(1.0f/1.8f);
const v4sf GAMMA_2_2 = _mm_set1_ps(1.0f/2.2f);
const v4sf GAMMA_2_6 = _mm_set1_ps(1.0f/2.6f);

const v4sf ZERO = _mm_set1_ps(0.f);
const v4sf ZERO_DOT_FIVE = _mm_set1_ps(0.5f);
const v4sf TWOFIVEFIVE = _mm_set1_ps(255.f);
#else
template<class T>
inline T clamp( T val, T min, T max )
{
    if ( val < min ) return min;
    if ( val > max ) return max;
    return val;
}

const float GAMMA_1_4 = 1.0f/1.4f;
const float GAMMA_1_8 = 1.0f/1.8f;
const float GAMMA_2_2 = 1.0f/2.2f;
const float GAMMA_2_6 = 1.0f/2.6f;
#endif
}

struct FloatRgbToQRgbImpl
{
    FloatRgbToQRgbImpl(float min_value,
                            float max_value,
                            LumMappingMethod mapping_method):
            //m_MappingMethod(mapping_method),
            m_MinValue(min_value),
            m_MaxValue(max_value),
            m_Range(max_value - min_value),
            m_LogRange( log2f(max_value/min_value) )
    {
        setMappingMethod( mapping_method );
    }

    ~FloatRgbToQRgbImpl()
    {}

    void setMinMax(float min, float max)
    {
        m_MinValue      = min;
        m_MaxValue      = max;
        m_Range         = max - min;
        m_LogRange      = log2f(max/min);
    }

#ifdef LUMINANCE_USE_SSE
    inline
    v4sf buildRgbSse(float r, float g, float b) {
        return (_mm_set_ps(0, b, g, r) - _mm_set1_ps(m_MinValue))
                / _mm_set1_ps(m_Range);
    }

    v4sf mappingLinear(float r, float g, float b) {
        return buildRgbSse(r,g,b);
    }

    v4sf mappingGamma14(float r, float g, float b) {
        return _mm_pow_ps(buildRgbSse(r,g,b), GAMMA_1_4);
    }

    v4sf mappingGamma18(float r, float g, float b) {
        return _mm_pow_ps(buildRgbSse(r,g,b), GAMMA_1_8);
    }

    v4sf mappingGamma22(float r, float g, float b) {
        return _mm_pow_ps(buildRgbSse(r,g,b), GAMMA_2_2);
    }

    v4sf mappingGamma26(float r, float g, float b) {
        return _mm_pow_ps(buildRgbSse(r,g,b), GAMMA_2_6);
    }

    v4sf mappingLog(float r, float g, float b) {
        return _mm_log2_ps(_mm_set_ps(0, b, g, r)/_mm_set1_ps(m_MinValue))
                / _mm_set1_ps(m_LogRange);
    }

    inline
    v4sf operator()(float r, float g, float b)
    {
        return (this->*m_MappingFunc)(r, g, b);
    }

    // pointer to function
    typedef v4sf (FloatRgbToQRgbImpl::*MappingFunc)(float, float, float);

    MappingFunc m_MappingFunc;
#endif

    void setMappingMethod(LumMappingMethod method)
    {
        m_MappingMethod = method;

#ifdef LUMINANCE_USE_SSE
        switch ( m_MappingMethod )
        {
        case MAP_LINEAR:
            m_MappingFunc = &FloatRgbToQRgbImpl::mappingLinear;
            break;
        case MAP_GAMMA1_4:
            m_MappingFunc = &FloatRgbToQRgbImpl::mappingGamma14;
            break;
        case MAP_GAMMA1_8:
            m_MappingFunc = &FloatRgbToQRgbImpl::mappingGamma18;
            break;
        case MAP_GAMMA2_6:
            m_MappingFunc = &FloatRgbToQRgbImpl::mappingGamma26;
            break;
        case MAP_LOGARITHMIC:
            m_MappingFunc = &FloatRgbToQRgbImpl::mappingLog;
            break;
        default:
        case MAP_GAMMA2_2:
            m_MappingFunc = &FloatRgbToQRgbImpl::mappingGamma22;
            break;
        }
#endif
    }

    LumMappingMethod m_MappingMethod;

    float m_MinValue;
    float m_MaxValue;
    float m_Range;
    float m_LogRange;
};

FloatRgbToQRgb::FloatRgbToQRgb(float min_value,
                               float max_value,
                               LumMappingMethod mapping_method)
    : m_Pimpl( new FloatRgbToQRgbImpl(min_value,
                                      max_value,
                                      mapping_method))
{}

FloatRgbToQRgb::~FloatRgbToQRgb()
{}

QRgb FloatRgbToQRgb::operator ()(float r, float g, float b)
{
#ifdef LUMINANCE_USE_SSE
    v4sf rgb = (*m_Pimpl)(r,g,b);

    rgb *= TWOFIVEFIVE;
    rgb = _mm_min_ps(rgb, TWOFIVEFIVE);
    rgb = _mm_max_ps(rgb, ZERO);
    rgb += ZERO_DOT_FIVE;
    float buf[4];
    _mm_store_ps(buf, rgb);
    return qRgb( static_cast<int>(buf[0]),
                 static_cast<int>(buf[1]),
                 static_cast<int>(buf[2]) );
#else
    float rgb[3] = {
        (r - m_Pimpl->m_MinValue)/m_Pimpl->m_Range,
        (g - m_Pimpl->m_MinValue)/m_Pimpl->m_Range,
        (b - m_Pimpl->m_MinValue)/m_Pimpl->m_Range
    };

    switch ( m_Pimpl->m_MappingMethod )
    {
    case MAP_GAMMA1_4:
        rgb[0] = powf(rgb[0], GAMMA_1_4);
        rgb[1] = powf(rgb[1], GAMMA_1_4);
        rgb[2] = powf(rgb[2], GAMMA_1_4);
        break;
    case MAP_GAMMA1_8:
        rgb[0] = powf(rgb[0], GAMMA_1_8);
        rgb[1] = powf(rgb[1], GAMMA_1_8);
        rgb[2] = powf(rgb[2], GAMMA_1_8);
        break;
    case MAP_GAMMA2_2:
        rgb[0] = powf(rgb[0], GAMMA_2_2);
        rgb[1] = powf(rgb[1], GAMMA_2_2);
        rgb[2] = powf(rgb[2], GAMMA_2_2);
        break;
    case MAP_GAMMA2_6:
        rgb[0] = powf(rgb[0], GAMMA_2_6);
        rgb[1] = powf(rgb[1], GAMMA_2_6);
        rgb[2] = powf(rgb[2], GAMMA_2_6);
        break;
    case MAP_LOGARITHMIC:
        // log(x) - log(y) = log(x/y)
        rgb[0] = log2f(r/m_Pimpl->m_MinValue)/m_Pimpl->m_LogRange;
        rgb[1] = log2f(g/m_Pimpl->m_MinValue)/m_Pimpl->m_LogRange;
        rgb[2] = log2f(b/m_Pimpl->m_MinValue)/m_Pimpl->m_LogRange;
        break;
    default:
    case MAP_LINEAR:
        // do nothing
        // final value is already calculated
        break;
    }
    return qRgb( clamp(rgb[0]*255.f, 0.0f, 255.f) + 0.5f,
                  clamp(rgb[1]*255.f, 0.0f, 255.f) + 0.5f,
                  clamp(rgb[2]*255.f, 0.0f, 255.f) + 0.5f );
#endif
}

void FloatRgbToQRgb::setMinMax(float min, float max)
{
    m_Pimpl->setMinMax(min, max);
}

void FloatRgbToQRgb::setMappingMethod(LumMappingMethod method)
{
    m_Pimpl->setMappingMethod( method );
}

LumMappingMethod FloatRgbToQRgb::getMappingMethod() const
{
    return m_Pimpl->m_MappingMethod;
}

float FloatRgbToQRgb::getMinLuminance() const
{
    return m_Pimpl->m_MinValue;
}

float FloatRgbToQRgb::getMaxLuminance() const
{
    return m_Pimpl->m_MaxValue;
}
