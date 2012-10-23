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

TEST(TestSRGB2Y, TestSRGB2Y)
{
    ColorSpaceSamples redInput;
    ColorSpaceSamples greenInput;
    ColorSpaceSamples blueInput;

    ColorSpaceSamples yOutput;

    redInput    += 0.f, 1.f, 0.f, 0.f, 0.2f, 1.f;
    greenInput  += 0.f, 0.f, 1.f, 0.f, 0.3f, 1.f;
    blueInput   += 0.f, 0.f, 0.f, 1.f, 0.4f, 1.f;
    yOutput     += 0.f, 0.212673f, 0.715152f, 0.072175f, 0.069007f, 1.f;

    ASSERT_TRUE( static_cast<bool>(yOutput.size()) );
    ASSERT_EQ( yOutput.size(), blueInput.size() );
    ASSERT_EQ( redInput.size(), blueInput.size() );
    ASSERT_EQ( greenInput.size(), blueInput.size() );

    ColorSpaceSamples yTemp(yOutput.size());

    pfs::Array2D A2DRed(yOutput.size(), 1, redInput.data());
    pfs::Array2D A2DGreen(yOutput.size(), 1, greenInput.data());
    pfs::Array2D A2DBlue(yOutput.size(), 1, blueInput.data());
    pfs::Array2D A2DY(yOutput.size(), 1, yTemp.data());

    // function under unit test!
    pfs::transformSRGB2Y( &A2DRed,
                          &A2DGreen,
                          &A2DBlue,
                          &A2DY );

    for (size_t idx = 0; idx < yTemp.size(); ++idx)
    {
        EXPECT_NEAR(yOutput[idx], yTemp[idx], 10e-6);
    }
}
