#include <HdrCreation/weights.h>

#include <cmath>
#include <iostream>
#include <map>
#include "arch/math.h"

#include <boost/assign.hpp>

#include <Libpfs/utils/string.h>

using namespace std;
using namespace boost;
using namespace boost::assign;

namespace libhdr {
namespace fusion {

WeightFunctionType WeightFunction::fromString(const std::string &type) {
    typedef map<string, WeightFunctionType, pfs::utils::StringUnsensitiveComp>
        Dict;
    static Dict v = map_list_of("triangular", WEIGHT_TRIANGULAR)(
        "gaussian", WEIGHT_GAUSSIAN)("plateau", WEIGHT_PLATEAU)("flat",
                                                                WEIGHT_FLAT);

    Dict::const_iterator it = v.find(type);
    if (it != v.end()) {
        return it->second;
    }
    return WEIGHT_GAUSSIAN;
}

WeightFunction::WeightFunction(WeightFunctionType type) : m_type(type) {
    setType(type);
}

namespace {
static const float s_triangularThreshold = 2.0f / WeightFunction::NUM_BINS;
}

static float getWeightTriangular(float input) {
    // ignore very low weights
    if (input < s_triangularThreshold ||
        input > 1.0f - s_triangularThreshold) {
        return 0.f;
    }
    float half = 0.5f;
    return input < half ? 2.0f * input : 2.0f * (1.0f - input);
}

static void fillWeightTriangular(WeightFunction::WeightContainer &weight) {
    size_t divider = (weight.size() - 1);
    for (size_t i = 0; i < weight.size(); ++i) {
        weight[i] = getWeightTriangular((float)i / divider);
    }
}

static float minTrustedValueTriangular() {
    return 0.f - std::numeric_limits<float>::epsilon();
}
static float maxTrustedValueTriangular() {
    return 1.f + std::numeric_limits<float>::epsilon();
}

namespace {
static const float s_gaussianThreshold = 2.0f / WeightFunction::NUM_BINS;
static const float s_mu = 0.5f;
}

static float getWeightGaussian(float input) {
    // ignore very low weights
    if (input < s_gaussianThreshold ||
        input > 1.0f - s_gaussianThreshold) {
        return 0.f;
    }
    return exp(-32.0f * (input - s_mu) * (input - s_mu));
}

static void fillWeightGaussian(WeightFunction::WeightContainer &weight) {
    size_t divider = (weight.size() - 1);
    for (size_t i = 0; i < weight.size(); ++i) {
        weight[i] = getWeightGaussian((float)i / divider);
    }
}

static float minTrustedValueGaussian() {
    return 0.f - std::numeric_limits<float>::epsilon();
}
static float maxTrustedValueGaussian() {
    return 1.f + std::numeric_limits<float>::epsilon();
}

namespace {
static const float s_plateauThreshold = 0.00025f;
}

static float getWeightPlateau(float input) {
    if ((input < s_plateauThreshold) ||
        (input > (1.f - s_plateauThreshold))) {
        return 0.f;
    }

    return 1.f - pow((2.0f * input - 1.0f), 12.0f);
}

static void fillWeightPlateau(WeightFunction::WeightContainer &weight) {
    size_t divider = (weight.size() - 1);
    for (size_t i = 0; i < weight.size(); ++i) {
        weight[i] = getWeightPlateau((float)i / divider);
    }
}

static float minTrustedValuePlateau() {
    return 0.f - std::numeric_limits<float>::epsilon();
}
static float maxTrustedValuePlateau() {
    return 1.f + std::numeric_limits<float>::epsilon();
}

static void fillWeightFlat(WeightFunction::WeightContainer &weight) {
    std::fill(weight.begin(), weight.end(), 1.f);
}

static float minTrustedValueFlat() {
    return 0.f - std::numeric_limits<float>::epsilon();
}
static float maxTrustedValueFlat() {
    return 1.f + std::numeric_limits<float>::epsilon();
}

void WeightFunction::setType(WeightFunctionType type) {
    typedef void (*WeightFunctionCalculator)(WeightContainer &);
    typedef float (*WeightTrustedValue)();

    struct WeightFunctionFiller {
        WeightFunctionCalculator fillData;
        WeightTrustedValue minTrustValue;
        WeightTrustedValue maxTrustValue;
    };

    typedef std::map<WeightFunctionType, WeightFunctionFiller>
        WeightFunctionFunc;
    WeightFunctionFiller fillter_t = {&fillWeightTriangular,
                                      &minTrustedValueTriangular,
                                      &maxTrustedValueTriangular};
    WeightFunctionFiller fillter_g = {&fillWeightGaussian,
                                      &minTrustedValueGaussian,
                                      &maxTrustedValueGaussian};
    WeightFunctionFiller fillter_p = {
        &fillWeightPlateau, &minTrustedValuePlateau, &maxTrustedValuePlateau};
    WeightFunctionFiller fillter_f = {&fillWeightFlat, &minTrustedValueFlat,
                                      &maxTrustedValueFlat};
    static WeightFunctionFunc funcs =
        map_list_of(WEIGHT_TRIANGULAR, fillter_t)(WEIGHT_GAUSSIAN, fillter_g)(
            WEIGHT_PLATEAU, fillter_p)(WEIGHT_FLAT, fillter_f);

    WeightFunctionType type_ = WEIGHT_TRIANGULAR;
    WeightFunctionFiller func_ = {&fillWeightTriangular,
                                  &minTrustedValueTriangular,
                                  &maxTrustedValueTriangular};

    WeightFunctionFunc::const_iterator it = funcs.find(type);
    if (it != funcs.end()) {
        type_ = it->first;
        func_ = it->second;
    }

    m_type = type_;
    func_.fillData(m_weights);
    m_minTrustedValue = func_.minTrustValue();
    m_maxTrustedValue = func_.maxTrustValue();
}

}  // fusion
}  // libhdr

bool weightsLoad(FILE *file, float *w, int M) {
    char line[1024];
    int m = 0, c = 0;

    // parse weighting function matrix header
    while (fgets(line, 1024, file))
        if (sscanf(line, "# rows: %d\n", &m) == 1) break;
    if (m != M) {
        std::cerr << "response: number of input levels is different,"
                  << " M=" << M << " m=" << m << std::endl;
        return false;
    }
    while (fgets(line, 1024, file))
        if (sscanf(line, "# columns: %d\n", &c) == 1) break;
    if (c != 2) return false;

    // read response
    for (int i = 0; i < M; i++)
        if (fscanf(file, " %f %d\n", &(w[i]), &m) != 2) return false;

    return true;
}

void weightsSave(FILE *file, const float *w, int M, const char *name) {
    // weighting function matrix header
    fprintf(file, "# Weighting function\n");
    fprintf(file, "# data layout: weight | camera output\n");
    fprintf(file, "# name: %s\n", name);
    fprintf(file, "# type: matrix\n");
    fprintf(file, "# rows: %d\n", M);
    fprintf(file, "# columns: 2\n");

    // save weights
    for (int m = 0; m < M; m++) fprintf(file, " %15.9f %4d\n", w[m], m);

    fprintf(file, "\n");
}
