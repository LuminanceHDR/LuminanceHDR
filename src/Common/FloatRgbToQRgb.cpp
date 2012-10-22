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
//! 8-bits or 16-bits integer RGB (QRgb or quint16)
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! \since Luminance HDR 2.3.0-beta1

#include "FloatRgbToQRgb.h"
#include "arch/math.h"
#include "Libpfs/vex.h"
#include <algorithm>
#include <cmath>

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
const v4sf TWOPOWER16 = _mm_set1_ps(65535.f);

inline
v4sf scaleAndRound(v4sf value, v4sf MIN_, v4sf MAX_)
{
    value *= MAX_;
    value = _mm_min_ps(value, MAX_);
    value = _mm_max_ps(value, MIN_);
    value += ZERO_DOT_FIVE;

    return value;
}
#else
template<class O_>
inline
O_ scaleAndRound(float value, float MIN_, float MAX_)
{
    value *= MAX_;
    value = std::min(value, MAX_);
    value = std::max(value, MIN_);
    value += 0.5f;

    return static_cast<O_>(value);
}

// useful data structure to implement a triple of float as RGB pixel
struct RgbF3
{
    RgbF3(float r, float g, float b)
        : red(r)
        , green(g)
        , blue(b)
    {}

    float red;
    float green;
    float blue;
};

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
                            LumMappingMethod mapping_method)
#ifdef LUMINANCE_USE_SSE
        : m_MinValue(_mm_set1_ps(min_value))
        , m_MaxValue(_mm_set1_ps(max_value))
        , m_Range(_mm_set1_ps(max_value - min_value))
        , m_LogRange( _mm_set1_ps(log2f(max_value/min_value)) )
#else
        : m_MinValue(min_value)
        , m_MaxValue(max_value)
        , m_Range(max_value - min_value)
        , m_LogRange( log2f(max_value/min_value) )
#endif
    {
        setMappingMethod( mapping_method );
    }

    ~FloatRgbToQRgbImpl()
    {}

    void setMinMax(float min, float max)
    {
#ifdef LUMINANCE_USE_SSE
        m_MinValue      = _mm_set1_ps(min);
        m_MaxValue      = _mm_set1_ps(max);
        m_Range         = _mm_set1_ps(max - min);
        m_LogRange      = _mm_set1_ps(log2f(max/min));
#else
        m_MinValue      = min;
        m_MaxValue      = max;
        m_Range         = max - min;
        m_LogRange      = log2f(max/min);
#endif
    }

    void setMappingMethod(LumMappingMethod method)
    {
        m_MappingMethod = method;

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
    }

    LumMappingMethod m_MappingMethod;

#ifdef LUMINANCE_USE_SSE
    inline
    v4sf buildRgb(float r, float g, float b) {
        return (_mm_set_ps(0, b, g, r) - m_MinValue)
                / m_Range;
    }

    v4sf mappingLinear(float r, float g, float b) {
        return buildRgb(r,g,b);
    }

    v4sf mappingGamma14(float r, float g, float b) {
        return _mm_pow_ps(buildRgb(r,g,b), GAMMA_1_4);
    }

    v4sf mappingGamma18(float r, float g, float b) {
        return _mm_pow_ps(buildRgb(r,g,b), GAMMA_1_8);
    }

    v4sf mappingGamma22(float r, float g, float b) {
        return _mm_pow_ps(buildRgb(r,g,b), GAMMA_2_2);
    }

    v4sf mappingGamma26(float r, float g, float b) {
        return _mm_pow_ps(buildRgb(r,g,b), GAMMA_2_6);
    }

    v4sf mappingLog(float r, float g, float b) {
        return _mm_log2_ps(_mm_set_ps(0, b, g, r)/m_MinValue)
                / m_LogRange;
    }

    inline
    v4sf operator()(float r, float g, float b)
    {
        return (this->*m_MappingFunc)(r, g, b);
    }

    // pointer to function
    typedef v4sf (FloatRgbToQRgbImpl::*MappingFunc)(float, float, float);

    MappingFunc m_MappingFunc;

    v4sf m_MinValue;
    v4sf m_MaxValue;
    v4sf m_Range;
    v4sf m_LogRange;
#else
    // pointer to function
    typedef RgbF3 (FloatRgbToQRgbImpl::*MappingFunc)(float, float, float);

    inline
    RgbF3 buildRgb(float r, float g, float b) {

        return RgbF3((r - m_MinValue)/m_Range,
                     (g - m_MinValue)/m_Range,
                     (b - m_MinValue)/m_Range);
    }

    RgbF3 mappingLinear(float r, float g, float b)
    {
        return buildRgb(r,g,b);
    }

    RgbF3 mappingGamma14(float r, float g, float b)
    {
        RgbF3 pixel = buildRgb(r,g,b);

        pixel.red = powf(pixel.red, GAMMA_1_4);
        pixel.green = powf(pixel.green, GAMMA_1_4);
        pixel.blue = powf(pixel.blue, GAMMA_1_4);

        return pixel;
    }

    RgbF3 mappingGamma18(float r, float g, float b)
    {
        RgbF3 pixel = buildRgb(r,g,b);

        pixel.red = powf(pixel.red, GAMMA_1_8);
        pixel.green = powf(pixel.green, GAMMA_1_8);
        pixel.blue = powf(pixel.blue, GAMMA_1_8);

        return pixel;
    }

    RgbF3 mappingGamma22(float r, float g, float b)
    {
        RgbF3 pixel = buildRgb(r,g,b);

        pixel.red = powf(pixel.red, GAMMA_2_2);
        pixel.green = powf(pixel.green, GAMMA_2_2);
        pixel.blue = powf(pixel.blue, GAMMA_2_2);

        return pixel;
    }

    RgbF3 mappingGamma26(float r, float g, float b)
    {
        RgbF3 pixel = buildRgb(r,g,b);

        // I have problems with Clang++ in using the powf function
        pixel.red = powf(pixel.red, GAMMA_2_6);
        pixel.green = powf(pixel.green, GAMMA_2_6);
        pixel.blue = powf(pixel.blue, GAMMA_2_6);

        return pixel;
    }

    RgbF3 mappingLog(float r, float g, float b)
    {
        return RgbF3(log2f(r/m_MinValue)/m_LogRange,
                     log2f(g/m_MinValue)/m_LogRange,
                     log2f(b/m_MinValue)/m_LogRange);
    }

    inline
    RgbF3 operator()(float r, float g, float b)
    {
        return (this->*m_MappingFunc)(r, g, b);
    }

    MappingFunc m_MappingFunc;

    float m_MinValue;
    float m_MaxValue;
    float m_Range;
    float m_LogRange;
#endif

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

void FloatRgbToQRgb::toQRgb(float r, float g, float b, QRgb& qrgb)
{
#ifdef LUMINANCE_USE_SSE
    if (isnan(r)) r = 0.0f;
    if (isnan(g)) g = 0.0f;
    if (isnan(b)) b = 0.0f;

    v4sf rgb = (*m_Pimpl)(r,g,b);

    rgb = scaleAndRound(rgb, ZERO, TWOFIVEFIVE);

    const float* buf = reinterpret_cast<const float*>(&rgb);

    qrgb = qRgb( static_cast<int>(buf[0]),
                 static_cast<int>(buf[1]),
                 static_cast<int>(buf[2]) );
#else
    RgbF3 rgb = (*m_Pimpl)(r,g,b);

    qrgb = qRgb( scaleAndRound<int>(rgb.red, 0.f, 255.f),
                 scaleAndRound<int>(rgb.green, 0.f, 255.f),
                 scaleAndRound<int>(rgb.blue, 0.f, 255.f) );
#endif
}

void FloatRgbToQRgb::toQUint16(float r, float g, float b,
                               quint16& red, quint16& green, quint16& blue)
{
#ifdef LUMINANCE_USE_SSE
    if (isnan(r)) r = 0.0f;
    if (isnan(g)) g = 0.0f;
    if (isnan(b)) b = 0.0f;

    v4sf rgb = (*m_Pimpl)(r,g,b);

    rgb = scaleAndRound(rgb, ZERO, TWOPOWER16);

    const float* buf = reinterpret_cast<const float*>(&rgb);

    red = static_cast<quint16>(buf[0]);
    green = static_cast<quint16>(buf[1]);
    blue = static_cast<quint16>(buf[2]);
#else
    RgbF3 rgb = (*m_Pimpl)(r,g,b);

    red = scaleAndRound<quint16>(rgb.red, 0.f, 65535.f);
    green = scaleAndRound<quint16>(rgb.green, 0.f, 65535.f);
    blue = scaleAndRound<quint16>(rgb.blue, 0.f, 65535.f);
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

//float FloatRgbToQRgb::getMinLuminance() const
//{
//    return m_Pimpl->m_MinValue;
//}

//float FloatRgbToQRgb::getMaxLuminance() const
//{
//    return m_Pimpl->m_MaxValue;
//}
