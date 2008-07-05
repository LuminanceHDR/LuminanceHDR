/**
 * @Brief Contrast mapping TMO
 *
 * From:
 * 
 * Rafal Mantiuk, Karol Myszkowski, Hans-Peter Seidel.
 * A Perceptual Framework for Contrast Processing of High Dynamic Range Images
 * In: ACM Transactions on Applied Perception 3 (3), pp. 286-308, 2006
 * http://www.mpi-inf.mpg.de/~mantiuk/contrast_domain/
 *
 * This file is a part of PFSTMO package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2007 Grzegorz Krawczyk
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
 * @author Radoslaw Mantiuk, <radoslaw.mantiuk@gmail.com>
 * @author Rafal Mantiuk, <mantiuk@gmail.com>
 * Updated 2007/12/17 by Ed Brambley <E.J.Brambley@damtp.cam.ac.uk>
 *  (more information on the changes:
 *  http://www.damtp.cam.ac.uk/user/ejb48/hdr/index.html)
 *
 * $Id: contrast_domain.cpp,v 1.9 2008/02/29 16:46:28 rafm Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef Q_WS_X11
#define exp10f( x ) exp( (x) * log (10))
#endif

#include "contrast_domain.h"

typedef struct pyramid_s {
  int rows;
  int cols;
  float* Gx;
  float* Gy;
  struct pyramid_s* next;
  struct pyramid_s* prev;
} pyramid_t;


extern float xyz2rgbD65Mat[3][3];
extern float rgb2xyzD65Mat[3][3];

#define PYRAMID_MIN_PIXELS 3
#define LOOKUP_W_TO_R 107


static void contrast_equalization( pyramid_t *pp, const float contrastFactor );

static void transform_to_luminance(pyramid_t* pyramid, float* const x, progress_callback progress_cb, const bool bcg, const int itmax, const float tol);
static void matrix_add(const int n, const float* const a, float* const b);
static void matrix_subtract(const int n, const float* const a, float* const b);
static void matrix_copy(const int n, const float* const a, float* const b);
static void matrix_multiply_const(const int n, float* const a, const float val);
static void matrix_divide(const int n, const float* const a, float* const b);
static float* matrix_alloc(const int size);
static void matrix_free(float* m);
static float matrix_DotProduct(const int n, const float* const a, const float* const b);
static void matrix_zero(const int n, float* const m);
static void calculate_and_add_divergence(const int rows, const int cols, const float* const Gx, const float* const Gy, float* const divG);
//static void pyramid_calculate_divergence(pyramid_t* pyramid);
static void pyramid_calculate_divergence_sum(pyramid_t* pyramid, float* divG_sum);
static void calculate_scale_factor(const int n, const float* const G, float* const C);
static void pyramid_calculate_scale_factor(pyramid_t* pyramid, pyramid_t* pC);
static void scale_gradient(const int n, float* const G, const float* const C);
static void pyramid_scale_gradient(pyramid_t* pyramid, pyramid_t* pC);
static void pyramid_free(pyramid_t* pyramid);
static pyramid_t* pyramid_allocate(const int cols, const int rows);
static void calculate_gradient(const int cols, const int rows, const float* const lum, float* const Gx, float* const Gy);
static void pyramid_calculate_gradient(pyramid_t* pyramid, float* lum);
static void solveX(const int n, const float* const b, float* const x);
static void multiplyA(pyramid_t* px, pyramid_t* pyramid, const float* const x, float* const divG_sum);
static void linbcg(pyramid_t* pyramid, pyramid_t* pC, float* const b, float* const x, const int itmax, const float tol, progress_callback progress_cb);
static void lincg(pyramid_t* pyramid, pyramid_t* pC, float* const b, float* const x, const int itmax, const float tol, progress_callback progress_cb);
static float lookup_table(const int n, const float* const in_tab, const float* const out_tab, const float val);
static void transform_to_R(const int n, float* const G);
static void pyramid_transform_to_R(pyramid_t* pyramid);
static void transform_to_G(const int n, float* const R);
static void pyramid_transform_to_G(pyramid_t* pyramid);
static void pyramid_gradient_multiply(pyramid_t* pyramid, const float val);

//static void dump_matrix_to_file(const int width, const int height, const float* const m, const char * const file_name);
static void matrix_show(const char* const text, int rows, int cols, const float* const data);
//static void pyramid_show(pyramid_t* pyramid);


static float W_table[] = {0.000000,0.010000,0.021180,0.031830,0.042628,0.053819,0.065556,0.077960,0.091140,0.105203,0.120255,0.136410,0.153788,0.172518,0.192739,0.214605,0.238282,0.263952,0.291817,0.322099,0.355040,0.390911,0.430009,0.472663,0.519238,0.570138,0.625811,0.686754,0.753519,0.826720,0.907041,0.995242,1.092169,1.198767,1.316090,1.445315,1.587756,1.744884,1.918345,2.109983,2.321863,2.556306,2.815914,3.103613,3.422694,3.776862,4.170291,4.607686,5.094361,5.636316,6.240338,6.914106,7.666321,8.506849,9.446889,10.499164,11.678143,13.000302,14.484414,16.151900,18.027221,20.138345,22.517282,25.200713,28.230715,31.655611,35.530967,39.920749,44.898685,50.549857,56.972578,64.280589,72.605654,82.100619,92.943020,105.339358,119.530154,135.795960,154.464484,175.919088,200.608905,229.060934,261.894494,299.838552,343.752526,394.651294,453.735325,522.427053,602.414859,695.706358,804.693100,932.229271,1081.727632,1257.276717,1463.784297,1707.153398,1994.498731,2334.413424,2737.298517,3215.770944,3785.169959,4464.187290,5275.653272,6247.520102,7414.094945,8817.590551,10510.080619};
static float R_table[] = {0.000000,0.009434,0.018868,0.028302,0.037736,0.047170,0.056604,0.066038,0.075472,0.084906,0.094340,0.103774,0.113208,0.122642,0.132075,0.141509,0.150943,0.160377,0.169811,0.179245,0.188679,0.198113,0.207547,0.216981,0.226415,0.235849,0.245283,0.254717,0.264151,0.273585,0.283019,0.292453,0.301887,0.311321,0.320755,0.330189,0.339623,0.349057,0.358491,0.367925,0.377358,0.386792,0.396226,0.405660,0.415094,0.424528,0.433962,0.443396,0.452830,0.462264,0.471698,0.481132,0.490566,0.500000,0.509434,0.518868,0.528302,0.537736,0.547170,0.556604,0.566038,0.575472,0.584906,0.594340,0.603774,0.613208,0.622642,0.632075,0.641509,0.650943,0.660377,0.669811,0.679245,0.688679,0.698113,0.707547,0.716981,0.726415,0.735849,0.745283,0.754717,0.764151,0.773585,0.783019,0.792453,0.801887,0.811321,0.820755,0.830189,0.839623,0.849057,0.858491,0.867925,0.877358,0.886792,0.896226,0.905660,0.915094,0.924528,0.933962,0.943396,0.952830,0.962264,0.971698,0.981132,0.990566,1.000000};


// display matrix in the console (debugging)
static void matrix_show(const char* const text, int cols, int rows, const float* const data)
{
  const int _cols = cols;

  if(rows > 8)
    rows = 8;
  if(cols > 8)
    cols = 8;

  printf("\n%s\n", text);
  for(int ky=0; ky<rows; ky++){
    for(int kx=0; kx<cols; kx++){
      printf("%.06f  ", data[kx + ky*_cols]);
    }
    printf("\n");
  }
}

#if 0
// display pyramid in the console (debugging)
static void pyramid_show(pyramid_t* pyramid)
{
  char ss[30];

  while (pyramid->next != NULL)
    pyramid = pyramid->next;

  while (pyramid != NULL)
    {
      printf("\n----- pyramid_t level %d,%d\n", pyramid->cols, pyramid->rows);
	
      sprintf(ss, "Gx %p ", pyramid->Gx);
      if(pyramid->Gx != NULL)
	matrix_show(ss,pyramid->cols, pyramid->rows, pyramid->Gx);
      sprintf(ss, "Gy %p ", pyramid->Gy);	
      if(pyramid->Gy != NULL)
	matrix_show(ss,pyramid->cols, pyramid->rows, pyramid->Gy);

      pyramid = pyramid->prev;
    }
}
#endif



static inline float max( float a, float b )
{
  return a > b ? a : b;
}

static inline float min( float a, float b )
{
  return a < b ? a : b;
}


// upsample the matrix
// upsampled matrix is twice bigger in each direction than data[]
// res should be a pointer to allocated memory for bigger matrix
// cols and rows are the dimmensions of the output matrix
static void matrix_upsample(const int outCols, const int outRows, const float* const in, float* const out)
{
  const int inRows = outRows/2;
  const int inCols = outCols/2;

  // Transpose of experimental downsampling matrix (theoretically the correct thing to do)

  const float dx = (float)inCols / ((float)outCols);
  const float dy = (float)inRows / ((float)outRows);
  const float factor = 1.0f / (dx*dy); // This gives a genuine upsampling matrix, not the transpose of the downsampling matrix
  // const float factor = 1.0f; // Theoretically, this should be the best.

  for (int y = 0; y < outRows; y++)
    {
      const float sy = y * dy;
      const int iy1 = (  y   * inRows) / outRows;
      const int iy2 = ((y+1) * inRows) / outRows;

      for (int x = 0; x < outCols; x++)
	{
	  const float sx = x * dx;
	  const int ix1 = (  x   * inCols) / outCols;
	  const int ix2 = ((x+1) * inCols) / outCols;

	  out[x + y*outCols] = (
	    ((ix1+1) - sx)*((iy1+1 - sy)) * in[ix1 + iy1*inCols] +
	    ((ix1+1) - sx)*(sy+dy - (iy1+1)) * in[ix1 + iy2*inCols] +
	    (sx+dx - (ix1+1))*((iy1+1 - sy)) * in[ix2 + iy1*inCols] +
	    (sx+dx - (ix1+1))*(sy+dx - (iy1+1)) * in[ix2 + iy2*inCols])*factor;
	}
    }
}


// downsample the matrix
static void matrix_downsample(const int inCols, const int inRows, const float* const data, float* const res)
{
  const int outRows = inRows / 2;
  const int outCols = inCols / 2;

  const float dx = (float)inCols / ((float)outCols);
  const float dy = (float)inRows / ((float)outRows);

  // New downsampling by Ed Brambley:
  // Experimental downsampling that assumes pixels are square and
  // integrates over each new pixel to find the average value of the
  // underlying pixels.
  //
  // Consider the original pixels laid out, and the new (larger)
  // pixels layed out over the top of them.  Then the new value for
  // the larger pixels is just the integral over that pixel of what
  // shows through; i.e., the values of the pixels underneath
  // multiplied by how much of that pixel is showing.
  //
  // (ix1, iy1) is the coordinate of the top left visible pixel.
  // (ix2, iy2) is the coordinate of the bottom right visible pixel.
  // (fx1, fy1) is the fraction of the top left pixel showing.
  // (fx2, fy2) is the fraction of the bottom right pixel showing.

  const float normalize = 1.0f/(dx*dy);
  for (int y = 0; y < outRows; y++)
    {
      const int iy1 = (  y   * inRows) / outRows;
      const int iy2 = ((y+1) * inRows) / outRows;
      const float fy1 = (iy1+1) - y * dy;
      const float fy2 = (y+1) * dy - iy2;

      for (int x = 0; x < outCols; x++)
	{
	  const int ix1 = (  x   * inCols) / outCols;
	  const int ix2 = ((x+1) * inCols) / outCols;
	  const float fx1 = (ix1+1) - x * dx;
	  const float fx2 = (x+1) * dx - ix2;
	  
	  float pixVal = 0.0f;
	  float factorx, factory;
	  for (int i = iy1; i <= iy2 && i < inRows; i++)
	    {
	      if (i == iy1)
		factory = fy1;  // We're just getting the bottom edge of this pixel
	      else if (i == iy2)
		factory = fy2;  // We're just gettting the top edge of this pixel
	      else
		factory = 1.0f; // We've got the full height of this pixel
	      for (int j = ix1; j <= ix2 && j < inCols; j++)
		{
		  if (j == ix1)
		    factorx = fx1;  // We've just got the right edge of this pixel
		  else if (j == ix2)
		    factorx = fx2; // We've just got the left edge of this pixel
		  else
		    factorx = 1.0f; // We've got the full width of this pixel

		  pixVal += data[j + i*inCols] * factorx * factory;
		}
	    }

	  res[x + y * outCols] = pixVal * normalize;  // Normalize by the area of the new pixel
	}
    }
}

// return = a + b
static inline void matrix_add(const int n, const float* const a, float* const b)
{
  for(int i=0; i<n; i++)
    b[i] += a[i];
}

// return = a - b
static inline void matrix_subtract(const int n, const float* const a, float* const b)
{
  for(int i=0; i<n; i++)
    b[i] = a[i] - b[i];
}

// copy matix a to b, return = a 
static inline void matrix_copy(const int n, const float* const a, float* const b)
{
  memcpy(b, a, sizeof(float)*n);
}

// multiply matrix a by scalar val
static inline void matrix_multiply_const(const int n, float* const a, const float val)
{
  for(int i=0; i<n; i++)
    a[i] *= val;
}

// b = a[i] / b[i]
static inline void matrix_divide(const int n, const float* const a, float* const b)
{
  for(int i=0; i<n; i++)
    b[i] = a[i] / b[i];
}


// alloc memory for the float table
static inline float* matrix_alloc(int size){

  float* m = (float*)malloc(sizeof(float)*size);
  if(m == NULL){
    fprintf(stderr, "ERROR: malloc in matrix_alloc() (size:%d)", size);
    exit(155);
  }

  return m;
}

// free memory for matrix
static inline void matrix_free(float* m){
  if(m != NULL)
    free(m);
}

// multiply vector by vector (each vector should have one dimension equal to 1)
static inline float matrix_DotProduct(const int n, const float* const a, const float* const b){
  float val = 0;
  for(int j=0;j<n;j++)
    val += a[j] * b[j];
  return val;
}

// set zeros for matrix elements
static inline void matrix_zero(int n, float* m)
{
  memset(m, 0, n*sizeof(float));
  //bzero(m, n*sizeof(float));
}

// calculate divergence of two gradient maps (Gx and Gy)
// divG(x,y) = Gx(x,y) - Gx(x-1,y) + Gy(x,y) - Gy(x,y-1)  
static inline void calculate_and_add_divergence(const int cols, const int rows, const float* const Gx, const float* const Gy, float* const divG)
{
  float divGx, divGy;
	
  for(int ky=0; ky<rows; ky++)
    for(int kx=0; kx<cols; kx++)
      {
	const int idx = kx + ky*cols;
	
	if(kx == 0)
	  divGx = Gx[idx];
	else	
	  divGx = Gx[idx] - Gx[idx-1];
	
	if(ky == 0)
	  divGy = Gy[idx];
	else	
	  divGy = Gy[idx] - Gy[idx - cols];			
		
	divG[idx] += divGx + divGy;			
    }
}

// calculate the sum of divergences for the all pyramid level
// the smaller divergence map is upsamled and added to the divergence map for the higher level of pyramid
// temp is a temporary matrix of size (cols, rows), assumed to already be allocated
static void pyramid_calculate_divergence_sum(pyramid_t* pyramid, float* divG_sum)
{
  float* temp = matrix_alloc(pyramid->rows*pyramid->cols);

  // Find the coarsest pyramid, and the number of pyramid levels
  int levels = 1;
  while (pyramid->next != NULL)
    {
      levels++;
      pyramid = pyramid->next;
    }

  // For every level, we swap temp and divG_sum.  So, if there are an odd number of levels...
  if (levels % 2)
    {
      float* const dummy = divG_sum;
      divG_sum = temp;
      temp = dummy;
    }
	
  // Add them all together
  while (pyramid != NULL)
    {
      // Upsample or zero as needed
      if (pyramid->next != NULL)
	matrix_upsample(pyramid->cols, pyramid->rows, divG_sum, temp);
      else
	matrix_zero(pyramid->rows * pyramid->cols, temp);

      // Add in the (freshly calculated) divergences
      calculate_and_add_divergence(pyramid->cols, pyramid->rows, pyramid->Gx, pyramid->Gy, temp);

//   char name[256];
//   sprintf( name, "Up_%d.pfs", pyramid->cols );
//   dump_matrix_to_file( pyramid->cols, pyramid->rows, temp, name );  

      // matrix_copy(pyramid->rows*pyramid->cols, temp, divG_sum);

      // Rather than copying, just switch round the pointers: we know we get them the right way round at the end.
      float* const dummy = divG_sum;
      divG_sum = temp;
      temp = dummy;

      pyramid = pyramid->prev;
    }

  matrix_free(temp);
}

// calculate scale factors (Cx,Cy) for gradients (Gx,Gy)
// C is equal to EDGE_WEIGHT for gradients smaller than GFIXATE or 1.0 otherwise
static inline void calculate_scale_factor(const int n, const float* const G, float* const C)
{
//  float GFIXATE = 0.1f;
//  float EDGE_WEIGHT = 0.01f;
  const float detectT = 0.001f;
  const float a = 0.038737;
  const float b = 0.537756;
	
  for(int i=0; i<n; i++)
    {
#if 1
      const float g = max( detectT, fabsf(G[i]) );    
      C[i] = 1.0f / (a*powf(g,b));
#else
      if(fabsf(G[i]) < GFIXATE)
	C[i] = 1.0f / EDGE_WEIGHT;
      else	
	C[i] = 1.0f;
#endif
    }
}

// calculate scale factor for the whole pyramid
static void pyramid_calculate_scale_factor(pyramid_t* pyramid, pyramid_t* pC)
{
  while (pyramid != NULL)
    {
      const int size = pyramid->rows * pyramid->cols;
      calculate_scale_factor(size, pyramid->Gx, pC->Gx);
      calculate_scale_factor(size, pyramid->Gy, pC->Gy);
      pyramid = pyramid->next;
      pC = pC->next;
    }
}

// Scale gradient (Gx and Gy) by C (Cx and Cy)
// G = G / C
static inline void scale_gradient(const int n, float* const G, const float* const C)
{
  for(int i=0; i<n; i++)
    G[i] *= C[i];
}

// scale gradients for the whole one pyramid with the use of (Cx,Cy) from the other pyramid
static void pyramid_scale_gradient(pyramid_t* pyramid, pyramid_t* pC)
{
  while (pyramid != NULL)
    {
      const int size = pyramid->rows * pyramid->cols;
      scale_gradient(size, pyramid->Gx, pC->Gx);	
      scale_gradient(size, pyramid->Gy, pC->Gy);
      pyramid = pyramid->next;
      pC = pC->next;
    }
}


// free memory allocated for the pyramid
static void pyramid_free(pyramid_t* pyramid)
{
  while (pyramid)
    {
      if(pyramid->Gx != NULL)
	{
	  free(pyramid->Gx);
	  pyramid->Gx = NULL;
	}
      if(pyramid->Gy != NULL)
	{
	  free(pyramid->Gy);
	  pyramid->Gy = NULL;
	}
      pyramid_t* const next = pyramid->next;
      pyramid->prev = NULL;
      pyramid->next = NULL;
      free(pyramid);
      pyramid = next;
    }			
}


// allocate memory for the pyramid
static pyramid_t * pyramid_allocate(int cols, int rows)
{
  pyramid_t* level = NULL;
  pyramid_t* pyramid = NULL;
  pyramid_t* prev = NULL;

  while(rows >= PYRAMID_MIN_PIXELS && cols >= PYRAMID_MIN_PIXELS)
    {
      level = (pyramid_t *) malloc(sizeof(pyramid_t));
      if(level == NULL)
	{
	  fprintf(stderr, "ERROR: malloc in pyramid_alloc() (size:%d)", sizeof(pyramid_t));
	  exit(155);
	}
      memset( level, 0, sizeof(pyramid_t) );
      
      level->rows = rows;
      level->cols = cols;
      const int size = level->rows * level->cols;
      level->Gx = matrix_alloc(size);
      level->Gy = matrix_alloc(size);
      
      level->prev = prev;
      if(prev != NULL)
	prev->next = level;
      prev = level;
      
      if(pyramid == NULL)
	pyramid = level;
      
      rows /= 2;
      cols /= 2;		
  }
	
  return pyramid;
}


// calculate gradients
static inline void calculate_gradient(const int cols, const int rows, const float* const lum, float* const Gx, float* const Gy)
{
  for(int ky=0; ky<rows; ky++){
    for(int kx=0; kx<cols; kx++){
			
      const int idx = kx + ky*cols;
			
      if(kx == (cols - 1))
        Gx[idx] = 0;
      else	
        Gx[idx] = lum[idx+1] - lum[idx];
			
      if(ky == (rows - 1))
        Gy[idx] = 0;
      else	
        Gy[idx] = lum[idx + cols] - lum[idx];
    }
  }
}

#if 0
#define PFSEOL "\x0a"
static void dump_matrix_to_file(const int width, const int height, const float* const m, const char * const file_name)
{
  FILE *fh = fopen( file_name, "wb" );
  // assert( fh != NULL );
  
  fprintf( fh, "PFS1" PFSEOL "%d %d" PFSEOL "1" PFSEOL "0" PFSEOL
    "%s" PFSEOL "0" PFSEOL "ENDH", width, height, "Y");

  for( int y = 0; y < height; y++ )
    for( int x = 0; x < width; x++ ) {
      int idx = x + y*width;
      fwrite( &(m[idx]), sizeof( float ), 1, fh );
    }
  
  fclose( fh );
}  
#endif

// calculate gradients for the pyramid
// lum_temp gets overwritten!
static void pyramid_calculate_gradient(pyramid_t* pyramid, float* lum_temp)
{
  float* temp = matrix_alloc((pyramid->rows/2)*(pyramid->cols/2));
  float* const temp_saved = temp;

  calculate_gradient(pyramid->cols, pyramid->rows, lum_temp, pyramid->Gx, pyramid->Gy);	

  pyramid = pyramid->next;

  //  int l = 1;
  while(pyramid)
    {
      matrix_downsample(pyramid->prev->cols, pyramid->prev->rows, lum_temp, temp);
		
//     fprintf( stderr, "%d x %d -> %d x %d\n", pyramid->cols, pyramid->rows,
//       prev_pp->cols, prev_pp->rows );
    
//      char name[40];
//      sprintf( name, "ds_%d.pfs", l++ );
//      dump_matrix_to_file( pyramid->cols, pyramid->rows, temp, name );    
		
      calculate_gradient(pyramid->cols, pyramid->rows, temp, pyramid->Gx, pyramid->Gy);
		
      float* const dummy = lum_temp;
      lum_temp = temp;	
      temp = dummy;
			
      pyramid = pyramid->next;
  }

  matrix_free(temp_saved);
}


// x = -0.25 * b
static inline void solveX(const int n, const float* const b, float* const x)
{
  matrix_copy(n, b, x); // x = b
  matrix_multiply_const(n, x, -0.25f);
}

// divG_sum = A * x = sum(divG(x))
// memory for the temporary pyramid px should be allocated
static inline void multiplyA(pyramid_t* px, pyramid_t* pC, const float* const x, float* const divG_sum)
{
  matrix_copy(pC->rows*pC->cols, x, divG_sum); // use divG_sum as a temp variable
  pyramid_calculate_gradient(px, divG_sum);
  pyramid_scale_gradient(px, pC); // scale gradients by Cx,Cy from main pyramid
  pyramid_calculate_divergence_sum(px, divG_sum); // calculate the sum of divergences
} 


// bi-conjugate linear equation solver
// overwrites pyramid!
static void linbcg(pyramid_t* pyramid, pyramid_t* pC, float* const b, float* const x, const int itmax, const float tol, progress_callback progress_cb)
{
  const int rows = pyramid->rows;
  const int cols = pyramid->cols;
  const int n = rows*cols;
  const float tol2 = tol*tol;
	
  float* const z = matrix_alloc(n);
  float* const zz = matrix_alloc(n);
  float* const p = matrix_alloc(n);
  float* const pp = matrix_alloc(n);
  float* const r = b; // save memory by overwriting b
  float* const rr = matrix_alloc(n);	
	
  matrix_zero(n, x); // x = 0
	
  // multiplyA(pyramid, pC, x, r); // r = A*x = divergence(x)
  // matrix_zero(n, r);

  // matrix_substract(n, b, r); // r = b - r
  // matrix_copy(n, b, r);  // Not needed, as r == b anyway.

  // matrix_copy(n, r, rr); // rr = r
  multiplyA(pyramid, pC, r, rr); // rr = A*r
  
  const float bnrm2 = matrix_DotProduct(n, b, b);
	
  float bkden = 0;
  float err2 = bnrm2;

  int iter = 0;
  int num_backwards = 0;
  const int num_backwards_ceiling = (itmax / 10 < 3 ? 3 : itmax / 10);
  for (; iter < itmax; iter++)
    {
      if( progress_cb != NULL )
	progress_cb( (int) (logf(bnrm2/err2)*100.0f/(-logf(tol2))));    
      
      solveX(n, r, z);   //  z = ~A(-1) *  r = -0.25 *  r
      solveX(n, rr, zz); // zz = ~A(-1) * rr = -0.25 * rr
		
      const float bknum = matrix_DotProduct(n, z, rr);
		
      if(iter == 0)
	{
	  matrix_copy(n, z, p);
	  matrix_copy(n, zz, pp); 
	}
      else
	{
	  const float bk = bknum / bkden; // beta = ...
	  for (int i = 0; i < n; i++)
	    {
	      p[i] = z[i] + bk * p[i];
	      pp[i] = zz[i] + bk * pp[i];
	    }
	}
		
      bkden = bknum; // numerato becomes the dominator for the next iteration
      
      multiplyA(pyramid, pC, p, z); // z = A*p = divergence(p)
      multiplyA(pyramid, pC, pp, zz); // zz = A*pp = divergence(pp)
      
      const float ak = bknum / matrix_DotProduct(n, z, pp); // alfa = ...
      for(int i = 0 ; i < n ; i++ )
	{
	  x[i] = x[i] + ak * p[i];	// x = x + alfa * p
	  r[i] = r[i] - ak * z[i];		// r = r - alfa * z
	  rr[i] = rr[i] - ak * zz[i];	//rr = rr - alfa * zz
	}
      
      const float old_err2 = err2;
      err2 = matrix_DotProduct(n, r, r);

      // Have we gone unstable?
      if (err2 > old_err2)
	num_backwards++;
      else
	num_backwards = 0;

      // fprintf(stderr, "iter:%d err:%f\n", iter+1, sqrtf(err2/bnrm2));
      if(err2/bnrm2 < tol2 || num_backwards > num_backwards_ceiling)
	break;
    }
  if (err2/bnrm2 > tol2)
    {
      // Not converged
      if (iter == itmax)
	fprintf(stderr, "\npfstmo_mantiuk06: Warning: Not converged (hit maximum iterations), error = %g (should be below %g).\n", sqrtf(err2/bnrm2), tol);  
      else
	fprintf(stderr, "\npfstmo_mantiuk06: Warning: Not converged (going unstable), error = %g (should be below %g).\n", sqrtf(err2/bnrm2), tol);  
    }
  else if (progress_cb != NULL)
    progress_cb(100);
    
  
  matrix_free(p);
  matrix_free(pp);
  matrix_free(z);
  matrix_free(zz);
  // matrix_free(r);  // Not needed, as r == b
  matrix_free(rr);
}


// conjugate linear equation solver
// overwrites pyramid!
// overwrites b!
static void lincg(pyramid_t* pyramid, pyramid_t* pC, float* const r /* == b */, float* const x, const int itmax, const float tol, progress_callback progress_cb)
{
  const int rows = pyramid->rows;
  const int cols = pyramid->cols;
  const int n = rows*cols;
  const float tol2 = tol*tol;
	
  // float* r = matrix_alloc(n); // Unnecessary, as r == b
  float* p = matrix_alloc(n);
  float* Ap = matrix_alloc(n);	
	
  // x = 0
  matrix_zero(n, x);
	
  // r = b - Ax
  // multiplyA(pyramid, pC, x, r);
  // matrix_subtract(n, b, r);
  // matrix_copy(n, b, r); // Unnecessary, as r == b

  // bnrm2 = ||b||
  const float bnrm2 = matrix_DotProduct(n, r, r);
	
  // p = r
  matrix_copy(n, r, p);

  // rdotr = r.r
  float rdotr = bnrm2;

  int iter = 0;
  int num_backwards = 0;
  const int num_backwards_ceiling = (itmax / 10 < 3 ? 3 : itmax / 10);
  for (; iter < itmax; iter++)
    {
      if( progress_cb != NULL )
	progress_cb( (int) (logf(bnrm2/rdotr)*100.0f/(-logf(tol2))));    
      
      // Ap = A p
      multiplyA(pyramid, pC, p, Ap);
      
      // alpha = r.r / (p . Ap)
      const float alpha = rdotr / matrix_DotProduct(n, p, Ap);
      
      // x = x + alpha p
      // r = r - alpha Ap
      for (int i = 0; i < n; i++)
	{
	  x[i] += alpha *  p[i];
	  r[i] -= alpha * Ap[i];
	}
            
      // rdotr = r.r
      const float old_rdotr = rdotr;
      rdotr = matrix_DotProduct(n, r, r);
      
      // Have we gone unstable?
      if (rdotr > old_rdotr)
	num_backwards++;
      else
	num_backwards = 0;

      // Exit if we're done
      // fprintf(stderr, "iter:%d err:%f\n", iter+1, sqrtf(rdotr/bnrm2));
      if(rdotr/bnrm2 < tol2 || num_backwards > num_backwards_ceiling)
	break;
      
      // p = r + beta p
      const float beta = rdotr/old_rdotr;
      for (int i = 0; i < n; i++)
	p[i] = r[i] + beta*p[i];
    }
  if (rdotr/bnrm2 > tol2)
    {
      // Not converged
      if (iter == itmax)
	fprintf(stderr, "\npfstmo_mantiuk06: Warning: Not converged (hit maximum iterations), error = %g (should be below %g).\n", sqrtf(rdotr/bnrm2), tol);  
      else
	fprintf(stderr, "\npfstmo_mantiuk06: Warning: Not converged (going unstable), error = %g (should be below %g).\n", sqrtf(rdotr/bnrm2), tol);  
    }
  else if (progress_cb != NULL)
    progress_cb(100);
    
  matrix_free(p);
  matrix_free(Ap);
  // matrix_free(r); // Unnecessary, as r == b
}


// in_tab and out_tab should contain inccreasing float values
static inline float lookup_table(const int n, const float* const in_tab, const float* const out_tab, const float val)
{
  if(val < in_tab[0])
    return out_tab[0];

  for (int j = 1; j < n; j++)
    if(val < in_tab[j])
      {
	const float dd = (val - in_tab[j-1]) / (in_tab[j] - in_tab[j-1]);
	return out_tab[j-1] + (out_tab[j] - out_tab[j-1]) * dd;
      }

  return out_tab[n-1];
}


// transform gradient (Gx,Gy) to R
static inline void transform_to_R(const int n, float* const G)
{
  int sign;
  float absG;
  for(int j=0;j<n;j++)
    {
      // G to W
      absG = fabsf(G[j]);
      if(G[j] < 0)
	sign = -1;
      else
	sign = 1;	
      G[j] = (exp10f(absG) - 1.0f) * sign;
		
      // W to RESP
      if(G[j] < 0)
	sign = -1;
      else
	sign = 1;	
      
      G[j] = sign * lookup_table(LOOKUP_W_TO_R, W_table, R_table, fabsf(G[j]));
    }
}

// transform gradient (Gx,Gy) to R for the whole pyramid
static inline void pyramid_transform_to_R(pyramid_t* pyramid)
{
  while (pyramid != NULL)
    {
      const int size = pyramid->rows * pyramid->cols;
      transform_to_R(size, pyramid->Gx);	
      transform_to_R(size, pyramid->Gy);	
      pyramid = pyramid->next;
    }
}

// transform from R to G
static inline void transform_to_G(const int n, float* const R){

  int sign;
  for(int j=0;j<n;j++){
	
    // RESP to W
    if(R[j] < 0)
      sign = -1;
    else
      sign = 1;			
    R[j] = sign * lookup_table(LOOKUP_W_TO_R, R_table, W_table, fabsf(R[j]));
	
    // W to G
    if(R[j] < 0)
      sign = -1;
    else
      sign = 1;	
    R[j] = log10f(fabsf(R[j]) + 1.0f) * sign;
		
  }
}

// transform from R to G for the pyramid
static inline void pyramid_transform_to_G(pyramid_t* pyramid)
{
  while (pyramid != NULL)
    {
      transform_to_G(pyramid->rows*pyramid->cols, pyramid->Gx);	
      transform_to_G(pyramid->rows*pyramid->cols, pyramid->Gy);	
      pyramid = pyramid->next;
    }
}

// multiply gradient (Gx,Gy) values by float scalar value for the whole pyramid
static inline void pyramid_gradient_multiply(pyramid_t* pyramid, const float val)
{
  while (pyramid != NULL)
    {
      matrix_multiply_const(pyramid->rows*pyramid->cols, pyramid->Gx, val);	
      matrix_multiply_const(pyramid->rows*pyramid->cols, pyramid->Gy, val);	
      pyramid = pyramid->next;
    }
}


static int sort_float(const void* const v1, const void* const v2)
{
  if (*((float*)v1) < *((float*)v2))
    return -1;

  if(*((float*)v1) > *((float*)v2))
    return 1;

  return 0;
}


// transform gradients to luminance
static void transform_to_luminance(pyramid_t* pp, float* const x, progress_callback progress_cb, const bool bcg, const int itmax, const float tol)
{
  pyramid_t* pC = pyramid_allocate(pp->cols, pp->rows);
  pyramid_calculate_scale_factor(pp, pC); // calculate (Cx,Cy)
  pyramid_scale_gradient(pp, pC); // scale small gradients by (Cx,Cy);

  float* b = matrix_alloc(pp->cols * pp->rows);
  pyramid_calculate_divergence_sum(pp, b); // calculate the sum of divergences (equal to b)
  
  // calculate luminances from gradients
  if (bcg)
    linbcg(pp, pC, b, x, itmax, tol, progress_cb);
  else
    lincg(pp, pC, b, x, itmax, tol, progress_cb);
  
  matrix_free(b);
  pyramid_free(pC);
}


struct hist_data
{
  float size;
  float cdf;
  int index;
};

static int hist_data_order(const void* const v1, const void* const v2)
{
  if (((struct hist_data*) v1)->size < ((struct hist_data*) v2)->size)
    return -1;
  
  if (((struct hist_data*) v1)->size > ((struct hist_data*) v2)->size)
    return 1;

  return 0;
}


static int hist_data_index(const void* const v1, const void* const v2)
{
  return ((struct hist_data*) v1)->index - ((struct hist_data*) v2)->index;
}


static void contrast_equalization( pyramid_t *pp, const float contrastFactor )
{
  // Count sizes
  int total_pixels = 0;
  pyramid_t* l = pp;
  while (l != NULL)
    {
      total_pixels += l->rows * l->cols;
      l = l->next;
    }

  // Allocate memory
  struct hist_data* hist = (struct hist_data*) malloc(sizeof(struct hist_data) * total_pixels);
  if (hist == NULL)
    {
      fprintf(stderr, "ERROR: malloc in contrast_equalization() (size:%d)", sizeof(struct hist_data) * total_pixels);
      exit(155);
    }
    
  // Build histogram info
  l = pp;
  int index = 0;
  while( l != NULL )
    {
      const int pixels = l->rows*l->cols;
      for(int c = 0; c < pixels; c++ , index++)
	{
	  hist[index].size = sqrtf( l->Gx[c]*l->Gx[c] + l->Gy[c]*l->Gy[c] );
	  hist[index].index = index;
	} 
      l = l->next;
    }
  
  // Generate histogram
  qsort(hist, total_pixels, sizeof(struct hist_data), hist_data_order);

  // Calculate cdf
  const float norm = 1.0f / (float) total_pixels;
  for (int i = 0; i < total_pixels; i++)
    hist[i].cdf = ((float) i) * norm;

  // Recalculate in terms of indexes
  qsort(hist, total_pixels, sizeof(struct hist_data), hist_data_index);

  //Remap gradient magnitudes
  l = pp;
  index = 0;
  while( l != NULL ) {
    const int pixels = l->rows*l->cols;
    
    for( int c = 0; c < pixels; c++ , index++ )
      {
	const float scale = contrastFactor * hist[index].cdf/hist[index].size;
	l->Gx[c] *= scale;
	l->Gy[c] *= scale;        
      } 
    l = l->next;
  }

  free(hist);
}


// tone mapping
void tmo_mantiuk06_contmap(const int c, const int r, float* const R, float* const G, float* const B, float* const Y, const float contrastFactor, const float saturationFactor, const bool bcg, const int itmax, const float tol, progress_callback progress_cb)
{
  
  const int n = c*r;
  
  const float clip_min = 1e-5;
  
  for (int j = 0; j < n; j++)
    {
    // clipping
    if( R[j] < clip_min ) R[j] = clip_min;
    if( G[j] < clip_min ) G[j] = clip_min;
    if( B[j] < clip_min ) B[j] = clip_min;
    if( Y[j] < clip_min ) Y[j] = clip_min;    

    R[j] /= Y[j];
    G[j] /= Y[j];
    B[j] /= Y[j];
    }
	
  for(int j=0;j<n;j++)
    Y[j] = log10f(Y[j]);
	
  pyramid_t* pp = pyramid_allocate(c,r); // create pyramid
  pyramid_calculate_gradient(pp,Y); // calculate gradients for pyramid, destroys Y
  pyramid_transform_to_R(pp); // transform gradients to R

  /* Contrast map */
  if( contrastFactor > 0.0f )
    pyramid_gradient_multiply(pp, contrastFactor); // Contrast mapping
  else
    contrast_equalization(pp, -contrastFactor); // Contrast equalization
	
  pyramid_transform_to_G(pp); // transform R to gradients
  transform_to_luminance(pp, Y, progress_cb, bcg, itmax, tol); // transform gradients to luminance Y
  pyramid_free(pp);

  /* Renormalize luminance */
  float* temp = matrix_alloc(n);
	
  matrix_copy(n, Y, temp); // copy Y to temp
  qsort(temp, n, sizeof(float), sort_float); // sort temp in ascending order
	
  // const float median = (temp[(int)((n-1)/2)] + temp[(int)((n-1)/2+1)]) * 0.5f; // calculate median
  const float CUT_MARGIN = 0.1f;
	
  float trim = (n-1) * CUT_MARGIN * 0.01f;
  float delta = trim - floorf(trim);
  const float l_min = temp[(int)floorf(trim)] * delta + temp[(int)ceilf(trim)] * (1.0f-delta);	

  trim = (n-1) * (100.0f - CUT_MARGIN) * 0.01f;
  delta = trim - floorf(trim);
  const float l_max = temp[(int)floorf(trim)] * delta + temp[(int)ceilf(trim)] * (1.0f-delta);	
	
  matrix_free(temp);
	
  const float disp_dyn_range = 2.3f;
  for(int j=0;j<n;j++)
    Y[j] = (Y[j] - l_min) / (l_max - l_min) * disp_dyn_range - disp_dyn_range; // x scaled
                                                                               // 
  /* Transform to linear scale RGB */
  for(int j=0;j<n;j++)
    {
      Y[j] = exp10f(Y[j]);
      R[j] = powf( R[j], saturationFactor) * Y[j];
      G[j] = powf( G[j], saturationFactor) * Y[j];
      B[j] = powf( B[j], saturationFactor) * Y[j];
    }
}

