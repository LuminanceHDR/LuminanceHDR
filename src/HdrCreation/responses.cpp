/*
 * This file is a part of Luminance HDR package
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2004 Grzegorz Krawczyk
 * Copyright (C) 2006-2007 Giuseppe Rota
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
 */

//! \brief Standard response functions
//! \author Grzegorz Krawczyk, <gkrawczyk@users.sourceforge.net>
//! \author Giuseppe Rota <grota@users.sourceforge.net>
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#include <string>
#include <map>
#include <iostream>
#include <vector>
#include <cassert>

#include <cstdio>
#include <cstdlib>
#include <cmath>

#include <boost/assign.hpp>

#include "responses.h"
#include <Libpfs/colorspace/rgb.h>
#include <Libpfs/utils/string.h>

using namespace std;
using namespace boost;
using namespace boost::assign;

namespace libhdr {
namespace fusion {

ResponseFunction fromString(const std::string& type)
{
    typedef map<string, ResponseFunction, pfs::utils::StringUnsensitiveComp> Dict;
    static Dict v =
            map_list_of
            ("log10", RESPONSE_LOG10)
            ("log", RESPONSE_LOG10)
            ("linear", RESPONSE_LINEAR)
            ("gamma", RESPONSE_GAMMA)
            ("srgb", RESPONSE_SRGB)
            ;

    Dict::const_iterator it = v.find(type);
    if ( it != v.end() ) {
        return it->second;
    }
    return RESPONSE_LINEAR;
}


float ResponseGamma::getResponse(float input) const
{
    return std::pow( 4.f * input, 1.7f ) + 1e-4;
}

// I use a namespace to avoid name collision
namespace details_log10
{
const float s_mid               = 0.5f;
const float s_norm              = 0.0625f;
// the value 8.f is s_mid/s_norm. The actual formula is 10^((i - mid)/norm)
const float s_inverseMaxValue   = 1.f/1e8; // == 1.f/std::pow(10.0f, (1.f/norm - 8.f));
}

float ResponseLog10::getResponse(float input) const
{
    return details_log10::s_inverseMaxValue *   // normalization between [0, 1]
            std::pow(10.0f, ((input/details_log10::s_norm) - 8.f) );
}

float ResponseSRGB::getResponse(float input) const
{
    return pfs::colorspace::ConvertSRGB2RGB()(input);
}

}   // fusion
}   // libhdr

void dump_gnuplot( const char* filename, const float* array, int M )
{
  FILE* fp = fopen(filename, "w");
  fprintf(fp, "# GNUPlot dump\n");
  for( int i=0 ; i<M ; i++ )
    fprintf(fp, "%4d %16.9f\n", i, array[i]);
  fclose(fp);
  std::cerr << "GNUPLOT: save data to " << filename << std::endl;
}

void dump_gnuplot( const char* filename, const float* array, int M, int counter )
{
  char fn[2048];
  sprintf(fn, filename, counter);
  dump_gnuplot(fn, array, M);
}

void responseLinear( float* I, int M )
{
  for( int m=0 ; m<M ; m++ )
    I[m] = m / float(M-1); // range is not important, values are normalized later
}


void responseGamma( float* I, int M )
{
  float norm = M / 4.0f;
  
  // response curve decided empirically
  for( int m=0 ; m<M ; m++ )
    I[m] = powf( m/norm, 1.7f ) + 1e-4;
}


void responseLog10( float* I, int M )
{
    const float mid = 0.5f * M;
    const float norm = 0.0625f * M;
    const float maxValue = powf(10.0f, float(M-1 - mid) / norm);

    for( int m=0 ; m < M; ++m)
    {
        I[m] = powf(10.0f, float(m - mid) / norm) / maxValue;
    }
}

void responseSave( FILE* file, const float* Ir, const float* Ig, const float* Ib, int M)
{
  // response curve matrix header
  fprintf(file, "# Camera response curve, channels Ir, Ig, Ib \n");
  fprintf(file, "# data layout: camera output | log10(response Ir) | response Ir | log10(response Ig) | response Ig | log10(response Ib) | response Ib \n");
  fprintf(file, "# type: matrix\n");
  fprintf(file, "# rows: %d\n", M);
  fprintf(file, "# columns: 7\n");

  // save response
  for( int m=0 ; m<M ; m++ ) {
     float logR = Ir[m] == 0.0f ? -6.0f : log10f(Ir[m]);
     float logG = Ig[m] == 0.0f ? -6.0f : log10f(Ig[m]);
     float logB = Ib[m] == 0.0f ? -6.0f : log10f(Ib[m]);
     fprintf(file, " %4d %15.9f %15.9f %15.9f %15.9f %15.9f %15.9f \n", m, logR, Ir[m], logG, Ig[m], logB, Ib[m]);
  }
  fprintf(file, "\n");
}


void weightsSave( FILE* file, const float* w, int M, const char* name)
{
  // weighting function matrix header
  fprintf(file, "# Weighting function\n");
  fprintf(file, "# data layout: weight | camera output\n");
  fprintf(file, "# name: %s\n", name);
  fprintf(file, "# type: matrix\n");
  fprintf(file, "# rows: %d\n", M);
  fprintf(file, "# columns: 2\n");
  
  // save weights
  for( int m=0 ; m<M ; m++ )
    fprintf(file, " %15.9f %4d\n", w[m], m);
  
  fprintf(file, "\n");
}


bool responseLoad( FILE* file, float* Ir, float* Ig, float* Ib, int M)
{
  char line[1024];
  int m=0,c=0;
  
  // parse response curve matrix header
  while( fgets(line, 1024, file) )
    if( sscanf(line, "# rows: %d\n", &m) == 1 )
      break;
  if( m!=M )
  {
    std::cerr << "response: number of input levels is different,"
              << " M=" << M << " m=" << m << std::endl; 
    return false;
  }
  while( fgets(line, 1024, file) )
    if( sscanf(line, "# columns: %d\n", &c) == 1 )
      break;
  if( c!=7 )
    return false;
  
  // read response
  float ignoreR, ignoreG, ignoreB;
  for( int i=0 ; i<M ; i++ )
  {
    float valR, valG, valB;
    if( fscanf(file, " %d %f %f %f %f %f %f \n", &m, &ignoreR, &valR, &ignoreG, &valG, &ignoreB, &valB) !=7 )
      return false;
    if( m<0 || m>M )
      std::cerr << "response: camera value out of range,"
                << " m=" << m << std::endl;
    else {
      Ir[m] = valR;
      Ig[m] = valG;
      Ib[m] = valB;
	}
  }
  
  return true;
}


bool weightsLoad( FILE* file, float* w, int M)
{
  char line[1024];
  int m=0,c=0;

  // parse weighting function matrix header
  while( fgets(line, 1024, file) )
    if( sscanf(line, "# rows: %d\n", &m) == 1 )
      break;
  if( m!=M )
  {
    std::cerr << "response: number of input levels is different,"
              << " M=" << M << " m=" << m << std::endl; 
    return false;
  }
  while( fgets(line, 1024, file) )
    if( sscanf(line, "# columns: %d\n", &c) == 1 )
      break;
  if( c!=2 )
    return false;
  
  // read response 
  for( int i=0 ; i<M ; i++ )
    if( fscanf(file, " %f %d\n", &(w[i]), &m) !=2 )
      return false;
  
  return true;
}
