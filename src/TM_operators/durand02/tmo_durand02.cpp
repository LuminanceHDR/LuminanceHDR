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
 * $Id: tmo_durand02.cpp,v 1.6 2009/02/23 19:09:41 rafm Exp $
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <math.h>

#include "../pfstmo.h"

//#undef HAVE_FFTW3F

#ifdef HAVE_FFTW3F
#include "fastbilateral.h"
#else
#include "bilateral.h"
#endif

#ifdef BRANCH_PREDICTION
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif


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
  float *nR, float *nG, float *nB,
  float sigma_s, float sigma_r, float baseContrast, int downsample,
  const bool color_correction,
  ProgressHelper *ph) 
{
  pfstmo::Array2D* R = new pfstmo::Array2D(width, height, nR);
  pfstmo::Array2D* G = new pfstmo::Array2D(width, height, nG);
  pfstmo::Array2D* B = new pfstmo::Array2D(width, height, nB);

  int i;
  int w = R->getCols();
  int h = R->getRows();
  int size = w*h;
  pfstmo::Array2D* I = new pfstmo::Array2D(w,h); // intensities
  pfstmo::Array2D* BASE = new pfstmo::Array2D(w,h); // base layer
  pfstmo::Array2D* DETAIL = new pfstmo::Array2D(w,h); // detail layer

  float min_pos = 1e10f; // minimum positive value (to avoid log(0))
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

    (*I)(i) = logf( L );
  }

#ifdef HAVE_FFTW3F
  fastBilateralFilter( I, BASE, sigma_s, sigma_r, downsample, ph );
#else
  bilateralFilter( I, BASE, sigma_s, sigma_r, ph );
#endif

  //!! FIX: find minimum and maximum luminance, but skip 1% of outliers
  float maxB,minB;
  findMaxMinPercentile(BASE, 0.01f, minB, 0.99f, maxB);

  float compressionfactor = baseContrast / (maxB-minB);

  // Color correction factor
  const float k1 = 1.48;
  const float k2 = 0.82;
  const float s = ( (1 + k1)*pow(compressionfactor,k2) )/( 1 + k1*pow(compressionfactor,k2) );
  
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

    if( likely( color_correction ) ) {
      (*R)(i) =  powf( (*R)(i), s ) *  expf( (*I)(i) );
      (*G)(i) =  powf( (*G)(i), s ) *  expf( (*I)(i) );
      (*B)(i) =  powf( (*B)(i), s ) *  expf( (*I)(i) );
    } else {
      (*R)(i) *= expf( (*I)(i) );
      (*G)(i) *= expf( (*I)(i) );
      (*B)(i) *= expf( (*I)(i) );
    }
  }

  delete I;
  delete BASE;
  delete DETAIL;

  delete B;
  delete G;
  delete R;

  ph->newValue( 100 );
}



/**
 * @brief Find minimum and maximum value skipping the extreems
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
