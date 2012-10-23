/*
 * This file is a part of Luminance HDR package
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

#include <gtest/gtest.h>
#include <boost/assign/std/vector.hpp> // for 'operator+=()'
#include <boost/assert.hpp>

#include <Libpfs/colorspace.h>

typedef std::vector<float> ColorSpaceSamples;

using namespace std;
using namespace boost::assign;  // bring 'operator+=()' into scope

inline
float computeLuminance(float red, float green, float blue)
{
    return (red*0.2126729f + green*0.7151522f + blue*0.0721750f);
}


TEST(TestSRGB2Y, TestSRGB2Y)
{
    ColorSpaceSamples redInput;
    ColorSpaceSamples greenInput;
    ColorSpaceSamples blueInput;

    redInput    += 0.f, 1.f, 0.f, 0.f, 0.2f, 1.f;
    greenInput  += 0.f, 0.f, 1.f, 0.f, 0.3f, 1.f;
    blueInput   += 0.f, 0.f, 0.f, 1.f, 0.4f, 1.f;

    ASSERT_EQ( redInput.size(), blueInput.size() );
    ASSERT_EQ( greenInput.size(), blueInput.size() );

    ColorSpaceSamples yTemp(redInput.size());

    pfs::Array2D A2DRed(redInput.size(), 1, redInput.data());
    pfs::Array2D A2DGreen(redInput.size(), 1, greenInput.data());
    pfs::Array2D A2DBlue(redInput.size(), 1, blueInput.data());
    pfs::Array2D A2DY(redInput.size(), 1, yTemp.data());

    // function under unit test!
    pfs::transformRGB2Y( &A2DRed,
                         &A2DGreen,
                         &A2DBlue,
                         &A2DY );

    for (size_t idx = 0; idx < yTemp.size(); ++idx)
    {
        EXPECT_NEAR(yTemp[idx],
                    computeLuminance(redInput[idx],
                                     greenInput[idx],
                                     blueInput[idx]),
                    10e-6);
    }
}
