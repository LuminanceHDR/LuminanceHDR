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

#include <Libpfs/array2d.h>
#include <Libpfs/frame.h>

using namespace pfs;

TEST(TestArray2D, Resize1)
{
    Array2D array(100, 200);

    EXPECT_EQ(array.getCols(), 100);
    EXPECT_EQ(array.getRows(), 200);

    float* d1 = array.getRawData();

    array.resize(150, 220);

    EXPECT_EQ(array.getCols(), 150);
    EXPECT_EQ(array.getRows(), 220);

    float* d2 = array.getRawData();

    EXPECT_NE(d1, d2);
}

TEST(TestArray2D, Resize2)
{
    Array2D array(100, 200);

    EXPECT_EQ(array.getCols(), 100);
    EXPECT_EQ(array.getRows(), 200);

    float* d1 = array.getRawData();

    array.resize(90, 180);

    EXPECT_EQ(array.getCols(), 90);
    EXPECT_EQ(array.getRows(), 180);

    float* d2 = array.getRawData();

    EXPECT_EQ(d1, d2);
}
