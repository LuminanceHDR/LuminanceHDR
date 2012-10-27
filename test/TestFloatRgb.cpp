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
#include "Common/FloatRgbToQRgb.h"

TEST(FloatRgbConverter, Uint16_Test1)
{
    FloatRgbToQRgb d;

    float inRed = 1.0f;
    float inGreen = 0.0f;
    float inBlue = 0.0f;

    quint16 outRed;
    quint16 outGreen;
    quint16 outBlue;

    d.toQUint16(inRed, inGreen, inBlue,
                outRed, outGreen, outBlue);

    EXPECT_EQ(static_cast<int>(outRed), 65535);
    EXPECT_EQ(static_cast<int>(outGreen), 0);
    EXPECT_EQ(static_cast<int>(outBlue), 0);
}

TEST(FloatRgbConverter, Uint16_Test2)
{
    FloatRgbToQRgb d;

    float inRed = 1.2f;
    float inGreen = 0.0f;
    float inBlue = 0.0f;

    quint16 outRed;
    quint16 outGreen;
    quint16 outBlue;

    d.toQUint16(inRed, inGreen, inBlue,
                outRed, outGreen, outBlue);

    EXPECT_EQ(static_cast<int>(outRed), 65535);
    EXPECT_EQ(static_cast<int>(outGreen), 0);
    EXPECT_EQ(static_cast<int>(outBlue), 0);
}

TEST(FloatRgbConverter, Uint16_Test3)
{
    FloatRgbToQRgb d;

    float inRed = 1.2f;
    float inGreen = -0.0f;
    float inBlue = 0.0f;

    quint16 outRed;
    quint16 outGreen;
    quint16 outBlue;

    d.toQUint16(inRed, inGreen, inBlue,
                outRed, outGreen, outBlue);

    EXPECT_EQ(static_cast<int>(outRed), 65535);
    EXPECT_EQ(static_cast<int>(outGreen), 0);
    EXPECT_EQ(static_cast<int>(outBlue), 0);
}

TEST(FloatRgbConverter, Qrgb_Test1)
{
    FloatRgbToQRgb d;

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
    FloatRgbToQRgb d;

    float inRed = 1.2f;
    float inGreen = 0.5f;
    float inBlue = -0.5f;

    QRgb rgb;

    d.toQRgb(inRed, inGreen, inBlue, rgb);

    EXPECT_EQ(qRed(rgb), 255);
    EXPECT_EQ(qGreen(rgb), 128);
    EXPECT_EQ(qBlue(rgb), 0);
}
