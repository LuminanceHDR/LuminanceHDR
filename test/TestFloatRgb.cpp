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
#include <QRgb>
#include <QDebug>

#include <Libpfs/colorspace/rgbremapper.h>
#include <Libpfs/colorspace/copy.h>
#include <Libpfs/utils/clamp.h>
#include <Libpfs/utils/chain.h>

using namespace pfs::colorspace;
using namespace pfs::utils;

typedef Chain<
    Clamp<float>,
    Remapper<uint16_t>
> MyRemapperUint16;

typedef Chain<
    Clamp<float>,
    Remapper<uint8_t>
> MyRemapperUint8;

TEST(FloatRgbConverter, Uint16_Test1)
{
    MyRemapperUint16 d(Clamp<float>(0.f, 1.f), Remapper<uint16_t>(MAP_LINEAR));

    float inRed = 1.0f;
    float inGreen = 0.0f;
    float inBlue = 0.0f;

    uint16_t outRed;
    uint16_t outGreen;
    uint16_t outBlue;

    d(inRed, inGreen, inBlue, outRed, outGreen, outBlue);

    EXPECT_EQ(outRed, 65535u);
    EXPECT_EQ(outGreen, 0u);
    EXPECT_EQ(outBlue, 0u);
}

TEST(FloatRgbConverter, Uint16_Test2)
{
    MyRemapperUint16 d(Clamp<float>(0.f, 1.f), Remapper<uint16_t>(MAP_LINEAR));

    float inRed = 1.2f;
    float inGreen = 0.0f;
    float inBlue = 0.0f;

    uint16_t outRed;
    uint16_t outGreen;
    uint16_t outBlue;

    d(inRed, inGreen, inBlue, outRed, outGreen, outBlue);

    EXPECT_EQ(static_cast<int>(outRed), 65535);
    EXPECT_EQ(static_cast<int>(outGreen), 0);
    EXPECT_EQ(static_cast<int>(outBlue), 0);
}

TEST(FloatRgbConverter, Uint16_Test3)
{
    MyRemapperUint16 d(Clamp<float>(0.f, 1.f), Remapper<uint16_t>(MAP_LINEAR));

    float inRed = 1.2f;
    float inGreen = -0.0f;
    float inBlue = 0.0f;

    uint16_t outRed;
    uint16_t outGreen;
    uint16_t outBlue;

    d(inRed, inGreen, inBlue, outRed, outGreen, outBlue);

    EXPECT_EQ(static_cast<int>(outRed), 65535);
    EXPECT_EQ(static_cast<int>(outGreen), 0);
    EXPECT_EQ(static_cast<int>(outBlue), 0);
}

TEST(FloatRgbConverter, Qrgb_Test1)
{
    RGBRemapper d;

    float inRed = 1.2f;
    float inGreen = -0.0f;
    float inBlue = 0.5f;

    QRgb rgb;

    d.toQRgb(inRed, inGreen, inBlue, rgb);

    EXPECT_EQ(qRed(rgb), 255);
    EXPECT_EQ(qGreen(rgb), 0);
    EXPECT_EQ(qBlue(rgb), 128);
}

TEST(FloatRgbConverter, Qrgb_Test2)
{
    RGBRemapper d;

    float inRed = 1.2f;
    float inGreen = 0.5f;
    float inBlue = -0.5f;

    QRgb rgb;

    d.toQRgb(inRed, inGreen, inBlue, rgb);

    EXPECT_EQ(qRed(rgb), 255);
    EXPECT_EQ(qGreen(rgb), 128);
    EXPECT_EQ(qBlue(rgb), 0);
}

TEST(FloatRgbConverter, Uint8_Test1)
{
    MyRemapperUint8 d(Clamp<float>(0.f, 1.f), Remapper<uint8_t>(MAP_LINEAR));

    float inRed = 1.0f;
    float inGreen = 0.0f;
    float inBlue = 0.0f;

    uint8_t outRed;
    uint8_t outGreen;
    uint8_t outBlue;

    d(inRed, inGreen, inBlue, outRed, outGreen, outBlue);

    EXPECT_EQ(static_cast<int>(outRed), 255);
    EXPECT_EQ(static_cast<int>(outGreen), 0);
    EXPECT_EQ(static_cast<int>(outBlue), 0);
}

TEST(FloatRgbConverter, Uint8_Test2)
{
    MyRemapperUint8 d(Clamp<float>(0.f, 1.f), Remapper<uint8_t>(MAP_LINEAR));

    float inRed = 1.2f;
    float inGreen = 0.0f;
    float inBlue = 0.0f;

    uint8_t outRed;
    uint8_t outGreen;
    uint8_t outBlue;

    d(inRed, inGreen, inBlue, outRed, outGreen, outBlue);

    EXPECT_EQ(static_cast<int>(outRed), 255);
    EXPECT_EQ(static_cast<int>(outGreen), 0);
    EXPECT_EQ(static_cast<int>(outBlue), 0);
}

TEST(FloatRgbConverter, Uint8_Test3)
{
    MyRemapperUint8 d(Clamp<float>(0.f, 1.f), Remapper<uint8_t>(MAP_LINEAR));

    float inRed = 1.2f;
    float inGreen = -0.0f;
    float inBlue = 0.0f;

    uint8_t outRed;
    uint8_t outGreen;
    uint8_t outBlue;

    d(inRed, inGreen, inBlue, outRed, outGreen, outBlue);

    EXPECT_EQ(static_cast<int>(outRed), 255);
    EXPECT_EQ(static_cast<int>(outGreen), 0);
    EXPECT_EQ(static_cast<int>(outBlue), 0);
}
