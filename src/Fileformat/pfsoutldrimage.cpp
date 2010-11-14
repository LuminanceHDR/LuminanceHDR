/**
 * @brief Writing QImage from PFS stream (which is a tonemapped LDR)
 *
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2006 Giuseppe Rota
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
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 *
 */

#include <QImage>
#include <QSysInfo>
#include <iostream>
#include <assert.h>

#include "Libpfs/pfs.h"
#include "Libpfs/colorspace.h"
#include "Common/msec_timer.h"

static inline unsigned char clamp( const float v, const unsigned char minV, const unsigned char maxV )
{
    if( v < /*(float)*/minV ) return minV;
    if( v > /*(float)*/maxV ) return maxV;
    return (unsigned char)v;
}

QImage fromLDRPFStoQImage( pfs::Frame* inpfsframe , pfs::ColorSpace display_colorspace )
{
#ifdef TIMER_PROFILING
  msec_timer __timer;
  __timer.start();
#endif
  
	assert(inpfsframe!=NULL);
  
	pfs::Channel *Xc, *Yc, *Zc;
	inpfsframe->getXYZChannels( Xc, Yc, Zc );
	assert( Xc != NULL && Yc != NULL && Zc != NULL );
  
  pfs::Array2DImpl  *X = Xc->getChannelData();
	pfs::Array2DImpl  *Y = Yc->getChannelData();
  pfs::Array2DImpl  *Z = Zc->getChannelData();
  
	// Back to CS_RGB for the Viewer
	pfs::transformColorSpace(pfs::CS_XYZ, X, Y, Z, display_colorspace, X, Y, Z);	
	
	const int width   = Xc->getWidth();
	const int height  = Xc->getHeight();
	const int elems   = width*height;
  
	unsigned char* data = new uchar[elems*4]; //this will contain the image data: data must be 32-bit aligned, in Format: 0xffRRGGBB
  
  if (QSysInfo::ByteOrder == QSysInfo::LittleEndian)
  {
    // Intel Processors    
    float* p_Z = Z->getRawData();
    float* p_Y = Y->getRawData();
    float* p_X = X->getRawData();
    
    unsigned char* p_data = data;
    
    for ( int c = elems; c > 0; c-- )
    {
      (*p_data) = clamp((*p_Z)*255.f, 0, 255);  p_Z++;  p_data++;
      (*p_data) = clamp((*p_Y)*255.f, 0, 255);  p_Y++;  p_data++;
      (*p_data) = clamp((*p_X)*255.f, 0, 255);  p_X++;  p_data++;
      (*p_data) = 0xFF;                                 p_data++;
    }
  }
  else
  {
    // Motorola Processors
    // I have a feeling that this piece of code is not right!
    for( int y = 0; y < height; y++ ) // For each row of the image
    { 
      for( int x = 0; x < width; x++ )
      {
        *(data + 3 + (y*width+x)*4) = ( clamp( (*X)( x, y )*255.f, 0, 255) );
        *(data + 2 + (y*width+x)*4) = ( clamp( (*Y)( x, y )*255.f, 0, 255) );
        *(data + 1 + (y*width+x)*4) = ( clamp( (*Z)( x, y )*255.f, 0, 255) );
        *(data + 0 + (y*width+x)*4) = 0xff;
      }
    }
  }
  
#ifdef TIMER_PROFILING
  __timer.stop_and_update();
  std::cout << "fromLDRPFStoQImage() = " << __timer.get_time() << " msec" << std::endl;
#endif
  
	return QImage (const_cast<unsigned char*>(data), width, height, QImage::Format_ARGB32);
}
