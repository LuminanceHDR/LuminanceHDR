#include <HdrCreation/weights.h>

#include "arch/math.h"
#include <cmath>
#include <map>
#include <iostream>

#include <boost/assign.hpp>

using namespace boost;
using namespace boost::assign;

namespace libhdr {
namespace fusion {

WeightFunction::WeightFunction(WeightFunctionType type)
    : m_type(type)
{
    setType(type);
}

float WeightFunction::getWeight(float input) const
{
    assert(input >= 0.f);
    assert(input <= 1.f);

    return m_weights[getIdx(input)];
}

namespace {
static const float s_triangularThreshold = 0.03f;
}

static float getWeightTriangular(float input)
{
    if ( (input < s_triangularThreshold) || (input > (1.f - s_triangularThreshold)) ) {
        return 0.f;
    }

    input *= 2.f;
    if ( input >= 1.f ) {
        input = 2.f - input;
    }
    return input;
}

static void fillWeightTriangular(WeightFunction::WeightContainer& weight)
{
    size_t divider = (weight.size() - 1);
    for (size_t i = 0; i < weight.size(); ++i)
    {
        weight[i] = getWeightTriangular((float)i/divider);
    }
}

static float minTrustedValueTriangular() { return s_triangularThreshold; }
static float maxTrustedValueTriangular() { return 1.f - s_triangularThreshold; }

namespace {
static const float s_gaussianThreshold = 0.0354f;
static const float s_mu = 0.5f;
}

static float getWeightGaussian(float input)
{
    // ignore very low weights
    if ( (input < s_gaussianThreshold) || (input > (1.f - s_gaussianThreshold)) ) {
        return 0.f;
    }

    return (exp( -32*(input - s_mu)*(input - s_mu) ));
}

static void fillWeightGaussian(WeightFunction::WeightContainer& weight)
{
    size_t divider = (weight.size() - 1);
    for (size_t i = 0; i < weight.size(); ++i)
    {
        weight[i] = getWeightGaussian((float)i/divider);
    }
}

static float minTrustedValueGaussian() { return s_gaussianThreshold; }
static float maxTrustedValueGaussian() { return 1.f - s_gaussianThreshold; }

namespace {
static const float s_plateauThreshold = 0.005f;
}

static float getWeightPlateau(float input)
{
    if ((input < s_plateauThreshold) || (input > (1.f - s_plateauThreshold)))
    {
        return 0.f;
    }

    return 1.0f - pow( (2.0f*input - 1.0f), 12.0f);
}

static void fillWeightPlateau(WeightFunction::WeightContainer& weight)
{
    size_t divider = (weight.size() - 1);
    for (size_t i = 0; i < weight.size(); ++i)
    {
        weight[i] = getWeightPlateau((float)i/divider);
    }
}

static float minTrustedValuePlateau()   { return s_plateauThreshold; }
static float maxTrustedValuePlateau()   { return 1.f - s_plateauThreshold; }

static void fillWeightFlat(WeightFunction::WeightContainer& weight)
{
    std::fill(weight.begin(), weight.end(), 1.f);
}

static float minTrustedValueFlat()   { return 0.f - std::numeric_limits<float>::epsilon(); }
static float maxTrustedValueFlat()   { return 1.f + std::numeric_limits<float>::epsilon(); }

void WeightFunction::setType(WeightFunctionType type)
{
    typedef void (*WeightFunctionCalculator)(WeightContainer&);
    typedef float (*WeightTrustedValue)();

    struct WeightFunctionFiller
    {
        WeightFunctionFiller(WeightFunctionCalculator calculator,
                             WeightTrustedValue minValue,
                             WeightTrustedValue maxValue)
            : fillData(calculator)
            , minTrustValue(minValue)
            , maxTrustValue(maxValue)
        {}

        WeightFunctionCalculator fillData;
        WeightTrustedValue minTrustValue;
        WeightTrustedValue maxTrustValue;
    };

    typedef std::map<WeightFunctionType, WeightFunctionFiller> WeightFunctionFunc;
    static WeightFunctionFunc funcs =
            map_list_of
            (WEIGHT_TRIANGULAR, WeightFunctionFiller(&fillWeightTriangular, &minTrustedValueTriangular, &maxTrustedValueTriangular))
            (WEIGHT_GAUSSIAN, WeightFunctionFiller(&fillWeightGaussian, &minTrustedValueGaussian, &maxTrustedValueGaussian))
            (WEIGHT_PLATEAU, WeightFunctionFiller(&fillWeightPlateau, &minTrustedValuePlateau, &maxTrustedValuePlateau))
            (WEIGHT_FLAT, WeightFunctionFiller(&fillWeightFlat, &minTrustedValueFlat, &maxTrustedValueFlat))
            ;

    WeightFunctionType type_ = WEIGHT_TRIANGULAR;
    WeightFunctionFiller func_ = {&fillWeightTriangular, &minTrustedValueTriangular, &maxTrustedValueTriangular};

    WeightFunctionFunc::const_iterator it = funcs.find(type);
    if (it != funcs.end())
    {
        type_ = it->first;
        func_ = it->second;
    }

    m_type = type_;
    func_.fillData(m_weights);
    m_minTrustedValue = func_.minTrustValue();
    m_maxTrustedValue = func_.maxTrustValue();
}


}   // fusion
}   // libhdr

bool weightsLoad( FILE* file, float* w, int M)
{
    char line[1024];
    int m=0,c=0;

    // parse weighting function matrix header
    while( fgets(line, 1024, file) )
        if( sscanf(line, "# rows: %d\n", &m) == 1 )
            break;
    if( m!=M )
    {
        std::cerr << "response: number of input levels is different,"
                  << " M=" << M << " m=" << m << std::endl;
        return false;
    }
    while( fgets(line, 1024, file) )
        if( sscanf(line, "# columns: %d\n", &c) == 1 )
            break;
    if( c!=2 )
        return false;

    // read response
    for( int i=0 ; i<M ; i++ )
        if( fscanf(file, " %f %d\n", &(w[i]), &m) !=2 )
            return false;

    return true;
}

void weightsSave( FILE* file, const float* w, int M, const char* name)
{
    // weighting function matrix header
    fprintf(file, "# Weighting function\n");
    fprintf(file, "# data layout: weight | camera output\n");
    fprintf(file, "# name: %s\n", name);
    fprintf(file, "# type: matrix\n");
    fprintf(file, "# rows: %d\n", M);
    fprintf(file, "# columns: 2\n");

    // save weights
    for( int m=0 ; m<M ; m++ )
        fprintf(file, " %15.9f %4d\n", w[m], m);

    fprintf(file, "\n");
}
