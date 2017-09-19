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

#include <array>
#include <cassert>
#include <cstdio>
#include <string>

namespace libhdr {
namespace fusion {

enum ResponseCurveType {
    RESPONSE_CUSTOM = -1,
    RESPONSE_LINEAR = 0,
    RESPONSE_GAMMA = 1,
    RESPONSE_LOG10 = 2,
    RESPONSE_SRGB = 3
};

enum ResponseChannel {
    RESPONSE_CHANNEL_RED = 0,
    RESPONSE_CHANNEL_GREEN = 1,
    RESPONSE_CHANNEL_BLUE = 2
};

class ResponseCurve {
   public:
    static const size_t NUM_BINS = (1 << 12);
    typedef std::array<float, NUM_BINS> ResponseContainer;

    static size_t getIdx(float sample);
    static ResponseCurveType fromString(const std::string &type);

    explicit ResponseCurve(ResponseCurveType type = RESPONSE_LINEAR);

    void setType(ResponseCurveType type);
    //! \return type of response function implemented
    ResponseCurveType getType() const;

    //! \brief return the response of the value \c input. \c input is in the
    //! range [0, 1]
    float getResponse(float input,
                      ResponseChannel channel = RESPONSE_CHANNEL_RED) const;

    float operator()(float input,
                     ResponseChannel channel = RESPONSE_CHANNEL_RED) const {
        return getResponse(input, channel);
    }

    void writeToFile(const std::string &fileName) const;
    bool readFromFile(const std::string &fileName);

    ResponseContainer &get(ResponseChannel channel);
    const ResponseContainer &get(ResponseChannel channel) const;

   protected:
    ResponseCurveType m_type;
    std::array<ResponseContainer, 3> m_responses;
};

inline ResponseCurveType ResponseCurve::getType() const { return m_type; }

inline size_t ResponseCurve::getIdx(float sample)
//{ return size_t(sample*(NUM_BINS - 1) + 0.45f); } TODO: check this one
{
    return size_t(sample * (NUM_BINS - 1));
}

inline ResponseCurve::ResponseContainer &ResponseCurve::get(
    ResponseChannel channel) {
    return m_responses[channel];
}

inline const ResponseCurve::ResponseContainer &ResponseCurve::get(
    ResponseChannel channel) const {
    return m_responses[channel];
}

inline float ResponseCurve::getResponse(float input,
                                        ResponseChannel channel) const {
    assert(channel >= 0);
    assert(channel <= 2);
    assert(input >= 0.f);
    assert(input <= 1.f);

    return m_responses[channel][getIdx(input)];
}

}  // fusion
}  // libhdr

//! \brief Save response curve to a MatLab file for further reuse
//!
//! \param file file handle to save response curve
//! \param I camera response function (array size of M)
//! \param M number of camera output levels
//! \param name matrix name for use in Octave or Matlab
void responseSave(FILE *file, const float *Ir, const float *Ig, const float *Ib,
                  int M);

//! \brief Load response curve (saved with responseSave();)
//!
//! \param file file handle to save response curve
//! \param I [out] camera response function (array size of M)
//! \param M number of camera output levels
//! \return false means file has different output levels or is wrong for some
//! other reason
bool responseLoad(FILE *file, float *Ir, float *Ig, float *Ib, int M);

#endif  // RESPONSES_H
