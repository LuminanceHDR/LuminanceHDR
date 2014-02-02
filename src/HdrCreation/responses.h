/*
 * This file is a part of Luminance HDR package
 * ----------------------------------------------------------------------
 * Copyright (C) 2013-2014 Davide Anastasia
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

//! \brief Standard response functions
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#ifndef LIBHDR_FUSION_RESPONSES_H
#define LIBHDR_FUSION_RESPONSES_H

#include <string>
#include <array>

namespace libhdr {
namespace fusion {

enum ResponseFunction
{
    RESPONSE_GAMMA,
    RESPONSE_LINEAR,
    RESPONSE_LOG10,
    RESPONSE_SRGB
};

enum ResponseChannel
{
    RESPONSE_CHANNEL_RED = 0,
    RESPONSE_CHANNEL_GREEN = 1,
    RESPONSE_CHANNEL_BLUE = 2
};

class IResponseFunction {
public:
    static const size_t NUM_BINS = 65536;
    typedef std::array<float, NUM_BINS> ResponseContainer;

    static ResponseFunction fromString(const std::string& type);

    virtual ~IResponseFunction() {}

    //! \brief return the response of the value \c input. \c input is in the
    //! range [0, 1]
    float getResponse(float input, ResponseChannel channel = RESPONSE_CHANNEL_RED) const;

    //! \return type of response function implemented
    virtual ResponseFunction getType() const = 0;

protected:
    std::array<ResponseContainer, 3> m_responses;
};

class ResponseGamma : public IResponseFunction
{
public:
    ResponseGamma();

    ResponseFunction getType() const {
        return RESPONSE_GAMMA;
    }

private:
    void fillResponse(ResponseContainer& response);
};

class ResponseLinear : public IResponseFunction
{
public:
    ResponseLinear();

    ResponseFunction getType() const {
        return RESPONSE_LINEAR;
    }

private:
    void fillResponse(ResponseContainer& response);
};

class ResponseLog10 : public IResponseFunction
{
public:
    ResponseLog10();

    ResponseFunction getType() const {
        return RESPONSE_LOG10;
    }

private:
    void fillResponse(ResponseContainer& response);
};

class ResponseSRGB : public IResponseFunction
{
public:
    // float getResponse(float input) const;

    ResponseFunction getType() const {
        return RESPONSE_SRGB;
    }
};

}   // fusion
}   // libhdr

// Old Stuff

#include <cstdio>

/**
 * @brief Save response curve to a MatLab file for further reuse
 *
 * @param file file handle to save response curve
 * @param I camera response function (array size of M)
 * @param M number of camera output levels
 * @param name matrix name for use in Octave or Matlab
 */
void responseSave( FILE* file, const float* Ir, const float* Ig, const float* Ib, int M);


/**
 * @brief Save response curve to a MatLab file for further reuse
 *
 * @param file file handle to save response curve
 * @param w weights (array size of M)
 * @param M number of camera output levels
 * @param name matrix name for use in Octave or Matlab
 */
void weightsSave( FILE* file, const float* w, int M, const char* name);


/**
 * @brief Load response curve (saved with responseSave();)
 *
 * @param file file handle to save response curve
 * @param I [out] camera response function (array size of M)
 * @param M number of camera output levels
 * @return false means file has different output levels or is wrong for some other reason
 */
bool responseLoad( FILE* file, float* Ir, float* Ig, float* Ib, int M);

/**
 * @brief Load response curve (saved with responseSave();)
 *
 * @param file file handle to save response curve
 * @param w [out] weights (array size of M)
 * @param M number of camera output levels
 * @return false means file has different output levels or is wrong for some other reason
 */
bool weightsLoad( FILE* file, float* w, int M);

#endif  // RESPONSES_H
