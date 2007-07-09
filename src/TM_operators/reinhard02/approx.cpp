/**
 * @file approx.cpp
 * @brief Approximate computation of local adaptation
 *
 * Source code courtesy of Erik Reinhard
 * This file is a part of Qtpfsgui package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2003-2007 Grzegorz Krawczyk
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
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 *
 * Original source code:
 * approx.c  University of Utah / Erik Reinhard / October 2001
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// extern "C" {
extern double **luminance; 
int      ImageWidth, ImageHeight;

double ***Pyramid;
int       PyramidHeight;
int       PyramidWidth0;

void build_pyramid( double **luminance, int ImageWidth, int ImageHeight );
double V1( int x, int y, int level );

int div2( const unsigned int n )
{
  const unsigned int q = n/2;
  return(2*q < n ? q + 1 : q);
}

double pyramid_lookup( int x, int y, int level )
  /* PRE:  */
{
  int n, s;
  
  /* Level 0 is a special case, the value is just the image */
  if (level == 0) {
    if ( (x < 0) || (y < 0) || (x >= ImageWidth) || (y >= ImageHeight) )
      return(0.0);
    else
      return(luminance[y][x]);
  }

  /* Compute the size of the slice */
  level--;
  n = 1 << level;
  s = PyramidWidth0 >> level;
  
  //x = x >> level;
  //y = y >> level;

  if ( (x < 0) || (y < 0) || (x >= s) || (y >= s) )
    return(0.0);
  else
    return(Pyramid[level][y][x]);
}

void build_pyramid( double **/*luminance*/, int image_width, int image_height )
{
  int k;
  int x, y;
  int i, j;
  int width/*, height*/;
  int max_dim;
//   int pyramid_height;
  double sum = 0;
  
  double a = 0.4;
  double b = 0.25;
  double c = b - a/2;
  double w[5];

  /* Compute the "filter kernel" */
  w[0] = c;
  w[1] = b;
  w[2] = a;
  w[3] = w[1];
  w[4] = w[0];
  
  /* Build the pyramid slices.  The bottom of the pyramid is the luminace  */
  /* image, and is not in the Pyramid array.                               */
  /* For simplicity, the first level is padded to a square whose side is a */
  /* power of two.                                                         */

  ImageWidth = image_width;
  ImageHeight = image_height;
  
  /* Compute the size of the Pyramid array */
  max_dim = (ImageHeight > ImageWidth ? ImageHeight : ImageWidth);
  PyramidHeight = (int) floor(log(max_dim - 0.5)/log(2)) + 1;

  /* Compute the dimensions of the first level */
  width = 1 << (PyramidHeight - 1);
  PyramidWidth0 = width;

//  fprintf(stderr, "max_dim %d   height %d     PyramidWidth0=%d\n", max_dim, PyramidHeight,PyramidWidth0);
  
  /* Allocate the outer Pyramid array */
  Pyramid = (double***) calloc(PyramidHeight, sizeof(double**));
  if (!Pyramid) {
    fprintf(stderr, "Unable to allocate pyramid array.\n");
    exit(1);
  }

  /* Allocate and assign the Pyramid slices */
  k = 0;

  while (width) {
//    fprintf(stderr, "level %d, width = %d\n", k, width);
    /* Allocate the slice */
    Pyramid[k] = (double**) calloc(width, sizeof(double*));
    if (!Pyramid[k]) {
      fprintf(stderr, "Unable to allocate pyramid array.\n");
      exit(1);
    }
    for (y = 0; y < width; y++) {
      Pyramid[k][y] = (double*) calloc(width, sizeof(double));
      if (!Pyramid[k][y]) {
        fprintf(stderr, "Unable to allocate pyramid array.\n");
        exit(1);
      }
    }

    /* Compute the values in the slice */
    for (y = 0; y < width; y++) {
      for (x = 0; x < width; x++) {

        sum = 0;
        for (i = 0; i < 5; i++) {
          for (j = 0; j < 5; j++) {
            sum += w[i]*w[j]*pyramid_lookup(2*x + i - 2, 2*y + j - 2, k);
          }
        }
        Pyramid[k][y][x] = sum; 
      }
    }

    /* compute the width of the next slice */
    width /= 2;
    k++;
  }
}

void clean_pyramid()
{
  int k=0;
  int width = PyramidWidth0;
  while(width)
  {
    for( int y=0 ; y<width ; y++ ) {
      free(Pyramid[k][y]);
    }
    free(Pyramid[k]);
    k++;
    width /= 2;
  }
  free(Pyramid);
}

double V1( int x, int y, int level )
  /* PRE:  */
{
  int x0, y0;
  int l, size;
  double s, t;
  /* Level 0 is a special case, the value is just the image */
  if (level == 0)
      return(luminance[y][x]);

  /* Compute the size of the slice */
  l = 1 << level;
  x0 = x >> level;
  y0 = y >> level;
  size = PyramidWidth0 >> (level - 1);

  x0 = (x0 >= size ? size - 1 : x0);
  y0 = (y0 >= size ? size - 1 : y0);

  s = (double)(x - x0*l)/(double)l;
  t = (double)(y - y0*l)/(double)l;

  level--;
  int x01,y01;
  x01 = (x0 == size-1 ? x0 : x0+1);
  y01 = (y0 == size-1 ? y0 : y0+1);

  return((1-s)*(1-t)*Pyramid[level][y0][x0] + s*(1-t)*Pyramid[level][y0][x01]
          + (1-s)*t*Pyramid[level][y01][x0] + s*t*Pyramid[level][y01][x01]);
}
// }
