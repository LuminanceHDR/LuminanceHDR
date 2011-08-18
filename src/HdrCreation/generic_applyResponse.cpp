/**
 * @brief Generic algorithm for combining images.
 *
 * This file is a part of Luminance HDR package.
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

#include <iostream>
#include <QVector>
#include <vector>
#include <math.h>

#include "responses.h"
#include "generic_applyResponse.h"
#include "Common/msec_timer.h"

#define   CHANNEL_NUM           3
#define   RED_CHANNEL_IDX       0
#define   GREEN_CHANNEL_IDX     1
#define   BLUE_CHANNEL_IDX      2
#define   ALPHA_CHANNEL_IDX     3

inline void getChannelValues( int** values, const int pixel_offset, const int N, const int M, const bool ldrinput, QList<QImage*> *listldr, Array2DList *listhdrR, Array2DList *listhdrG, Array2DList *listhdrB);
int getN(const bool ldrinput, QList<QImage*> *listldr, Array2DList *listhdrR, Array2DList*, Array2DList*);
int getFrameSize(const bool ldrinput, QList<QImage*> *listldr, Array2DList *listhdrR, Array2DList* , Array2DList*);


//void generic_applyResponse(float (*sumf)(float, float, float, float), float (*divf)(float, float, float, float), float (*outf)(float), 
//                           pfs::Array2D* Rout, pfs::Array2D* Gout, pfs::Array2D* Bout, const float * arrayofexptime, 
//                           const float* Ir,  const float* Ig, const float* Ib, const float* w, const int M, 
//                           const bool ldrinput, QList<QImage*> *listldr, Array2DList *listhdrR, Array2DList *listhdrG, Array2DList *listhdrB)
void generic_applyResponse(float (*sumf)(float, float, float, float), float (*divf)(float, float, float, float), float (*outf)(float), 
                           pfs::Array2D* Rout, pfs::Array2D* Gout, pfs::Array2D* Bout, const QVector<float> arrayofexptime, 
                           const float* Ir,  const float* Ig, const float* Ib, const float* w, const int M, 
                           const bool ldrinput, QList<QImage*> *listldr, Array2DList *listhdrR, Array2DList *listhdrG, Array2DList *listhdrB)
{
#ifdef TIMER_PROFILING
    msec_timer f_timer;
    f_timer.start();
#endif

    const int N         = getN(ldrinput, listldr, listhdrR, listhdrG, listhdrB);
    const int frameSize = getFrameSize(ldrinput, listldr, listhdrR, listhdrG, listhdrB);

    // --- anti saturation: calculate trusted camera output range
    int minM = 0;
    for( int m=0 ; m<M ; m++ )
    {
        if( w[m]>0 ) {
            minM = m;
            break;
        }
    }
    int maxM = M-1;
    for( int m=M-1 ; m>=0 ; m-- )
    {
        if( w[m]>0 ) {
            maxM = m;
            break;
        }
    }

    // --- anti ghosting: for each image i, find images with
    // the immediately higher and lower exposure times
    int* i_lower = new int[N];
    int* i_upper = new int[N];
    for( int i=0; i < N ; i++ )
    {
        i_lower[i]=-1;
        i_upper[i]=-1;
        float ti        = arrayofexptime[i];
        float ti_upper  = 1.0e8;
        float ti_lower  = 0;

        for ( int j=0 ; j<N ; j++ )
        {
            if ( i != j )
            {
                if ( arrayofexptime[j]>ti && arrayofexptime[j]<ti_upper )
                {
                    ti_upper = arrayofexptime[j];
                    i_upper[i] = j;
                }
                if ( arrayofexptime[j]<ti && arrayofexptime[j]>ti_lower )
                {
                    ti_lower = arrayofexptime[j];
                    i_lower[i] = j;
                }
            }
        }
        if ( i_lower[i] == -1 )
        {
            i_lower[i] = i;
        }
        if ( i_upper[i] == -1 )
        {
            i_upper[i]=i;
        }
    }

    const float* I[CHANNEL_NUM] = {Ir, Ig, Ib};
    //  I[0] = Ir;
    //  I[1] = Ig;
    //  I[2] = Ib;
    float out[CHANNEL_NUM];
    int** m_all = new int*[N];
    for (int idx = 0; idx < N; ++idx)
    {
        m_all[idx] = new int[CHANNEL_NUM+1];
    }

    //for all pixels
    for (int j=0 ; j < frameSize; ++j)
    {
        // all exposures for each pixel
        float sum[CHANNEL_NUM]    = {0.0f, 0.0f, 0.0f};
        float div[CHANNEL_NUM]    = {0.0f, 0.0f, 0.0f};
        float maxti[CHANNEL_NUM]  = {-1e6f, -1e6f, -1e6f};
        float minti[CHANNEL_NUM]  = {+1e6f, +1e6f, +1e6f};

        getChannelValues(m_all, j, N, M, ldrinput, listldr, listhdrR, listhdrG, listhdrB);

        //for all exposures
        for (int i=0 ; i < N; ++i)
        {
            //pick the 4 channels' values (red, green, blue, and alpha)
            int* m = m_all[i];

            float ti = arrayofexptime[i];
            // --- anti saturation: observe minimum exposure time at which
            // saturated value is present, and maximum exp time at which
            // black value is present
            // this needs to be done separately for each channel
            for (int chan = 0; chan < CHANNEL_NUM; ++chan)
            {
                if (m[chan]>maxM) minti[chan] = qMin(minti[chan],ti);
                if (m[chan]<minM) maxti[chan] = qMax(maxti[chan],ti);
            }

            // --- anti ghosting: monotonous increase in time should result
            // in monotonous increase in intensity; make forward and
            // backward check, ignore value if condition not satisfied
            // if this is violated for one channel then the pixel is ignored for all channels
            int* lower = m_all[i_lower[i]];
            int* upper = m_all[i_upper[i]];
            if ((lower[0]>m[0] || upper[0]<m[0])||(lower[1]>m[1] || upper[1]<m[1])||(lower[2]>m[2] || upper[2]<m[2]))
            {
                continue;
            }

            float average_weight = (1.0/3.0)*(w[m[RED_CHANNEL_IDX]]+w[m[GREEN_CHANNEL_IDX]]+w[m[BLUE_CHANNEL_IDX]]);

            // m[3] (alpha channel) assumed to handle de-ghosting masks
            // m[3] values assumed to be in [0, M]
            // m[3]=0 assummed to mean that the pixel should be excluded
            float fmA = m[ALPHA_CHANNEL_IDX]/((float)M);
            for (int chan = 0; chan < CHANNEL_NUM; ++chan)
            {
                sum[chan] += (*sumf)(fmA * w[m[chan]], average_weight, ti, I[chan][m[chan]]);
                div[chan] += (*divf)(fmA * w[m[chan]], average_weight, ti, I[chan][m[chan]]);
            }
        }

        for (int chan = 0; chan < CHANNEL_NUM; ++chan)
        {
            // --- anti saturation: if a meaningful representation of pixel
            // was not found, replace it with information from observed data
            // this needs to be done separately for each channel
            if (div[chan] == 0.0f && maxti[chan] > -1e6f)
            {
                sum[chan] = (*sumf)(1.0f, 1.0f, maxti[chan], I[chan][minM]);
                div[chan] = (*divf)(1.0f, 1.0f, maxti[chan], I[chan][minM]);
            }
            else if (div[chan] == 0.0f && minti[chan] < +1e6f)
            {
                sum[chan] = (*sumf)(1.0f, 1.0f, minti[chan], I[chan][maxM]);
                div[chan] = (*divf)(1.0f, 1.0f, minti[chan], I[chan][maxM]);
            }

            if (div[chan] != 0.0f)
            {
                out[chan] = (*outf)(sum[chan]/div[chan]);
            }
            else
            {
                out[chan] = 0.0f;
            }
        }

        (*Rout)(j) = out[RED_CHANNEL_IDX];
        (*Gout)(j) = out[GREEN_CHANNEL_IDX];
        (*Bout)(j) = out[BLUE_CHANNEL_IDX];
    }
    //for all pixels

    for (int idx = 0; idx < N; ++idx) delete[] m_all[idx];
    delete[] m_all;

    delete[] i_lower;
    delete[] i_upper;

#ifdef TIMER_PROFILING
    f_timer.stop_and_update();
    std::cout << "generic_applyResponse() = " << f_timer.get_time() << " msec" << std::endl;
#endif 
}

/////////////////////////////////////////////////////////////////////////////////
// private part

/*
 * Unifies the way the value for a specific channel of a pixel is accessed for all channels and LDR and HDR
 * The channels are ennumerated as follows:
 * 0 = red
 * 1 = green
 * 2 = blue
 * 3 = alpha
 */

inline void getChannelValues(int** values, const int pixel_offset, const int N, const int M, const bool ldrinput, QList<QImage*> *listldr, Array2DList *listhdrR, Array2DList *listhdrG, Array2DList *listhdrB)
{
    if (ldrinput)
    {
        for (int index = 0; index < N; index++) {
            QRgb* pixel = (QRgb*)( (listldr->at(index) )->bits() ) + pixel_offset;
            values[index][0] = qRed(*pixel);
            values[index][1] = qGreen(*pixel);
            values[index][2] = qBlue(*pixel);
            values[index][3] = qAlpha(*pixel);
        }
    }
    else
    {
        for (int index = 0; index < N; index++)
        {
            values[index][0] = ( *( ( (*listhdrR)[index] ) ) ) (pixel_offset);
            values[index][1] = ( *( ( (*listhdrG)[index] ) ) ) (pixel_offset);
            values[index][2] = ( *( ( (*listhdrB)[index] ) ) ) (pixel_offset);
            //
            // As of now there is no listhdr for the alpha channel so we return M (fully opaque)
            values[index][3] = M;
        }
    }
}

int getN(const bool ldrinput, QList<QImage*> *listldr, Array2DList *listhdrR, Array2DList*, Array2DList*)
{
	if (ldrinput) return listldr->count();
	else return listhdrR->size();		
}

int getFrameSize(const bool ldrinput, QList<QImage*> *listldr, Array2DList *listhdrR, Array2DList*, Array2DList*)
{
	if(ldrinput) {
		return (listldr->at(0))->width()*(listldr->at(0))->height();
	} else {
		return ((*listhdrR)[0])->getCols()*((*listhdrR)[0])->getRows();
	}
}
