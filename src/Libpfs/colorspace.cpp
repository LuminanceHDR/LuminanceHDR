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

#include "Libpfs/pfs.h"
#include "Libpfs/array2d.h"
#include "Libpfs/utils/msec_timer.h"

#include <boost/assign.hpp>

using namespace std;
using namespace boost::assign;

namespace pfs 
{
namespace
{
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
  
inline
float inverseSRGBCompanding(float sample)
{
    if ( sample > 0.04045f ) {
        return std::pow((sample + 0.055f)*(1.f/1.055f), 2.4f);
    }
    if ( sample >= -0.04045f )
    {
        return sample*(1.f/12.92f);
    }
    return -std::pow((0.055f - sample)*(1.f/1.055f), 2.4f);
}

inline
float directSRGBCompanding(float sample)
{
    if ( sample > 0.0031308f ) {
        return ((1.055f * std::pow(sample, 1.f/2.4f)) - 0.055f);
    }
    if ( sample >= -0.0031308f ) {
        return (sample * 12.92f);
    }
    return ((0.055f - 1.f)*std::pow(-sample, 1.f/2.4f) - 0.055f);
}

inline
float kernelTrasformRGB2Y(float red, float green, float blue)
{
    return ( rgb2xyzD65Mat[1][0]*red +
             rgb2xyzD65Mat[1][1]*green +
             rgb2xyzD65Mat[1][2]*blue );
}

inline
float kernelTrasformSRGB2Y(float red, float green, float blue)
{
    return ( kernelTrasformRGB2Y(
                 inverseSRGBCompanding(red),
                 inverseSRGBCompanding(green),
                 inverseSRGBCompanding(blue)
                 ) );
}

inline
void kernelTrasformRGB2XYZ(float red, float green, float blue, float& X, float&Y, float& Z)
{
    X = rgb2xyzD65Mat[0][0]*red + rgb2xyzD65Mat[0][1]*green + rgb2xyzD65Mat[0][2]*blue;
    Y = rgb2xyzD65Mat[1][0]*red + rgb2xyzD65Mat[1][1]*green + rgb2xyzD65Mat[1][2]*blue;
    Z = rgb2xyzD65Mat[2][0]*red + rgb2xyzD65Mat[2][1]*green + rgb2xyzD65Mat[2][2]*blue;
}

inline
void kernelTrasformSRGB2XYZ(float red, float green, float blue, float& X, float&Y, float& Z)
{
    kernelTrasformRGB2XYZ( inverseSRGBCompanding(red),
                           inverseSRGBCompanding(green),
                           inverseSRGBCompanding(blue),
                           X, Y, Z);
}

inline
void kernelTrasformXYZ2RGB(float X, float Y, float Z, float& red, float& green, float& blue)
{
    red   = xyz2rgbD65Mat[0][0]*X + xyz2rgbD65Mat[0][1]*Y + xyz2rgbD65Mat[0][2]*Z;
    green = xyz2rgbD65Mat[1][0]*X + xyz2rgbD65Mat[1][1]*Y + xyz2rgbD65Mat[1][2]*Z;
    blue  = xyz2rgbD65Mat[2][0]*X + xyz2rgbD65Mat[2][1]*Y + xyz2rgbD65Mat[2][2]*Z;
}

inline
void kernelTrasformXYZ2SRGB(float X, float Y, float Z, float& red, float& green, float& blue)
{
    kernelTrasformXYZ2RGB(X, Y, Z, X, Y, Z);    // use X, Y and Z as temporary!

    red = directSRGBCompanding(X);
    green = directSRGBCompanding(Y);
    blue = directSRGBCompanding(Z);
}

} // anonymous namespace


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

    Array2Df::const_iterator r = inC1->begin();
    Array2Df::const_iterator rEnd = inC1->end();
    Array2Df::const_iterator g = inC2->begin();
    Array2Df::const_iterator b = inC3->begin();

    Array2Df::iterator x = outC1->begin();
    Array2Df::iterator y = outC2->begin();
    Array2Df::iterator z = outC3->begin();

    while ( r != rEnd )
    {
        kernelTrasformSRGB2XYZ(*r++, *g++, *b++, *x++, *y++, *z++);
    }

#ifdef TIMER_PROFILING
    f_timer.stop_and_update();
    std::cout << "transformSRGB2XYZ() = " << f_timer.get_time() << " msec" << std::endl;
#endif

}
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

//-----------------------------------------------------------
// RGB conversion functions
//-----------------------------------------------------------
void transformRGB2XYZ(const Array2Df *inC1, const Array2Df *inC2, const Array2Df *inC3,
                      Array2Df *outC1, Array2Df *outC2, Array2Df *outC3)
{
#ifdef TIMER_PROFILING
    msec_timer f_timer;
    f_timer.start();
#endif

    Array2Df::const_iterator r = inC1->begin();
    Array2Df::const_iterator rEnd = inC1->end();
    Array2Df::const_iterator g = inC2->begin();
    Array2Df::const_iterator b = inC3->begin();

    Array2Df::iterator x = outC1->begin();
    Array2Df::iterator y = outC2->begin();
    Array2Df::iterator z = outC3->begin();

    while ( r != rEnd )
    {
        kernelTrasformRGB2XYZ(*r++, *g++, *b++, *x++, *y++, *z++);
    }

#ifdef TIMER_PROFILING
    f_timer.stop_and_update();
    std::cout << "transformRGB2XYZ() = " << f_timer.get_time() << " msec" << std::endl;
#endif
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
        *y++ = kernelTrasformRGB2Y(*r++, *g++, *b++);
    }
}

void transformXYZ2SRGB(const Array2Df *inC1, const Array2Df *inC2, const Array2Df *inC3,
                       Array2Df *outC1, Array2Df *outC2, Array2Df *outC3)
{
#ifdef TIMER_PROFILING
    msec_timer f_timer;
    f_timer.start();
#endif

    Array2Df::const_iterator x = inC1->begin();
    Array2Df::const_iterator xEnd = inC1->end();
    Array2Df::const_iterator y = inC2->begin();
    Array2Df::const_iterator z = inC3->begin();

    Array2Df::iterator r = outC1->begin();
    Array2Df::iterator g = outC2->begin();
    Array2Df::iterator b = outC3->begin();

    while ( x != xEnd )
    {
        kernelTrasformXYZ2SRGB(*x++, *y++, *z++, *r++, *g++, *b++);
    }

#ifdef TIMER_PROFILING
    f_timer.stop_and_update();
    std::cout << "transformXYZ2SRGB() = " << f_timer.get_time() << " msec" << std::endl;
#endif
}

void transformXYZ2RGB(const Array2Df *inC1, const Array2Df *inC2, const Array2Df *inC3,
                      Array2Df *outC1, Array2Df *outC2, Array2Df *outC3 )
{
#ifdef TIMER_PROFILING
    msec_timer f_timer;
    f_timer.start();
#endif

    Array2Df::const_iterator x = inC1->begin();
    Array2Df::const_iterator xEnd = inC1->end();
    Array2Df::const_iterator y = inC2->begin();
    Array2Df::const_iterator z = inC3->begin();

    Array2Df::iterator r = outC1->begin();
    Array2Df::iterator g = outC2->begin();
    Array2Df::iterator b = outC3->begin();

    while ( x != xEnd )
    {
        kernelTrasformXYZ2RGB(*x++, *y++, *z++, *r++, *g++, *b++);
    }

#ifdef TIMER_PROFILING
    f_timer.stop_and_update();
    std::cout << "transformXYZ2RGB() = " << f_timer.get_time() << " msec" << std::endl;
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
