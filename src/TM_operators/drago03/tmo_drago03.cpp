/**
 * @brief Frederic Drago logmapping operator
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
 * $Id: tmo_drago03.cpp,v 1.1 2004/09/22 10:00:23 krawczyk Exp $
 */

#include <iostream>
#include <math.h>
#include <assert.h>
#include "../../Libpfs/array2d.h"
#include "tmo_drago03.h"

/// Type of algorithm
#define FAST 0


inline float biasFunc(float b, float x)
{
  return pow(x, b);		// pow(x, log(bias)/log(0.5)
}

//-------------------------------------------


void tmo_drago03(pfs::Array2D* Y, pfs::Array2D* L, float maxLum, float avLum, float bias)
{
  const float LOG05 = -0.693147f; // log(0.5)

  assert(Y!=NULL);
  assert(L!=NULL);

  int nrows = Y->getRows();			// image size
  int ncols = Y->getCols();
  assert(nrows==L->getRows() && ncols==L->getCols() );

  maxLum /= avLum;							// normalize maximum luminance by average luminance

  float divider = log10(maxLum+1.0f);
  float biasP = log(bias)/LOG05;

#if !FAST
  // Normal tone mapping of every pixel
  for( int y=0 ; y<nrows; y++) 
    for( int x=0 ; x<ncols; x++)
    {
      float Yw = (*Y)(x,y) / avLum;
      float interpol = log (2.0f + biasFunc(biasP, Yw / maxLum) * 8.0f);
      (*L)(x,y) = ( log(Yw+1.0f)/interpol ) / divider;
    }
#else
  // 	Approximation of log(x+1)
  // 		x(6+x)/(6+4x) good if x < 1
  //     x*(6 + 0.7662x)/(5.9897 + 3.7658x) between 1 and 2
  //     http://users.pandora.be/martin.brown/home/consult/logx.htm
  int i,j;
  for(int y=0; y<nrows; y+=3) 
    for(int x=0; x<ncols; x+=3)
    {
      float average = 0.0f;
      for (i=0; i<3; i++)
	for (j=0; j<3; j++) 
	  average += (*Y)(x+i,y+j) / avLum;
      average = average / 9.0f - (*Y)(x,y);
			
      if (average>-1.0f && average<1.0f ) 
      {
	float interpol = log(2.0f + biasFunc(biasP, (*Y)(x+1,y+1)/maxLum) * 8.0f);
	for (i=0; i<3; i++)
	  for (j=0; j<3; j++) 
	  {
	    float Yw = (*Y)(x+i,y+j);
	    if( Yw<1.0f ) 
	    {
	      float L = Yw*(6.0f+Yw) / (6.0f+4.0f*Yw);
	      Yw = (L/interpol) / divider;
	    }
	    else if( Yw>=1.0f && Yw<2.0f ) 
	    {
	      float L = Yw*(6.0f+0.7662*Yw) / (5.9897f+3.7658f*Yw);
	      Yw = (L/interpol) / divider;
	    }
	    else
	      Yw = ( log(Yw+1.0f)/interpol ) / divider;
	    (*L)(x+i,y+j) = Yw;
	  }
      }
      else 
      {
	for (i=0; i<3; i++)
	  for (j=0; j<3; j++) 
	  {
	    float Yw = (*Y)(x+i,y+j);
	    float interpol = log(2.0f+biasFunc(biasP, Yw/maxLum)*8.0f);
	    (*L)(x+i,y+j) = ( log(Yw+1.0f)/interpol ) / divider;
	  }
      }
    }	
#endif // #else #ifndef FAST

}

