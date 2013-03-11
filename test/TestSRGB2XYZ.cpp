/*
 * This file is a part of Luminance HDR package
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

#include <gtest/gtest.h>
#include <boost/assign/std/vector.hpp> // for 'operator+=()'
#include <boost/assert.hpp>

#include <Libpfs/array2d.h>
#include <Libpfs/colorspace/colorspace.h>

#include "PrintArray2D.h"

typedef std::vector<float> ColorSpaceSamples;

using namespace std;
using namespace boost::assign;  // bring 'operator+=()' into scope

TEST(TestSRGB2Y, TestSRGB2XYZ)
{
    ColorSpaceSamples redInput;
    ColorSpaceSamples greenInput;
    ColorSpaceSamples blueInput;
    ColorSpaceSamples xOutput;
    ColorSpaceSamples yOutput;
    ColorSpaceSamples zOutput;

    redInput    += 0.f, 1.f,        0.f,        0.f,        0.2f,       1.f,        1.2f,       1.2f,       2.f,        -0.2f;
    greenInput  += 0.f, 0.f,        1.f,        0.f,        0.3f,       1.f,        0.f,        0.0f,       1.2f,       -0.9f;
    blueInput   += 0.f, 0.f,        0.f,        1.f,        0.4f,       1.f,        0.f,        1.4f,       2.f,        0.3f;
    xOutput     += 0.f, 0.412456f,  0.357576f,  0.180437f,  0.063817f,  0.950470f,  0.625629f,  1.015924f,  3.479490f,  -0.281999f;
    yOutput     += 0.f, 0.212673f,  0.715152f,  0.072175f,  0.069007f,  1.f,        0.322590f,  0.478708f,  2.495864f,  -0.564874f;
    zOutput     += 0.f, 0.019334f,  0.119192f,  0.950304f,  0.135635f,  1.088830f,  0.029326f,  2.084876f,  4.984232f,  -0.024894f;

    ASSERT_TRUE( static_cast<bool>(yOutput.size()) );
    ASSERT_EQ( zOutput.size(), xOutput.size() );
    ASSERT_EQ( xOutput.size(), yOutput.size() );
    ASSERT_EQ( yOutput.size(), blueInput.size() );
    ASSERT_EQ( redInput.size(), blueInput.size() );
    ASSERT_EQ( greenInput.size(), blueInput.size() );

    pfs::Array2Df A2DRed(redInput.size(), 1);
    pfs::Array2Df A2DGreen(greenInput.size(), 1);
    pfs::Array2Df A2DBlue(blueInput.size(), 1);

    std::copy(redInput.begin(), redInput.end(), A2DRed.begin());
    std::copy(greenInput.begin(), greenInput.end(), A2DGreen.begin());
    std::copy(blueInput.begin(), blueInput.end(), A2DBlue.begin());

    pfs::Array2Df A2DX(xOutput.size(), 1);
    pfs::Array2Df A2DY(yOutput.size(), 1);
    pfs::Array2Df A2DZ(zOutput.size(), 1);

    // function under unit test!
    pfs::transformSRGB2XYZ(&A2DRed, &A2DGreen, &A2DBlue, &A2DX, &A2DY, &A2DZ);

    print(A2DX);
    print(xOutput);
    print(A2DY);
    print(yOutput);
    print(A2DZ);
    print(zOutput);

    for (size_t idx = 0; idx < A2DY.size(); ++idx)
    {
        EXPECT_NEAR(xOutput[idx], A2DX(idx), 10e-6);
        EXPECT_NEAR(yOutput[idx], A2DY(idx), 10e-6);
        EXPECT_NEAR(zOutput[idx], A2DZ(idx), 10e-6);
    }
}

TEST(TestSRGB2Y, TestSRGB2Y)
{
    ColorSpaceSamples redInput;
    ColorSpaceSamples greenInput;
    ColorSpaceSamples blueInput;
    ColorSpaceSamples yOutput;

    redInput    += 0.f, 1.f, 0.f, 0.f, 0.2f, 1.f, 1.2f, 1.2f, 2.f;
    greenInput  += 0.f, 0.f, 1.f, 0.f, 0.3f, 1.f, 0.f, 0.0f, 1.2f;
    blueInput   += 0.f, 0.f, 0.f, 1.f, 0.4f, 1.f, 0.f, 1.4f, 2.f;
    yOutput     += 0.f, 0.212673f, 0.715152f, 0.072175f, 0.069007f, 1.f, 0.322590f, 0.478708f, 2.495864;

    ASSERT_TRUE( static_cast<bool>(yOutput.size()) );
    ASSERT_EQ( yOutput.size(), blueInput.size() );
    ASSERT_EQ( redInput.size(), blueInput.size() );
    ASSERT_EQ( greenInput.size(), blueInput.size() );

    pfs::Array2Df A2DRed(redInput.size(), 1);
    pfs::Array2Df A2DGreen(greenInput.size(), 1);
    pfs::Array2Df A2DBlue(blueInput.size(), 1);
    pfs::Array2Df A2DY(yOutput.size(), 1);

    std::copy(redInput.begin(), redInput.end(), A2DRed.begin());
    std::copy(greenInput.begin(), greenInput.end(), A2DGreen.begin());
    std::copy(blueInput.begin(), blueInput.end(), A2DBlue.begin());

    // function under unit test!
    pfs::transformSRGB2Y(&A2DRed, &A2DGreen, &A2DBlue,
                         &A2DY);

    for (size_t idx = 0; idx < A2DY.size(); ++idx)
    {
        EXPECT_NEAR(yOutput[idx], A2DY(idx), 10e-6);
    }
}
