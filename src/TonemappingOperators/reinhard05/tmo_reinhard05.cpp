/**
 * @file tmo_reinhard05.cpp
 * @brief Tone map XYZ channels using Reinhard05 model
 *
 * Dynamic Range Reduction Inspired by Photoreceptor Physiology.
 * E. Reinhard and K. Devlin.
 * In IEEE Transactions on Visualization and Computer Graphics, 2005.
 *
 * 
 * This file is a part of LuminanceHDR package, based on pfstmo.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2007 Grzegorz Krawczyk
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
 * $Id: tmo_reinhard05.cpp,v 1.3 2008/11/04 23:43:08 rafm Exp $
 */

#include <math.h>

#include "TonemappingOperators/pfstmo.h"
#include "tmo_reinhard05.h"

#include <assert.h>


void tmo_reinhard05(unsigned int width, unsigned int height,
  float* nR, float* nG, float* nB, 
  const float* nY, float br, float ca, float la, ProgressHelper *ph )
{
  const pfs::Array2DImpl* Y = new pfs::Array2DImpl(width, height, const_cast<float*>(nY));
  pfs::Array2DImpl* R = new pfs::Array2DImpl(width, height, nR);
  pfs::Array2DImpl* G = new pfs::Array2DImpl(width, height, nG);
  pfs::Array2DImpl* B = new pfs::Array2DImpl(width, height, nB);

  float max_lum = (*Y)(0);
  float min_lum = (*Y)(0);
  float world_lum = 0.0;
  float Cav[] = { 0.0f, 0.0f, 0.0f};
  float Lav = 0.0f;
  int im_width = Y->getCols();
  int im_height = Y->getRows();
  int im_size = im_width * im_height;

  for( int i=1 ; i<im_size ; i++ )
  {
    float lum = (*Y)(i);
    max_lum = (max_lum > lum) ? max_lum : lum;
    min_lum = (min_lum < lum) ? min_lum : lum;
    world_lum += log(2.3e-5+lum);
    Cav[0] += (*R)(i);
    Cav[1] += (*G)(i);
    Cav[2] += (*B)(i);
    Lav += lum;
  }
  world_lum /= im_size;
  Cav[0] /= im_size;
  Cav[1] /= im_size;
  Cav[2] /= im_size;
  Lav /= im_size;

  //--- tone map image
  max_lum = log( max_lum );
  min_lum = log( min_lum );

  // image key
  float k = (max_lum - world_lum) / (max_lum - min_lum);
  // image contrast based on key value
  float m = 0.3f+0.7f*pow(k,1.4f);
  // image brightness
  float f = exp(-br);

  float max_col = 0.0f;
  float min_col = 1.0f;

  int x,y;
  for( x=0 ; x<im_width ; x++ ) {
    ph->newValue(100*x/im_width);
	if (ph->isTerminationRequested())
		break;
    for( y=0 ; y<im_height ; y++ )
    {
      float l = (*Y)(x,y);
      float col;
      if( l != 0.0f )
      {
        for( int c=0 ; c<3 ; c++ )
        {
          switch(c)
          {
          case 0: col = (*R)(x,y); break;
          case 1: col = (*G)(x,y); break;
          case 2: col = (*B)(x,y); break;
          };

          if( col!=0.0f)
          {
            // local light adaptation
            float Il = ca * col + (1-ca)*l;
            // global light adaptation
            float Ig = ca*Cav[c] + (1-ca)*Lav;
            // interpolated light adaptation
            float Ia = la*Il + (1-la)*Ig;
            // photoreceptor equation
            col /= col + pow(f*Ia, m);
          }
        
          max_col = (col>max_col) ? col : max_col;
          min_col = (col<min_col) ? col : min_col;

          switch(c)
          {
          case 0: (*R)(x,y) = col; break;
          case 1: (*G)(x,y) = col; break;
          case 2: (*B)(x,y) = col; break;
          };
        }
      }
    }
  }
  //--- normalize intensities
  for( x=0 ; x<im_width ; x++ ) {
    ph->newValue(100*x/im_width);
	if (ph->isTerminationRequested())
		break;
    for( y=0 ; y<im_height ; y++ )
    {
      (*R)(x,y) = ((*R)(x,y)-min_col)/(max_col-min_col);
      (*G)(x,y) = ((*G)(x,y)-min_col)/(max_col-min_col);
      (*B)(x,y) = ((*B)(x,y)-min_col)/(max_col-min_col);
    }
  }
  delete B;
  delete G;
  delete R;
  delete Y;
}
