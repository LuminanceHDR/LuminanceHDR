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

#ifndef PFSFRAME_TO_QIMAGE_MAPPING_H
#define PFSFRAME_TO_QIMAGE_MAPPING_H

//! \file FloatRgbToQRgb.h
//! \brief This file creates common routines for mapping float RGB values into
//! 255 levels QImage
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! \since Luminance HDR 2.3.0-beta1

#include <cmath>
#include <QRgb>
#include <QScopedPointer>

// Are you changing the order?
// Feel free, but it's a fast track for troubles!
enum LumMappingMethod
{
    MAP_LINEAR = 0,
    MAP_GAMMA1_4 = 1,
    MAP_GAMMA1_8 = 2,
    MAP_GAMMA2_2 = 3,
    MAP_GAMMA2_6 = 4,
    MAP_LOGARITHMIC = 5
};

//! \brief Private implementation for the class \c FloatRgbToQRgb
struct FloatRgbToQRgbImpl;

class FloatRgbToQRgb
{
public:
    // ctor
    FloatRgbToQRgb(float min_value = 1.0f,
                   float max_value = 1.0f,
                   LumMappingMethod mapping_method = MAP_LINEAR);

    ~FloatRgbToQRgb();

    // non-const functions
    void setMinMax(float min, float max);
    void setMappingMethod(LumMappingMethod method);

    // const functions
    float getMinLuminance() const;
    float getMaxLuminance() const;
    LumMappingMethod getMappingMethod() const;

    QRgb operator()(float r, float g, float b);

private:
    // private implementation, useful to get a more performant implementation
    QScopedPointer<FloatRgbToQRgbImpl> m_Pimpl;
};

#endif // PFSFRAME_TO_QIMAGE_MAPPING_H
