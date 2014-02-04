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

namespace libhdr {
namespace fusion {

enum WeightFunction
{
    WEIGHT_TRIANGULAR = 0,
    WEIGHT_GAUSSIAN = 1,
    WEIGHT_PLATEAU = 2
};

class IWeightFunction {
public:
    virtual ~IWeightFunction() {}

    virtual float getWeight(float input) const = 0;
    virtual WeightFunction getType() const = 0;

    virtual float minTrustedValue() const {
        return std::numeric_limits<float>::epsilon();
    }

    virtual float maxTrustedValue() const {
        return 1.f - std::numeric_limits<float>::epsilon();
    }
};

class WeightTriangular : public IWeightFunction {
public:
    float getWeight(float input) const;
    WeightFunction getType() const {
        return WEIGHT_TRIANGULAR;
    }

    float minTrustedValue() const;
    float maxTrustedValue() const;
};

class WeightGaussian : public IWeightFunction {
public:
    float getWeight(float input) const;
    WeightFunction getType() const {
        return WEIGHT_GAUSSIAN;
    }

    float minTrustedValue() const;
    float maxTrustedValue() const;
};

class WeightPlateau : public IWeightFunction {
public:
    float getWeight(float input) const;
    WeightFunction getType() const {
        return WEIGHT_PLATEAU;
    }

    float minTrustedValue() const;
    float maxTrustedValue() const;
};

}   // fusion
}   // libhdr

// OLD STUFF
/**
 * @brief Weighting function with "flat" distribution (as in icip06)
 *
 * @param w [out] weights (array size of M)
 * @param M number of camera output levels
 */
void exposure_weights_icip06( float* w, int M, int Mmin, int Mmax );

/**
 * @brief Weighting function with triangle distribution (as in debevec)
 *
 * @param w [out] weights (array size of M)
 * @param M number of camera output levels
 */
void weights_triangle( float* w, int M/*, int Mmin, int Mmax */);

/**
 * @brief Weighting function with gaussian distribution
 *
 * @param w [out] weights (array size of M)
 * @param M number of camera output levels
 * @param Mmin minimum registered camera output level
 * @param Mmax maximum registered camera output level
 * @param sigma sigma value for gaussian
 */
void weightsGauss(float* w, int M, int Mmin, int Mmax, float sigma  = 8.0f);




#endif // LIBHDR_FUSION_WEIGHTS_H
