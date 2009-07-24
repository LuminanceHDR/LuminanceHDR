/**
 * @brief Standard response functions
 *
 * 
 * This file is a part of Luminance package.
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
 * 
 * @author Grzegorz Krawczyk, <gkrawczyk@users.sourceforge.net>
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 *
 * $Id: responses.cpp,v 1.6 2006/09/13 14:27:06 gkrawczyk Exp $
 */

#include <iostream>
#include <vector>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "responses.h"

#define MIN_WEIGHT 1e-3

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

void exposure_weights_icip06( float* w, int M, int Mmin, int Mmax )
{
  for( int m=0 ; m<M ; m++ )
    if( m<Mmin || m>Mmax )
      w[m] = 0.0f;
    else
      w[m]=1.0f-pow( ( (2.0f*float(m-Mmin)/float(Mmax-Mmin)) - 1.0f), 12.0f);
}

void weightsGauss( float* w, int M, int Mmin, int Mmax, float sigma )
{
  float mid = Mmin + (Mmax-Mmin)/2.0f - 0.5f;
  float mid2 = (mid-Mmin) * (mid-Mmin);
  for( int m=0 ; m<M ; m++ )
    if( m<Mmin || m>Mmax )
      w[m] = 0.0f;
    else
    {
      // gkrawczyk: that's not really a gaussian, but equation is
      // taken from Robertson02 paper.
      float weight = exp( -sigma * (m-mid) * (m-mid) / mid2 );

      if( weight<MIN_WEIGHT )           // ignore very low weights
        w[m] = 0.0f;
      else
        w[m] = weight;
    }
}

void weights_triangle( float* w, int M/*, int Mmin, int Mmax*/ )
{
	for(int i=0;i<int(float(M)/2.0f);i++) {
	  w[i]=i/ (float(M)/2.0f);
// 	  if (w[i]<0.06f)w[i]=0;
	}
	for(int i=int(float(M)/2.0f);i<M;i++) {
	  w[i]=(M-1-i)/(float(M)/2.0f);
// 	  if (w[i]<0.06f)w[i]=0;
	}
//   for( int m=0 ; m<M ; m++ )
//     if( m<Mmin || m>Mmax )
//       w[m] = 0.0f;
//     else
//     {
// 	if ( m<int(Mmin+ (Mmax-Mmin)/2.0f +1) )
// 		w[m]=(m-Mmin)/float(Mmin+(Mmax-Mmin)/2.0f);
// 	else
// 		w[m]=( -m+Mmin+((Mmax-Mmin)) )/float(Mmin+(Mmax-Mmin)/2.0f);
//     }

// 	  if (w[i]<0.06f)w[i]=0;
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
  float mid = 0.5f * M;
  float norm = 0.0625f * M;
  
  for( int m=0 ; m<M ; m++ )
    I[m] = powf(10.0f, float(m - mid) / norm);
}


void responseSave( FILE* file, const float* I, int M, const char* name)
{
  // response curve matrix header
  fprintf(file, "# Camera response curve, channel %s\n", name);
  fprintf(file, "# data layout: log10(response) | camera output | response\n");
  fprintf(file, "# name: %s\n", name);
  fprintf(file, "# type: matrix\n");
  fprintf(file, "# rows: %d\n", M);
  fprintf(file, "# columns: 3\n");

  // save response
  for( int m=0 ; m<M ; m++ )
    if( I[m]!=0.0f )
      fprintf(file, " %15.9f %4d %15.9f\n", log10f(I[m]), m, I[m]);
    else
      fprintf(file, " %15.9f %4d %15.9f\n", -6.0f, m, I[m]);
  
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


bool responseLoad( FILE* file, float* I, int M)
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
  if( c!=3 )
    return false;
  
  // read response
  float ignore;
  for( int i=0 ; i<M ; i++ )
  {
    float val;
    if( fscanf(file, " %f %d %f\n", &ignore, &m, &val) !=3 )
      return false;
    if( m<0 || m>M )
      std::cerr << "response: camera value out of range,"
                << " m=" << m << std::endl;
    else
      I[m] = val;
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
