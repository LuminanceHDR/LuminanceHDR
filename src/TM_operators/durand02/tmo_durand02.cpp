/**
 * @file tmo_bilateral.cpp
 * @brief Local tone mapping operator based on bilateral filtering.
 * Durand et al. 2002
 *
 * Fast Bilateral Filtering for the Display of High-Dynamic-Range Images.
 * F. Durand and J. Dorsey.
 * In ACM Transactions on Graphics, 2002.
 *
 * 
 * This file is a part of Qtpfsgui package, based on pfstmo.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2003,2004 Grzegorz Krawczyk
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
 * @author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
 *
 * $Id: tmo_durand02.cpp,v 1.4 2008/09/09 18:10:49 rafm Exp $
 */
#include <iostream>
#include <vector>
#include <algorithm>
#include <math.h>
#include "../pfstmo.h"
#include "../tmo_config.h"

#ifdef HAVE_FFTW3F
#include "fastbilateral.h"
#else
#include "bilateral.h"
#endif
#define FLT_MIN 1e-6

static void findMaxMinPercentile(pfstmo::Array2D* I, float minPrct, float& minLum,
  float maxPrct, float& maxLum);


/*

From Durand's webpage:
<http://graphics.lcs.mit.edu/~fredo/PUBLI/Siggraph2002/>

Here is the high-level set of operation that you need to do in order
to perform contrast reduction

input intensity= 1/61*(R*20+G*40+B)
r=R/(input intensity), g=G/input intensity, B=B/input intensity
log(base)=Bilateral(log(input intensity))
log(detail)=log(input intensity)-log(base)
log (output intensity)=log(base)*compressionfactor+log(detail)
R output = r*exp(log(output intensity)), etc.

*/

void tmo_durand02(unsigned int width, unsigned int height,
  float *_R, float *_G, float *_B,
  float sigma_s, float sigma_r, float baseContrast, int downsample/*,
  pfstmo_progress_callback progress_cb */) 
 {
  pfstmo::Array2D* R = new pfstmo::Array2D(width, height, _R);
  pfstmo::Array2D* G = new pfstmo::Array2D(width, height, _G);
  pfstmo::Array2D* B = new pfstmo::Array2D(width, height, _B);

  int i;
  int w = R->getCols();
  int h = R->getRows();
  int size = w*h;
  pfstmo::Array2D* I = new pfstmo::Array2D(w,h); // intensities
  pfstmo::Array2D* BASE = new pfstmo::Array2D(w,h); // base layer
  pfstmo::Array2D* DETAIL = new pfstmo::Array2D(w,h); // detail layer

  float min_pos = 1e10f;
  for( i=0 ; i<size ; i++ )
  {
    (*I)(i) = 1.0f/61.0f * ( 20.0f*(*R)(i) + 40.0f*(*G)(i) + (*B)(i) );
    if( unlikely((*I)(i) < min_pos && (*I)(i) > 0) )
      min_pos = (*I)(i);
  }
  
  for( i=0 ; i<size ; i++ )
  {
    float L = (*I)(i);
    if( unlikely( L <= 0 ) )
      L = min_pos;
    
    (*R)(i) /= L;
    (*G)(i) /= L;
    (*B)(i) /= L;

    (*I)(i) = log(FLT_MIN+L );
//     if (!finite((*I)(i))) {
//     fprintf(stderr,"nf");
//     }
  }

#ifdef HAVE_FFTW3F
  fastBilateralFilter( I, BASE, sigma_s, sigma_r, downsample/*, progress_cb */);
#else
  bilateralFilter( I, BASE, sigma_s, sigma_r/*, progress_cb */);
#endif

  //!! FIX: find minimum and maximum luminance, but skip 1% of outliers
  float maxB,minB;
  findMaxMinPercentile(BASE, 0.01f, minB, 0.99f, maxB);

//   DEBUG_STR << "Base contrast: " << "maxB=" << maxB << " minB=" << minB
//             << " c=" << maxB-minB << std::endl;

  float compressionfactor = baseContrast / (maxB-minB);

//   DEBUG_STR << "Base contrast (compressed): " << "maxB=" << maxB*compressionfactor
//             << " minB=" << minB*compressionfactor
//             << " c=" << (maxB-minB)*compressionfactor << std::endl;
  
  for( i=0 ; i<size ; i++ )
  {
    (*DETAIL)(i) = (*I)(i) - (*BASE)(i);
    (*I)(i) = (*BASE)(i) * compressionfactor + (*DETAIL)(i);

    //!! FIX: this to keep the output in normalized range 0.01 - 1.0
    //intensitites are related only to minimum luminance because I
    //would say this is more stable over time than using maximum
    //luminance and is also robust against random peaks of very high
    //luminance
    (*I)(i) -=  4.3f+minB*compressionfactor;

    (*R)(i) *= expf( (*I)(i) );
    (*G)(i) *= expf( (*I)(i) );
    (*B)(i) *= expf( (*I)(i) );
  }

  delete I;
  delete BASE;
  delete DETAIL;

  delete B;
  delete G;
  delete R;
}



/**
 * @brief Find minimum and maximum value skipping the extremes
 *
 */
static void findMaxMinPercentile(pfstmo::Array2D* I, float minPrct, float& minLum, 
  float maxPrct, float& maxLum)
{
	int size = I->getRows() * I->getCols();
	std::vector<float> vI;
	
	for( int i=0 ; i<size ; i++ )
		if( (*I)(i)!=0.0f )
			vI.push_back((*I)(i));
	
	std::sort(vI.begin(), vI.end());
	minLum = vI.at( int(minPrct*vI.size()) );
	maxLum = vI.at( int(maxPrct*vI.size()) );
}
