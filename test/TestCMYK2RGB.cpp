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
#include <Libpfs/utils/transform.h>
#include <Libpfs/colorspace/cmyk.h>
#include <Libpfs/colorspace/colorspace.h>

#include "PrintArray2D.h"

typedef std::vector<float> Samples32f;
typedef std::vector<uint8_t> SamplesUint8;

using namespace std;
using namespace boost::assign;  // bring 'operator+=()' into scope

TEST(TestCMYK2RGB, Test_32f_Uint8)
{
    Samples32f cInput;
    Samples32f mInput;
    Samples32f yInput;
    Samples32f kInput;
    SamplesUint8 rOutput;
    SamplesUint8 gOutput;
    SamplesUint8 bOutput;

    cInput      += 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 0.f;
    mInput      += 0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f, 1.f;
    yInput      += 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 0.f, 0.f;
    kInput      += 1.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f;
    rOutput     += 0,   255,    255,    0,      0,  255,   0,  255;
    gOutput     += 0,   255,    0,      255,    0,  255, 255,   0;
    bOutput     += 0,   255,    0,      0,      255,    0, 255, 255;

    ASSERT_EQ( cInput.size(), mInput.size() );
    ASSERT_EQ( mInput.size(), yInput.size() );
    ASSERT_EQ( yInput.size(), kInput.size() );
    ASSERT_EQ( kInput.size(), rOutput.size() );
    ASSERT_EQ( rOutput.size(), gOutput.size() );
    ASSERT_EQ( gOutput.size(), bOutput.size() );

    SamplesUint8 A2DRed(rOutput.size(), 0);
    SamplesUint8 A2DGreen(gOutput.size(), 0);
    SamplesUint8 A2DBlue(bOutput.size(), 0);

    // under test!
    pfs::utils::transform(cInput.begin(), cInput.end(), mInput.begin(), yInput.begin(), kInput.begin(),
                          A2DRed.begin(), A2DGreen.begin(), A2DBlue.begin(),
                          pfs::colorspace::ConvertCMYK2RGB());

//    print(A2DX);
//    print(A2DY);
//    print(A2DZ);

    for (size_t idx = 0; idx < A2DRed.size(); ++idx)
    {
        EXPECT_NEAR(rOutput[idx], A2DRed[idx], 10e-6);
        EXPECT_NEAR(gOutput[idx], A2DGreen[idx], 10e-6);
        EXPECT_NEAR(bOutput[idx], A2DBlue[idx], 10e-6);
    }
}

TEST(TestCMYK2RGB, Test_Uint8_Uint8)
{
    SamplesUint8 cInput;
    SamplesUint8 mInput;
    SamplesUint8 yInput;
    SamplesUint8 kInput;
    SamplesUint8 rOutput;
    SamplesUint8 gOutput;
    SamplesUint8 bOutput;

    cInput      += 0, 0, 0, 255, 255, 0, 255, 0;
    mInput      += 0, 0, 255, 0, 255, 0, 0, 255;
    yInput      += 0, 0, 255, 255, 0, 255, 0, 0;
    kInput      += 255, 0, 0, 0, 0, 0, 0, 0;
    rOutput     += 0,   255,    255,    0,      0,  255,   0,  255;
    gOutput     += 0,   255,    0,      255,    0,  255, 255,   0;
    bOutput     += 0,   255,    0,      0,      255,    0, 255, 255;

    ASSERT_EQ( cInput.size(), mInput.size() );
    ASSERT_EQ( mInput.size(), yInput.size() );
    ASSERT_EQ( yInput.size(), kInput.size() );
    ASSERT_EQ( kInput.size(), rOutput.size() );
    ASSERT_EQ( rOutput.size(), gOutput.size() );
    ASSERT_EQ( gOutput.size(), bOutput.size() );

    SamplesUint8 A2DRed(rOutput.size(), 0);
    SamplesUint8 A2DGreen(gOutput.size(), 0);
    SamplesUint8 A2DBlue(bOutput.size(), 0);

    // under test!
    pfs::utils::transform(cInput.begin(), cInput.end(), mInput.begin(), yInput.begin(), kInput.begin(),
                          A2DRed.begin(), A2DGreen.begin(), A2DBlue.begin(),
                          pfs::colorspace::ConvertCMYK2RGB());

//    print(A2DX);
//    print(A2DY);
//    print(A2DZ);

    for (size_t idx = 0; idx < A2DRed.size(); ++idx)
    {
        EXPECT_NEAR(rOutput[idx], A2DRed[idx], 10e-6);
        EXPECT_NEAR(gOutput[idx], A2DGreen[idx], 10e-6);
        EXPECT_NEAR(bOutput[idx], A2DBlue[idx], 10e-6);
    }
}

TEST(TestCMYK2RGB, Test_32f_32f)
{
    Samples32f cInput;
    Samples32f mInput;
    Samples32f yInput;
    Samples32f kInput;
    Samples32f rOutput;
    Samples32f gOutput;
    Samples32f bOutput;

    cInput      += 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 0.f;
    mInput      += 0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f, 1.f;
    yInput      += 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 0.f, 0.f;
    kInput      += 1.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f;
    rOutput     += 0.f, 1.f, 1.f, 0.f, 0.f, 1.f, 0.f, 1.f;
    gOutput     += 0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 1.f, 0.f;
    bOutput     += 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 1.f;

    ASSERT_EQ( cInput.size(), mInput.size() );
    ASSERT_EQ( mInput.size(), yInput.size() );
    ASSERT_EQ( yInput.size(), kInput.size() );
    ASSERT_EQ( kInput.size(), rOutput.size() );
    ASSERT_EQ( rOutput.size(), gOutput.size() );
    ASSERT_EQ( gOutput.size(), bOutput.size() );

    Samples32f A2DRed(rOutput.size(), 0);
    Samples32f A2DGreen(gOutput.size(), 0);
    Samples32f A2DBlue(bOutput.size(), 0);

    // under test!
    pfs::utils::transform(cInput.begin(), cInput.end(), mInput.begin(), yInput.begin(), kInput.begin(),
                          A2DRed.begin(), A2DGreen.begin(), A2DBlue.begin(),
                          pfs::colorspace::ConvertCMYK2RGB());

//    print(A2DX);
//    print(A2DY);
//    print(A2DZ);

    for (size_t idx = 0; idx < A2DRed.size(); ++idx)
    {
        EXPECT_NEAR(rOutput[idx], A2DRed[idx], 10e-6);
        EXPECT_NEAR(gOutput[idx], A2DGreen[idx], 10e-6);
        EXPECT_NEAR(bOutput[idx], A2DBlue[idx], 10e-6);
    }
}

TEST(TestCMYK2RGB, Test_Uint8_32f)
{
    SamplesUint8 cInput;
    SamplesUint8 mInput;
    SamplesUint8 yInput;
    SamplesUint8 kInput;
    Samples32f rOutput;
    Samples32f gOutput;
    Samples32f bOutput;

    cInput      += 0, 0, 0, 255, 255, 0, 255, 0;
    mInput      += 0, 0, 255, 0, 255, 0, 0, 255;
    yInput      += 0, 0, 255, 255, 0, 255, 0, 0;
    kInput      += 255, 0, 0, 0, 0, 0, 0, 0;
    rOutput     += 0.f, 1.f, 1.f, 0.f, 0.f, 1.f, 0.f, 1.f;
    gOutput     += 0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 1.f, 0.f;
    bOutput     += 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 1.f;

    ASSERT_EQ( cInput.size(), mInput.size() );
    ASSERT_EQ( mInput.size(), yInput.size() );
    ASSERT_EQ( yInput.size(), kInput.size() );
    ASSERT_EQ( kInput.size(), rOutput.size() );
    ASSERT_EQ( rOutput.size(), gOutput.size() );
    ASSERT_EQ( gOutput.size(), bOutput.size() );

    Samples32f A2DRed(rOutput.size(), 0);
    Samples32f A2DGreen(gOutput.size(), 0);
    Samples32f A2DBlue(bOutput.size(), 0);

    // under test!
    pfs::utils::transform(cInput.begin(), cInput.end(), mInput.begin(), yInput.begin(), kInput.begin(),
                          A2DRed.begin(), A2DGreen.begin(), A2DBlue.begin(),
                          pfs::colorspace::ConvertCMYK2RGB());

//    print(A2DX);
//    print(A2DY);
//    print(A2DZ);

    for (size_t idx = 0; idx < A2DRed.size(); ++idx)
    {
        EXPECT_NEAR(rOutput[idx], A2DRed[idx], 10e-6);
        EXPECT_NEAR(gOutput[idx], A2DGreen[idx], 10e-6);
        EXPECT_NEAR(bOutput[idx], A2DBlue[idx], 10e-6);
    }
}
