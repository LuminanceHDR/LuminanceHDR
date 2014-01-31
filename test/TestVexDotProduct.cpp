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
#include <numeric>
#include <algorithm>
#include <cmath>
#include <boost/bind.hpp>

#include <Libpfs/utils/dotproduct.h>

using namespace pfs::utils;

float myRand()
{
    return ( static_cast<float>(rand())/(RAND_MAX) );
}

TEST(TestVexDotProduct, SmallSize)
{
    std::vector<float> inputVector(100);    // 100 pixels
    std::generate(inputVector.begin(), inputVector.end(), &myRand);

    float avg = std::inner_product(inputVector.begin(), inputVector.end(),
                                   inputVector.begin(), (double)0.0);

    float avgVex = dotProduct(inputVector.data(), inputVector.size());

    EXPECT_NEAR(avg, avgVex, 10e2);
}

TEST(TestVexDotProduct, MediumSize)
{
    std::vector<float> inputVector(1000000);    // 1 mpixels
    std::generate(inputVector.begin(), inputVector.end(), &myRand);

    float avg = std::inner_product(inputVector.begin(), inputVector.end(),
                                   inputVector.begin(), (double)0.0);

    float avgVex = dotProduct(inputVector.data(), inputVector.size());

    EXPECT_NEAR(avg, avgVex, 10e2);
}

TEST(TestVexDotProduct, BigSize)
{
    std::vector<float> inputVector(10000000);   // 10 mpixels
    std::generate(inputVector.begin(), inputVector.end(), &myRand);

    float avg = std::inner_product(inputVector.begin(), inputVector.end(),
                                   inputVector.begin(), (double)0.0);

    float avgVex = dotProduct(inputVector.data(), inputVector.size());

    EXPECT_NEAR(avg, avgVex, 10e2);
}

TEST(TestVexDotProduct, HugeSize)
{
    std::vector<float> inputVector(36000000); // 36 mpixels
    std::generate(inputVector.begin(), inputVector.end(), &myRand);

    float avg = std::inner_product(inputVector.begin(), inputVector.end(),
                                   inputVector.begin(), (double)0.0);

    float avgVex = dotProduct(inputVector.data(), inputVector.size());

    EXPECT_NEAR(avg, avgVex, 10e2);
}
