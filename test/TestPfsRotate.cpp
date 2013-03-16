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

#if GTEST_HAS_COMBINE

#include <algorithm>
#ifdef __clang__
#include <tr1/tuple>
#else
#include <tuple>
#endif

#include <Libpfs/array2d.h>
#include <Libpfs/manip/rotate.h>

#include "CompareVector.h"
#include "SeqInt.h"

using ::testing::TestWithParam;
using ::testing::Values;
using ::testing::Combine;

template <typename InputType>
void rotate_ccw(const InputType* input,
                InputType* output,
                size_t width,
                size_t height)
{
    for (size_t j = 0; j < height; j++)
    {
        for (size_t i = 0; i < width; i++)
        {
            output[(width - i - 1)*height + j] = input[j*width + i];
        }
    }
}

template <typename InputType>
void rotate_cw(const InputType* input,
               InputType* output,
               size_t width,
               size_t height)
{
    for (size_t j = 0; j < height; j++)
    {
        for (size_t i = 0; i < width; i++)
        {
            output[(i+1)*height - 1 - j] = input[j*width + i];
        }
    }
}

class TestPfsRotate : public TestWithParam< ::std::tr1::tuple<size_t, size_t> >
{
protected:
    size_t m_rows;
    size_t m_cols;

    pfs::Array2Df inputVector;
    pfs::Array2Df referenceOutput;
    pfs::Array2Df computedOutput;
    
public:
    TestPfsRotate()
        : m_rows( ::std::tr1::get<0>( GetParam() ))
        , m_cols( ::std::tr1::get<1>( GetParam() ))
        , inputVector(m_cols, m_rows)
        , referenceOutput(m_rows, m_cols)
        , computedOutput(m_rows, m_cols)
    {
        std::generate(inputVector.begin(), inputVector.end(), SeqInt());
    }

    size_t cols() const { return m_cols; }
    size_t rows() const { return m_rows; }
};

TEST_P(TestPfsRotate, ClockWise)
{
    rotate_cw(inputVector.data(),
              referenceOutput.data(),
              cols(), rows());
    pfs::rotate(&inputVector,
                     &computedOutput,
                     true);
    
    compareVectors(referenceOutput.data(),
                   computedOutput.data(),
                   cols()*rows());
}

TEST_P(TestPfsRotate, CClockWise)
{
    rotate_ccw(inputVector.data(),
               referenceOutput.data(),
               cols(), rows());
    pfs::rotate(&inputVector,
                &computedOutput,
                false);
    
    compareVectors(referenceOutput.data(),
                   computedOutput.data(),
                   cols()*rows());
}

INSTANTIATE_TEST_CASE_P(Test,
                        TestPfsRotate,
                        Combine(Values(91, 352, 403),
                                Values(27, 256, 511))
                        );

#else

TEST(DummyTest, CombineIsNotSupportedOnThisPlatform) {}

#endif
