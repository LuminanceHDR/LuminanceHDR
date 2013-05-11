/*
 * This file is a part of Luminance HDR package
 * ----------------------------------------------------------------------
 * Copyright (C) 2006,2007 Giuseppe Rota
 * Copyright (C) 2013 Davide Anastasia
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
 *
 */

//! \author Giuseppe Rota <grota@users.sourceforge.net>
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#include <cassert>
#include <iostream>
#include <vector>
#include <boost/numeric/conversion/bounds.hpp>
#include <boost/limits.hpp>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "HdrCreation/debevec.h"
#include "Libpfs/array2d.h"

#ifndef NDEBUG
#define PRINT_DEBUG(str) std::cerr << "Debevec: " << str << std::endl
#else
#define PRINT_DEBUG(str)
#endif

using namespace pfs;
using namespace std;

namespace libhdr {
namespace fusion {

typedef vector<float*> DataList;

struct ColorData {
    ColorData()
        : numerator_(0.f)
        , denominator_(0.f)
        , blackValue_(0.f)
        , whiteValue_(1.f)
    {}

    float value() {
        return ((denominator_ != 0.0f) ? numerator_/denominator_ : 0.0f);
    }

    float numerator_;
    float denominator_;

    float blackValue_;
    float whiteValue_;
};

static
void fillDataLists(const vector<FrameEnhanced> &frames,
                   DataList& redChannels, DataList& greenChannels, DataList& blueChannels)
{
    assert(frames.size() == redChannels.size());
    assert(frames.size() == greenChannels.size());
    assert(frames.size() == blueChannels.size());

    // build temporary data structure
    for ( size_t exp = 0; exp < frames.size(); ++exp )
    {
        Channel* red;
        Channel* green;
        Channel* blue;
        frames[exp].frame()->getXYZChannels(red, green, blue);

        redChannels[exp] = red->data();
        greenChannels[exp] = green->data();
        blueChannels[exp] = blue->data();
    }
}

void DebevecOperator::computeFusion(const vector<FrameEnhanced> &frames,
                                    pfs::Frame &frame) const
{
    assert(frames.size());

    size_t numExposures = frames.size();
    Frame tempFrame ( frames[0].frame()->getWidth(), frames[0].frame()->getHeight() );

    Channel* outputRed;
    Channel* outputGreen;
    Channel* outputBlue;
    tempFrame.createXYZChannels(outputRed, outputGreen, outputBlue);

    DataList redChannels(numExposures);
    DataList greenChannels(numExposures);
    DataList blueChannels(numExposures);

    fillDataLists(frames, redChannels, greenChannels, blueChannels);

    // size_t saturated_pixels = 0;
    float maxAllowedValue = maxTrustedValue();
    float minAllowedValue = minTrustedValue();

#pragma omp parallel for
    for (size_t idx = 0; idx < tempFrame.size(); ++idx)
    {
        // data...
        ColorData redData;
        ColorData greenData;
        ColorData blueData;
        //
        float maxAvgLum = boost::numeric::bounds<float>::lowest();
        float minAvgLum = boost::numeric::bounds<float>::highest();
        //
        // for all exposures
        for (size_t exp = 0; exp < numExposures; ++exp)
        {
            // average luminance for this exposure
            float avgLum    = frames[exp].averageLuminance();
            // pick the 3 channel values
            float red       = redChannels[exp][idx];
            float green     = greenChannels[exp][idx];
            float blue      = blueChannels[exp][idx];

            float w_red     = weight(red);
            float w_green   = weight(green);
            float w_blue    = weight(blue);

            // if at least one of the color channel's values are in the bright
            // "not-trusted zone" and we have min exposure time
            //            if ( (mR > maxM || mG > maxM || mB > maxM) && (ti < minti) ) {
            //                // update the indexes_for_whiteRGB, minti
            //                index_for_whiteR = mR;
            //                index_for_whiteG = mG;
            //                index_for_whiteB = mB;
            //                minti = ti;
            //            }

            // if at least one of the color channel's values are in the dim
            // "not-trusted zone" and we have max exposure time
            //            if ( (mR < minM || mG < minM || mB < minM) && (ti > maxti) ) {
            //                // update the indexes_for_blackRGB, maxti
            //                index_for_blackR = mR;
            //                index_for_blackG = mG;
            //                index_for_blackB = mB;
            //                maxti = ti;
            //            }

            // if at least one of the color channel's values are in the bright
            // "untrusted zone" and we have min exposure time
            // check red channel
            if ( (avgLum < minAvgLum) &&
                 ((red > maxAllowedValue) || (green > maxAllowedValue) || (blue > maxAllowedValue)) )
            {
                // PRINT_DEBUG("new min average luminance");

                minAvgLum               = avgLum;
                redData.blackValue_     = red;
                greenData.blackValue_   = green;
                blueData.blackValue_    = blue;
            }

            // if at least one of the color channel's values are in the dim
            // "not-trusted zone" and we have max exposure time
            if ( (avgLum > maxAvgLum) &&
                 ((red < minAllowedValue) || (green < minAllowedValue) || (blue < minAllowedValue)) )
            {
                // PRINT_DEBUG("new max average luminance");

                maxAvgLum               = avgLum;
                redData.whiteValue_     = red;
                greenData.whiteValue_   = green;
                blueData.whiteValue_    = blue;
            }

            // float w_average = (w_red + w_green + w_blue)/3.0f;
            redData.numerator_      += (w_red * response(red))/avgLum;
            redData.denominator_    += w_red;
            greenData.numerator_    += (w_green * response(green))/avgLum;
            greenData.denominator_  += w_green;
            blueData.numerator_     += (w_blue * response(blue))/avgLum;
            blueData.denominator_   += w_blue;
        }
        // END for all the exposures

        //            if ( divR==0.0f || divG==0.0f || divB==0.0f ) {
        //                saturated_pixels++;
        //                if ( maxti > boost::numeric::bounds<float>::lowest() ) {
        //                    sumR = Ir[index_for_blackR] / maxti;
        //                    sumG = Ig[index_for_blackG] / maxti;
        //                    sumB = Ib[index_for_blackB] / maxti;
        //                    divR = divG = divB = 1.0f;
        //                }
        //                if ( minti < boost::numeric::bounds<float>::highest() ) {
        //                    sumR = Ir[index_for_whiteR] / minti;
        //                    sumG = Ig[index_for_whiteG] / minti;
        //                    sumB = Ib[index_for_whiteB] / minti;
        //                    divR = divG = divB = 1.0f;
        //                }
        //            }

        // ...fix red...
        if ( (redData.denominator_ == 0.f) || (greenData.denominator_ == 0.f) || (blueData.denominator_ == 0.f) ) {
            // ++saturated_pixels;

            PRINT_DEBUG("saturated pixel");

            if ( maxAvgLum > boost::numeric::bounds<float>::lowest() )
            {
                redData.numerator_      = response(redData.blackValue_) / maxAvgLum;
                greenData.numerator_    = response(greenData.blackValue_) / maxAvgLum;
                blueData.numerator_     = response(blueData.blackValue_) / maxAvgLum;

                redData.denominator_    = 1.f;
                greenData.denominator_  = 1.f;
                blueData.denominator_   = 1.f;
            }

            if ( minAvgLum < boost::numeric::bounds<float>::highest() )
            {
                redData.numerator_      = response(redData.whiteValue_) / minAvgLum;
                greenData.numerator_    = response(greenData.whiteValue_) / minAvgLum;
                blueData.numerator_     = response(blueData.whiteValue_) / minAvgLum;

                redData.denominator_    = 1.f;
                greenData.denominator_  = 1.f;
                blueData.denominator_   = 1.f;
            }
        }

        (*outputRed)(idx)   = redData.value();
        (*outputGreen)(idx) = greenData.value();
        (*outputBlue)(idx)  = blueData.value();
    }

    frame.swap( tempFrame );

    //return saturated_pixels;
}

}   // libhdr
}   // fusion


/*
//anti-saturation shouldn't be needed
int debevec_applyResponse( const float * arrayofexptime,
                           pfs::Array2Df* xj, const float* I1,
                           pfs::Array2Df* yj, const float* I2,
                           pfs::Array2Df* zj, const float* I3,
                           const Array2DList &P,
                           const bool ldrinput, ... )
{
    int N=-1;
    QList<QImage*> *list=NULL;
    // Array2DList *listhdrR=NULL;
    // Array2DList *listhdrG=NULL;
    // Array2DList *listhdrB=NULL;
    va_list arg_pointer;
    va_start(arg_pointer,ldrinput); // Initialize the argument list.

    // if (ldrinput) {
    //TODO hdr input...
    assert(ldrinput);
    list=va_arg(arg_pointer,QList<QImage*>*);
    // number of exposures
    N = list->count();
    // } else {
    // 	listhdrR=va_arg(arg_pointer,Array2DList*);
    // 	listhdrG=va_arg(arg_pointer,Array2DList*);
    // 	listhdrB=va_arg(arg_pointer,Array2DList*);
    // 	// number of exposures
    // 	N = listhdrR->size();
    // }
    va_end(arg_pointer); // Clean up.


    // frame size
    int width = xj->getCols();
    int height = xj->getRows();

    // number of saturated pixels
    int saturated_pixels = 0;

    // for all pixels
    for (int j=0 ; j<width*height ; ++j)
    {
        float sum1 = 0.0f;
        float sum2 = 0.0f;
        float sum3 = 0.0f;

        float div1 = 0.0f;
        float div2 = 0.0f;
        float div3 = 0.0f;

        // for all exposures for each pixel
        for( int i=0 ; i<N ; i++ ) {
            //pick the 3 channel values
            int m1 = qRed(* ( (QRgb*)( (list->at(i) )->bits() ) + j ) );
            int m2 = qGreen(* ( (QRgb*)( (list->at(i) )->bits() ) + j ) );
            int m3 = qBlue(* ( (QRgb*)( (list->at(i) )->bits() ) + j ) );

            float ti = arrayofexptime[i];

            sum1 += (*P[i])(j) * I1[m1] / float(ti);
            div1 += (*P[i])(j);
            sum2 += (*P[i])(j) * I2[m2] / float(ti);
            div2 += (*P[i])(j);
            sum3 += (*P[i])(j) * I3[m3] / float(ti);
            div3 += (*P[i])(j);
        } //END for all the exposures

        if( div1==0.0f || div2==0.0f || div3==0.0f ) {
            saturated_pixels++;
        }

        if( div1!=0.0f && div2!=0.0f && div3!=0.0f ) {
            (*xj)(j) = sum1/div1;
            (*yj)(j) = sum2/div2;
            (*zj)(j) = sum3/div3;
        }
        else {
            (*xj)(j) = 0.0f;
            (*yj)(j) = 0.0f;
            (*zj)(j) = 0.0f;
        }
    }
    return saturated_pixels;
}
*/

/*


typedef vector<int> VectorInt;

#ifdef NDEBUG
inline
#endif
int debevecBuildPixel(float& outRed, float& outGreen, float& outBlue,
                      const float* arrayofexptime,
                      const float* Ir, const float* Ig, const float* Ib,
                      const float* w, int minM, int maxM,
                      const VectorInt& inReds, const VectorInt& inGreens, const VectorInt& inBlues)
{
    int N = inReds.size();
    int saturated_pixels = 0;

    float sumR           = 0.0f;
    float sumG           = 0.0f;
    float sumB           = 0.0f;
    float divR           = 0.0f;
    float divG           = 0.0f;
    float divB           = 0.0f;
    float maxti          = boost::numeric::bounds<float>::lowest();
    float minti          = boost::numeric::bounds<float>::highest();
    int index_for_whiteR = -1;
    int index_for_whiteG = -1;
    int index_for_whiteB = -1;
    int index_for_blackR = -1;
    int index_for_blackG = -1;
    int index_for_blackB = -1;

    // for all exposures
    for (int i = 0; i < N; ++i)
    {
        // pick the 3 channel values
        int mR      = inReds[i];
        int mG      = inGreens[i];
        int mB      = inBlues[i];
        float ti    = arrayofexptime[i];

        // if at least one of the color channel's values are in the bright
        // "not-trusted zone" and we have min exposure time
        if ( (mR > maxM || mG > maxM || mB > maxM) && (ti < minti) ) {
            // update the indexes_for_whiteRGB, minti
            index_for_whiteR = mR;
            index_for_whiteG = mG;
            index_for_whiteB = mB;
            minti = ti;
        }

        // if at least one of the color channel's values are in the dim
        // "not-trusted zone" and we have max exposure time
        if ( (mR < minM || mG < minM || mB < minM) && (ti > maxti) ) {
            // update the indexes_for_blackRGB, maxti
            index_for_blackR = mR;
            index_for_blackG = mG;
            index_for_blackB = mB;
            maxti = ti;
        }
        float w_average=(w[mR] + w[mG] + w[mB])/3.0f;
        sumR += w_average * Ir[mR] / float(ti);
        divR += w_average;
        sumG += w_average * Ig[mG] / float(ti);
        divG += w_average;
        sumB += w_average * Ib[mB] / float(ti);
        divB += w_average;

    } // END for all the exposures

    if ( divR==0.0f || divG==0.0f || divB==0.0f ) {
        saturated_pixels++;
        if ( maxti > boost::numeric::bounds<float>::lowest() ) {
            sumR = Ir[index_for_blackR] / maxti;
            sumG = Ig[index_for_blackG] / maxti;
            sumB = Ib[index_for_blackB] / maxti;
            divR = divG = divB = 1.0f;
        }
        if ( minti < boost::numeric::bounds<float>::highest() ) {
            sumR = Ir[index_for_whiteR] / minti;
            sumG = Ig[index_for_whiteG] / minti;
            sumB = Ib[index_for_whiteB] / minti;
            divR = divG = divB = 1.0f;
        }
    }

    if ( divR != 0.0f && divG != 0.0f && divB != 0.0f ) {
        outRed = sumR/divR;
        outGreen = sumG/divG;
        outBlue = sumB/divB;
    } else {
        // we shouldn't be here anyway...
        outRed = 0.0f;
        outGreen = 0.0f;
        outBlue = 0.0f;
    }

    return saturated_pixels;
}

template <typename InputDataAdapter>
int debevecApplyResponseCore(pfs::Array2Df& xj,  pfs::Array2Df& yj, pfs::Array2Df& zj,
                             const float* arrayofexptime,
                             const float* Ir, const float* Ig, const float* Ib,
                             const float* w, int M,
                             const InputDataAdapter& inputData)
{
    int N       = inputData.numFrames();
    int width   = xj.getCols();
    int height  = xj.getRows();

    // number of saturated pixels
    int saturatedPixels = 0;

    // compute trust area weights
    int minM = 0;
    int maxM = M-1;
    computeTrustRange(w, M, minM, maxM);

    // private variables for each thread...
    VectorInt inputRed(N);
    VectorInt inputGreen(N);
    VectorInt inputBlue(N);

    // for all pixels
// #pragma omp parallel for reduction(+:saturatedPixels) \
//    private(inputRed, inputGreen, inputBlue) \
//    shared(xj, yj, zj, arrayofexptime, Ir, Ig, Ib, w, M, minM, maxM, N)
    for (int j = 0; j < width*height; ++j)
    {
        // accumulate pixel values
        for (int i = 0; i < N; ++i)
        {
            inputRed[i] = inputData.getRed(i, j);
            inputGreen[i] = inputData.getGreen(i, j);
            inputBlue[i] = inputData.getBlue(i, j);
        }

        saturatedPixels += debevecBuildPixel(xj(j), yj(j), zj(j),
                                             arrayofexptime, Ir, Ig, Ib,
                                             w, minM, maxM,
                                             inputRed, inputGreen, inputBlue);
    } // all pixels

    return saturatedPixels;
}

struct Array2DListAdapter
{
    Array2DListAdapter(const Array2DfList& listhdrR,
                       const Array2DfList& listhdrG,
                       const Array2DfList& listhdrB)
        : m_listhdrR(listhdrR)
        , m_listhdrG(listhdrG)
        , m_listhdrB(listhdrB)
    {}

    int numFrames() const {
        return m_listhdrR.size();
    }

    int getRed(int frameIdx, int pixelIdx) const {
        return static_cast<int>( (*(m_listhdrR[frameIdx]))(pixelIdx) );
    }

    int getGreen(int frameIdx, int pixelIdx) const {
        return static_cast<int>( (*(m_listhdrG[frameIdx]))(pixelIdx) );
    }

    int getBlue(int frameIdx, int pixelIdx) const {
        return static_cast<int>( (*(m_listhdrB[frameIdx]))(pixelIdx) );
    }

private:
    const Array2DfList& m_listhdrR;
    const Array2DfList& m_listhdrG;
    const Array2DfList& m_listhdrB;
};

struct QImageQListAdapter
{
    QImageQListAdapter(const QList<QImage*>& list)
        : m_list(list)
    {}

    int numFrames() const {
        return m_list.size();
    }

    int getRed(int frameIdx, int pixelIdx) const {
        return qRed( reinterpret_cast<const QRgb*>(m_list.at(frameIdx)->bits())[pixelIdx] );
    }

    int getGreen(int frameIdx, int pixelIdx) const {
        return qGreen( reinterpret_cast<const QRgb*>(m_list.at(frameIdx)->bits())[pixelIdx] );
    }

    int getBlue(int frameIdx, int pixelIdx) const {
        return qBlue( reinterpret_cast<const QRgb*>(m_list.at(frameIdx)->bits())[pixelIdx] );
    }

private:
    const QList<QImage*>& m_list;
};

// LDR version
int debevec_applyResponse(pfs::Array2Df& xj, pfs::Array2Df& yj, pfs::Array2Df& zj,
                          const float* arrayofexptime,
                          const float* Ir, const float* Ig, const float* Ib,
                          const float* w, int M,
                          const QList<QImage*>& list)
{
#ifndef NDEBUG
    cerr << "debevec fusion, LDR\n";
#endif

    assert(list.size() != 0);
    assert(M == 256);

    return debevecApplyResponseCore(xj, yj, zj,
                                    arrayofexptime, Ir, Ig, Ib,
                                    w, M,
                                    QImageQListAdapter(list));
}

// HDR version
int debevec_applyResponse(pfs::Array2Df& xj,  pfs::Array2Df& yj, pfs::Array2Df& zj,
                          const float* arrayofexptime,
                          const float* Ir, const float* Ig, const float* Ib,
                          const float* w, int M,
                          const Array2DfList& listhdrR, const Array2DfList& listhdrG, const Array2DfList& listhdrB)
{
#ifndef NDEBUG
    cerr << "debevec fusion, HDR\n";
#endif

    assert(listhdrR.size() != 0);
    assert(listhdrB.size() == listhdrG.size());
    assert(listhdrB.size() == listhdrR.size());
    assert(M == (int)(1 << 16));

    return debevecApplyResponseCore(xj, yj, zj,
                                    arrayofexptime, Ir, Ig, Ib,
                                    w, M,
                                    Array2DListAdapter(listhdrR, listhdrG, listhdrB));
}
*/
