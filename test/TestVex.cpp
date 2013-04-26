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

#include <iostream>
#include <cmath>
#include "arch/math.h"
#include <cstddef>
#include <stdint.h>
#include <algorithm>
#include <numeric>
#include <functional>

#include <Libpfs/vex/vex.h>
#include <boost/assign/std/vector.hpp> // for 'operator+=()'

using namespace std;

template <typename T>
struct FillValue;

template<>
struct FillValue<float>
{
    float operator()() { return (static_cast<float>(rand())/RAND_MAX)*2.f - 1.f; }
};

template<>
struct FillValue<uint8_t>
{
    uint8_t operator()() { return (static_cast<uint8_t>(rand())%255) + 1; }
};

template<>
struct FillValue<uint16_t>
{
    uint16_t operator()() { return (static_cast<uint16_t>(rand())%255) + 1; }
};

template<>
struct FillValue<int32_t>
{
    int32_t operator()() {
        int32_t v = static_cast<int32_t>(rand())%256 - 128;
        return (!v) ? 1 : v;
    }
};

template<typename T>
T getConst();

template<>
float getConst<float>() { return static_cast<float>(M_PI); }

template<>
uint8_t getConst<uint8_t>() { return 3u; }

template<>
uint16_t getConst<uint16_t>() { return 3u; }

template<>
int32_t getConst<int32_t>() { return 5; }

template <typename T>
struct SmallSize
{
    typedef T value_type;

    static size_t elements() { return static_cast<size_t>(1e6); }    // 1Mp
};

template <typename T>
struct MediumSize
{
    typedef T value_type;

    static size_t elements() { return static_cast<size_t>(1e7); }    // 10Mp
};

template <typename T>
struct BigSize
{
    typedef T value_type;

    static size_t elements() { return static_cast<size_t>(24e6); }    // 24Mp
};

template <class T>
class TestVex : public testing::Test
{
protected:
    typedef typename T::value_type ValueType;
    typedef std::vector< ValueType > TestVexContainer;


    TestVexContainer input1;
    TestVexContainer input2;
    TestVexContainer outputReference;
    TestVexContainer outputComputed;
    ValueType s_;

    TestVex()
        : input1( T::elements() )
        , input2( T::elements() )
        , outputReference( T::elements() )
        , outputComputed(  T::elements() )
        , s_(getConst<ValueType>())
    {
        std::generate(input1.begin(), input1.end(), FillValue<ValueType>());
        std::generate(input2.begin(), input2.end(), FillValue<ValueType>());
        std::fill(outputReference.begin(), outputReference.end(), ValueType());
        std::fill(outputComputed.begin(), outputComputed.end(), ValueType());
    }

    void compareResult()
    {
        for (size_t idx = 0; idx < this->outputComputed.size(); ++idx)
        {
            if ( this->outputComputed[idx] != this->outputReference[idx])
            {
                EXPECT_NEAR(this->outputComputed[idx],
                            this->outputReference[idx],
                            10e-9);
            }
        }
    }

    void computeVmul()
    {
        std::transform(this->input1.begin(),
                       this->input1.end(),
                       this->input2.begin(),
                       this->outputReference.begin(),
                       std::multiplies<ValueType>());
    }

    void computeVdiv()
    {
        std::transform(this->input1.begin(),
                       this->input1.end(),
                       this->input2.begin(),
                       this->outputReference.begin(),
                       std::divides<ValueType>());
    }

    void computeVadd()
    {
        std::transform(this->input1.begin(),
                       this->input1.end(),
                       this->input2.begin(),
                       this->outputReference.begin(),
                       std::plus<ValueType>());
    }
    void computeVadds()
    {
        std::transform(this->input1.begin(),
                       this->input1.end(),
                       this->input2.begin(),
                       this->outputReference.begin(),
                       vex::numeric::vadds<ValueType>(s_));
    }

    void computeVsub()
    {
        std::transform(this->input1.begin(),
                       this->input1.end(),
                       this->input2.begin(),
                       this->outputReference.begin(),
                       std::minus<ValueType>());
    }
    void computeVsubs()
    {
        std::transform(this->input1.begin(),
                       this->input1.end(),
                       this->input2.begin(),
                       this->outputReference.begin(),
                       vex::numeric::vsubs<ValueType>(s_));
    }

    void compareTime(double timeOld, double timeNew)
    {
        std::cout << "Speed up: " << timeOld/timeNew << std::endl;
    }
};

using testing::Types;
// The list of types we want to test.
typedef testing::Types<
    SmallSize<float>,
    SmallSize<uint8_t>,
    SmallSize<uint16_t>,
    SmallSize<int32_t>,
    MediumSize<float>,
    MediumSize<uint8_t>,
    MediumSize<uint16_t>,
    MediumSize<int32_t>,
    BigSize<float>,
    BigSize<uint8_t>,
    BigSize<uint16_t>,
    BigSize<int32_t> > Implementations;

TYPED_TEST_CASE(TestVex, Implementations);

TYPED_TEST(TestVex, vmul)
{
    vex::vmul(this->input1.data(),
              this->input2.data(),
              this->outputComputed.data(),
              this->outputComputed.size());

    this->computeVmul();
    this->compareResult();
}

TYPED_TEST(TestVex, vdiv)
{
    vex::vdiv(this->input1.data(),
              this->input2.data(),
              this->outputComputed.data(),
              this->outputComputed.size());

    this->computeVdiv();
    this->compareResult();
}

TYPED_TEST(TestVex, vadd)
{
    vex::vadd(this->input1.data(),
              this->input2.data(),
              this->outputComputed.data(),
              this->outputComputed.size());

    this->computeVadd();
    this->compareResult();
}
TYPED_TEST(TestVex, vadds)
{
    vex::vadds(this->input1.data(),
               this->s_,
               this->input2.data(),
               this->outputComputed.data(),
               this->outputComputed.size());

    this->computeVadds();
    this->compareResult();
}

TYPED_TEST(TestVex, vsub)
{
    vex::vsub(this->input1.data(),
              this->input2.data(),
              this->outputComputed.data(),
              this->outputComputed.size());

    this->computeVsub();
    this->compareResult();
}
TYPED_TEST(TestVex, vsubs)
{
    vex::vsubs(this->input1.data(),
               this->s_,
               this->input2.data(),
               this->outputComputed.data(),
               this->outputComputed.size());

    this->computeVsubs();
    this->compareResult();
}
