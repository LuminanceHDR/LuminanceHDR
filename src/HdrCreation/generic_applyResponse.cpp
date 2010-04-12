/**
 * @brief Generic algorithm for combining images.
 *
 * This file is a part of LuminanceHDR package.
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
#include <vector>

#include <math.h>

#include "responses.h"
#include "generic_applyResponse.h"

#define PROG_NAME "robertson02"

int getChannelValue( const int index, const int pixel_offset, const int channel, const int M, const bool ldrinput, QList<QImage*> *listldr, Array2DList *listhdrR, Array2DList *listhdrG, Array2DList *listhdrB);
int getN(const bool ldrinput, QList<QImage*> *listldr, Array2DList *listhdrR, Array2DList*, Array2DList*);
int getFrameSize(const bool ldrinput, QList<QImage*> *listldr, Array2DList *listhdrR, Array2DList* , Array2DList*);


void generic_applyResponse( 
    float (*sumf)(float, float, float, float), float (*divf)(float, float, float, float), float (*outf)(float), 
    pfs::Array2D* Rout, pfs::Array2D* Gout, pfs::Array2D* Bout, const float * arrayofexptime, 
    const float* Ir,  const float* Ig, const float* Ib, const float* w, const int M, 
    const bool ldrinput, QList<QImage*> *listldr, Array2DList *listhdrR, Array2DList *listhdrG, Array2DList *listhdrB
    ) {

int N = getN(ldrinput, listldr, listhdrR, listhdrG, listhdrB);
int frameSize = getFrameSize(ldrinput, listldr, listhdrR, listhdrG, listhdrB);

// --- anti saturation: calculate trusted camera output range
int minM = 0;
for( int m=0 ; m<M ; m++ )
	if( w[m]>0 ) {
		minM = m;
		break;
	}
int maxM = M-1;
for( int m=M-1 ; m>=0 ; m-- )
	if( w[m]>0 ) {
		maxM = m;
		break;
	}

// --- anti ghosting: for each image i, find images with
// the immediately higher and lower exposure times
int* i_lower = new int[N];
int* i_upper = new int[N];
for( int i=0 ; i<N ; i++ )
{
	i_lower[i]=-1;
	i_upper[i]=-1;
	float ti =  arrayofexptime[i];
	float ti_upper =  arrayofexptime[0];
	float ti_lower = arrayofexptime[0];

	for( int j=0 ; j<N ; j++ )
		if( i!=j )
		{
			if( arrayofexptime[j]>ti && arrayofexptime[j]<ti_upper )
			{
				ti_upper=arrayofexptime[j];
				i_upper[i]=j;
			}
			if(arrayofexptime[j]<ti && arrayofexptime[j]>ti_lower )
			{
				ti_lower=arrayofexptime[j];
				i_lower[i]=j;
			}
		}
	if( i_lower[i]==-1 )
		i_lower[i]=i;
	if( i_upper[i]==-1 )
		i_upper[i]=i;
}

const float* I[3];
I[0] = Ir;
I[1] = Ig;
I[2] = Ib;

//for all pixels
for( int j=0 ; j<frameSize ; j++ )
{
	// all exposures for each pixel
	float sum[3];
	float div[3];
	float out[3];
	float maxti[3];
	float minti[3];
	for (int chan = 0; chan < 3; ++chan) {
		sum[chan]=0.0f;
		div[chan]=0.0f;
		maxti[chan] = -1e6f;
		minti[chan] = +1e6f;
	}

	//for all exposures
	for( int i=0 ; i<N ; i++ ) 
	{

		//pick the 4 channels' values (red, green, blue, and alpha)
		int* m = new int[4];
		for (int chan = 0; chan < 4; ++chan ) {
			m[chan] = getChannelValue( i, j, chan, M, ldrinput, listldr, listhdrR, listhdrG, listhdrB);
		}

		float ti = arrayofexptime[i];
		// --- anti saturation: observe minimum exposure time at which
		// saturated value is present, and maximum exp time at which
		// black value is present
		// this needs to be done separately for each channel
		for (int chan = 0; chan < 3; ++chan ) {
			if (m[chan]>maxM) minti[chan] = fminf(minti[chan],ti);
			if (m[chan]<minM) maxti[chan] = fmaxf(maxti[chan],ti);
		}

		// --- anti ghosting: monotonous increase in time should result
		// in monotonous increase in intensity; make forward and
		// backward check, ignore value if condition not satisfied
		// if this is violated for one channel then the pixel is ignored for all channels
		int lower[3];
		int upper[3];
		for (int chan = 0; chan < 3; ++chan) {
			lower[chan] = getChannelValue( i_lower[i], j, chan, M, ldrinput, listldr, listhdrR, listhdrG, listhdrB);
			upper[chan] = getChannelValue( i_upper[i], j, chan, M, ldrinput, listldr, listhdrR, listhdrG, listhdrB);
		}
		if ((lower[0]>m[0] || upper[0]<m[0])||(lower[1]>m[1] || upper[1]<m[1])||(lower[2]>m[2] || upper[2]<m[2]))
			continue;

		float average_weight = 0;
		for (int chan = 0; chan < 3; ++chan) {
			average_weight += w[m[chan]]/3;
		}
		
		// m[3] (alpha channel) assumed to handle de-ghosting masks
		// m[3] values assumed to be in [0, M]
		// m[3]=0 assummed to mean that the pixel should be excluded
		float fmA = m[3]/ (float) M;
		for (int chan = 0; chan < 3; ++chan ) {
			sum[chan] += (*sumf)(fmA * w[m[chan]], average_weight, ti, I[chan][m[chan]]);
			div[chan] += (*divf)(fmA * w[m[chan]], average_weight, ti, I[chan][m[chan]]);
		}
	}

	for (int chan = 0; chan < 3; ++chan ) {
		// --- anti saturation: if a meaningful representation of pixel
		// was not found, replace it with information from observed data
		// this needs to be done separately for each channel
		if (div[chan] == 0.0f && maxti[chan] > -1e6f) {
			sum[chan] = (*sumf)(1.0f, 1.0f, maxti[chan], I[chan][minM]);
			div[chan] = (*divf)(1.0f, 1.0f, maxti[chan], I[chan][minM]);
		} else if (div[chan] == 0.0f && minti[chan] < +1e6f) {
			sum[chan] = (*sumf)(1.0f, 1.0f, minti[chan], I[chan][maxM]);
			div[chan] = (*divf)(1.0f, 1.0f, minti[chan], I[chan][maxM]);
		}
		
		if (div[chan] != 0.0f) {
			out[chan] = (*outf)(sum[chan]/div[chan]);
		} else {
			out[chan] = 0.0f;
		}
	}

	(*Rout)(j) = out[0];
	(*Gout)(j) = out[1];
	(*Bout)(j) = out[2];
}//for all pixels

delete[] i_lower;
delete[] i_upper;

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

int getChannelValue( const int index, const int pixel_offset, const int channel, const int M, const bool ldrinput, QList<QImage*> *listldr, Array2DList *listhdrR, Array2DList *listhdrG, Array2DList *listhdrB) {
	if(ldrinput) {
		QRgb* pixel;
		pixel = (QRgb*)( (listldr->at(index) )->bits() ) + pixel_offset;
		if(channel == 0) {
			return qRed(*pixel);
		} else if(channel == 1) {
			return qGreen(*pixel);
		} else if(channel == 2) {
			return qBlue(*pixel);
		} else if(channel == 3) {
			return qAlpha(*pixel);
		}
	} else {
		Array2DList* listhdr=NULL;

		if(channel == 0) {
			listhdr=listhdrR;
		} else if(channel == 1) {
			listhdr=listhdrG;
		} else if(channel == 2) {
			listhdr=listhdrB;
		} else if(channel == 3) {
			// As of now there is no listhdr for the alpha channel so we return M (fully opaque)
			return M;
		}
		
		return (int) ( ( *( ( (*listhdr)[index] ) ) ) (pixel_offset) );
	}

	return 0;
}

int getN(const bool ldrinput, QList<QImage*> *listldr, Array2DList *listhdrR, Array2DList*, Array2DList*) {
	if(ldrinput) return listldr->count();
	else return listhdrR->size();		
}

int getFrameSize(const bool ldrinput, QList<QImage*> *listldr, Array2DList *listhdrR, Array2DList*, Array2DList*) {
	if(ldrinput) {
		return (listldr->at(0))->width()*(listldr->at(0))->height();
	} else {
		return ((*listhdrR)[0])->getCols()*((*listhdrR)[0])->getRows();
	}
}