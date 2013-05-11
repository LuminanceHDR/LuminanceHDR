#include <HdrCreation/weights.h>

#include "arch/math.h"
#include <cmath>

namespace libhdr {
namespace fusion {

namespace {
static const float s_triangularThreshold = 0.03f;
}

float WeightTriangular::getWeight(float input) const
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

float WeightTriangular::minTrustedValue() const {
    return s_triangularThreshold;
}

float WeightTriangular::maxTrustedValue() const {
    return 1.f - s_triangularThreshold;
}

namespace {
static const float s_gaussianThreshold = 0.0354f;
static const float s_mu = 0.5f;
}

float WeightGaussian::getWeight(float input) const
{
    // ignore very low weights
    if ( (input < s_gaussianThreshold) || (input > (1.f - s_gaussianThreshold)) ) {
        return 0.f;
    }

    return (exp( -32*(input - s_mu)*(input - s_mu) ));
}

float WeightGaussian::minTrustedValue() const {
    return s_gaussianThreshold;
}

float WeightGaussian::maxTrustedValue() const {
    return 1.f - s_gaussianThreshold;
}

namespace {
static const float s_plateauThreshold = 0.005f;
}

float WeightPlateau::getWeight(float input) const
{
    if ( input < s_plateauThreshold || (input > (1.f - s_plateauThreshold)) ) {
        return 0.f;
    }

    return 1.0f - pow( (2.0f*input - 1.0f), 12.0f);
}

float WeightPlateau::minTrustedValue() const {
    return s_plateauThreshold;
}

float WeightPlateau::maxTrustedValue() const {
    return 1.f - s_plateauThreshold;
}

}   // fusion
}   // libhdr


// OLD STUFF
#define MIN_WEIGHT 1e-3

void exposure_weights_icip06( float* w, int M, int Mmin, int Mmax )
{
    for( int m=0 ; m<M ; m++ )
        if( m<Mmin || m>Mmax )
            w[m] = 0.0f;
        else
            w[m]=1.0f-pow( ( (2.0f*float(m-Mmin)/float(Mmax-Mmin)) - 1.0f), 12.0f);
}

void weightsGauss( float* w, int M, int Mmin, int Mmax, float sigma )
{
    float mid = Mmin + (Mmax-Mmin)/2.0f - 0.5f;
    float mid2 = (mid-Mmin) * (mid-Mmin);
    for( int m=0 ; m<M ; m++ ) {
        if ( m<Mmin || m>Mmax ) {
            w[m] = 0.0f;
        } else {
            // gkrawczyk: that's not really a gaussian, but equation is
            // taken from Robertson02 paper.
            float weight = exp( -sigma * (m-mid) * (m-mid) / mid2 );

            if( weight < MIN_WEIGHT )           // ignore very low weights
                w[m] = 0.0f;
            else
                w[m] = weight;
        }
    }
}

void weights_triangle( float* w, int M /*, int Mmin, int Mmax*/ )
{
    for(int i=0;i<int(float(M)/2.0f);i++) {
        w[i]=i/ (float(M)/2.0f);
        if (w[i]<0.06f)w[i]=0;
    }
    for(int i=int(float(M)/2.0f);i<M;i++) {
        w[i]=(M-1-i)/(float(M)/2.0f);
        if (w[i]<0.06f)w[i]=0;
    }
    //   for( int m=0 ; m<M ; m++ )
    //     if( m<Mmin || m>Mmax )
    //       w[m] = 0.0f;
    //     else
    //     {
    // 	if ( m<int(Mmin+ (Mmax-Mmin)/2.0f +1) )
    // 		w[m]=(m-Mmin)/float(Mmin+(Mmax-Mmin)/2.0f);
    // 	else
    // 		w[m]=( -m+Mmin+((Mmax-Mmin)) )/float(Mmin+(Mmax-Mmin)/2.0f);
    //     }

    // 	  if (w[i]<0.06f)w[i]=0;
}
