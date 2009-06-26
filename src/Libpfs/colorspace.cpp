/**
 * @brief PFS library - color space transformations
 *
 * This file is a part of PFSTOOLS package.
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
 * $Id: colorspace.cpp,v 1.6 2007/07/18 08:49:25 rafm Exp $
 */

#include <math.h>
#include "pfs.h"
#include <assert.h>
#include <list>

#include <iostream>

namespace pfs 
{

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


static void multiplyByMatrix( const Array2D *inC1, const Array2D *inC2, const Array2D *inC3,
  Array2D *outC1, Array2D *outC2, Array2D *outC3, const float mat[3][3] )
{
  int imgSize = inC1->getRows()*inC1->getCols();
  for( int index = 0; index < imgSize ; index++ ) {
    const float x1 = (*inC1)(index), x2 = (*inC2)(index), x3 = (*inC3)(index);
    float &y1 = (*outC1)(index), &y2 = (*outC2)(index), &y3 = (*outC3)(index);
    y1 = mat[0][0]*x1 + mat[0][1]*x2 + mat[0][2]*x3;
    y2 = mat[1][0]*x1 + mat[1][1]*x2 + mat[1][2]*x3;
    y3 = mat[2][0]*x1 + mat[2][1]*x2 + mat[2][2]*x3;        
  }    
}

//-----------------------------------------------------------
// sRGB conversion functions
//-----------------------------------------------------------

static inline float clamp( const float v, const float min, const float max )
{
  if( v < min ) return min;
  if( v > max ) return max;
  return v;
}


static void transformSRGB2XYZ( const Array2D *inC1, const Array2D *inC2, const Array2D *inC3,
  Array2D *outC1, Array2D *outC2, Array2D *outC3 )
{
  int imgSize = inC1->getRows()*inC1->getCols();
  for( int index = 0; index < imgSize ; index++ ) {
    float r = (*inC1)(index), g = (*inC2)(index), b = (*inC3)(index);
    float &x = (*outC1)(index), &y = (*outC2)(index), &z = (*outC3)(index);
    r = clamp( r, 0, 1 );
    g = clamp( g, 0, 1 );
    b = clamp( b, 0, 1 );
    x = (r <= 0.04045 ? r / 12.92f : powf( (r + 0.055f) / 1.055f, 2.4f )  );
    y = (g <= 0.04045 ? g / 12.92f : powf( (g + 0.055f) / 1.055f, 2.4f )  );
    z = (b <= 0.04045 ? b / 12.92f : powf( (b + 0.055f) / 1.055f, 2.4f )  );
  }
  multiplyByMatrix( outC1, outC2, outC3, outC1, outC2, outC3, rgb2xyzD65Mat );
}

static void transformXYZ2SRGB( const Array2D *inC1, const Array2D *inC2,
  const Array2D *inC3, Array2D *outC1, Array2D *outC2, Array2D *outC3 )
{
  multiplyByMatrix( outC1, outC2, outC3, outC1, outC2, outC3, xyz2rgbD65Mat );

  int imgSize = inC1->getRows()*inC1->getCols();
  for( int index = 0; index < imgSize ; index++ ) {
    float r = (*inC1)(index), g = (*inC2)(index), b = (*inC3)(index);
    float &o_r = (*outC1)(index), &o_g = (*outC2)(index), &o_b = (*outC3)(index);
    
    r = clamp( r, 0, 1 );
    g = clamp( g, 0, 1 );
    b = clamp( b, 0, 1 );

    o_r = (r <= 0.0031308 ? r *= 12.92f : 1.055f * powf( r, 1./2.4 ) - 0.055);
    o_g = (g <= 0.0031308 ? g *= 12.92f : 1.055f * powf( g, 1./2.4 ) - 0.055);
    o_b = (b <= 0.0031308 ? b *= 12.92f : 1.055f * powf( b, 1./2.4 ) - 0.055);    
  }
}

static void transformXYZ2Yuv( const Array2D *inC1, const Array2D *inC2, const Array2D *inC3,
  Array2D *outC1, Array2D *outC2, Array2D *outC3 )
{
  int imgSize = inC1->getRows()*inC1->getCols();
  for( int index = 0; index < imgSize ; index++ ) {
    const float &X = (*inC1)(index), Y = (*inC2)(index), &Z = (*inC3)(index);
    float &outY = (*outC1)(index), &u = (*outC2)(index), &v = (*outC3)(index);
        
    float x = X/(X+Y+Z);
    float y = Y/(X+Y+Z);

//        assert((4.f*nx / (-2.f*nx + 12.f*ny + 3.f)) <= 0.62 );
//        assert( (9.f*ny / (-2.f*nx + 12.f*ny + 3.f)) <= 0.62 );
        
    u = 4.f*x / (-2.f*x + 12.f*y + 3.f);
    v = 9.f*y / (-2.f*x + 12.f*y + 3.f);
    outY = Y;
  }
    
}

static void transformYuv2XYZ( const Array2D *inC1, const Array2D *inC2, const Array2D *inC3,
  Array2D *outC1, Array2D *outC2, Array2D *outC3 )
{
  int imgSize = inC1->getRows()*inC1->getCols();
  for( int index = 0; index < imgSize ; index++ ) {
    const float Y = (*inC1)(index), &u = (*inC2)(index), &v = (*inC3)(index);
    float &X = (*outC1)(index), &outY = (*outC2)(index), &Z = (*outC3)(index);
        
    float x = 9.f*u / (6.f*u - 16.f*v + 12.f);
    float y = 4.f*v / (6.f*u - 16.f*v + 12.f);

    X = x/y * Y;
    Z = (1.f-x-y)/y * Y;
    outY = Y;
  }
}

static void transformYxy2XYZ( const Array2D *inC1, const Array2D *inC2, const Array2D *inC3,
  Array2D *outC1, Array2D *outC2, Array2D *outC3 )
{
  int imgSize = inC1->getRows()*inC1->getCols();
  for( int index = 0; index < imgSize ; index++ ) {
    const float Y = (*inC1)(index), x = (*inC2)(index), y = (*inC3)(index);
    float &X = (*outC1)(index), &outY = (*outC2)(index), &Z = (*outC3)(index);
        
    X = x/y * Y;
    Z = (1.f-x-y)/y * Y;
    outY = Y;
  }
}

static void transformXYZ2Yxy( const Array2D *inC1, const Array2D *inC2, const Array2D *inC3,
  Array2D *outC1, Array2D *outC2, Array2D *outC3 )
{
  int imgSize = inC1->getRows()*inC1->getCols();
  for( int index = 0; index < imgSize ; index++ ) {
    const float X = (*inC1)(index), Y = (*inC2)(index), Z = (*inC3)(index);
    float &outY = (*outC1)(index), &x = (*outC2)(index), &y = (*outC3)(index);
        
    x = X/(X+Y+Z);
    y = Y/(X+Y+Z);

    outY = Y;
  }
    
}

static void transformRGB2XYZ( const Array2D *inC1, const Array2D *inC2, const Array2D *inC3, Array2D *outC1, Array2D *outC2, Array2D *outC3 )
{
  multiplyByMatrix( inC1, inC2, inC3, outC1, outC2, outC3, rgb2xyzD65Mat );
}

static void transformXYZ2RGB( const Array2D *inC1, const Array2D *inC2, const Array2D *inC3, Array2D *outC1, Array2D *outC2, Array2D *outC3 )
{
  multiplyByMatrix( inC1, inC2, inC3, outC1, outC2, outC3, xyz2rgbD65Mat );
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
  while( !bfsList.empty() ) {
    ColorSpace node = bfsList.front();
    bfsList.pop_front();
//    std::cerr << "Graph Node: " << node << "\n";

    if( node == outCS ) {
      found = true;
      break;
    }
    for( CSTransEdge *edge = CSTransGraph[node]; edge != NULL;
       edge = edge->next ) {
      if( edge->destCS != inCS && gotByEdge[ edge->destCS ] == NULL ) {
        bfsList.push_back( edge->destCS );
        gotByEdge[ edge->destCS ] = edge;
      }
    }
  } 

  if( !found ) {
    // TODO: All transforms should be supported
    throw Exception( "Not supported color tranform" );
  } else {
    // Reverse path
    std::list<CSTransEdge *> step;
    ColorSpace currentNode = outCS;
    while( currentNode != inCS ) {
//       std::cerr << "edge: " << gotByEdge[ currentNode ]->srcCS << " -- "
//                 << gotByEdge[ currentNode ]->destCS << "\n";
      step.push_front( gotByEdge[ currentNode ] );
      currentNode = gotByEdge[ currentNode ]->srcCS;      
    }
    // Execute path
    std::list<CSTransEdge *>::iterator it;
    for( it = step.begin(); it != step.end(); it++ ) {
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
