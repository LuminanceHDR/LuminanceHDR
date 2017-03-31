/*
 * This file is a part of Luminance HDR package
 * ----------------------------------------------------------------------
 * Copyright (C) 2013 Davide Anastasia
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ----------------------------------------------------------------------
 */

#ifndef LIBHDR_FUSION_WEIGHTS_H
#define LIBHDR_FUSION_WEIGHTS_H

#include <limits>
#include <vector>
#include <string>
#include <cassert>

namespace libhdr {
namespace fusion {

enum WeightFunctionType
{
    WEIGHT_TRIANGULAR = 0,
    WEIGHT_GAUSSIAN = 1,
    WEIGHT_PLATEAU = 2,
    WEIGHT_FLAT = 3
};

class WeightFunction
{
public:
    //static const size_t NUM_BINS = (1 << 8);
    typedef std::vector<float> WeightContainer;

    static WeightFunctionType fromString(const std::string& type);

    size_t getIdx(float sample);

    void setBPS(int bps) { m_num_bins = (1 << bps); setType(m_type); }
    size_t getNum_Bins() { return m_num_bins; }

    WeightFunction(WeightFunctionType type);

    float getWeight(float input);
    WeightContainer getWeights() const;
    float operator()(float input) { return getWeight(input); }

    void setType(WeightFunctionType type);
    WeightFunctionType getType() const  { return m_type; }

    float minTrustedValue() const       { return m_minTrustedValue; }
    float maxTrustedValue() const       { return m_maxTrustedValue; }

private:
    WeightFunctionType m_type;
    WeightContainer m_weights;

    float m_minTrustedValue;
    float m_maxTrustedValue;
    size_t m_num_bins;
};

inline
size_t WeightFunction::getIdx(float sample)
{ return size_t(sample*(m_num_bins - 1) + 0.45f); }

inline
float WeightFunction::getWeight(float input)
{
    assert(input >= 0.f);
    assert(input <= 1.f);

    size_t idx = getIdx(input);
    return m_weights[idx];
}

inline
WeightFunction::WeightContainer WeightFunction::getWeights() const
{
    assert(input >= 0.f);
    assert(input <= 1.f);

    return m_weights;
}

}   // fusion
}   // libhdr

//! \brief Load weighting function
//!
//! \param file file handle to save response curve
//! \param w [out] weights (array size of M)
//! \param M number of camera output levels
//! \return false means file has different output levels or is wrong for some other reason
bool weightsLoad(FILE* file, float* w, int M);

//! \brief Save weighting function
//!
//! \param file file handle to save response curve
//! \param w weights (array size of M)
//! \param M number of camera output levels
//! \param name matrix name for use in Octave or Matlab
void weightsSave(FILE* file, const float* w, int M, const char* name);

#endif // LIBHDR_FUSION_WEIGHTS_H
