/**
 * @file fastbilateral.cpp
 * @brief Fast bilateral filtering
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
 * $Id: fastbilateral.cpp,v 1.5 2005/12/15 15:53:37 krawczyk Exp $
 */


#include <iostream>

#include <fftw3.h>
#include <math.h>

#include "../../Libpfs/pfs.h"

using namespace std;

void downsampleArray( const pfs::Array2D *in, pfs::Array2D *out );

inline float max( float a, float b )
{
  return a > b ? a : b;
}

inline float min( float a, float b )
{
  return a < b ? a : b;
}


void convolveArray( const pfs::Array2D *I, float sigma, pfs::Array2D *J )
{
  int i,x,y;

  int nx = I->getCols();
  int ny = I->getRows();
  int nsize = nx * ny;

  int ox = nx;
  int oy = ny/2 + 1;            // saves half of the data
  int osize = ox * oy;
 
  fftwf_plan fplan;             // fft transformation plan
  float* source = (float*) fftwf_malloc(sizeof(float) * nx * 2 * (ny/2+1) );
  fftwf_complex* freq = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * osize);

  for( x=0 ; x<nx ; x++ )
    for( y=0 ; y<ny ; y++ )
      source[x*ny+y] = (*I)(x,y);

  fplan = fftwf_plan_dft_r2c_2d(nx, ny, source, freq, FFTW_ESTIMATE);
  fftwf_execute(fplan);
  fftwf_destroy_plan(fplan);

  // filter
  float sig = nx/(2.0f*sigma);
  float sig2 = 2.0f*sig*sig;
  for( x=0 ; x<ox/2 ; x++ )
    for( y=0 ; y<oy ; y++ )
    {
      float d2 = x*x + y*y;
      float kernel = exp( -d2 / sig2 );

      freq[x*oy+y][0] *= kernel;
      freq[x*oy+y][1] *= kernel;
      freq[(ox-x-1)*oy+y][0] *= kernel;
      freq[(ox-x-1)*oy+y][1] *= kernel;
    }

  fplan = fftwf_plan_dft_c2r_2d(nx, ny, freq, source, FFTW_ESTIMATE);
  fftwf_execute(fplan);
  fftwf_destroy_plan(fplan);

  for( x=0 ; x<nx ; x++ )
    for( y=0 ; y<ny ; y++ )
      (*J)(x,y) = source[x*ny+y] / nsize;
  
  fftwf_free(source); 
  fftwf_free(freq);
}


/**
 * @brief upsampling and downsampling
 *
 * original code from pfssize
 */
void upsampleArray( const pfs::Array2D *in, pfs::Array2D *out )
{
  float dx = (float)in->getCols() / (float)out->getCols();
  float dy = (float)in->getRows() / (float)out->getRows();

  float pad;
  
  float filterSamplingX = max( modff( dx, &pad ), 0.01f );
  float filterSamplingY = max( modff( dy, &pad ), 0.01f );

  const int outRows = out->getRows();
  const int outCols = out->getCols();

  const float inRows = in->getRows();
  const float inCols = in->getCols();

  const float filterSize = 1;

  float sx, sy;
  int x, y;
  for( y = 0, sy = -0.5 + dy/2; y < outRows; y++, sy += dy )
    for( x = 0, sx = -0.5 + dx/2; x < outCols; x++, sx += dx ) {

      float pixVal = 0;
      float weight = 0;
      
      for( float ix = max( 0, ceilf( sx-filterSize ) ); ix <= min( floorf(sx+filterSize), inCols-1 ); ix++ )
        for( float iy = max( 0, ceilf( sy-filterSize ) ); iy <= min( floorf( sy+filterSize), inRows-1 ); iy++ ) {
          float fx = fabs( sx - ix );
          float fy = fabs( sy - iy );

          const float fval = (1.0f-fx)*(1.0f-fy);
          
          pixVal += (*in)( (int)ix, (int)iy ) * fval;
          weight += fval;
        }

      if( weight == 0 ) {
        fprintf( stderr, "%g %g %g %g\n", sx, sy, dx, dy );
      }    
//      assert( weight != 0 );
      (*out)(x,y) = pixVal / weight;

    } 
}

// void downsampleArray( const pfs::Array2D *in, pfs::Array2D *out )
// {
//   const float inRows = in->getRows();
//   const float inCols = in->getCols();
// 
//   const int outRows = out->getRows();
//   const int outCols = out->getCols();
// 
//   const float dx = (float)in->getCols() / (float)out->getCols();
//   const float dy = (float)in->getRows() / (float)out->getRows();
// 
//   const float filterSize = 0.5;
//   
//   float sx, sy;
//   int x, y;
//   
//   for( y = 0, sy = dy/2-0.5; y < outRows; y++, sy += dy )
//     for( x = 0, sx = dx/2-0.5; x < outCols; x++, sx += dx ) {
// 
//       float pixVal = 0;
//       float w = 0;
//       for( float ix = max( 0, ceilf( sx-dx*filterSize ) ); ix <= min( floorf( sx+dx*filterSize ), inCols-1 ); ix++ )
//         for( float iy = max( 0, ceilf( sy-dx*filterSize ) ); iy <= min( floorf( sy+dx*filterSize), inRows-1 ); iy++ ) {
//           pixVal += (*in)( (int)ix, (int)iy );
//           w += 1;
//         }     
//       (*out)(x,y) = pixVal/w;      
//     }
// }



/* 
Pseudocode from paper:

PiecewiseBilateral (Image I, spatial kernel fs , intensity influence gr )
  J=0 // set the output to zero
  for j=0..NB SEGMENTS 
    ij= minI+j.*(max(I)-min(I))/NB SEGMENTS
    Gj=gr (I - ij ) // evaluate gr at each pixel
    Kj=Gj x fs // normalization factor
    Hj=Gj .* I // compute H for each pixel
    Hj=Hj x fs 
    Jj=Hj ./ Kj // normalize
    J=J+Jj .*  InterpolationWeight(I, ij )
*/

void fastBilateralFilter( const pfs::Array2D *I,
  pfs::Array2D *J, float sigma_s, float sigma_r, int downsample)
{
  int i;
  int w = I->getCols();
  int h = I->getRows();
  int size = w * h;

  // find range of values in the input array
  float maxI = (*I)(0);
  float minI = (*I)(0);
  for(i=0 ; i<size ; i++)
  {
    float v = (*I)(i);
    if( v>maxI ) maxI = v;
    if( v<minI ) minI = v;
    (*J)(i) = 0.0f;             // zero output
  }

  pfs::Array2DImpl* JJ = new pfs::Array2DImpl(w,h);
  
  w /= downsample;
  h /= downsample;
  int sizeZ = w*h;
  pfs::Array2DImpl* Iz = new pfs::Array2DImpl(w,h);
  downsampleArray(I,Iz);
  sigma_s /= downsample;
  
  pfs::Array2DImpl* jJ = new pfs::Array2DImpl(w,h);
  pfs::Array2DImpl* jG = new pfs::Array2DImpl(w,h);
  pfs::Array2DImpl* jK = new pfs::Array2DImpl(w,h);
  pfs::Array2DImpl* jH = new pfs::Array2DImpl(w,h);

  const int NB_SEGMENTS = 17;
  float stepI = (maxI-minI)/NB_SEGMENTS;

  // piecewise bilateral
  for( int j=0 ; j<NB_SEGMENTS ; j++ )
  {
    float jI = minI + j*stepI;        // current intensity value
    
    for( i=0 ; i<sizeZ ; i++ )
    {
      float dI = (*Iz)(i)-jI;
      (*jG)(i) = exp( -(dI*dI) / (sigma_r*sigma_r) );
      (*jH)(i) = (*jG)(i) * (*I)(i);
    }

    convolveArray(jG, sigma_s, jK);
    convolveArray(jH, sigma_s, jH);

    for( i=0 ; i<sizeZ ; i++ )
      if( (*jK)(i)!=0.0f )
        (*jJ)(i) = (*jH)(i) / (*jK)(i);
      else
        (*jJ)(i) = 0.0f;

    upsampleArray(jJ,JJ);

    for( i=0 ; i<size ; i++ )
    {
      float wi = (stepI - fabs( (*I)(i)-jI )) / stepI;
      if( wi>0.0f )
        (*J)(i) += (*JJ)(i)*wi;
    }
  }

  delete JJ;
  delete jJ;
  delete jG;
  delete jK;
  delete jH;
  delete Iz;
}
