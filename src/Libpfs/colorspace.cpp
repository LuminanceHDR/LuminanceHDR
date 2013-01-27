/**
 * @brief PFS library - color space transformations
 *
 * This file is a part of Luminance HDR package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
 * 
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ---------------------------------------------------------------------- 
 * 
 * @author Rafal Mantiuk, <mantiuk@mpi-sb.mpg.de>
 *
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net> (2010 10 13)
 *  Reimplementation of most of the functions (in particular the ones involving RGB and XYZ)
 *
 * $Id: colorspace.cpp,v 1.6 2007/07/18 08:49:25 rafm Exp $
 */

#include "colorspace.h"

#include <cassert>
#include <iostream>
#include <map>

#include <cmath>
// #include "arch/math.h"

#include "Libpfs/pfs.h"
#include "Libpfs/array2d.h"
#include "Libpfs/utils/msec_timer.h"
#include "Libpfs/vex/sse.h"

#include <boost/assign.hpp>

using namespace std;
using namespace boost::assign;

namespace pfs 
{
namespace
{
template <typename T>
inline
T clamp(T v, T min, T max)
{
    if ( v < min ) return min;
    else if( v > max ) return max;
    else return v;
}
}

//! \brief Basic matrices for the SRGB <-> XYZ conversion
//! \ref http://www.brucelindbloom.com/Eqn_RGB_XYZ_Matrix.html
static const float rgb2xyzD65Mat[3][3] =
{ { 0.4124564f, 0.3575761f, 0.1804375f },
  { 0.2126729f, 0.7151522f, 0.0721750f },
  { 0.0193339f, 0.1191920f, 0.9503041f } };

static const float xyz2rgbD65Mat[3][3] =
{ {  3.2404542f, -1.5371385f, -0.4985314f },
  { -0.9692660f,  1.8760108f,  0.0415560f },
  {  0.0556434f, -0.2040259f,  1.0572252f } };
  
  //-----------------------------------------------------------
  // sRGB conversion functions
  //-----------------------------------------------------------
  void transformSRGB2XYZ(const Array2Df *inC1, const Array2Df *inC2, const Array2Df *inC3,
                         Array2Df *outC1, Array2Df *outC2, Array2Df *outC3)
  {
#ifdef TIMER_PROFILING
      msec_timer f_timer;
      f_timer.start();
#endif

      const float* r = inC1->getRawData();
      const float* g = inC2->getRawData();
      const float* b = inC3->getRawData();

      float* x = outC1->getRawData();
      float* y = outC2->getRawData();
      float* z = outC3->getRawData();

      float i1, i2, i3;
      float t1, t2, t3;

      int elems = inC1->getRows()*inC1->getCols();

#pragma omp parallel for private(i1,i2,i3,t1,t2,t3)
      for( int idx = 0; idx < elems ; idx++ )
      {
          i1 = clamp(r[idx], 0.f, 1.f);
          i2 = clamp(g[idx], 0.f, 1.f);
          i3 = clamp(b[idx], 0.f, 1.f);

          t1 = (i1 <= 0.04045f ? i1 / 12.92f : powf( (i1 + 0.055f) / 1.055f, 2.4f )  );
          t2 = (i2 <= 0.04045f ? i2 / 12.92f : powf( (i2 + 0.055f) / 1.055f, 2.4f )  );
          t3 = (i3 <= 0.04045f ? i3 / 12.92f : powf( (i3 + 0.055f) / 1.055f, 2.4f )  );

          x[idx] = rgb2xyzD65Mat[0][0]*t1 + rgb2xyzD65Mat[0][1]*t2 + rgb2xyzD65Mat[0][2]*t3;
          y[idx] = rgb2xyzD65Mat[1][0]*t1 + rgb2xyzD65Mat[1][1]*t2 + rgb2xyzD65Mat[1][2]*t3;
          z[idx] = rgb2xyzD65Mat[2][0]*t1 + rgb2xyzD65Mat[2][1]*t2 + rgb2xyzD65Mat[2][2]*t3;
      }

#ifdef TIMER_PROFILING
      f_timer.stop_and_update();
      std::cout << "transformSRGB2XYZ() = " << f_timer.get_time() << " msec" << std::endl;
#endif
  }

  namespace
  {

  static const float SRGB_INVERSE_COMPANDING_THRESHOLD = 0.04045f;
  static const float SRGB_INVERSE_DIVIDER_SHADOW = 1.f/12.92f;
  static const float SRGB_INVERSE_SHIFT = 0.055f;
  static const float SRGB_INVERSE_DIVIDER_HIGHLIGHT = 1.f/1.055f;
  static const float SRGB_INVERSE_GAMMA = 2.4f;

  inline
  float inverseSRGBCompanding(float sample)
  {
      if ( sample <= SRGB_INVERSE_COMPANDING_THRESHOLD )
          return sample*SRGB_INVERSE_DIVIDER_SHADOW;
      else
          return std::pow( (sample + SRGB_INVERSE_SHIFT)*SRGB_INVERSE_DIVIDER_HIGHLIGHT,
                             SRGB_INVERSE_GAMMA );
  }

  inline
  float kernelTrasformSRGB2Y(float red, float green, float blue)
  {
      return ( rgb2xyzD65Mat[1][0]*inverseSRGBCompanding(red) +
               rgb2xyzD65Mat[1][1]*inverseSRGBCompanding(green) +
               rgb2xyzD65Mat[1][2]*inverseSRGBCompanding(blue) );
  }

  }   // anonymous namespace

  void transformSRGB2Y(const Array2Df *inC1, const Array2Df *inC2, const Array2Df *inC3,
                       Array2Df *outC1)
  {
      Array2Df::const_iterator r = inC1->begin();
      Array2Df::const_iterator rEnd = inC1->end();
      Array2Df::const_iterator g = inC2->begin();
      Array2Df::const_iterator b = inC3->begin();

      Array2Df::iterator y = outC1->begin();

      while ( r != rEnd )
      {
          *y++ = kernelTrasformSRGB2Y(*r++, *g++, *b++);
      }
  }
  
  void transformXYZ2SRGB(const Array2Df *inC1, const Array2Df *inC2, const Array2Df *inC3,
                         Array2Df *outC1, Array2Df *outC2, Array2Df *outC3)
  {
#ifdef TIMER_PROFILING
      msec_timer f_timer;
      f_timer.start();
#endif

      const float* x = inC1->getRawData();
      const float* y = inC2->getRawData();
      const float* z = inC3->getRawData();

      float* r = outC1->getRawData();
      float* g = outC2->getRawData();
      float* b = outC3->getRawData();

      float i1, i2, i3;
      float t1, t2, t3;

      const int ELEMS = inC1->getRows()*inC1->getCols();

#pragma omp parallel for private(i1,i2,i3,t1,t2,t3)
      for( int idx = 0; idx < ELEMS; idx++ )
      {
          i1 = x[idx];
          i2 = y[idx];
          i3 = z[idx];

          t1 = xyz2rgbD65Mat[0][0]*i1 + xyz2rgbD65Mat[0][1]*i2 + xyz2rgbD65Mat[0][2]*i3;
          t2 = xyz2rgbD65Mat[1][0]*i1 + xyz2rgbD65Mat[1][1]*i2 + xyz2rgbD65Mat[1][2]*i3;
          t3 = xyz2rgbD65Mat[2][0]*i1 + xyz2rgbD65Mat[2][1]*i2 + xyz2rgbD65Mat[2][2]*i3;

          t1 = clamp( t1, 0.f, 1.f );
          t2 = clamp( t2, 0.f, 1.f );
          t3 = clamp( t3, 0.f, 1.f );

          r[idx] = (t1 <= 0.0031308f ? t1 *= 12.92f : 1.055f * powf( t1, 1.f/2.4f ) - 0.055f);
          g[idx] = (t2 <= 0.0031308f ? t2 *= 12.92f : 1.055f * powf( t2, 1.f/2.4f ) - 0.055f);
          b[idx] = (t3 <= 0.0031308f ? t3 *= 12.92f : 1.055f * powf( t3, 1.f/2.4f ) - 0.055f);
      }

#ifdef TIMER_PROFILING
      f_timer.stop_and_update();
      std::cout << "transformXYZ2SRGB() = " << f_timer.get_time() << " msec" << std::endl;
#endif
  }
  
  void transformXYZ2Yuv( const Array2Df *inC1, const Array2Df *inC2, const Array2Df *inC3,
                        Array2Df *outC1, Array2Df *outC2, Array2Df *outC3 )
  {
    const int elems = inC1->getRows()*inC1->getCols();
    for ( int idx = 0; idx < elems; idx++ )
    {
      const float &X = (*inC1)(idx), Y = (*inC2)(idx), &Z = (*inC3)(idx);
      float &outY = (*outC1)(idx), &u = (*outC2)(idx), &v = (*outC3)(idx);
      
      float x = X/(X+Y+Z);
      float y = Y/(X+Y+Z);
      
      //        assert((4.f*nx / (-2.f*nx + 12.f*ny + 3.f)) <= 0.62 );
      //        assert( (9.f*ny / (-2.f*nx + 12.f*ny + 3.f)) <= 0.62 );
      
      u = 4.f*x / (-2.f*x + 12.f*y + 3.f);
      v = 9.f*y / (-2.f*x + 12.f*y + 3.f);
      outY = Y;
    }
    
  }
  
  void transformYuv2XYZ( const Array2Df *inC1, const Array2Df *inC2, const Array2Df *inC3,
                        Array2Df *outC1, Array2Df *outC2, Array2Df *outC3 )
  {
    const int elems = inC1->getRows()*inC1->getCols();
    for( int idx = 0; idx < elems ; idx++ )
    {
      const float Y = (*inC1)(idx), &u = (*inC2)(idx), &v = (*inC3)(idx);
      float &X = (*outC1)(idx), &outY = (*outC2)(idx), &Z = (*outC3)(idx);
      
      float x = 9.f*u / (6.f*u - 16.f*v + 12.f);
      float y = 4.f*v / (6.f*u - 16.f*v + 12.f);
      
      X = x/y * Y;
      Z = (1.f-x-y)/y * Y;
      outY = Y;
    }
  }
  
  void transformYxy2XYZ( const Array2Df *inC1, const Array2Df *inC2, const Array2Df *inC3,
                        Array2Df *outC1, Array2Df *outC2, Array2Df *outC3 )
  {
    const int elems = inC1->getRows()*inC1->getCols();
    for( int idx = 0; idx < elems; idx++ )
    {
      const float Y = (*inC1)(idx), x = (*inC2)(idx), y = (*inC3)(idx);
      float &X = (*outC1)(idx), &outY = (*outC2)(idx), &Z = (*outC3)(idx);
      
      X = x/y * Y;
      Z = (1.f-x-y)/y * Y;
      outY = Y;
    }
  }
  
  void transformXYZ2Yxy( const Array2Df *inC1, const Array2Df *inC2, const Array2Df *inC3,
                        Array2Df *outC1, Array2Df *outC2, Array2Df *outC3 )
  {
    const int elems = inC1->getRows()*inC1->getCols();
    for( int idx = 0; idx < elems; idx++ )
    {
      const float X = (*inC1)(idx), Y = (*inC2)(idx), Z = (*inC3)(idx);
      float &outY = (*outC1)(idx), &x = (*outC2)(idx), &y = (*outC3)(idx);
      
      x = X/(X+Y+Z);
      y = Y/(X+Y+Z);
      
      outY = Y;
    }
  }
  
  void transformRGB2XYZ(const Array2Df *inC1, const Array2Df *inC2, const Array2Df *inC3,
                        Array2Df *outC1, Array2Df *outC2, Array2Df *outC3)
  {
#ifdef TIMER_PROFILING
      msec_timer f_timer;
      f_timer.start();
#endif

      const float* r = inC1->getRawData();
      const float* g = inC2->getRawData();
      const float* b = inC3->getRawData();

      float* x = outC1->getRawData();
      float* y = outC2->getRawData();
      float* z = outC3->getRawData();

      float i1, i2, i3;
      const int ELEMS = inC1->getRows()*inC1->getCols();

#pragma omp parallel for private(i1,i2,i3)
      for( int idx = 0; idx < ELEMS; idx++ )
      {
          i1 = r[idx];
          i2 = g[idx];
          i3 = b[idx];

          x[idx] = rgb2xyzD65Mat[0][0]*i1 + rgb2xyzD65Mat[0][1]*i2 + rgb2xyzD65Mat[0][2]*i3;
          y[idx] = rgb2xyzD65Mat[1][0]*i1 + rgb2xyzD65Mat[1][1]*i2 + rgb2xyzD65Mat[1][2]*i3;
          z[idx] = rgb2xyzD65Mat[2][0]*i1 + rgb2xyzD65Mat[2][1]*i2 + rgb2xyzD65Mat[2][2]*i3;
      }

#ifdef TIMER_PROFILING
      f_timer.stop_and_update();
      std::cout << "transformRGB2XYZ() = " << f_timer.get_time() << " msec" << std::endl;
#endif
  }
  
  namespace
  {
  inline
  void kernelTrasformRGB2Y(float red, float green, float blue, float& y)
  {
      y = rgb2xyzD65Mat[1][0]*red
              + rgb2xyzD65Mat[1][1]*green
              + rgb2xyzD65Mat[1][2]*blue;
  }

  }

  void transformRGB2Y(const Array2Df *inC1, const Array2Df *inC2, const Array2Df *inC3,
                      Array2Df *outC1)
  {
      Array2Df::const_iterator r = inC1->begin();
      Array2Df::const_iterator rEnd = inC1->end();
      Array2Df::const_iterator g = inC2->begin();
      Array2Df::const_iterator b = inC3->begin();

      Array2Df::iterator y = outC1->begin();

      while ( r != rEnd )
      {
          kernelTrasformRGB2Y(*r++, *g++, *b++, *y++);
      }
  }


  void transformXYZ2RGB(const Array2Df *inC1, const Array2Df *inC2, const Array2Df *inC3,
                        Array2Df *outC1, Array2Df *outC2, Array2Df *outC3 )
  {
#ifdef TIMER_PROFILING
      msec_timer f_timer;
      f_timer.start();
#endif

      const float* x = inC1->getRawData();
      const float* y = inC2->getRawData();
      const float* z = inC3->getRawData();

      float* r = outC1->getRawData();
      float* g = outC2->getRawData();
      float* b = outC3->getRawData();

      float i1, i2, i3;
      const int ELEMS = inC1->getRows()*inC1->getCols();

#pragma omp parallel for schedule(static, 5120) private(i1,i2,i3)
      for( int idx = 0; idx < ELEMS; idx++ )
      {
          i1 = x[idx];
          i2 = y[idx];
          i3 = z[idx];

          r[idx] = xyz2rgbD65Mat[0][0]*i1 + xyz2rgbD65Mat[0][1]*i2 + xyz2rgbD65Mat[0][2]*i3;
          g[idx] = xyz2rgbD65Mat[1][0]*i1 + xyz2rgbD65Mat[1][1]*i2 + xyz2rgbD65Mat[1][2]*i3;
          b[idx] = xyz2rgbD65Mat[2][0]*i1 + xyz2rgbD65Mat[2][1]*i2 + xyz2rgbD65Mat[2][2]*i3;
      }

#ifdef TIMER_PROFILING
      f_timer.stop_and_update();
      std::cout << "transformXYZ2RGB() = " << f_timer.get_time() << " msec" << std::endl;
#endif
  }

typedef void (*CSTransformFunc)(const Array2Df *inC1, const Array2Df *inC2, const Array2Df *inC3,
                                Array2Df *outC1, Array2Df *outC2, Array2Df *outC3 );
typedef std::pair<ColorSpace, ColorSpace> CSTransformProfile;
typedef std::map<CSTransformProfile, CSTransformFunc> CSTransformMap;

void transformColorSpace(ColorSpace inCS, const Array2Df *inC1, const Array2Df *inC2, const Array2Df *inC3,
                           ColorSpace outCS, Array2Df *outC1, Array2Df *outC2, Array2Df *outC3)
{
    assert( inC1->getCols() == inC2->getCols() &&
            inC2->getCols() == inC3->getCols() &&
            inC3->getCols() == outC1->getCols() &&
            outC1->getCols() == outC2->getCols() &&
            outC2->getCols() == outC3->getCols() );
    
    assert( inC1->getRows() == inC2->getRows() &&
            inC2->getRows() == inC3->getRows() &&
            inC3->getRows() == outC1->getRows() &&
            outC1->getRows() == outC2->getRows() &&
            outC2->getRows() == outC3->getRows() );
    
    // static dictionary... I already know in advance the subscription I want
    // to perform, hence this approach is far easier than try to create
    // automatic subscription to the factory
    static CSTransformMap s_csTransformMap =
            map_list_of
            // XYZ -> *
            (CSTransformProfile(CS_XYZ, CS_SRGB), transformXYZ2SRGB)
            (CSTransformProfile(CS_XYZ, CS_RGB), transformXYZ2RGB)
            (CSTransformProfile(CS_XYZ, CS_YUV), transformXYZ2Yuv)
            (CSTransformProfile(CS_XYZ, CS_Yxy), transformXYZ2Yxy)
            // sRGB -> *
            (CSTransformProfile(CS_SRGB, CS_XYZ), transformSRGB2XYZ)
            // RGB -> *
            (CSTransformProfile(CS_RGB, CS_XYZ), transformRGB2XYZ)
            // Yuv -> *
            (CSTransformProfile(CS_YUV, CS_XYZ), transformYuv2XYZ)
            // Yxy -> *
            (CSTransformProfile(CS_Yxy, CS_XYZ), transformYxy2XYZ)
            ;

    CSTransformMap::const_iterator itTransform =
            s_csTransformMap.find( CSTransformProfile(inCS, outCS) );

    if ( itTransform == s_csTransformMap.end() )
    {
        throw Exception( "Unsupported color tranform" );
    }
#ifndef NDEBUG
    std::cerr << __FUNCTION__ << ": Found right match for colorspace conversion\n";
#endif
    // CSTransformFunc func =
    (itTransform->second)( inC1, inC2, inC3, outC1, outC2, outC3 );
}
} // namespace pfs
