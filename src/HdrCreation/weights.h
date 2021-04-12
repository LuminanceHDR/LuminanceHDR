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

#include <array>
#include <cassert>
#include <limits>
#include <string>

namespace libhdr {
namespace fusion {

enum WeightFunctionType {
    WEIGHT_TRIANGULAR = 0,
    WEIGHT_GAUSSIAN = 1,
    WEIGHT_PLATEAU = 2,
    WEIGHT_FLAT = 3
};

class WeightFunction {
   public:
    static const size_t NUM_BINS = (1 << 12);
    typedef std::array<float, NUM_BINS> WeightContainer;

    static WeightFunctionType fromString(const std::string &type);

    static size_t getIdx(float sample);

    WeightFunction(WeightFunctionType type);

    float getWeight(float input) const;
    WeightContainer getWeights() const;
    float operator()(float input) const { return getWeight(input); }

    void setType(WeightFunctionType type);
    WeightFunctionType getType() const { return m_type; }

    float minTrustedValue() const { return m_minTrustedValue; }
    float maxTrustedValue() const { return m_maxTrustedValue; }

   private:
    WeightFunctionType m_type;
    WeightContainer m_weights;

    float m_minTrustedValue;
    float m_maxTrustedValue;
};

inline size_t WeightFunction::getIdx(float sample)
//{ return size_t(sample*(NUM_BINS - 1) + 0.45f); } TODO: check this one
{
    return size_t(sample * (NUM_BINS - 1));
}

inline float WeightFunction::getWeight(float input) const {
    assert(input >= 0.f);
    assert(input <= 1.f);

    return m_weights[getIdx(input)];
}

inline WeightFunction::WeightContainer WeightFunction::getWeights() const {
    return m_weights;
}

}  // fusion
}  // libhdr

//! \brief Load weighting function
//!
//! \param file file handle to save response curve
//! \param w [out] weights (array size of M)
//! \param M number of camera output levels
//! \return false means file has different output levels or is wrong for some
//! other reason
bool weightsLoad(FILE *file, float *w, int M);

//! \brief Save weighting function
//!
//! \param file file handle to save response curve
//! \param w weights (array size of M)
//! \param M number of camera output levels
//! \param name matrix name for use in Octave or Matlab
void weightsSave(FILE *file, const float *w, int M, const char *name);

#endif  // LIBHDR_FUSION_WEIGHTS_H
