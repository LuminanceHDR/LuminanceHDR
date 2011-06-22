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

#include <math.h>
#include <assert.h>
#include <list>
#include <iostream>

#include "pfs.h"
#include "array2d.h"
#include "colorspace.h"

#include "Common/msec_timer.h"
#include "vex.h"

using namespace std;

namespace pfs 
{
  static inline float clamp( const float v, const float min, const float max )
  {
    if( v < min ) return min;
    if( v > max ) return max;
    return v;
  }
  
  //--- 7 digits approximation of precise values
  static const float rgb2xyzD65Mat[3][3] =
  { { 0.412424f, 0.357579f, 0.180464f },
    { 0.212656f, 0.715158f, 0.072186f },
    { 0.019332f, 0.119193f, 0.950444f } };
  
  static const float xyz2rgbD65Mat[3][3] =
  { {  3.240708f, -1.537259f, -0.498570f },
    { -0.969257f,  1.875995f,  0.041555f },
    {  0.055636f, -0.203996f,  1.057069f } };
  
  // //--- precise values for matrix convertion (above float precission)
  // static const float rgb2xyzD65Mat[3][3] =
  // { { 0.412424,  0.357579, 0.180464 },
  //   { 0.212656,  0.715158, 0.0721856 },
  //   { 0.0193324, 0.119193, 0.950444 } };
  
  // static const float xyz2rgbD65Mat[3][3] =
  // { {  3.24071,   -1.53726,  -0.498571 },
  //   { -0.969258,   1.87599,   0.0415557 },
  //   {  0.0556352, -0.203996,  1.05707 } };
  
  // //--- original values which lead to mean sq error of above 3 for green channel
  // static const float rgb2xyzD65Mat[3][3] =
  // { { 0.4124f, 0.3576f, 0.1805f },
  //   { 0.2126f, 0.7152f, 0.0722f },
  //   { 0.0193f, 0.1192f, 0.9505f } };
  
  // static const float xyz2rgbD65Mat[3][3] =
  // { { 3.2406f, -1.5372f, -0.4986f },
  //   { -0.9689f, 1.8758f,  0.0415f },
  //   { 0.0557f, -0.2040f,  1.0570f } };
  
  //-----------------------------------------------------------
  // sRGB conversion functions
  //-----------------------------------------------------------
  void transformSRGB2XYZ(const Array2D *inC1, const Array2D *inC2, const Array2D *inC3,
                         Array2D *outC1, Array2D *outC2, Array2D *outC3)
  {
#ifdef TIMER_PROFILING
      msec_timer f_timer;
      f_timer.start();
#endif

      const float* __r = inC1->getRawData();
      const float* __g = inC2->getRawData();
      const float* __b = inC3->getRawData();

      float* __x = outC1->getRawData();
      float* __y = outC2->getRawData();
      float* __z = outC3->getRawData();

      float i1, i2, i3;
      float t1, t2, t3;

      int elems = inC1->getRows()*inC1->getCols();

#pragma omp parallel for private(i1,i2,i3,t1,t2,t3)
      for( int idx = 0; idx < elems ; idx++ )
      {
          i1 = clamp(__r[idx], 0, 1);
          i2 = clamp(__g[idx], 0, 1);
          i3 = clamp(__b[idx], 0, 1);

          t1 = (i1 <= 0.04045f ? i1 / 12.92f : powf( (i1 + 0.055f) / 1.055f, 2.4f )  );
          t2 = (i2 <= 0.04045f ? i2 / 12.92f : powf( (i2 + 0.055f) / 1.055f, 2.4f )  );
          t3 = (i3 <= 0.04045f ? i3 / 12.92f : powf( (i3 + 0.055f) / 1.055f, 2.4f )  );

          __x[idx] = rgb2xyzD65Mat[0][0]*t1 + rgb2xyzD65Mat[0][1]*t2 + rgb2xyzD65Mat[0][2]*t3;
          __y[idx] = rgb2xyzD65Mat[1][0]*t1 + rgb2xyzD65Mat[1][1]*t2 + rgb2xyzD65Mat[1][2]*t3;
          __z[idx] = rgb2xyzD65Mat[2][0]*t1 + rgb2xyzD65Mat[2][1]*t2 + rgb2xyzD65Mat[2][2]*t3;
      }

#ifdef TIMER_PROFILING
      f_timer.stop_and_update();
      std::cout << "transformSRGB2XYZ() = " << f_timer.get_time() << " msec" << std::endl;
#endif
  }
  
  void transformXYZ2SRGB(const Array2D *inC1, const Array2D *inC2, const Array2D *inC3,
                         Array2D *outC1, Array2D *outC2, Array2D *outC3)
  {
#ifdef TIMER_PROFILING
      msec_timer f_timer;
      f_timer.start();
#endif

      const float* __x = inC1->getRawData();
      const float* __y = inC2->getRawData();
      const float* __z = inC3->getRawData();

      float* __r = outC1->getRawData();
      float* __g = outC2->getRawData();
      float* __b = outC3->getRawData();

      float i1, i2, i3;
      float t1, t2, t3;

      const int ELEMS = inC1->getRows()*inC1->getCols();

#pragma omp parallel for private(i1,i2,i3,t1,t2,t3)
      for( int idx = 0; idx < ELEMS; idx++ )
      {
          i1 = __x[idx];
          i2 = __y[idx];
          i3 = __z[idx];

          t1 = xyz2rgbD65Mat[0][0]*i1 + xyz2rgbD65Mat[0][1]*i2 + xyz2rgbD65Mat[0][2]*i3;
          t2 = xyz2rgbD65Mat[1][0]*i1 + xyz2rgbD65Mat[1][1]*i2 + xyz2rgbD65Mat[1][2]*i3;
          t3 = xyz2rgbD65Mat[2][0]*i1 + xyz2rgbD65Mat[2][1]*i2 + xyz2rgbD65Mat[2][2]*i3;

          t1 = clamp( t1, 0, 1 );
          t2 = clamp( t2, 0, 1 );
          t3 = clamp( t3, 0, 1 );

          __r[idx] = (t1 <= 0.0031308f ? t1 *= 12.92f : 1.055f * powf( t1, 1.f/2.4f ) - 0.055f);
          __g[idx] = (t2 <= 0.0031308f ? t2 *= 12.92f : 1.055f * powf( t2, 1.f/2.4f ) - 0.055f);
          __b[idx] = (t3 <= 0.0031308f ? t3 *= 12.92f : 1.055f * powf( t3, 1.f/2.4f ) - 0.055f);
      }

#ifdef TIMER_PROFILING
      f_timer.stop_and_update();
      std::cout << "transformXYZ2SRGB() = " << f_timer.get_time() << " msec" << std::endl;
#endif
  }
  
  void transformXYZ2Yuv( const Array2D *inC1, const Array2D *inC2, const Array2D *inC3,
                        Array2D *outC1, Array2D *outC2, Array2D *outC3 )
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
  
  void transformYuv2XYZ( const Array2D *inC1, const Array2D *inC2, const Array2D *inC3,
                        Array2D *outC1, Array2D *outC2, Array2D *outC3 )
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
  
  void transformYxy2XYZ( const Array2D *inC1, const Array2D *inC2, const Array2D *inC3,
                        Array2D *outC1, Array2D *outC2, Array2D *outC3 )
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
  
  void transformXYZ2Yxy( const Array2D *inC1, const Array2D *inC2, const Array2D *inC3,
                        Array2D *outC1, Array2D *outC2, Array2D *outC3 )
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
  
  void transformRGB2XYZ(const Array2D *inC1, const Array2D *inC2, const Array2D *inC3,
                        Array2D *outC1, Array2D *outC2, Array2D *outC3)
  {
#ifdef TIMER_PROFILING
      msec_timer f_timer;
      f_timer.start();
#endif

      const float* __r = inC1->getRawData();
      const float* __g = inC2->getRawData();
      const float* __b = inC3->getRawData();

      float* __x = outC1->getRawData();
      float* __y = outC2->getRawData();
      float* __z = outC3->getRawData();

      float i1, i2, i3;
      const int ELEMS = inC1->getRows()*inC1->getCols();

#pragma omp parallel for private(i1,i2,i3)
      for( int idx = 0; idx < ELEMS; idx++ )
      {
          i1 = __r[idx];
          i2 = __g[idx];
          i3 = __b[idx];

          __x[idx] = rgb2xyzD65Mat[0][0]*i1 + rgb2xyzD65Mat[0][1]*i2 + rgb2xyzD65Mat[0][2]*i3;
          __y[idx] = rgb2xyzD65Mat[1][0]*i1 + rgb2xyzD65Mat[1][1]*i2 + rgb2xyzD65Mat[1][2]*i3;
          __z[idx] = rgb2xyzD65Mat[2][0]*i1 + rgb2xyzD65Mat[2][1]*i2 + rgb2xyzD65Mat[2][2]*i3;
      }

#ifdef TIMER_PROFILING
      f_timer.stop_and_update();
      std::cout << "transformRGB2XYZ() = " << f_timer.get_time() << " msec" << std::endl;
#endif
  }
  
  void transformXYZ2RGB(const Array2D *inC1, const Array2D *inC2, const Array2D *inC3,
                        Array2D *outC1, Array2D *outC2, Array2D *outC3 )
  {
#ifdef TIMER_PROFILING
      msec_timer f_timer;
      f_timer.start();
#endif

      const float* __x = inC1->getRawData();
      const float* __y = inC2->getRawData();
      const float* __z = inC3->getRawData();

      float* __r = outC1->getRawData();
      float* __g = outC2->getRawData();
      float* __b = outC3->getRawData();

      float i1, i2, i3;
      const int ELEMS = inC1->getRows()*inC1->getCols();

#pragma omp parallel for schedule(static, 5120) private(i1,i2,i3)
      for( int idx = 0; idx < ELEMS; idx++ )
      {
          i1 = __x[idx];
          i2 = __y[idx];
          i3 = __z[idx];

          __r[idx] = xyz2rgbD65Mat[0][0]*i1 + xyz2rgbD65Mat[0][1]*i2 + xyz2rgbD65Mat[0][2]*i3;
          __g[idx] = xyz2rgbD65Mat[1][0]*i1 + xyz2rgbD65Mat[1][1]*i2 + xyz2rgbD65Mat[1][2]*i3;
          __b[idx] = xyz2rgbD65Mat[2][0]*i1 + xyz2rgbD65Mat[2][1]*i2 + xyz2rgbD65Mat[2][2]*i3;
      }

#ifdef TIMER_PROFILING
      f_timer.stop_and_update();
      std::cout << "transformXYZ2RGB() = " << f_timer.get_time() << " msec" << std::endl;
#endif
  }
  
  typedef void(*CSTransformFunc)( const Array2D *inC1, const Array2D *inC2, const Array2D *inC3, Array2D *outC1, Array2D *outC2, Array2D *outC3 );
  
  struct CSTransEdge
  {
    CSTransEdge *next;
    ColorSpace srcCS;
    ColorSpace destCS;
    CSTransformFunc func;
  };
  
  CSTransEdge TN_XYZRGB = { NULL, CS_XYZ, CS_RGB, transformXYZ2RGB };
  CSTransEdge TN_XYZYUV = { &TN_XYZRGB, CS_XYZ, CS_YUV, transformXYZ2Yuv };
  CSTransEdge TN_XYZYxy = { &TN_XYZYUV, CS_XYZ, CS_Yxy, transformXYZ2Yxy };
  CSTransEdge TN_XYZSRGB = { &TN_XYZYxy, CS_XYZ, CS_SRGB, transformXYZ2SRGB };
  
  CSTransEdge TN_RGBXYZ = { NULL, CS_RGB, CS_XYZ, transformRGB2XYZ };
  
  CSTransEdge TN_SRGBXYZ = { NULL, CS_SRGB, CS_XYZ, transformSRGB2XYZ };
  
  CSTransEdge TN_YUV2XYZ = { NULL, CS_YUV, CS_XYZ, transformYuv2XYZ };
  
  CSTransEdge TN_Yxy2XYZ = { NULL, CS_Yxy, CS_XYZ, transformYxy2XYZ };
  
  CSTransEdge *CSTransGraph[] =
  {
    &TN_XYZSRGB,
    &TN_RGBXYZ,
    &TN_SRGBXYZ,
    &TN_YUV2XYZ,
    &TN_Yxy2XYZ
  };
  
  void transformColorSpace(ColorSpace inCS, const Array2D *inC1, const Array2D *inC2, const Array2D *inC3,
                           ColorSpace outCS, Array2D *outC1, Array2D *outC2, Array2D *outC3)
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
    
    
    CSTransEdge *gotByEdge[ CS_LAST ] = { NULL };
    
    // Breadth First Search
    std::list<ColorSpace> bfsList;
    bfsList.push_back( inCS );
    
    bool found = false;
    while( !bfsList.empty() )
    {
      ColorSpace node = bfsList.front();
      bfsList.pop_front();
      //    std::cerr << "Graph Node: " << node << "\n";
      
      if( node == outCS )
      {
        found = true;
        break;
      }
      for( CSTransEdge *edge = CSTransGraph[node]; edge != NULL; edge = edge->next )
      {
        if( edge->destCS != inCS && gotByEdge[ edge->destCS ] == NULL ) {
          bfsList.push_back( edge->destCS );
          gotByEdge[ edge->destCS ] = edge;
        }
      }
    } 
    
    if( !found )
    {
      // TODO: All transforms should be supported
      throw Exception( "Not supported color tranform" );
    }
    else
    {
      // Reverse path
      std::list<CSTransEdge *> step;
      ColorSpace currentNode = outCS;
      while( currentNode != inCS )
      {
        //       std::cerr << "edge: " << gotByEdge[ currentNode ]->srcCS << " -- "
        //                 << gotByEdge[ currentNode ]->destCS << "\n";
        step.push_front( gotByEdge[ currentNode ] );
        currentNode = gotByEdge[ currentNode ]->srcCS;      
      }
      // Execute path
      std::list<CSTransEdge *>::iterator it;
      for( it = step.begin(); it != step.end(); it++ )
      {
        //       std::cerr << "edge: " << (*it)->srcCS << " -- "
        //                 << (*it)->destCS << "\n";
        if( it == step.begin() )
          (*it)->func( inC1, inC2, inC3, outC1, outC2, outC3 );
        else
          (*it)->func( outC1, outC2, outC3, outC1, outC2, outC3 );      
      }
    }
  } 
}

