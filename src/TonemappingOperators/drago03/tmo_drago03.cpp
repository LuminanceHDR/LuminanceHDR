/**
 * @brief Frederic Drago logmapping operator
 * 
 * Adaptive logarithmic mapping for displaying high contrast
 * scenes. 
 * F. Drago, K. Myszkowski, T. Annen, and N. Chiba. In Eurographics 2003.
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
 * @author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
 *
 * $Id: tmo_drago03.cpp,v 1.4 2008/11/04 23:43:08 rafm Exp $
 */

#include "tmo_drago03.h"

#include <cmath>
#include <cassert>

#include "Libpfs/frame.h"
#include "Libpfs/progress.h"
#include "TonemappingOperators/pfstmo.h"

namespace
{

inline
float biasFunc(float b, float x)
{
    return std::pow(x, b);		// pow(x, log(bias)/log(0.5)
}

const float LOG05 = -0.693147f; // log(0.5)
}

//-------------------------------------------

void calculateLuminance(unsigned int width, unsigned int height,
                        const float* Y, float& avLum, float& maxLum)
{
    avLum = 0.0f;
    maxLum = 0.0f;

    int size = width * height;

    for( int i=0 ; i<size; i++ )
    {
        avLum += log( Y[i] + 1e-4 );
        maxLum = ( Y[i] > maxLum ) ? Y[i] : maxLum ;
    }
    avLum =exp( avLum/ size);
}


void tmo_drago03(const pfs::Array2D& Y, pfs::Array2D& L,
                 float maxLum, float avLum, float bias, 
                 pfs::Progress &ph)
{
    assert(Y.getRows() == L.getRows());
    assert(Y.getCols() == L.getCols());

    // normalize maximum luminance by average luminance
    maxLum /= avLum;

    float divider = std::log10(maxLum + 1.0f);
    float biasP = log(bias)/LOG05;

    // Normal tone mapping of every pixel
    for (int y=0, yEnd = Y.getRows(); y < yEnd; y++)
    {
        ph.setValue(100*y/yEnd);
        if (ph.canceled())
            break;

        for (int x=0, xEnd = Y.getCols(); x < xEnd; x++)
        {
            float Yw = Y(x,y) / avLum;
            float interpol = std::log (2.0f + biasFunc(biasP, Yw / maxLum) * 8.0f);
            L(x,y) = ( std::log(Yw+1.0f)/interpol ) / divider;
        }
    }

#if 0
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
#endif
}

