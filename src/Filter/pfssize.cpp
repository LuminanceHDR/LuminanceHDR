/**
 * @brief Resize images in PFS stream
 * 
 * This file is a part of PFSTOOLS package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
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
 *
 * $Id: pfssize.cpp,v 1.4 2009/01/29 00:44:30 rafm Exp $
 */

#include <cmath>
#include <assert.h>
#include <iostream>

#include "Common/msec_timer.h"
#include "Libpfs/array2d.h"
#include "Libpfs/frame.h"
#include "Libpfs/domio.h"

#define ROUNDING_ERROR 0.000001

namespace pfs
{
class ResampleFilter
{
public:
  /**
   * Size of the filter in samples.
   */
  virtual float getSize() = 0;
  /**
   * Get value of the filter for x. x is always positive.
   */
  virtual float getValue( const float x ) = 0;

  virtual ~ResampleFilter()
  {
  }
  
};

void resize( const pfs::Array2D *src, pfs::Array2D *dest );
void resampleMitchell( const pfs::Array2D *in, pfs::Array2D *out );
void resampleArray( const pfs::Array2D *in, pfs::Array2D *out, ResampleFilter *filter );

#include "arch/minmax.h"
// --------- Filters --------

class MitchellFilter : public ResampleFilter
{
public:
  float getSize() { return 2; }
  float getValue( const float x ) 
  {
    const float B = 0.3333f, C = 0.3333f;
    const float x2 = x*x;
    if( x < 1 )
      return 1.f/6.f * (
        (12-9*B-6*C)*x2*x +
        (-18+12*B+6*C)*x2 +
        (6-2*B) );
    if( x >=1 && x < 2 )
      return 1.f/6.f * (
        (-B-6*C)*x2*x +
        (6*B + 30*C)*x2 +
        (-12*B-48*C)*x +
        (8*B+24*C) );
    return 0;
  }
};

class LinearFilter : public ResampleFilter
{
public:
  float getSize() { return 1; }
  float getValue( const float x ) 
  {
    if( x < 1 ) return 1 - x;
    return 0;
  }
};

// TODO: double checked! 
// Davide Anastasia <davideanastasia@users.sourceforge.net>

//class BoxFilter : public ResampleFilter
//{
//public:
//  float getSize() { return 0.5; }
//  float getValue( const float x ) 
//  {
//    return 1;
//  }
//};

pfs::Frame* resizeFrame(pfs::Frame* frame, int xSize)
{
#ifdef TIMER_PROFILING
  msec_timer f_timer;
  f_timer.start();
#endif

  LinearFilter filter;
  
  int new_x = xSize;
  int new_y = (int)((float)frame->getHeight() * (float)xSize / (float)frame->getWidth());
  
  pfs::Frame *resizedFrame = pfs::DOMIO::createFrame( new_x, new_y );
  
  const ChannelContainer& channels = frame->getChannels();

  for ( ChannelContainer::const_iterator it = channels.begin();
        it != channels.end();
        ++it)
  {
      const pfs::Channel* originalCh = *it;
      pfs::Channel* newCh = resizedFrame->createChannel( originalCh->getName() );

      resampleArray(originalCh->getChannelData(), newCh->getChannelData(), &filter);
  }

  pfs::copyTags( frame, resizedFrame );
  
#ifdef TIMER_PROFILING
  f_timer.stop_and_update();
  std::cout << "resizeFrame() = " << f_timer.get_time() << " msec" << std::endl;
#endif 
  
  return resizedFrame;
}


void upsampleArray( const pfs::Array2D *in, pfs::Array2D *out, ResampleFilter *filter )
{
  float dx = (float)in->getCols() / (float)out->getCols();
  float dy = (float)in->getRows() / (float)out->getRows();

  //float pad;
  
  //float filterSamplingX = max( modff( dx, &pad ), 0.01f );
  //float filterSamplingY = max( modff( dy, &pad ), 0.01f );

  const int outRows = out->getRows();
  const int outCols = out->getCols();

  const float inRows = (float)in->getRows();
  const float inCols = (float)in->getCols();

  const float filterSize = filter->getSize();

// TODO: possible optimization: create lookup table for the filter
  
  float sx, sy;
  int x, y;
  for( y = 0, sy = -0.5f + dy/2; y < outRows; y++, sy += dy )
    for( x = 0, sx = -0.5f + dx/2; x < outCols; x++, sx += dx ) {

      float pixVal = 0;
      float weight = 0;
      
      for( float ix = max( 0, ceilf( sx-filterSize ) ); ix <= min( floorf(sx+filterSize), inCols-1 ); ix++ )
        for( float iy = max( 0, ceilf( sy-filterSize ) ); iy <= min( floorf( sy+filterSize), inRows-1 ); iy++ ) {
          float fx = fabs( sx - ix );
          float fy = fabs( sy - iy );

          const float fval = filter->getValue( fx )*filter->getValue( fy );
          
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

void downsampleArray(const pfs::Array2D *in, pfs::Array2D *out)
{
  const float* Vin  = in->getRawData();
  float* Vout       = out->getRawData();
  
  const unsigned int inRows = in->getRows();
  const unsigned int inCols = in->getCols();

  const unsigned int outRows = out->getRows();
  const unsigned int outCols = out->getCols();

  const float dx = (float)inCols/(float)outCols;
  const float dy = (float)inRows/(float)outRows;
  
  const float filterSize = 0.5;
  
  float sx, sy;
  unsigned int x, y;
  unsigned int IY_L, IY_U, IX_L, IX_U;
  
  float pixVal, w;
  
  for (y = 0, sy = (dy/2.0f - 0.5f); y < outRows; y++, sy += dy)
  {
    IY_L = (unsigned int)max( 0, ceilf( sy-dx*filterSize ) );
    IY_U = (unsigned int)min( floorf(sy+dx*filterSize), inRows-1 );
    
    for (x = 0, sx = (dx/2.0f - 0.5f); x < outCols; x++, sx += dx)
    {
      pixVal  = 0.0f;
      w       = 0.0f;
      
      IX_L = (unsigned int)max( 0, ceilf( sx-dx*filterSize ) );
      IX_U = (unsigned int)min( floorf(sx+dx*filterSize), inCols-1 );
      
      for (unsigned int iy = IY_L; iy <= IY_U; iy++)
      {        
        for (unsigned int ix = IX_L; ix <= IX_U; ix++)
        {
          pixVal  += Vin[iy*inCols + ix];
          w       += 1.0f;
        }
      }
      Vout[y*outCols+x] = pixVal/w;
    }
  }
}

void resampleArray(const pfs::Array2D *in, pfs::Array2D *out, ResampleFilter *filter )
{
  if ( in->getCols() == out->getCols() && in->getRows() == out->getRows() )
  {
    pfs::copyArray(in, out);
  }
  else if( in->getCols() > out->getCols() || in->getRows() > out->getRows() )
  {
    downsampleArray(in, out);
  }
  else
  {
    upsampleArray(in, out, filter);
  }
}
}
