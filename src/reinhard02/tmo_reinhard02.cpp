/**
 * @file tmo_reinhard02.cpp
 * @brief Tone map luminance channel using Reinhard02 model
 *
 * Implementation courtesy of Erik Reinhard. 
 *
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
 * Original source code note:
 * Tonemap.c  University of Utah / Erik Reinhard / October 2001
 *
 * File taken from:
 * http://www.cs.utah.edu/~reinhard/cdrom/source.html
 *
 * Port to PFS tools library by Grzegorz Krawczyk <krawczyk@mpi-sb.mpg.de>
 *
 * $Id: tmo_reinhard02.cpp,v 1.5 2005/12/05 13:03:39 mantiuk Exp $
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <pfs.h>

extern "C" {
void print_parameter_settings ();
//--- from defines.h
typedef struct {
  int     xmax, ymax;     /* image dimensions */
} CVTS;

typedef double  COLOR[3];       /* red, green, blue (or X,Y,Z) */
//--- end of defines.h


// static int       width, height, scale;
static COLOR   **image;
// static double ***convolved_image;
static double    sigma_0, sigma_1;
double         **luminance;

// static double    k                = 1. / (2. * 1.4142136);
static double    key              = 0.18;
static double    threshold        = 0.05;
static double    phi              = 8.;
static double    white            = 1e20;
static int       scale_low        = 1;
static int       scale_high       = 43;  // 1.6^8 = 43
static int       range            = 8;
static int       use_scales       = 0;
static int       use_border       = 0;
static CVTS      cvts             = {0, 0};

static bool temporal_coherent;

extern double V1 (int x, int y, int level);
extern void build_pyramid (double **luminance, int image_width, int image_height);
extern void clean_pyramid ();
extern int PyramidHeight;

#define SIGMA_I(i)       (sigma_0 + ((double)i/(double)range)*(sigma_1 - sigma_0))
#define S_I(i)           (exp (SIGMA_I(i)))
#define V2(x,y,i)        (V1(x,y,i+1))
#define ACTIVITY(x,y,i)  ((V1(x,y,i) - V2(x,y,i)) / (((key * pow (2., phi))/(S_I(i)*S_I(i))) + V1(x,y,i)))
}


/**
 * Used to achieve temporal coherence
 */
template<class T>
class TemporalSmoothVariable
{
//  const int hist_length = 100;
  T value;

  T getThreshold( T luminance )
  {
    return 0.01 * luminance;
  }
  
public:
  TemporalSmoothVariable() : value( -1 )
  {
  }
  
  void set( T new_value )
  {
    if( value == -1 )
      value = new_value;
    else {
      T delta = new_value - value;
      const T threshold = getThreshold( (new_value + value)/2 );
      if( delta > threshold ) delta = threshold;
      else if( delta < -threshold ) delta = -threshold;
      value += delta;
    }
  }
  
  T get() const
  {
    return value;
  }  
};


static TemporalSmoothVariable<double> avg_luminance, max_luminance;

extern "C" {

/*
 * Kaiser-Bessel stuff
 */

static double   alpha = 2.;         /* Kaiser-Bessel window parameter   */
static double   bbeta;              /* Will contain bessel (PI * alpha) */

/*
 * Modified zeroeth order bessel function of the first kind
 */

double bessel (double x)
{
  const double f = 1e-9;
  int          n = 1;
  double       s = 1.;
  double       d = 1.;

  double t;

  while (d > f * s)
  {
    t = x / (2. * n);
    n++;
    d *= t * t;
    s += d;
  }
  return s;
}

/*
 * Initialiser for Kaiser-Bessel computations
 */

void compute_bessel ()
{
  bbeta = bessel (M_PI * alpha);
}

/*
 * Kaiser-Bessel function with window length M and parameter beta = 2.
 * Window length M = min (width, height) / 2
 */

double kaiserbessel (double x, double y, double M)
{
  double d = 1. - ((x*x + y*y) / (M * M));
  if (d <= 0.)
    return 0.;
  return bessel (M_PI * alpha * sqrt (d)) / bbeta;
}


/*
 * Tonemapping routines
 */

double get_maxvalue ()
{
  double max = 0.;
  int    x, y;

  for (y = 0; y < cvts.ymax; y++)
    for (x = 0; x < cvts.xmax; x++)
      max = (max < image[y][x][0]) ? image[y][x][0] : max;
  return max;
}

void tonemap_image ()
{
  double Lmax2;
  int    x, y;
  int    scale;
  int prefscale;

  if (white < 1e20)
    Lmax2 = white * white;
  else
  {
    if( temporal_coherent ) {
      max_luminance.set( get_maxvalue() );
      Lmax2 = max_luminance.get();
    } else Lmax2  = get_maxvalue();
    Lmax2 *= Lmax2;
  }

  for (y = 0; y < cvts.ymax; y++) {
    for (x = 0; x < cvts.xmax; x++)
    {
      if (use_scales)
      {
	prefscale = range - 1;
	for (scale = 0; scale < range - 1; scale++) {
	  if (scale>=PyramidHeight) {
	  	prefscale = scale;
	  	break;
	  }
	  if (fabs(ACTIVITY(x,y,scale)) > threshold) {
	    prefscale = scale;
	    break;
	  }
	}
	image[y][x][0] /= 1. + V1(x,y,prefscale);
      }
      else
	image[y][x][0] = image[y][x][0] * (1. + (image[y][x][0] / Lmax2)) / (1. + image[y][x][0]);
      // image[y][x][0] /= (1. + image[y][x][0]);
    }
    }
}

/*
 * Miscellaneous functions
 */

void clamp_image ()
{
  int x, y;

  for (y = 0; y < cvts.ymax; y++)
    for (x = 0; x < cvts.xmax; x++)
    {
      image[y][x][0] = (image[y][x][0] > 1.) ? 1. : image[y][x][0];
      image[y][x][1] = (image[y][x][1] > 1.) ? 1. : image[y][x][1];
      image[y][x][2] = (image[y][x][2] > 1.) ? 1. : image[y][x][2];
    }
}

double log_average ()
{
  int    x, y;
  double sum = 0.;

  for (x = 0; x < cvts.xmax; x++)
    for (y = 0; y < cvts.ymax; y++)
      sum += log (0.00001 + luminance[y][x]);
  return exp (sum / (double)(cvts.xmax * cvts.ymax));
}

void scale_to_midtone ()
{
  int    x, y, u, v, d;
  double factor;
  double scale_factor;
  double low_tone    = key / 3.;
  int    border_size = (cvts.xmax < cvts.ymax) ? int(cvts.xmax / 5.) : int(cvts.ymax / 5.);
  int    hw          = cvts.xmax >> 1;
  int    hh          = cvts.ymax >> 1;

  double avg;
  if( temporal_coherent ) {
    avg_luminance.set( log_average() );
    avg = avg_luminance.get();
  } else avg = log_average();
  
  scale_factor = 1.0 / avg;
  for (y = 0; y < cvts.ymax; y++)
    for (x = 0; x < cvts.xmax; x++)
    {
      if (use_border)
      {
	u              = (x > hw) ? cvts.xmax - x : x;
	v              = (y > hh) ? cvts.ymax - y : y;
	d              = (u < v) ? u : v;	
	factor         = (d < border_size) ? (key - low_tone) * 
	                  kaiserbessel (border_size - d, 0, border_size) + 
                          low_tone : key;
      }
      else
	factor         = key;
      image[y][x][0]  *= scale_factor * factor;
      luminance[y][x] *= scale_factor * factor;
    }
}

void copy_luminance ()
{
  int x, y;

  for (x = 0; x < cvts.xmax; x++)
    for (y = 0; y < cvts.ymax; y++)
      luminance[y][x] = image[y][x][0];
}

/*
 * Memory allocation
 */
void allocate_memory ()
{
  int y;

  luminance = (double **) malloc (cvts.ymax * sizeof (double *));
  image     = (COLOR **) malloc (cvts.ymax * sizeof (COLOR *));
  for (y = 0; y < cvts.ymax; y++)
  {
    luminance[y] = (double *) malloc (cvts.xmax * sizeof (double));
    image[y]     = (COLOR *) malloc (cvts.xmax * sizeof (COLOR));
  }
}

void deallocate_memory ()
{
  int y;

  for (y = 0; y < cvts.ymax; y++)
  {
    free(luminance[y]);
    free(image[y]);
  }
  free( luminance );
  free( image );
}


void dynamic_range ()
{
  int x, y;
  double minval =  1e20;
  double maxval = -1e20;

  for (x = 0; x < cvts.xmax; x++)
    for (y = 0; y < cvts.ymax; y++)
    {
      if ((luminance[y][x] < minval) &&
	  (luminance[y][x] > 0.0))
	minval = luminance[y][x];
      if (luminance[y][x] > maxval)
	maxval = luminance[y][x];
    }
  fprintf (stderr, "\tRange of values  = %9.8f - %9.8f\n", minval, maxval);
  fprintf (stderr, "\tDynamic range    = %i:1\n", (int)(maxval/minval));
}

void print_parameter_settings ()
{
  fprintf (stderr, "\tImage size       = %i %i\n", cvts.xmax, cvts.ymax);
  fprintf (stderr, "\tLowest scale     = %i pixels\t\t(-low <integer>)\n", scale_low);
  fprintf (stderr, "\tHighest scale    = %i pixels\t\t(-high <integer>)\n", scale_high);
  fprintf (stderr, "\tNumber of scales = %i\t\t\t(-num <integer>)\n", range);
  fprintf (stderr, "\tScale spacing    = %f\n", S_I(1) / S_I(0));
  fprintf (stderr, "\tKey value        = %f\t\t(-key <float>)\n", key);
  fprintf (stderr, "\tWhite value      = %f\t\t(-white <float>)\n", white);
  fprintf (stderr, "\tPhi              = %f\t\t(-phi <float>)\n", phi);
  fprintf (stderr, "\tThreshold        = %f\t\t(-threshold <float>)\n", threshold);
  dynamic_range ();
}

/*
 * @brief Photographic tone-reproduction
 *
 * @param Y input luminance
 * @param L output tonemapped intensities
 * @param use_scales true: local version, false: global version of TMO
 * @param key maps log average luminance to this value (default: 0.18)
 * @param phi sharpening parameter (defaults to 1 - no sharpening)
 * @param num number of scales to use in computation (default: 8)
 * @param low size in pixels of smallest scale (should be kept at 1)
 * @param high size in pixels of largest scale (default 1.6^8 = 43)
 */
void tmo_reinhard02(const pfs::Array2D *Y, pfs::Array2D *L, 
  bool use_scales, float key, float phi, 
  int num, int low, int high, bool temporal_coherent )
{
  int x,y;
  ::key = key;
  ::phi = phi;
  ::range = num;
  ::scale_low = low;
  ::scale_high = high;
  ::use_scales = (use_scales) ? 1 : 0;
  ::temporal_coherent = temporal_coherent;

  cvts.xmax = Y->getCols();
  cvts.ymax = Y->getRows();

  sigma_0      = log (scale_low);
  sigma_1      = log (scale_high);

  compute_bessel();
  allocate_memory ();

  // reading image
  for( y=0 ; y<cvts.ymax ; y++ )
    for( x=0 ; x<cvts.xmax ; x++ )
      image[y][x][0] = (*Y)(x,y);

  copy_luminance();
  scale_to_midtone();

  if( use_scales )
  {
    build_pyramid(luminance, cvts.xmax, cvts.ymax);
  }

  tonemap_image();

  // saving image
  for( y=0 ; y<cvts.ymax ; y++ )
    for( x=0 ; x<cvts.xmax ; x++ )
      (*L)(x,y) = image[y][x][0];

//  print_parameter_settings();

  deallocate_memory();
  if( use_scales )
  clean_pyramid();
}

}
