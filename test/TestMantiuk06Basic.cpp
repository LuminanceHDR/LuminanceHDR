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
#include <boost/bind.hpp>
#include <tuple>
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

class TestMantiuk06 : public TestWithParam< ::std::tuple<int, int> >
{
public:
    TestMantiuk06()
        : m_rows( ::std::get<1>( GetParam() ))
        , m_cols( ::std::get<0>( GetParam() ))
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

protected:
    size_t m_rows;
    size_t m_cols;
};

void compareVectors(const float* in1, const float* in2, size_t size)
{
    for (size_t idx = 0; idx < size; idx++)
    {
        ASSERT_NEAR(in1[idx], in2[idx], 10e-2f);
    }
}

void compareVectors(const XYGradient* inXYGradient,
                    const float* inGx, const float* inGy, size_t size)
{
    for (size_t idx = 0; idx < size; idx++)
    {
        ASSERT_NEAR(inXYGradient[idx].gX(), inGx[idx], 10e-2f);
        ASSERT_NEAR(inXYGradient[idx].gY(), inGy[idx], 10e-2f);
    }
}

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

    std::transform(testOutput.begin(), testOutput.end(),
                   testOutput.begin(),
                   TransformToR(M_PI));

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

    std::transform(testOutput.begin(), testOutput.end(),
                   testOutput.begin(),
                   TransformToG(M_PI));

    compareVectors(referenceOutput.data(), testOutput.data(),
                   testOutput.size());
}

void copyData(PyramidS& out, const DataBuffer& gx, const DataBuffer& gy)
{
    for ( size_t idx = 0; idx < out.size(); idx++ )
    {
        out(idx).gX() = gx[idx];
        out(idx).gY() = gy[idx];
    }
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

    PyramidS inputGxGy(inputCols, inputRows);

    copyData(inputGxGy, Gx, Gy);

    // data buffer
    DataBuffer referenceOutput(inputCols*inputRows);
    pfs::Array2Df testOutput(inputCols + 10,
                             inputRows + 10);
    // set initial value
    std::fill(testOutput.begin(), testOutput.end(), 1.f);
    std::fill(referenceOutput.begin(), referenceOutput.end(), 1.f);

    test_mantiuk06::calculate_and_add_divergence(inputCols, inputRows,
                                                 Gx.data(), Gy.data(),
                                                 referenceOutput.data());

    calculateAndAddDivergence(inputGxGy, testOutput.data());

    compareVectors(referenceOutput.data(), testOutput.data(),
                   referenceOutput.size());
}

TEST_P(TestMantiuk06, TestMantiuk06CalculateGradient)
{
    const size_t inputCols = cols();
    const size_t inputRows = rows();

    pfs::Array2Df input(inputCols, inputRows);
    
    // fill data with samples between zero and one!
    generate(input.begin(), input.end(), RandZeroOne());

    // REFERENCE
    DataBuffer referenceGx(inputCols*inputRows);
    DataBuffer referenceGy(inputCols*inputRows);

    test_mantiuk06::calculate_gradient(inputCols, inputRows, input.data(),
                                  referenceGx.data(), referenceGy.data());

    // COMPUTED
    PyramidS computedGradient(inputCols, inputRows);

    calculateGradients(input.data(), computedGradient);

    // CHECK
    compareVectors(computedGradient.data(),
                   referenceGx.data(), referenceGy.data(),
                   computedGradient.size());
}

INSTANTIATE_TEST_CASE_P(Mantiuk06,
                        TestMantiuk06,
//                        Combine(Values(91, 352, 403, 1024),
//                                Values(27, 256, 511, 1334))
                        Combine(Values(765, 320, 96),
                                Values(521, 123))
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
