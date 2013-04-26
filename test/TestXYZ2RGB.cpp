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

TEST(TestXYZ2RGB, TestXYZ2RGB)
{
    ColorSpaceSamples redOutput;
    ColorSpaceSamples greenOutput;
    ColorSpaceSamples blueOutput;
    ColorSpaceSamples xInput;
    ColorSpaceSamples yInput;
    ColorSpaceSamples zInput;

    xInput      += 0.f, 1.0f,       0.412456f,  0.357576f,  0.180437f,  0.261939f,  0.950470f,  0.494948f,  0.747560f,  1.614879f;
    yInput      += 0.f, 0.0f,       0.212673f,  0.715152f,  0.072175f,  0.285950f,  1.f,        0.255207f,  0.356252f,  1.427878f;
    zInput      += 0.f, 0.0f,       0.019334f,  0.119192f,  0.950304f,  0.419746f,  1.088830f,  0.023201f,  1.353626f,  2.082306f;
    redOutput   += 0.f, 3.240454f,  1.f,        0.f,        0.f,        0.2f,       1.f,        1.2f,       1.2f,       2.f;
    greenOutput += 0.f, -0.969266f, 0.f,        1.f,        0.f,        0.3f,       1.f,        0.f,        0.0f,       1.2f;
    blueOutput  += 0.f, 0.055643f,  0.f,        0.f,        1.f,        0.4f,       1.f,        0.f,        1.4f,       2.f;

    ASSERT_EQ( zInput.size(), xInput.size() );
    ASSERT_EQ( xInput.size(), yInput.size() );
    ASSERT_EQ( yInput.size(), blueOutput.size() );
    ASSERT_EQ( redOutput.size(), blueOutput.size() );
    ASSERT_EQ( greenOutput.size(), blueOutput.size() );

    pfs::Array2Df A2DX(xInput.size(), 1);
    pfs::Array2Df A2DY(yInput.size(), 1);
    pfs::Array2Df A2DZ(zInput.size(), 1);

    std::copy(xInput.begin(), xInput.end(), A2DX.begin());
    std::copy(yInput.begin(), yInput.end(), A2DY.begin());
    std::copy(zInput.begin(), zInput.end(), A2DZ.begin());

    pfs::Array2Df A2DRed(redOutput.size(), 1);
    pfs::Array2Df A2DGreen(greenOutput.size(), 1);
    pfs::Array2Df A2DBlue(blueOutput.size(), 1);

    // function under unit test!
    pfs::transformXYZ2RGB(&A2DX, &A2DY, &A2DZ, &A2DRed, &A2DGreen, &A2DBlue);

//    print(A2DX);
//    print(A2DY);
//    print(A2DZ);

    for (size_t idx = 0; idx < A2DY.size(); ++idx)
    {
        EXPECT_NEAR(redOutput[idx], A2DRed(idx), 10e-6);
        EXPECT_NEAR(greenOutput[idx], A2DGreen(idx), 10e-6);
        EXPECT_NEAR(blueOutput[idx], A2DBlue(idx), 10e-6);
    }
}

TEST(TestXYZ2SRGB, TestXYZ2SRGB)
{
    ColorSpaceSamples redOutput;
    ColorSpaceSamples greenOutput;
    ColorSpaceSamples blueOutput;
    ColorSpaceSamples xInput;
    ColorSpaceSamples yInput;
    ColorSpaceSamples zInput;

    xInput      += 0.f, 1.0f,       0.412456f,  0.357576f,  0.180437f,  0.261939f,  0.950470f,  0.494948f,  0.747560f,  1.614879f;
    yInput      += 0.f, 0.0f,       0.212673f,  0.715152f,  0.072175f,  0.285950f,  1.f,        0.255207f,  0.356252f,  1.427878f;
    zInput      += 0.f, 0.0f,       0.019334f,  0.119192f,  0.950304f,  0.419746f,  1.088830f,  0.023201f,  1.353626f,  2.082306f;
    redOutput   += 0.f, 1.666888f,  0.999999f,  0.f,        -0.000020f, 0.484529f,  1.f,        1.083269f,  1.083268f,  1.353256f;
    greenOutput += 0.f, -0.986367f, 0.000009f,  1.f,        0.000006f,  0.583831f,  1.f,        -0.000013f, -0.000008f, 1.083268f;
    blueOutput  += 0.f, 0.261598f,  0.000001f,  0.f,        1.f,        0.665185f,  1.f,        0.000006f,  1.158778f,  1.353256f;

    ASSERT_EQ( zInput.size(), xInput.size() );
    ASSERT_EQ( xInput.size(), yInput.size() );
    ASSERT_EQ( yInput.size(), blueOutput.size() );
    ASSERT_EQ( redOutput.size(), blueOutput.size() );
    ASSERT_EQ( greenOutput.size(), blueOutput.size() );

    pfs::Array2Df A2DX(xInput.size(), 1);
    pfs::Array2Df A2DY(yInput.size(), 1);
    pfs::Array2Df A2DZ(zInput.size(), 1);

    std::copy(xInput.begin(), xInput.end(), A2DX.begin());
    std::copy(yInput.begin(), yInput.end(), A2DY.begin());
    std::copy(zInput.begin(), zInput.end(), A2DZ.begin());

    pfs::Array2Df A2DRed(redOutput.size(), 1);
    pfs::Array2Df A2DGreen(greenOutput.size(), 1);
    pfs::Array2Df A2DBlue(blueOutput.size(), 1);

    // function under unit test!
    pfs::transformXYZ2SRGB(&A2DX, &A2DY, &A2DZ, &A2DRed, &A2DGreen, &A2DBlue);

    print(A2DRed);
    print(redOutput);
    print(A2DGreen);
    print(greenOutput);
    print(A2DBlue);
    print(blueOutput);

    for (size_t idx = 0; idx < A2DY.size(); ++idx)
    {
        EXPECT_NEAR(redOutput[idx], A2DRed(idx), 10e-2);
        EXPECT_NEAR(greenOutput[idx], A2DGreen(idx), 10e-2);
        EXPECT_NEAR(blueOutput[idx], A2DBlue(idx), 10e-2);
    }
}
