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
 * @author Davide Anastasia <davide.anastasia@gmail.com> (2010 10 13)
 *  Reimplementation of most of the functions (in particular the ones involving RGB and XYZ)
 *
 * $Id: colorspace.cpp,v 1.6 2007/07/18 08:49:25 rafm Exp $
 */

#include <math.h>
#include <assert.h>
#include <list>
#include <iostream>

#include "Libpfs/pfs.h"
#include "Libpfs/array2d.h"
#include "Libpfs/colorspace.h"

#include "Common/msec_timer.h"
#include "Common/vex.h"

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
  { {  3.240708, -1.537259, -0.498570 },
    { -0.969257,  1.875995,  0.041555 },
    {  0.055636, -0.203996,  1.057069 } };
  
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
  
  
  //  void multiplyByMatrix( const Array2D *inC1, const Array2D *inC2, const Array2D *inC3,
  //                               Array2D *outC1, Array2D *outC2, Array2D *outC3, const float mat[3][3] )
  //  {
  //#ifdef TIMER_PROFILING
  //    msec_timer f_timer;
  //    f_timer.start();
  //#endif
  //
  //    float i1, i2, i3;
  //    const int elems = inC1->getRows()*inC1->getCols();
  //    
  //    for( int idx = 0; idx < elems; idx++ )
  //    {
  //      i1 = (*inC1)(idx);
  //      i2 = (*inC2)(idx);
  //      i3 = (*inC3)(idx);
  //      
  //      (*outC1)(idx) = mat[0][0]*i1 + mat[0][1]*i2 + mat[0][2]*i3;
  //      (*outC2)(idx) = mat[1][0]*i1 + mat[1][1]*i2 + mat[1][2]*i3;
  //      (*outC3)(idx) = mat[2][0]*i1 + mat[2][1]*i2 + mat[2][2]*i3;
  //    }
  //    
  //#ifdef TIMER_PROFILING
  //    f_timer.stop_and_update();
  //    std::cout << "multiplyByMatrix() = " << f_timer.get_time() << " msec" << std::endl;
  //#endif
  //  }
  
  //-----------------------------------------------------------
  // sRGB conversion functions
  //-----------------------------------------------------------
  void transformSRGB2XYZ( const Array2D *inC1, const Array2D *inC2, const Array2D *inC3,
                         Array2D *outC1, Array2D *outC2, Array2D *outC3 )
  {
#ifdef TIMER_PROFILING
    msec_timer f_timer;
    f_timer.start();
#endif
    
    const Array2DImpl* R = dynamic_cast<const Array2DImpl*> (inC1);
    const Array2DImpl* G = dynamic_cast<const Array2DImpl*> (inC2);
    const Array2DImpl* B = dynamic_cast<const Array2DImpl*> (inC3);
    
    Array2DImpl* X = dynamic_cast<Array2DImpl*> (outC1);
    Array2DImpl* Y = dynamic_cast<Array2DImpl*> (outC2);
    Array2DImpl* Z = dynamic_cast<Array2DImpl*> (outC3);
    
    assert ( X != NULL && Y != NULL && Z != NULL );
    assert ( R != NULL && G != NULL && B != NULL );
    
    const float* __r = R->data;
    const float* __g = G->data;
    const float* __b = B->data;
    
    float* __x = X->data;
    float* __y = Y->data;
    float* __z = Z->data;
    
    float i1, i2, i3;
    float t1, t2, t3;
    
    int elems = inC1->getRows()*inC1->getCols();
    for( int idx = 0; idx < elems ; idx++ )
    {
      i1 = clamp(__r[idx], 0, 1);
      i2 = clamp(__g[idx], 0, 1);
      i3 = clamp(__b[idx], 0, 1);
      
      t1 = (i1 <= 0.04045 ? i1 / 12.92f : powf( (i1 + 0.055f) / 1.055f, 2.4f )  );
      t2 = (i2 <= 0.04045 ? i2 / 12.92f : powf( (i2 + 0.055f) / 1.055f, 2.4f )  );
      t3 = (i3 <= 0.04045 ? i3 / 12.92f : powf( (i3 + 0.055f) / 1.055f, 2.4f )  );
      
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
    
    const Array2DImpl* X = dynamic_cast<const Array2DImpl*> (inC1);
    const Array2DImpl* Y = dynamic_cast<const Array2DImpl*> (inC2);
    const Array2DImpl* Z = dynamic_cast<const Array2DImpl*> (inC3);
    
    Array2DImpl* R = dynamic_cast<Array2DImpl*> (outC1);
    Array2DImpl* G = dynamic_cast<Array2DImpl*> (outC2);
    Array2DImpl* B = dynamic_cast<Array2DImpl*> (outC3);
    
    assert ( X != NULL && Y != NULL && Z != NULL );
    assert ( R != NULL && G != NULL && B != NULL );
    
    const float* __x = X->data;
    const float* __y = Y->data;
    const float* __z = Z->data;
    
    float* __r = R->data;
    float* __g = G->data;
    float* __b = B->data;
    
    float i1, i2, i3;
    float t1, t2, t3;
    
    const int ELEMS = inC1->getRows()*inC1->getCols();
    
    for( int idx = ELEMS; idx; idx-- )
    {
      i1 = (*__x);
      i2 = (*__y);
      i3 = (*__z);
      
      t1 = xyz2rgbD65Mat[0][0]*i1 + xyz2rgbD65Mat[0][1]*i2 + xyz2rgbD65Mat[0][2]*i3;
      t2 = xyz2rgbD65Mat[1][0]*i1 + xyz2rgbD65Mat[1][1]*i2 + xyz2rgbD65Mat[1][2]*i3;
      t3 = xyz2rgbD65Mat[2][0]*i1 + xyz2rgbD65Mat[2][1]*i2 + xyz2rgbD65Mat[2][2]*i3;
      
      t1 = clamp( t1, 0, 1 );
      t2 = clamp( t2, 0, 1 );
      t3 = clamp( t3, 0, 1 );
      
      (*__r) = (t1 <= 0.0031308 ? t1 *= 12.92f : 1.055f * powf( t1, 1./2.4 ) - 0.055);
      (*__g) = (t2 <= 0.0031308 ? t2 *= 12.92f : 1.055f * powf( t2, 1./2.4 ) - 0.055);
      (*__b) = (t3 <= 0.0031308 ? t3 *= 12.92f : 1.055f * powf( t3, 1./2.4 ) - 0.055);
      
      __x++; __y++; __z++;
      __r++; __g++; __b++;
    }
    
//    float i1, i2, i3;
//    float t1, t2, t3;
//    
//    int elems = inC1->getRows()*inC1->getCols();
//    for( int idx = 0; idx < elems; idx++ )
//    {
//      i1 = (*inC1)(idx);
//      i2 = (*inC2)(idx);
//      i3 = (*inC3)(idx);
//      
//      t1 = xyz2rgbD65Mat[0][0]*i1 + xyz2rgbD65Mat[0][1]*i2 + xyz2rgbD65Mat[0][2]*i3;
//      t2 = xyz2rgbD65Mat[1][0]*i1 + xyz2rgbD65Mat[1][1]*i2 + xyz2rgbD65Mat[1][2]*i3;
//      t3 = xyz2rgbD65Mat[2][0]*i1 + xyz2rgbD65Mat[2][1]*i2 + xyz2rgbD65Mat[2][2]*i3;
//      
//      t1 = clamp( t1, 0, 1 );
//      t2 = clamp( t2, 0, 1 );
//      t3 = clamp( t3, 0, 1 );
//      
//      (*outC1)(idx) = (t1 <= 0.0031308 ? t1 *= 12.92f : 1.055f * powf( t1, 1./2.4 ) - 0.055);
//      (*outC2)(idx) = (t2 <= 0.0031308 ? t2 *= 12.92f : 1.055f * powf( t2, 1./2.4 ) - 0.055);
//      (*outC3)(idx) = (t3 <= 0.0031308 ? t3 *= 12.92f : 1.055f * powf( t3, 1./2.4 ) - 0.055);    
//    }
    
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
  
  void transformRGB2XYZ( const Array2D *inC1, const Array2D *inC2, const Array2D *inC3, Array2D *outC1, Array2D *outC2, Array2D *outC3 )
  {
#ifdef TIMER_PROFILING
    msec_timer f_timer;
    f_timer.start();
#endif

    const Array2DImpl* R = dynamic_cast<const Array2DImpl*> (inC1);
    const Array2DImpl* G = dynamic_cast<const Array2DImpl*> (inC2);
    const Array2DImpl* B = dynamic_cast<const Array2DImpl*> (inC3);
    
    Array2DImpl* X = dynamic_cast<Array2DImpl*> (outC1);
    Array2DImpl* Y = dynamic_cast<Array2DImpl*> (outC2);
    Array2DImpl* Z = dynamic_cast<Array2DImpl*> (outC3);
    
    assert ( X != NULL && Y != NULL && Z != NULL );
    assert ( R != NULL && G != NULL && B != NULL );
    
    const float* __r = R->data;
    const float* __g = G->data;
    const float* __b = B->data;
    
    float* __x = X->data;
    float* __y = Y->data;
    float* __z = Z->data;
    
    float i1, i2, i3;
    const int ELEMS = inC1->getRows()*inC1->getCols();
    
    for( int idx = ELEMS; idx; idx-- )
    {
      i1 = (*__r);
      i2 = (*__g);
      i3 = (*__b);
      
      (*__x) = rgb2xyzD65Mat[0][0]*i1 + rgb2xyzD65Mat[0][1]*i2 + rgb2xyzD65Mat[0][2]*i3;
      (*__y) = rgb2xyzD65Mat[1][0]*i1 + rgb2xyzD65Mat[1][1]*i2 + rgb2xyzD65Mat[1][2]*i3;
      (*__z) = rgb2xyzD65Mat[2][0]*i1 + rgb2xyzD65Mat[2][1]*i2 + rgb2xyzD65Mat[2][2]*i3;
      
      __x++; __y++; __z++;
      __r++; __g++; __b++;
    }
    
//    float i1, i2, i3;
//    const int elems = inC1->getRows()*inC1->getCols();
//    
//    for( int idx = 0; idx < elems; idx++ )
//    {
//      i1 = (*inC1)(idx);
//      i2 = (*inC2)(idx);
//      i3 = (*inC3)(idx);
//      
//      (*outC1)(idx) = rgb2xyzD65Mat[0][0]*i1 + rgb2xyzD65Mat[0][1]*i2 + rgb2xyzD65Mat[0][2]*i3;
//      (*outC2)(idx) = rgb2xyzD65Mat[1][0]*i1 + rgb2xyzD65Mat[1][1]*i2 + rgb2xyzD65Mat[1][2]*i3;
//      (*outC3)(idx) = rgb2xyzD65Mat[2][0]*i1 + rgb2xyzD65Mat[2][1]*i2 + rgb2xyzD65Mat[2][2]*i3;
//    }
    
#ifdef TIMER_PROFILING
    f_timer.stop_and_update();
    std::cout << "transformRGB2XYZ() = " << f_timer.get_time() << " msec" << std::endl;
#endif
  }
  
  void transformXYZ2RGB( const Array2D *inC1, const Array2D *inC2, const Array2D *inC3, Array2D *outC1, Array2D *outC2, Array2D *outC3 )
  {
#ifdef TIMER_PROFILING
    msec_timer f_timer;
    f_timer.start();
#endif
    
    const Array2DImpl* X = dynamic_cast<const Array2DImpl*> (inC1);
    const Array2DImpl* Y = dynamic_cast<const Array2DImpl*> (inC2);
    const Array2DImpl* Z = dynamic_cast<const Array2DImpl*> (inC3);
        
    Array2DImpl* R = dynamic_cast<Array2DImpl*> (outC1);
    Array2DImpl* G = dynamic_cast<Array2DImpl*> (outC2);
    Array2DImpl* B = dynamic_cast<Array2DImpl*> (outC3);
    
    assert ( X != NULL && Y != NULL && Z != NULL );
    assert ( R != NULL && G != NULL && B != NULL );
    
    const float* __x = X->data;
    const float* __y = Y->data;
    const float* __z = Z->data;
    
    float* __r = R->data;
    float* __g = G->data;
    float* __b = B->data;
    
    float i1, i2, i3;
    const int ELEMS = inC1->getRows()*inC1->getCols();
    
    for( int idx = ELEMS; idx; idx-- )
    {
      i1 = (*__x);
      i2 = (*__y);
      i3 = (*__z);
      
      (*__r) = xyz2rgbD65Mat[0][0]*i1 + xyz2rgbD65Mat[0][1]*i2 + xyz2rgbD65Mat[0][2]*i3;
      (*__g) = xyz2rgbD65Mat[1][0]*i1 + xyz2rgbD65Mat[1][1]*i2 + xyz2rgbD65Mat[1][2]*i3;
      (*__b) = xyz2rgbD65Mat[2][0]*i1 + xyz2rgbD65Mat[2][1]*i2 + xyz2rgbD65Mat[2][2]*i3;
      
      __x++; __y++; __z++;
      __r++; __g++; __b++;
    }
    
//    float i1, i2, i3;
//    const int elems = inC1->getRows()*inC1->getCols();
//    
//    for( int idx = 0; idx < elems; idx++ )
//    {
//      i1 = (*inC1)(idx);
//      i2 = (*inC2)(idx);
//      i3 = (*inC3)(idx);
//      
//      (*outC1)(idx) = xyz2rgbD65Mat[0][0]*i1 + xyz2rgbD65Mat[0][1]*i2 + xyz2rgbD65Mat[0][2]*i3;
//      (*outC2)(idx) = xyz2rgbD65Mat[1][0]*i1 + xyz2rgbD65Mat[1][1]*i2 + xyz2rgbD65Mat[1][2]*i3;
//      (*outC3)(idx) = xyz2rgbD65Mat[2][0]*i1 + xyz2rgbD65Mat[2][1]*i2 + xyz2rgbD65Mat[2][2]*i3;
//    }
    
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
  
  void transformColorSpace( ColorSpace inCS,
                           const Array2D *inC1, const Array2D *inC2, const Array2D *inC3,
                           ColorSpace outCS, Array2D *outC1, Array2D *outC2, Array2D *outC3 )
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
