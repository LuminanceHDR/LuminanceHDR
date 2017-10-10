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

#include "responses.h"

#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <cmath>
#include <cstdio>
#include <cstdlib>

#include <boost/assign.hpp>

#include <Libpfs/colorspace/rgb.h>
#include <Libpfs/utils/resourcehandlerstdio.h>
#include <Libpfs/utils/string.h>

using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace pfs::utils;

namespace libhdr {
namespace fusion {

ResponseCurveType ResponseCurve::fromString(const std::string &type) {
    typedef map<string, ResponseCurveType, pfs::utils::StringUnsensitiveComp>
        Dict;
    static Dict v = map_list_of("log10", RESPONSE_LOG10)("log", RESPONSE_LOG10)(
        "linear", RESPONSE_LINEAR)("gamma", RESPONSE_GAMMA)("srgb",
                                                            RESPONSE_SRGB);

    Dict::const_iterator it = v.find(type);
    if (it != v.end()) {
        return it->second;
    }
    return RESPONSE_LINEAR;
}

ResponseCurve::ResponseCurve(ResponseCurveType type) : m_type(type) {
    setType(type);
}

void ResponseCurve::writeToFile(const std::string &fileName) const {
    ScopedStdIoFile outputFile(fopen(fileName.c_str(), "w"));
    responseSave(outputFile.data(), m_responses[RESPONSE_CHANNEL_RED].data(),
                 m_responses[RESPONSE_CHANNEL_GREEN].data(),
                 m_responses[RESPONSE_CHANNEL_BLUE].data(), NUM_BINS);
}

bool ResponseCurve::readFromFile(const string &fileName) {
    ScopedStdIoFile inputFile(fopen(fileName.c_str(), "r"));
    if (!responseLoad(inputFile.data(),
                      m_responses[RESPONSE_CHANNEL_RED].data(),
                      m_responses[RESPONSE_CHANNEL_GREEN].data(),
                      m_responses[RESPONSE_CHANNEL_BLUE].data(), NUM_BINS)) {
        throw std::runtime_error("Invalid response curve file");
    }

    m_type = RESPONSE_CUSTOM;
    return true;
}

void fillResponseLinear(ResponseCurve::ResponseContainer &response) {
    // fill response function
    float factor = 1.f / (float)(response.size() - 1);
    for (size_t i = 0; i < response.size(); ++i) {
        response[i] = (float)i * factor;
    }
    response[0] = response[1];
}

void fillResponseGamma(ResponseCurve::ResponseContainer &response) {
    size_t divider = (response.size() - 1);
    for (size_t i = 0; i < response.size(); ++i) {
        response[i] = std::pow(4.f * ((float)i / divider), 1.7f) + 1e-4;
    }
    response[0] = response[1];
}

// I use a namespace to avoid name collision
namespace details_log10 {
//const float s_mid = 0.5f;
const float s_norm = 0.0625f;
// the value 8.f is s_mid/s_norm. The actual formula is 10^((i - mid)/norm)
const float s_inverseMaxValue =
    1.f / 1e8;  // == 1.f/std::pow(10.0f, (1.f/norm - 8.f));
}

void fillResponseLog10(ResponseCurve::ResponseContainer &response) {
    size_t divider = (response.size() - 1);
    for (size_t i = 0; i < response.size(); ++i) {
        response[i] =
            details_log10::s_inverseMaxValue *
            std::pow(10.0f,
                     ((((float)i / divider) / details_log10::s_norm) - 8.f));
    }
    response[0] = response[1];
}

void fillResponseSRGB(ResponseCurve::ResponseContainer &response) {
    size_t divider = (response.size() - 1);
    pfs::colorspace::ConvertSRGB2RGB converter;

    for (size_t i = 0; i < response.size(); ++i) {
        response[i] = converter((float)i / divider);
    }
    response[0] = response[1];
}

void ResponseCurve::setType(ResponseCurveType type) {
    typedef void (*ResponseCurveCalculator)(ResponseContainer &);
    typedef map<ResponseCurveType, ResponseCurveCalculator> ResponseCurveFunc;
    static ResponseCurveFunc funcs = map_list_of(
        RESPONSE_LOG10, &fillResponseLog10)(RESPONSE_LINEAR,
                                            &fillResponseLinear)(
        RESPONSE_GAMMA, &fillResponseGamma)(RESPONSE_SRGB, &fillResponseSRGB);

    ResponseCurveType type_ = RESPONSE_LINEAR;
    ResponseCurveCalculator func_ = &fillResponseLinear;

    ResponseCurveFunc::const_iterator it = funcs.find(type);
    if (it != funcs.end()) {
        type_ = it->first;
        func_ = it->second;
    }

    m_type = type_;

    func_(m_responses[RESPONSE_CHANNEL_RED]);
    func_(m_responses[RESPONSE_CHANNEL_GREEN]);
    func_(m_responses[RESPONSE_CHANNEL_BLUE]);
}

}  // fusion
}  // libhdr

void dump_gnuplot(const char *filename, const float *array, int M) {
    FILE *fp = fopen(filename, "w");
    fprintf(fp, "# GNUPlot dump\n");
    for (int i = 0; i < M; i++) fprintf(fp, "%4d %16.9f\n", i, array[i]);
    fclose(fp);
    std::cerr << "GNUPLOT: save data to " << filename << std::endl;
}

void dump_gnuplot(const char *filename, const float *array, int M,
                  int counter) {
    char fn[2048];
    sprintf(fn, filename, counter);
    dump_gnuplot(fn, array, M);
}

void responseSave(FILE *file, const float *Ir, const float *Ig, const float *Ib,
                  int M) {
    // response curve matrix header
    fprintf(file, "# Camera response curve, channels Ir, Ig, Ib \n");
    fprintf(
        file,
        "# data layout: camera output | log10(response Ir) | response Ir | "
        "log10(response Ig) | response Ig | log10(response Ib) | response Ib "
        "\n");
    fprintf(file, "# type: matrix\n");
    fprintf(file, "# rows: %d\n", M);
    fprintf(file, "# columns: 7\n");

    // save response
    for (int m = 0; m < M; m++) {
        float logR = Ir[m] == 0.0f ? -6.0f : log10f(Ir[m]);
        float logG = Ig[m] == 0.0f ? -6.0f : log10f(Ig[m]);
        float logB = Ib[m] == 0.0f ? -6.0f : log10f(Ib[m]);
        fprintf(file, " %4d %15.9f %15.9f %15.9f %15.9f %15.9f %15.9f \n", m,
                logR, Ir[m], logG, Ig[m], logB, Ib[m]);
    }
    fprintf(file, "\n");
}

bool responseLoad(FILE *file, float *Ir, float *Ig, float *Ib, int M) {
    char line[1024];
    int m = 0, c = 0;

    // parse response curve matrix header
    while (fgets(line, 1024, file))
        if (sscanf(line, "# rows: %d\n", &m) == 1) break;
    if (m != M) {
        std::cerr << "response: number of input levels is different,"
                  << " M=" << M << " m=" << m << std::endl;
        return false;
    }
    while (fgets(line, 1024, file))
        if (sscanf(line, "# columns: %d\n", &c) == 1) break;
    if (c != 7) return false;

    // read response
    float ignoreR, ignoreG, ignoreB;
    for (int i = 0; i < M; i++) {
        float valR, valG, valB;
        if (fscanf(file, " %d %f %f %f %f %f %f \n", &m, &ignoreR, &valR,
                   &ignoreG, &valG, &ignoreB, &valB) != 7)
            return false;
        if (m < 0 || m > M)
            std::cerr << "response: camera value out of range,"
                      << " m=" << m << std::endl;
        else {
            Ir[m] = valR;
            Ig[m] = valG;
            Ib[m] = valB;
        }
    }

    return true;
}
