/**
 * This file is a part of Luminance HDR package
 * ----------------------------------------------------------------------
 * Copyright (C) 2006,2007 Giuseppe Rota
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
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 */

#include <cassert>
#include <iostream>

#include "HdrCreation/debevec.h"

/*
//anti-saturation shouldn't be needed
int debevec_applyResponse( const float * arrayofexptime,
                           pfs::Array2D* xj, const float* I1,
                           pfs::Array2D* yj, const float* I2,
                           pfs::Array2D* zj, const float* I3,
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

void computeTrustRange(const float* w, int M, int& minM, int& maxM)
{
    for ( int m = 0 ; m < M ; ++m ) {
        if ( w[m] > 0 ) {
            minM = m;
            break;
        }
    }

    for ( int m = M-1 ; m >= 0 ; --m ) {
        if ( w[m] > 0 ) {
            maxM = m;
            break;
        }
    }
}

template <typename InputDataAdapter>
int debevecApplyResponseCore(pfs::Array2D& xj,  pfs::Array2D& yj, pfs::Array2D& zj,
                             const float* arrayofexptime,
                             const float* Ir, const float* Ig, const float* Ib,
                             const float* w, int M,
                             const InputDataAdapter& inputData)
{
    int N       = inputData.numFrames();
    int width   = xj.getCols();
    int height  = xj.getRows();

    // number of saturated pixels
    int saturated_pixels = 0;

    // compute trust area weights
    int minM = 0;
    int maxM = M-1;
    computeTrustRange(w, M, minM, maxM);

    ///////////////////////////////// HDR INPUT
    // for all pixels
    for (int j = 0 ; j < width*height ; ++j)
    {
        float sumR = 0.0f;
        float sumG = 0.0f;
        float sumB = 0.0f;
        float divR = 0.0f;
        float divG = 0.0f;
        float divB = 0.0f;
        float maxti = -1e6f;
        float minti = +1e6f;
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
            int mR = inputData.getRed(i, j);
            int mG = inputData.getGreen(i, j);
            int mB = inputData.getBlue(i, j);
            float ti = arrayofexptime[i];

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
            if (maxti>-1e6f) {
                sumR = Ir[index_for_blackR] / float(maxti);
                sumG = Ig[index_for_blackG] / float(maxti);
                sumB = Ib[index_for_blackB] / float(maxti);
                divR = divG = divB = 1.0f;
            }
            if (minti<+1e6f) {
                sumR = Ir[index_for_whiteR] / float(minti);
                sumG = Ig[index_for_whiteG] / float(minti);
                sumB = Ib[index_for_whiteB] / float(minti);
                divR = divG = divB = 1.0f;
            }
        }

        if ( divR != 0.0f && divG != 0.0f && divB != 0.0f ) {
            xj(j) = sumR/divR;
            yj(j) = sumG/divG;
            zj(j) = sumB/divB;
        } else {
            // we shouldn't be here anyway...
            xj(j) = 0.0f;
            yj(j) = 0.0f;
            zj(j) = 0.0f;
        }
    } // END for all pixels

    return saturated_pixels;
}

struct Array2DListAdapter
{
    Array2DListAdapter(const Array2DList& listhdrR, const Array2DList& listhdrG, const Array2DList& listhdrB)
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
    const Array2DList& m_listhdrR;
    const Array2DList& m_listhdrG;
    const Array2DList& m_listhdrB;
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
int debevec_applyResponse(pfs::Array2D& xj, pfs::Array2D& yj, pfs::Array2D& zj,
                          const float* arrayofexptime,
                          const float* Ir, const float* Ig, const float* Ib,
                          const float* w, int M,
                          const QList<QImage*>& list)
{
#ifndef NDEBUG
    std::cerr << "debevec fusion, LDR" << std::endl;
#endif

    assert(list.size() != 0);
    assert(M == 256);

    return debevecApplyResponseCore(xj, yj, zj,
                                    arrayofexptime, Ir, Ig, Ib,
                                    w, M,
                                    QImageQListAdapter(list));
}

// HDR version
int debevec_applyResponse(pfs::Array2D& xj,  pfs::Array2D& yj, pfs::Array2D& zj,
                          const float* arrayofexptime,
                          const float* Ir, const float* Ig, const float* Ib,
                          const float* w, int M,
                          const Array2DList& listhdrR, const Array2DList& listhdrG, const Array2DList& listhdrB)
{
#ifndef NDEBUG
    std::cerr << "debevec fusion, HDR" << std::endl;
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
