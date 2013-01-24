/**
 * @file bilateral.cpp
 * @brief Bilateral filtering
 *
 * 
 * This file is a part of LuminanceHDR package, based on pfstmo.
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
 * @author Rafal Mantiuk, <mantiuk@mpi-sb.mpg.de>
 * @author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
 *
 * $Id: bilateral.cpp,v 1.3 2008/09/09 00:56:49 rafm Exp $
 */

#include "arch/math.h"

#include "TonemappingOperators/pfstmo.h"
#include "Libpfs/array2d.h"
#include "Libpfs/progress.h"

#ifdef BRANCH_PREDICTION
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif

inline int max( int a, int b )
{ return (a>b) ? a : b; }

inline int min( int a, int b )
{ return (a<b) ? a : b; }


// support functions

void gaussianKernel( pfs::Array2D *kern, float sigma )
{
  for( int y = 0; y < kern->getRows(); y++ ) {
    for( int x = 0; x < kern->getCols(); x++ ) {
      float rx = (float)(x - kern->getCols()/2);
      float ry = (float)(y - kern->getRows()/2);
      float d2 = rx*rx + ry*ry;
      (*kern)(x, y) = exp( -d2 / (2.*sigma*sigma) );
    }
  }
}

class GaussLookup {
  float *gauss;
  float maxVal;
  float scaleFactor;
  int N;
public:
  GaussLookup( float sigma, int N ) : N(N)
    {
      float sigma2 = sigma*sigma;
      maxVal = sqrt(-log(0.01)*2.0*sigma2);
      gauss = new float[N];
      for( int i = 0; i < N; i++ ) {
	float x = (float)i/(float)(N-1)*maxVal;
	gauss[i] = exp(-x*x/(2.0*sigma2));
      }
      scaleFactor = (float)(N-1) / maxVal;
    }

  float getValue( float x )
    {
      x = fabs( x );
      if( unlikely( x > maxVal ) ) return 0;
      return gauss[ (int)(x*scaleFactor) ];
    }

};



void bilateralFilter(const pfs::Array2D *I, pfs::Array2D *J,
                     float sigma_s, float sigma_r,
                     pfs::Progress& ph)
{
  const pfs::Array2D *X1 = I;     // intensity data     // DAVIDE : CHECK THIS!

  // x +- sigma_s*2 should contain 95% of the Gaussian distrib
  int sKernelSize = (int)( sigma_s*4 + 0.5 ) + 1;

  pfs::Array2D sKernel(sKernelSize, sKernelSize);
  gaussianKernel( &sKernel, sigma_s );
  GaussLookup gauss( sigma_r, 256 );

  for( int y = 0; y < I->getRows(); y++ )
  {
    ph.setValue( y * 100 / I->getRows() );
    
    for( int x = 0; x < I->getCols(); x++ )
    {
      float val = 0;
      float k = 0;
      float I_s = (*X1)(x,y);	//!! previously 'I' not 'X1'

      if( unlikely( !finite( I_s ) ) )
        I_s = 0.0f;

      for( int py = max( 0, y - sKernelSize/2);
	   py < min( I->getRows(), y + sKernelSize/2); py++ )
      {
	for( int px = max( 0, x - sKernelSize/2);
	     px < min( I->getCols(), x + sKernelSize/2); px++ )
	{
	  float I_p = (*X1)(px, py);	//!! previously 'I' not 'X1'
          if( unlikely( !finite( I_p ) ) )
            I_p = 0.0f;
	  
	  float mult = sKernel(px-x + sKernelSize/2, py-y + sKernelSize/2) *
	      gauss.getValue( I_p - I_s );

	  float Ixy = (*I)(px, py);
          if( unlikely( !finite( Ixy ) ) )
            Ixy = 0.0f;          
	  
	  val += Ixy*mult;	//!! but here we want 'I'
	  k += mult;
	}
      }
      //avoid division by 0 when k is close to 0
//         (*J)(x,y) = fabs(k) > 0.00000001 ? val/k : 0.;
      (*J)(x,y) = val/k;
    }
  }
}
