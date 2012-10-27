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
#include <stdlib.h>
#include <vector>
#include <algorithm>
#ifdef __clang__
#include <tr1/tuple>
#else
#include <tuple>
#endif
#include "arch/math.h"

#if GTEST_HAS_COMBINE

#include "mantiuk06/contrast_domain.h"
#include "TonemappingOperators/mantiuk06/pyramid.h"

struct RandZeroOne
{
    float operator()()
    {
        return (static_cast<float>(rand())/(RAND_MAX))*10000.f - 500.f;
    }
};

typedef std::vector<float> DataBuffer;

using ::testing::TestWithParam;
using ::testing::Values;
using ::testing::Combine;

class TestMantiuk06 : public TestWithParam< ::std::tr1::tuple<int, int> >
{
public:
    TestMantiuk06()
        : m_rows( ::std::tr1::get<1>( GetParam() ))
        , m_cols( ::std::tr1::get<0>( GetParam() ))
    {
        // std::cout << "(" << m_rows << "," << m_cols << ")" << std::endl;
    }

    size_t halfCols() const
    {
        return (m_cols >> 1);
    }

    size_t halfRows() const
    {
        return (m_rows >> 1);
    }

    size_t cols() const
    {
        return m_cols;
    }

    size_t rows() const
    {
        return m_rows;
    }

    static void compareVectors(const float* in1, const float* in2, size_t size)
    {
        for (size_t idx = 0; idx < size; idx++)
        {
            ASSERT_NEAR(in1[idx], in2[idx], 10e-2f);
        }
    }

protected:
    size_t m_rows;
    size_t m_cols;
};


TEST_P(TestMantiuk06, Upsample)
{
    const size_t outputCols = cols();
    const size_t outputRows = rows();
    const size_t inputCols  = halfCols();
    const size_t inputRows  = halfRows();

    DataBuffer origin(inputCols*inputRows);

    // fill data with samples between zero and one!
    generate(origin.begin(), origin.end(), RandZeroOne());

    DataBuffer referenceOutput(outputCols*outputRows);
    DataBuffer testOutput(outputCols*outputRows);

    test_mantiuk06::matrix_upsample(outputCols, outputRows,
                                    origin.data(), referenceOutput.data());

    matrixUpsample(outputCols, outputRows,
                   origin.data(), testOutput.data());

    compareVectors(referenceOutput.data(), testOutput.data(),
                   testOutput.size());
}

TEST_P(TestMantiuk06, DownsampleFull)
{
    const size_t inputCols = cols();
    const size_t inputRows = rows();

    const size_t outputCols = halfCols();
    const size_t outputRows = halfRows();

    DataBuffer origin(inputCols*inputRows);

    // fill data with samples between zero and one!
    generate(origin.begin(), origin.end(), RandZeroOne());

    DataBuffer referenceOutput(outputCols*outputRows);
    DataBuffer testOutput(outputCols*outputRows);

    test_mantiuk06::matrix_downsample(inputCols, inputRows,
                                      origin.data(),
                                      referenceOutput.data());
    matrixDownsample(inputCols, inputRows,
                     origin.data(),
                     testOutput.data());

    compareVectors(referenceOutput.data(), testOutput.data(),
                   testOutput.size());
}

TEST_P(TestMantiuk06, TransformToR)
{
    const size_t inputCols = cols();
    const size_t inputRows = rows();

    DataBuffer referenceOutput(inputCols*inputRows);
    generate(referenceOutput.begin(), referenceOutput.end(),
             RandZeroOne());
    DataBuffer testOutput(inputCols*inputRows);
    copy(referenceOutput.begin(), referenceOutput.end(),
         testOutput.begin());

    compareVectors(referenceOutput.data(), testOutput.data(),
                   testOutput.size());

    test_mantiuk06::transform_to_R(inputCols*inputRows,
                                   referenceOutput.data(),
                                   M_PI);
    transformToR(testOutput.data(),
                 M_PI,
                 inputCols*inputRows);

    compareVectors(referenceOutput.data(), testOutput.data(),
                   testOutput.size());
}

TEST_P(TestMantiuk06, TransformToG)
{
    const size_t inputCols = cols();
    const size_t inputRows = rows();

    DataBuffer referenceOutput(inputCols*inputRows);
    generate(referenceOutput.begin(), referenceOutput.end(),
             RandZeroOne());
    DataBuffer testOutput(inputCols*inputRows);
    copy(referenceOutput.begin(), referenceOutput.end(),
         testOutput.begin());

    compareVectors(referenceOutput.data(), testOutput.data(),
                   testOutput.size());

    test_mantiuk06::transform_to_G(inputCols*inputRows,
                                   referenceOutput.data(),
                                   M_PI);
    transformToG(testOutput.data(),
                 M_PI,
                 inputCols*inputRows);

    compareVectors(referenceOutput.data(), testOutput.data(),
                   testOutput.size());
}

TEST_P(TestMantiuk06, TestMantiuk06AddDivergence)
{
    const size_t inputCols = cols();
    const size_t inputRows = rows();

    DataBuffer Gx(inputCols*inputRows);
    DataBuffer Gy(inputCols*inputRows);

    // fill data with samples between zero and one!
    generate(Gx.begin(), Gx.end(), RandZeroOne());
    generate(Gy.begin(), Gy.end(), RandZeroOne());

    // data buffer
    DataBuffer referenceOutput(inputCols*inputRows);
    DataBuffer testOutput(inputCols*inputRows);
    // set initial value
    std::fill(testOutput.begin(), testOutput.end(), 1.f);
    std::fill(referenceOutput.begin(), referenceOutput.end(), 1.f);

    test_mantiuk06::calculate_and_add_divergence(inputCols, inputRows,
                                                 Gx.data(), Gy.data(),
                                                 referenceOutput.data());
    calculateAndAddDivergence(inputCols, inputRows,
                              Gx.data(), Gy.data(),
                              testOutput.data());

    compareVectors(referenceOutput.data(), testOutput.data(),
                   testOutput.size());
}

TEST_P(TestMantiuk06, TestMantiuk06CalculateGradient)
{
    const size_t inputCols = cols();
    const size_t inputRows = rows();

    std::vector<float> input(inputCols*inputRows);
    // fill data with samples between zero and one!
    generate(input.begin(), input.end(), RandZeroOne());

    // REFERENCE
    DataBuffer referenceGx(inputCols*inputRows);
    DataBuffer referenceGy(inputCols*inputRows);

    test_mantiuk06::calculate_gradient(inputCols, inputRows, input.data(),
                                  referenceGx.data(), referenceGy.data());

    // COMPUTED
    DataBuffer computedGx(inputCols*inputRows);
    DataBuffer computedGy(inputCols*inputRows);

    calculateGradients(inputCols, inputRows, input.data(),
                       computedGx.data(), computedGy.data());

    // CHECK
    compareVectors(referenceGx.data(), computedGx.data(),
                   computedGx.size());
    compareVectors(referenceGy.data(), computedGy.data(),
                   computedGy.size());
}

INSTANTIATE_TEST_CASE_P(Mantiuk06,
                        TestMantiuk06,
                        Combine(Values(91, 352, 403, 1024),
                                Values(27, 256, 511, 1334))
                        );
#else

// Google Test may not support Combine() with some compilers. If we
// use conditional compilation to compile out all code referring to
// the gtest_main library, MSVC linker will not link that library at
// all and consequently complain about missing entry point defined in
// that library (fatal error LNK1561: entry point must be
// defined). This dummy test keeps gtest_main linked in.
TEST(DummyTest, CombineIsNotSupportedOnThisPlatform) {}

#endif  // GTEST_HAS_COMBINE
