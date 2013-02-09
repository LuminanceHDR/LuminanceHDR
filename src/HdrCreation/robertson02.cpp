/**
 * @brief Robertson02 algorithm for automatic self-calibration.
 *
 * This file is a part of Luminance HDR package
 * ----------------------------------------------------------------------
 * Copyright (C) 2004 Grzegorz Krawczyk
 * Copyright (C) 2006-2007 Giuseppe Rota
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
 * @author Grzegorz Krawczyk, <gkrawczyk@users.sourceforge.net>
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 *
 * $Id: robertson02.cpp,v 1.7 2006/11/16 15:06:17 gkrawczyk Exp $
 */

#include "arch/math.h"

#include <cassert>
#include <iostream>
#include <vector>
#include <algorithm>

#include "HdrCreation/responses.h"
#include "HdrCreation/robertson02.h"

namespace {

// maximum iterations after algorithm accepts local minima
const int MAXIT = 35; //500;

// maximum accepted error
const float MAX_DELTA = 1e-3f; //1e-5f;

float normalizeI(float* I, int M)
{
    int Mmin, Mmax;
    // find min max
    for (Mmin=0 ; Mmin<M && I[Mmin]==0 ; ++Mmin);
    for (Mmax=M-1 ; Mmax>0 && I[Mmax]==0 ; --Mmax);

    int Mmid = Mmin+(Mmax-Mmin)/2;
    float mid = I[Mmid];

#ifndef NDEBUG
    std::cerr << "robertson02: middle response, mid = " << mid
              << " [" << Mmid << "]"
              << " " << Mmin << ".." << Mmax << std::endl;
#endif

    if ( mid == 0.0f ) {
        // find first non-zero middle response
        while ( Mmid < Mmax && I[Mmid] == 0.0f ) { Mmid++; }
        mid = I[Mmid];
    }

    if ( mid != 0.0f ) {
        for (int m = 0; m < M; ++m) {
            I[m] /= mid;
        }
    }
    return mid;
}

typedef int (*PixelToChannelMapper)(QRgb);

static PixelToChannelMapper s_pixelToChannel[] = { &qRed, &qGreen, &qBlue };

}

int robertson02_applyResponse(pfs::Array2D* xj, const float * arrayofexptime,
                              const float* I, const float* w, int M, int channelRGB, bool ldrinput, ... )
{
    assert(channelRGB >= 1);
    assert(channelRGB <= 3);
    
    QList<QImage*> *listldr=NULL;
    Array2DList *listhdr=NULL;
    va_list arg_pointer;
    va_start(arg_pointer,ldrinput); /* Initialize the argument list. */
    int N=-1; int width=-1; int height=-1;
    
    if (ldrinput) {
        listldr=va_arg(arg_pointer,QList<QImage*>*);
        // number of exposures
        N = listldr->count();
        // frame size
        width = (listldr->at(0))->width();
        height = (listldr->at(0))->height();
    } else {
        listhdr=va_arg(arg_pointer,Array2DList*);
        // number of exposures
        N = listhdr->size();
        // frame size
        width= ((*listhdr)[0])->getCols();
        height=((*listhdr)[0])->getRows();
    }
    va_end(arg_pointer); /* Clean up. */
    
    PixelToChannelMapper ldrfp = s_pixelToChannel[channelRGB-1];
    
    // number of saturated pixels
    int saturated_pixels = 0;
    
    // --- anti saturation: calculate trusted camera output range
    int minM = 0;
    for ( int m = 0; m < M; m++ ) {
        if ( w[m]>0 ) {
            minM = m;
            break;
        }
    }
    int maxM = M-1;
    for ( int m = M-1; m >= 0; m-- ) {
        if( w[m]>0 ) {
            maxM = m;
            break;
        }
    }
    
    // --- anti ghosting: for each image i, find images with
    // the immediately higher and lower exposure times
    std::vector<int> i_lower(N); // int* i_lower = new int[N];
    std::vector<int> i_upper(N); // int* i_upper = new int[N];

    for ( int i = 0; i < N; ++i )
    {
        i_lower[i] = -1;
        i_upper[i] = -1;
        float ti = arrayofexptime[i];
        float ti_upper = arrayofexptime[0];
        float ti_lower = arrayofexptime[0];
        
        for ( int j = 0; j < N; ++j ) {
            if ( i != j ) {
                if ( arrayofexptime[j] > ti && arrayofexptime[j] < ti_upper ) {
                    ti_upper = arrayofexptime[j];
                    i_upper[i] = j;
                }
                if ( arrayofexptime[j] < ti && arrayofexptime[j] > ti_lower ) {
                    ti_lower = arrayofexptime[j];
                    i_lower[i] = j;
                }
            }
        }
        if ( i_lower[i] == -1 ) i_lower[i] = i;
        if ( i_upper[i] == -1 ) i_upper[i] = i;
    }
    
    // all pixels
    for( int j=0 ; j<width*height ; j++ )
    {
        // all exposures for each pixel
        float sum = 0.0f;
        float div = 0.0f;
        float maxti = -1e6f;
        float minti = +1e6f;
        
        if (ldrinput)
        {
            //for all exposures
            for( int i=0 ; i<N ; i++ )
            {
                int m=ldrfp(* ( (QRgb*)( (listldr->at(i) )->bits() ) + j ) );
                float ti = arrayofexptime[i];
                // --- anti saturation: observe minimum exposure time at which
                // saturated value is present, and maximum exp time at which
                // black value is present
                if ( m>maxM )
                    minti = std::min(minti,ti);
                if ( m<minM )
                    maxti = std::max(maxti,ti);
                
                // --- anti ghosting: monotonous increase in time should result
                // in monotonous increase in intensity; make forward and
                // backward check, ignore value if condition not satisfied
                int m_lower = ldrfp(* ( (QRgb*)( (listldr->at(i_lower[i]) )->bits() ) + j ) );
                int m_upper = ldrfp(* ( (QRgb*)( (listldr->at(i_upper[i]) )->bits() ) + j ) );
                if( m_lower>m || m_upper<m)
                    continue;
                
                sum += w[m] * ti * I[m];
                div += w[m] * ti * ti;
            }
        }
        else
        {
            // for all exposures
            for ( int i=0 ; i<N ; i++ )
            {
                int m = (int) ( ( *( ( (*listhdr)[i] ) ) ) (j) );
                float ti = arrayofexptime[i];
                // --- anti saturation: observe minimum exposure time at which
                // saturated value is present, and maximum exp time at which
                // black value is present
                if( m>maxM )
                    minti = std::min(minti,ti);
                if( m<minM )
                    maxti = std::max(maxti,ti);
                
                // --- anti ghosting: monotonous increase in time should result
                // in monotonous increase in intensity; make forward and
                // backward check, ignore value if condition not satisfied
                int m_lower = (int) ( ( *( ( (*listhdr)[i_lower[i]] ) ) ) (j) );
                int m_upper = (int) ( ( *( ( (*listhdr)[i_upper[i]] ) ) ) (j) );
                
                if ( m_lower > m || m_upper < m)
                    continue;
                
                sum += w[m] * ti * I[m];
                div += w[m] * ti * ti;
            }
        }
        
        // --- anti saturation: if a meaningful representation of pixel
        // was not found, replace it with information from observed data
        if( div==0.0f )
            saturated_pixels++;
        if( div==0.0f && maxti>-1e6f ) {
            sum = I[minM];
            div = maxti;
        }
        if( div==0.0f && minti<+1e6f ) {
            sum = I[maxM];
            div = minti;
        }
        
        if( div!=0.0f )
            (*xj)(j) = sum/div;
        else
            (*xj)(j) = 0.0f;
    }

    return saturated_pixels;
}

////////////////////////////////     GET RESPONSE    /////////////////////////////////////
int robertson02_getResponse(pfs::Array2D& xj, const float * arrayofexptime,
                            float* I, const float* w, int M, int channelRGB,
                            const Array2DList& listhdr)
{
    int N       = listhdr.size();
    int width   = xj.getCols();
    int height  = xj.getRows();
        
    // number of saturated pixels
    int saturated_pixels = 0;
    
    std::vector<float> Ip(M);       //float* Ip = new float[M]; // previous response
    
    // 0. Initialization
    normalizeI( I, M );
    std::copy(I, I + M, Ip.begin()); // for (int m = 0; m < M; ++m) Ip[m] = I[m];
    
    robertson02_applyResponse(&xj, arrayofexptime, I, w, M, channelRGB, false, &listhdr);
    
    // Optimization process
    bool converged = false;

    std::vector<long> cardEm(M);    // long* cardEm = new long[M];
    std::vector<float> sum(M);      // float* sum = new float[M];
    
    int cur_it = 0;
    float pdelta = 0.0f;

    while ( !converged )
    {
        // 1. Minimize with respect to I
        for (int m = 0; m < M; ++m) {
            cardEm[m] = 0;
            sum[m]    = 0.0f;
        }
        
        for (int i = 0; i < N; ++i) {
            float ti = arrayofexptime[i];
            // this is probably uglier than necessary, (I copy th FOR in order
            // not to do the IFs inside them) but I don't know how to improve it
            for (int j = 0; j < width*height; ++j) {
                int sample = static_cast<int>( (*(listhdr[i])) (j) );
                if ( sample < M && sample >= 0 ) {
                    sum[sample] += ti * xj(j);
                    cardEm[sample]++;
                }
#ifndef NDEBUG
                else
                    std::cerr << "robertson02: m out of range: "
                              << sample << std::endl;
#endif
            }
        }
        
        for (int m = 0 ; m < M; ++m) {
            if ( cardEm[m] != 0 ) {
                I[m] = sum[m] / cardEm[m];
            } else {
                I[m] = 0.0f;
            }
        }
        
        // 2. Normalize I
        /*float middle_response = */normalizeI( I, M );
        
        // 3. Apply new response
        saturated_pixels = robertson02_applyResponse(&xj, arrayofexptime, I, w, M, channelRGB, false, &listhdr);
        
        // 4. Check stopping condition
        float delta = 0.0f;
        int hits=0;
        for (int m = 0; m < M; ++m) {
            if ( I[m] != 0.0f ) {
                float diff = I[m] - Ip[m];
                delta += diff * diff;
                Ip[m] = I[m];
                hits++;
            }
        }
        delta /= hits;
#ifndef NDEBUG
        std::cerr << " #" << cur_it << " delta=" << delta << " (coverage: " << 100*hits/M << "%)\n";
#endif
        if ( delta < MAX_DELTA )
        {
            converged = true;
        }
        else if ( isnan(delta) || (cur_it > MAXIT && pdelta < delta) )
        {
#ifndef NDEBUG
            std::cerr << "algorithm failed to converge, too noisy data in range\n";
#endif
            break;
        }
        
        pdelta = delta;
        cur_it++;
    }
#ifndef NDEBUG
    if ( converged )
    {
        std::cerr << " #" << cur_it << " delta=" << pdelta << " <- converged\n";
    }
#endif
    
    return saturated_pixels;
}

int robertson02_getResponse(pfs::Array2D& xj, const float* arrayofexptime,
                            float* I, const float* w, int M, int channelRGB,
                            const QList<QImage*>& listldr)
{
    int N       = listldr.size();
    int width   = xj.getCols();
    int height  = xj.getRows();

    PixelToChannelMapper ldrfp = s_pixelToChannel[channelRGB-1];

    // number of saturated pixels
    int saturated_pixels = 0;

    std::vector<float> Ip(M);       //float* Ip = new float[M]; // previous response

    // 0. Initialization
    normalizeI( I, M );
    std::copy(I, I + M, Ip.begin()); // for (int m = 0; m < M; ++m) Ip[m] = I[m];

    robertson02_applyResponse(&xj, arrayofexptime, I, w, M, channelRGB, true, &listldr);

    // Optimization process
    bool converged = false;

    std::vector<long> cardEm(M);    // long* cardEm = new long[M];
    std::vector<float> sum(M);      // float* sum = new float[M];

    int cur_it = 0;
    float pdelta = 0.0f;

    while ( !converged )
    {
        // 1. Minimize with respect to I
        for (int m = 0; m < M; ++m) {
            cardEm[m] = 0;
            sum[m]    = 0.0f;
        }

        for (int i = 0 ; i < N ; ++i) {
            float ti = arrayofexptime[i];
            // this is probably uglier than necessary, (I copy th FOR in order
            // not to do the IFs inside them) but I don't know how to improve it
            for (int j = 0; j < width*height; ++j) {
                int sample = ldrfp( reinterpret_cast<const QRgb*>(listldr.at(i)->bits())[j] );
                if ( sample < M && sample >= 0 ) {
                    sum[sample] += ti * xj(j);
                    cardEm[sample]++;
                }
#ifndef NDEBUG
                else
                    std::cerr << "robertson02: m out of range: "
                              << sample << std::endl;
#endif
            }
        }

        for (int m = 0 ; m < M; ++m) {
            if ( cardEm[m] != 0 ) {
                I[m] = sum[m] / cardEm[m];
            } else {
                I[m] = 0.0f;
            }
        }

        // 2. Normalize I
        /*float middle_response = */normalizeI( I, M );

        // 3. Apply new response
        saturated_pixels = robertson02_applyResponse(&xj, arrayofexptime, I, w, M, channelRGB, true, &listldr);

        // 4. Check stopping condition
        float delta = 0.0f;
        int hits=0;
        for (int m = 0; m < M; ++m) {
            if ( I[m] != 0.0f ) {
                float diff = I[m] - Ip[m];
                delta += diff * diff;
                Ip[m] = I[m];
                hits++;
            }
        }
        delta /= hits;
#ifndef NDEBUG
        std::cerr << " #" << cur_it << " delta=" << delta << " (coverage: " << 100*hits/M << "%)\n";
#endif
        if ( delta < MAX_DELTA )
        {
            converged = true;
        }
        else if ( isnan(delta) || (cur_it > MAXIT && pdelta < delta) )
        {
#ifndef NDEBUG
            std::cerr << "algorithm failed to converge, too noisy data in range\n";
#endif
            break;
        }

        pdelta = delta;
        cur_it++;
    }
#ifndef NDEBUG
    if ( converged )
    {
        std::cerr << " #" << cur_it << " delta=" << pdelta << " <- converged\n";
    }
#endif

    return saturated_pixels;
}

int robertson02_getResponse(pfs::Array2D& Rj, pfs::Array2D& Gj, pfs::Array2D& Bj,
                            const float* arrayofexptime,
                            float* Ir, float* Ig, float* Ib,
                            const float* w, int M,
                            const QList<QImage*>& listldr)
{
    int saturatedPixels = 0;
    saturatedPixels += robertson02_getResponse(Rj, arrayofexptime, Ir, w, M, 1, listldr);
    saturatedPixels += robertson02_getResponse(Gj, arrayofexptime, Ig, w, M, 2, listldr);
    saturatedPixels += robertson02_getResponse(Bj, arrayofexptime, Ib, w, M, 3, listldr);

    return saturatedPixels;
}

int robertson02_getResponse(pfs::Array2D& Rj, pfs::Array2D& Gj, pfs::Array2D& Bj,
                            const float* arrayofexptime,
                            float* Ir, float* Ig, float* Ib,
                            const float* w, int M,
                            const Array2DList& listhdrR, const Array2DList& listhdrG, const Array2DList& listhdrB)
{
    int saturatedPixels = 0;
    saturatedPixels += robertson02_getResponse(Rj, arrayofexptime, Ir, w, M, 1, listhdrR);
    saturatedPixels += robertson02_getResponse(Gj, arrayofexptime, Ig, w, M, 2, listhdrG);
    saturatedPixels += robertson02_getResponse(Bj, arrayofexptime, Ib, w, M, 3, listhdrB);

    return saturatedPixels;
}
