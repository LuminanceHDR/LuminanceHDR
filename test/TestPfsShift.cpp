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

#include <algorithm>
#include <numeric>
#include <boost/scoped_ptr.hpp>

#include "Libpfs/array2d.h"
#include "Libpfs/manip/shift.h"

#include "SeqInt.h"
#include "PrintArray2D.h"

using namespace pfs;

TEST(TestPfsShift, MinusMinus)
{
    const float ref[] = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                         0.f, 0.f, 0.f, 1.f, 2.f, 3.f,
                         0.f, 0.f, 6.f, 7.f, 8.f, 9.f,
                         0.f, 0.f, 12.f, 13.f, 14.f, 15.f,
                         0.f, 0.f, 18.f, 19.f, 20.f, 21.f};

    size_t rows = 5;
    size_t cols = 6;

    Array2D input(cols, rows);
    std::generate(input.begin(), input.end(), SeqInt());

    boost::scoped_ptr<Array2D> output( shiftPfsArray2D(input, -2, -1) );

    const float* outData = output->getRawData();

    // print(input);
    // print(*output);

    for (int idx = 0; idx < static_cast<int>(rows*cols); idx++)
    {
        ASSERT_NEAR(ref[idx], outData[idx], 10e-5f);
    }
}

TEST(TestPfsShift, PlusMinus)
{
    const float ref[] = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                         2.f, 3.f, 4.f, 5.f, 0.f, 0.f,
                         8.f, 9.f, 10.f, 11.f, 0.f, 0.f,
                         14.f, 15.f, 16.f, 17.f, 0.f, 0.f,
                         20.f, 21.f, 22.f, 23.f, 0.f, 0.f};

    size_t rows = 5;
    size_t cols = 6;

    Array2D input(cols, rows);
    std::generate(input.begin(), input.end(), SeqInt());

    boost::scoped_ptr<Array2D> output( shiftPfsArray2D(input, 2, -1) );

    const float* outData = output->getRawData();

    // print(input);
    // print(*output);

    for (int idx = 0; idx < static_cast<int>(rows*cols); idx++)
    {
        ASSERT_NEAR(ref[idx], outData[idx], 10e-5f);
    }
}

TEST(TestPfsShift, StillMinus)
{
    const float ref[] = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
                         0.f, 1.f, 2.f, 3.f, 4.f, 5.f,
                         6.f, 7.f, 8.f, 9.f, 10.f, 11.f,
                         12.f, 13.f, 14.f, 15.f, 16.f, 17.f,
                         18.f, 19.f, 20.f, 21.f, 22.f, 23.f};

    size_t rows = 5;
    size_t cols = 6;

    Array2D input(cols, rows);
    std::generate(input.begin(), input.end(), SeqInt());

    boost::scoped_ptr<Array2D> output( shiftPfsArray2D(input, 0, -1) );

    const float* outData = output->getRawData();

    // print(input);
    // print(*output);

    for (int idx = 0; idx < static_cast<int>(rows*cols); idx++)
    {
        ASSERT_NEAR(ref[idx], outData[idx], 10e-5f);
    }
}

TEST(TestPfsShift, MinusPlus)
{
    const float ref[] = {0.f, 0.f, 6.f, 7.f, 8.f, 9.f,
                         0.f, 0.f, 12.f, 13.f, 14.f, 15.f,
                         0.f, 0.f, 18.f, 19.f, 20.f, 21.f,
                        0.f, 0.f, 24.f, 25.f, 26.f, 27.f,
                        0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
    size_t rows = 5;
    size_t cols = 6;

    Array2D input(cols, rows);
    std::generate(input.begin(), input.end(), SeqInt());

    boost::scoped_ptr<Array2D> output( shiftPfsArray2D(input, -2, 1) );

    const float* outData = output->getRawData();

    // print(input);
    // print(*output);

    for (int idx = 0; idx < static_cast<int>(rows*cols); idx++)
    {
        ASSERT_NEAR(ref[idx], outData[idx], 10e-5f);
    }
}

TEST(TestPfsShift, PlusPlus)
{
    const float ref[] = {8.f, 9.f, 10.f, 11.f, 0.f, 0.f,
                         14.f, 15.f, 16.f, 17.f, 0.f, 0.f,
                         20.f, 21.f, 22.f, 23.f, 0.f, 0.f,
                         26.f, 27.f, 28.f, 29.f, 0.f, 0.f,
                         0.f, 0.f, 0.f, 0.f, 0.f, 0.f};

    size_t rows = 5;
    size_t cols = 6;

    Array2D input(cols, rows);
    std::generate(input.begin(), input.end(), SeqInt());

    boost::scoped_ptr<Array2D> output( shiftPfsArray2D(input, 2, 1) );

    const float* outData = output->getRawData();

    // print(input);
    // print(*output);

    for (int idx = 0; idx < static_cast<int>(rows*cols); idx++)
    {
        ASSERT_NEAR(ref[idx], outData[idx], 10e-5f);
    }
}

TEST(TestPfsShift, StillStill)
{
    const float ref[] = {0.f, 1.f, 2.f, 3.f, 4.f, 5.f,
                         6.f, 7.f, 8.f, 9.f, 10.f, 11.f,
                         12.f, 13.f, 14.f, 15.f, 16.f, 17.f,
                         18.f, 19.f, 20.f, 21.f, 22.f, 23.f,
                           24.f, 25.f, 26.f, 27.f, 28.f, 29.f};

    size_t rows = 5;
    size_t cols = 6;

    Array2D input(cols, rows);
    std::generate(input.begin(), input.end(), SeqInt());

    boost::scoped_ptr<Array2D> output( shiftPfsArray2D(input, 0, 0) );

    const float* outData = output->getRawData();

    // print(input);
    // print(*output);

    for (int idx = 0; idx < static_cast<int>(rows*cols); idx++)
    {
        ASSERT_NEAR(ref[idx], outData[idx], 10e-5f);
    }
}
