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
 * This file is a part of LuminanceHDR package, based on pfstmo.
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
 * Updated 2008/06/25 by Ed Brambley <E.J.Brambley@damtp.cam.ac.uk>
 *  bug fixes and openMP patches
 *  more on this:
 *  http://groups.google.com/group/pfstools/browse_thread/thread/de2378af98ec6185/0dee5304fc14e99d?hl=en#0dee5304fc14e99d
 *  Optimization improvements by Lebed Dmytry
 *
 * Updated 2008/07/26 by Dejan Beric <dejan.beric@live.com>
 *  Added the detail factor slider which offers more control over contrast in details
 * Update 2010/10/06 by Axel Voitier <axel.voitier@gmail.com>
 *  detail_factor patch in order to remove potential issues in a multithreading environment
 *
 * $Id: contrast_domain.cpp,v 1.14 2008/08/26 17:08:49 rafm Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <iostream>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "contrast_domain.h"
#include "Common/vex.h"
#include "Common/msec_timer.h"

#ifdef BRANCH_PREDICTION
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif

typedef struct pyramid_s {
  int rows;
  int cols;
  float* Gx;
  float* Gy;
  struct pyramid_s* next;
  struct pyramid_s* prev;
} pyramid_t;

#define PYRAMID_MIN_PIXELS      3
#define LOOKUP_W_TO_R           107
//#define DEBUG_MANTIUK06

void contrast_equalization( pyramid_t *pp, const float contrastFactor );

void transform_to_luminance(pyramid_t* pyramid, float* const x, ProgressHelper *ph, const bool bcg);
void matrix_add(const int n, const float* const a, float* const b);
void matrix_subtract(const int n, const float* const a, float* const b);
void matrix_copy(const int n, const float* const a, float* const b);
void matrix_multiply_const(const int n, float* const a, const float val);
void matrix_divide(const int n, const float* a, float* b);
float* matrix_alloc(const int size);
void matrix_free(float* m);
float matrix_DotProduct(const int n, const float* const a, const float* const b);
void matrix_zero(const int n, float* const m);
void calculate_and_add_divergence(const int rows, const int cols, const float* const Gx, const float* const Gy, float* const divG);
void pyramid_calculate_divergence(pyramid_t* pyramid);
void pyramid_calculate_divergence_sum(pyramid_t* pyramid, float* divG_sum);
void calculate_scale_factor(const int n, const float* const G, float* const C);
void pyramid_calculate_scale_factor(pyramid_t* pyramid, pyramid_t* pC);
void scale_gradient(const int n, float* G, const float* C);
void pyramid_scale_gradient(pyramid_t* pyramid, pyramid_t* pC);
void pyramid_free(pyramid_t* pyramid);
pyramid_t* pyramid_allocate(const int cols, const int rows);
void calculate_gradient(const int cols, const int rows, const float* const lum, float* const Gx, float* const Gy);
void pyramid_calculate_gradient(const pyramid_t* pyramid, const float* lum);
void solveX(const int n, const float* const b, float* const x);
void multiplyA(pyramid_t* px, pyramid_t* pyramid, const float* const x, float* const divG_sum);
void linbcg(pyramid_t* pyramid, pyramid_t* pC, const float* const b, float* const x, const int itmax, const float tol, ProgressHelper *ph);
void lincg(pyramid_t* pyramid, pyramid_t* pC, const float* const b, float* const x, const int itmax, const float tol, ProgressHelper *ph);
float lookup_table(const int n, const float* const in_tab, const float* const out_tab, const float val);
void transform_to_R(const int n, float* const G, float detail_factor);
void pyramid_transform_to_R(pyramid_t* pyramid, float detail_factor);
void transform_to_G(const int n, float* const R, float detail_factor);
void pyramid_transform_to_G(pyramid_t* pyramid, float detail_factor);
void pyramid_gradient_multiply(pyramid_t* pyramid, const float val);

void swap_pointers(float* &pOne, float* &pTwo); // utility function

#ifdef DEBUG_MANTIUK06

void dump_matrix_to_file(const int width, const int height, const float* const m, const char * const file_name);
void matrix_show(const char* const text, int rows, int cols, const float* const data);
void pyramid_show(pyramid_t* pyramid);

#endif

static float W_table[] = {0.000000,0.010000,0.021180,0.031830,0.042628,0.053819,0.065556,0.077960,0.091140,0.105203,0.120255,0.136410,0.153788,0.172518,0.192739,0.214605,0.238282,0.263952,0.291817,0.322099,0.355040,0.390911,0.430009,0.472663,0.519238,0.570138,0.625811,0.686754,0.753519,0.826720,0.907041,0.995242,1.092169,1.198767,1.316090,1.445315,1.587756,1.744884,1.918345,2.109983,2.321863,2.556306,2.815914,3.103613,3.422694,3.776862,4.170291,4.607686,5.094361,5.636316,6.240338,6.914106,7.666321,8.506849,9.446889,10.499164,11.678143,13.000302,14.484414,16.151900,18.027221,20.138345,22.517282,25.200713,28.230715,31.655611,35.530967,39.920749,44.898685,50.549857,56.972578,64.280589,72.605654,82.100619,92.943020,105.339358,119.530154,135.795960,154.464484,175.919088,200.608905,229.060934,261.894494,299.838552,343.752526,394.651294,453.735325,522.427053,602.414859,695.706358,804.693100,932.229271,1081.727632,1257.276717,1463.784297,1707.153398,1994.498731,2334.413424,2737.298517,3215.770944,3785.169959,4464.187290,5275.653272,6247.520102,7414.094945,8817.590551,10510.080619};
static float R_table[] = {0.000000,0.009434,0.018868,0.028302,0.037736,0.047170,0.056604,0.066038,0.075472,0.084906,0.094340,0.103774,0.113208,0.122642,0.132075,0.141509,0.150943,0.160377,0.169811,0.179245,0.188679,0.198113,0.207547,0.216981,0.226415,0.235849,0.245283,0.254717,0.264151,0.273585,0.283019,0.292453,0.301887,0.311321,0.320755,0.330189,0.339623,0.349057,0.358491,0.367925,0.377358,0.386792,0.396226,0.405660,0.415094,0.424528,0.433962,0.443396,0.452830,0.462264,0.471698,0.481132,0.490566,0.500000,0.509434,0.518868,0.528302,0.537736,0.547170,0.556604,0.566038,0.575472,0.584906,0.594340,0.603774,0.613208,0.622642,0.632075,0.641509,0.650943,0.660377,0.669811,0.679245,0.688679,0.698113,0.707547,0.716981,0.726415,0.735849,0.745283,0.754717,0.764151,0.773585,0.783019,0.792453,0.801887,0.811321,0.820755,0.830189,0.839623,0.849057,0.858491,0.867925,0.877358,0.886792,0.896226,0.905660,0.915094,0.924528,0.933962,0.943396,0.952830,0.962264,0.971698,0.981132,0.990566,1.000000};

inline float max( float a, float b )
{
  return a > b ? a : b;
}

inline float min( float a, float b )
{
  return a < b ? a : b;
}

inline int imin(int a, int b)
{
  return a < b ? a : b;
}

// upsample the matrix
// upsampled matrix is twice bigger in each direction than data[]
// res should be a pointer to allocated memory for bigger matrix
// cols and rows are the dimensions of the output matrix
void matrix_upsample(const int outCols, const int outRows, const float* const in, float* const out)
{
  const int inRows = outRows/2;
  const int inCols = outCols/2;
  
  // Transpose of experimental downsampling matrix (theoretically the correct thing to do)
  
  const float dx = (float)inCols / ((float)outCols);
  const float dy = (float)inRows / ((float)outRows);
  const float factor = 1.0f / (dx*dy); // This gives a genuine upsampling matrix, not the transpose of the downsampling matrix
  // const float factor = 1.0f; // Theoretically, this should be the best.
  
  #pragma omp parallel for schedule(static)
  for (int y = 0; y < outRows; y++)
  {
    const float sy = y * dy;
    const int iy1 =      (  y   * inRows) / outRows;
    const int iy2 = imin(((y+1) * inRows) / outRows, inRows-1);
    
    for (int x = 0; x < outCols; x++)
    {
      const float sx = x * dx;
      const int ix1 =      (  x   * inCols) / outCols;
      const int ix2 = imin(((x+1) * inCols) / outCols, inCols-1);
      
      out[x + y*outCols] = (((ix1+1) - sx)*((iy1+1 - sy)) * in[ix1 + iy1*inCols] +
                            ((ix1+1) - sx)*(sy+dy - (iy1+1)) * in[ix1 + iy2*inCols] +
                            (sx+dx - (ix1+1))*((iy1+1 - sy)) * in[ix2 + iy1*inCols] +
                            (sx+dx - (ix1+1))*(sy+dx - (iy1+1)) * in[ix2 + iy2*inCols])*factor;
    }
  }
}

// downsample the matrix
void matrix_downsample(const int inCols, const int inRows, const float* const data, float* const res)
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
  #pragma omp parallel for schedule(static)
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
inline void matrix_add(const int n, const float* const a, float* const b)
{
#ifdef __SSE__  
  VEX_vadd(b, a, b, n);
#else
  #pragma omp parallel for schedule(static)
  for(int i=0; i<n; i++)
  {
    b[i] += a[i];
  }
#endif
}

// return = a - b
inline void matrix_subtract(const int n, const float* const a, float* const b)
{
#ifdef __SSE__  
  VEX_vsub(a, b, b, n);
#else
  #pragma omp parallel for schedule(static)
  for(int i=0; i<n; i++)
  {
    b[i] = a[i] - b[i];
  }
#endif
}

// copy matix a to b, return = a 
inline void matrix_copy(const int n, const float* const a, float* const b)
{
#ifdef __SSE__
  VEX_vcopy(a, b, n);
#else
  memcpy(b, a, sizeof(float)*n);
#endif
}

// multiply matrix a by scalar val
inline void matrix_multiply_const(const int n, float* const a, const float val)
{
#ifdef __SSE__
  VEX_vsmul(a, val, a, n);
#else
  #pragma omp parallel for schedule(static)
  for(int i=0; i<n; i++)
  {
    a[i] *= val;
  }
#endif
}

// b = a[i] / b[i]
inline void matrix_divide(const int n, float* a, float* b)
{
#ifdef __SSE__  
  VEX_vdiv(a, b, b, n);
#else  
  #pragma omp parallel for schedule(static)
  for(int i=0; i<n; i++)
  {
    b[i] = a[i] / b[i];
  }
#endif
}

// alloc memory for the float table
inline float* matrix_alloc(int size)
{
#ifdef __APPLE__
  float* m  = (float*)malloc      (sizeof(float)*size);
#else
  float* m    = (float*)_mm_malloc  (sizeof(float)*size, 16);
#endif
  if (m == NULL)
  {
    fprintf(stderr, "ERROR: malloc in matrix_alloc() (size:%d)", size);
    exit(155);
  }
  return m;
}

// free memory for matrix
inline void matrix_free(float* m)
{
  if (m != NULL)
  {
#ifdef __APPLE__
    free(m);
#else
    _mm_free(m);
#endif
    m = NULL;
  }
  else
  {
    fprintf(stderr, "ERROR: This pointer has already been freed");
  }
}

// multiply vector by vector (each vector should have one dimension equal to 1)
float matrix_DotProduct(const int n, const float* const a, const float* const b)
{
  float val = 0;
#ifdef __SSE__
  VEX_dotpr(a, b, val, n);
#else  
  #pragma omp parallel for reduction(+:val) schedule(static)
  for(int j=0; j<n; j++)
  {
    val += a[j] * b[j];
  }
#endif
  return val;
}

// set zeros for matrix elements
inline void matrix_zero(int n, float* m)
{
  //bzero(m, n*sizeof(float));
  memset(m, 0, n*sizeof(float));
}

// Davide Anastasia <davideanastasia@users.sourceforge.net> (2010 08 31)
// calculate divergence of two gradient maps (Gx and Gy)
// divG(x,y) = Gx(x,y) - Gx(x-1,y) + Gy(x,y) - Gy(x,y-1)  
inline void calculate_and_add_divergence(const int COLS, const int ROWS, const float* const Gx, const float* const Gy, float* const divG)
{
  float divGx, divGy;
  
  // kx = 0 AND ky = 0;
  divG[0] += Gx[0] + Gy[0];                       // OUT
  
  // ky = 0
  for(int kx=1; kx<COLS; kx++)
  {
    divGx = Gx[kx] - Gx[kx - 1];
    divGy = Gy[kx];
    divG[kx] += divGx + divGy;                    // OUT
  }
  
#pragma omp parallel for schedule(static, 5120) private(divGx, divGy)
  for(int ky=1; ky<ROWS; ky++)
  {
    // kx = 0
    divGx = Gx[ky*COLS];
    divGy = Gy[ky*COLS] - Gy[ky*COLS - COLS];			
    divG[ky*COLS] += divGx + divGy;               // OUT
    
    // kx > 0
    for(int kx=1; kx<COLS; kx++)
    {
      divGx = Gx[kx + ky*COLS] - Gx[kx + ky*COLS-1];
      divGy = Gy[kx + ky*COLS] - Gy[kx + ky*COLS - COLS];			
      divG[kx + ky*COLS] += divGx + divGy;        // OUT
    }
  }
}

// Calculate the sum of divergences for the all pyramid level
// The smaller divergence map is upsampled and added to the divergence map for the higher level of pyramid
void pyramid_calculate_divergence_sum(pyramid_t* pyramid, float* divG_sum)
{
  float* temp = matrix_alloc((pyramid->rows*pyramid->cols)/4);
  
  // Find the coarsest pyramid, and the number of pyramid levels
  bool swap = true;
  while (pyramid->next != NULL)
  {
    swap = (!swap);
    pyramid = pyramid->next;
  }
  
  // For every level, we swap temp and divG_sum
  // So, if there are an odd number of levels...
  if (swap) swap_pointers(divG_sum, temp);
	
  if (pyramid)
  {
    matrix_zero(pyramid->rows * pyramid->cols, temp);
    calculate_and_add_divergence(pyramid->cols, pyramid->rows, pyramid->Gx, pyramid->Gy, temp);
    
    swap_pointers(divG_sum, temp);
    pyramid = pyramid->prev;
  }
  
  while (pyramid)
  {
    matrix_upsample(pyramid->cols, pyramid->rows, divG_sum, temp);
    calculate_and_add_divergence(pyramid->cols, pyramid->rows, pyramid->Gx, pyramid->Gy, temp);
    
    swap_pointers(divG_sum, temp);
    pyramid = pyramid->prev;
  }
  
  matrix_free(temp);
}

// calculate scale factors (Cx,Cy) for gradients (Gx,Gy)
// C is equal to EDGE_WEIGHT for gradients smaller than GFIXATE or 1.0 otherwise
inline void calculate_scale_factor(const int n, const float* const G, float* const C)
{
  //  float GFIXATE = 0.1f;
  //  float EDGE_WEIGHT = 0.01f;
  const float detectT = 0.001f;
  const float a = 0.038737;
  const float b = 0.537756;
	
  #pragma omp parallel for schedule(static)
  for(int i=0; i<n; i++)
  {
    //#if 1
    const float g = max( detectT, fabsf(G[i]) );    
    C[i] = 1.0f / (a*powf(g,b));
    //#else
    //    if(fabsf(G[i]) < GFIXATE)
    //      C[i] = 1.0f / EDGE_WEIGHT;
    //    else	
    //      C[i] = 1.0f;
    //#endif
  }
}

// calculate scale factor for the whole pyramid
void pyramid_calculate_scale_factor(pyramid_t* pyramid, pyramid_t* pC)
{
  while (pyramid != NULL)
  {
    calculate_scale_factor(pyramid->rows * pyramid->cols, pyramid->Gx, pC->Gx);
    calculate_scale_factor(pyramid->rows * pyramid->cols, pyramid->Gy, pC->Gy);
    
    pyramid = pyramid->next;
    pC = pC->next;
  }
}

// Scale gradient (Gx and Gy) by C (Cx and Cy)
// G = G * C
inline void scale_gradient(const int n, float* G, const float* C)
{
#ifdef __SSE__  
  VEX_vmul(G, C, G, n);
#else
#pragma omp parallel for schedule(static)
  for(int i=0; i<n; i++)
  {
    G[i] *= C[i];
  }
#endif
}

// scale gradients for the whole one pyramid with the use of (Cx,Cy) from the other pyramid
void pyramid_scale_gradient(pyramid_t* pyramid, pyramid_t* pC)
{
  while (pyramid != NULL)
  {
    scale_gradient(pyramid->rows * pyramid->cols, pyramid->Gx, pC->Gx);	
    scale_gradient(pyramid->rows * pyramid->cols, pyramid->Gy, pC->Gy);
    
    pyramid = pyramid->next;
    pC = pC->next;
  }
}


// free memory allocated for the pyramid
void pyramid_free(pyramid_t* pyramid)
{
  pyramid_t* t_next; // = pyramid->next;
  
  while (pyramid)
  {
    t_next = pyramid->next;
    
    if (pyramid->Gx != NULL)
    {
      matrix_free(pyramid->Gx);   //free(pyramid->Gx);
      pyramid->Gx = NULL;
    }
    if (pyramid->Gy != NULL)
    {
      matrix_free(pyramid->Gy);   //free(pyramid->Gy);
      pyramid->Gy = NULL;
    }
    
    //pyramid->prev = NULL;
    //pyramid->next = NULL;
    
    free(pyramid);
    pyramid = t_next;
  }			
}


// allocate memory for the pyramid
pyramid_t * pyramid_allocate(int cols, int rows)
{
  pyramid_t* level = NULL;
  pyramid_t* pyramid = NULL;
  pyramid_t* prev = NULL;
  
  while (rows >= PYRAMID_MIN_PIXELS && cols >= PYRAMID_MIN_PIXELS)
  {
    level = (pyramid_t *) malloc(sizeof(pyramid_t));
    if (level == NULL)
    {
      fprintf(stderr, "ERROR: malloc in pyramid_alloc() (size:%zu)", sizeof(pyramid_t) );
      exit(155);
    }
    memset( level, 0, sizeof(pyramid_t) );
    
    level->rows = rows;
    level->cols = cols;
    const int size = level->rows * level->cols;
    level->Gx = matrix_alloc(size);
    level->Gy = matrix_alloc(size);
    
    level->prev = prev;
    if (prev != NULL)
      prev->next = level;
    prev = level;
    
    if (pyramid == NULL)
      pyramid = level;
    
    rows /= 2;
    cols /= 2;		
  }
	
  return pyramid;
}


// calculate gradients
//TODO: check this implementation in Linux, where the OMP is enabled!
inline void calculate_gradient(const int COLS, const int ROWS, const float* const lum, float* const Gx, float* const Gy)
{ 
  int Y_IDX, IDX;
  
  #pragma omp parallel for schedule(static, 64) private(Y_IDX, IDX)
  for (int ky = 0; ky < ROWS-1; ky++)
  {
    Y_IDX = ky*COLS;
    for (int kx = 0; kx < COLS-1; kx++)
    {
      IDX = Y_IDX + kx;
  
			Gx[IDX] = lum[IDX + 1]    - lum[IDX];
      Gy[IDX] = lum[IDX + COLS] - lum[IDX];
    }
    
    Gx[Y_IDX + COLS - 1] = 0.0f; // last columns (kx = COLS - 1)
    Gy[Y_IDX + COLS - 1] = lum[Y_IDX + COLS - 1 + COLS] - lum[Y_IDX + COLS - 1];
  }
  
  // last row (ky = ROWS-1)
  for (int kx = 0; kx < (COLS-1); kx++)
  {
    IDX = (ROWS - 1)*COLS + kx;
    
    Gx[IDX] = lum[IDX + 1] - lum[IDX];
    Gy[IDX] = 0.0f;
  }
  
  // last row & last col = last element
  Gx[ROWS*COLS - 1] = 0.0f;
  Gy[ROWS*COLS - 1] = 0.0f;
}

void swap_pointers(float* &pOne, float* &pTwo)
{
  float* pTemp = pOne;
  pOne = pTwo;
  pTwo = pTemp;
}

// calculate gradients for the pyramid
// lum_temp WILL NOT BE overwritten!
void pyramid_calculate_gradient(const pyramid_t* pyramid, const float* Y /*lum_temp*/)
{  
  float* buffer1 = matrix_alloc((pyramid->rows*pyramid->cols)/4); // /4
  float* buffer2 = matrix_alloc((pyramid->rows*pyramid->cols)/16); // /16
  
  float* p_t1 = buffer1;
  float* p_t2 = buffer2;
  
  calculate_gradient(pyramid->cols, pyramid->rows, Y, pyramid->Gx, pyramid->Gy);
  
  pyramid_t* py_curr = pyramid->next;
  pyramid_t* py_prev = py_curr->prev;
  
  if (py_curr)
  {
    matrix_downsample(py_prev->cols, py_prev->rows, Y, p_t1);
    calculate_gradient(py_curr->cols, py_curr->rows, p_t1, py_curr->Gx, py_curr->Gy);
    
    py_prev = py_curr;
    py_curr = py_curr->next;
  }
  
  while (py_curr)
  {
    matrix_downsample(py_prev->cols, py_prev->rows, p_t1, p_t2);
	  calculate_gradient(py_curr->cols, py_curr->rows, p_t2, py_curr->Gx, py_curr->Gy);
		
    // swap pointers
    swap_pointers(p_t1, p_t2);
    
    py_prev = py_curr;
    py_curr = py_curr->next;
  }
  
  matrix_free(buffer1);
  matrix_free(buffer2);
}

// x = -0.25 * b
inline void solveX(const int n, const float* const b, float* const x)
{
#ifdef __SSE__
  VEX_vsmul(b, (-0.25f), x, n);
#else
  #pragma omp parallel for schedule(static)
  for (int i=0; i<n; i++)
  {
    x[i] = (-0.25f) * b[i];
  }
#endif
}

// divG_sum = A * x = sum(divG(x))
inline void multiplyA(pyramid_t* px, pyramid_t* pC, const float* const x, float* const divG_sum)
{
  pyramid_calculate_gradient(px, x);                // x won't be changed
  pyramid_scale_gradient(px, pC);                   // scale gradients by Cx,Cy from main pyramid
  pyramid_calculate_divergence_sum(px, divG_sum);   // calculate the sum of divergences
} 


// bi-conjugate linear equation solver
// overwrites pyramid!
void linbcg(pyramid_t* pyramid, pyramid_t* pC, float* const b, float* const x, const int itmax, const float tol, ProgressHelper *ph)
{
  const int rows = pyramid->rows;
  const int cols = pyramid->cols;
  const int n = rows*cols;
  const float tol2 = tol*tol;
	
  float* const z = matrix_alloc(n);
  float* const zz = matrix_alloc(n);
  float* const p = matrix_alloc(n);
  float* const pp = matrix_alloc(n);
  float* const r = matrix_alloc(n);
  float* const rr = matrix_alloc(n);	
  float* const x_save = matrix_alloc(n);	
	
  const float bnrm2 = matrix_DotProduct(n, b, b);
	
  multiplyA(pyramid, pC, x, r);               // r = A*x = divergence(x)
  matrix_subtract(n, b, r);                   // r = b - r
  float err2 = matrix_DotProduct(n, r, r);    // err2 = r.r
  
  // matrix_copy(n, r, rr);                   // rr = r
  multiplyA(pyramid, pC, r, rr);              // rr = A*r
  
  float bkden = 0;
  float saved_err2 = err2;
  matrix_copy(n, x, x_save);
  
  const float ierr2 = err2;
  const float percent_sf = 100.0f/logf(tol2*bnrm2/ierr2);
  
  int iter = 0;
  bool reset = true;
  int num_backwards = 0;
  const int num_backwards_ceiling = 3;
  
  for (; iter < itmax; iter++)
  {
	  ph->newValue( (int) (logf(err2/ierr2)*percent_sf) );
    if (ph->isTerminationRequested()) //user request abort
		  break;
    
    solveX(n, r, z);   //  z = ~A(-1) *  r = -0.25 *  r
    solveX(n, rr, zz); // zz = ~A(-1) * rr = -0.25 * rr
		
    const float bknum = matrix_DotProduct(n, z, rr);
		
    if (reset)
    {
      reset = false;
      matrix_copy(n, z, p);
      matrix_copy(n, zz, pp); 
    }
    else
    {
      const float bk = bknum / bkden; // beta = ...
#ifdef __SSE__
      VEX_vadds(z, bk, p, p, n);
      VEX_vadds(zz, bk, pp, pp, n);
#else
      #pragma omp parallel for schedule(static)
      for (int i = 0; i < n; i++)
	    {
	      p[i]  =  z[i] + bk *  p[i];
	      pp[i] = zz[i] + bk * pp[i];
	    }
#endif
    }
		
    bkden = bknum; // numerato becomes the dominator for the next iteration
    
    multiplyA(pyramid, pC,  p,  z); //  z = A* p = divergence( p)
    multiplyA(pyramid, pC, pp, zz); // zz = A*pp = divergence(pp)
    
    const float ak = bknum / matrix_DotProduct(n, z, pp); // alfa = ...
    
#ifdef __SSE__
    VEX_vsubs(r, ak, z, r, n);
    VEX_vsubs(rr, ak, zz, rr, n);
#else
    #pragma omp parallel for schedule(static)
    for(int i = 0 ; i < n ; i++ )
    {
      r[i]  -= ak *  z[i];	// r =  r - alfa * z
      rr[i] -= ak * zz[i];	//rr = rr - alfa * zz
    }
#endif
    
    const float old_err2 = err2;
    err2 = matrix_DotProduct(n, r, r);
    
    // Have we gone unstable?
    if (err2 > old_err2)
    {
      // Save where we've got to if it's the best yet
      if (num_backwards == 0 && old_err2 < saved_err2)
	    {
	      saved_err2 = old_err2;
	      matrix_copy(n, x, x_save);
	    }
      
      num_backwards++;
    }
    else
    {
      num_backwards = 0;
    }
    
#ifdef __SSE__
    VEX_vadds(x, ak, p, x, n);
#else
    #pragma omp parallel for schedule(static)
    for(int i = 0 ; i < n ; i++ )
    {
      x[i] += ak * p[i];	// x =  x + alfa * p
    }
#endif
    
    if (num_backwards > num_backwards_ceiling)
    {
      // Reset
      reset = true;
      num_backwards = 0;
      
      // Recover saved value
      matrix_copy(n, x_save, x);
      
      // r = Ax
      multiplyA(pyramid, pC, x, r);
      
      // r = b - r
      matrix_subtract(n, b, r);
      
      // err2 = r.r
      err2 = matrix_DotProduct(n, r, r);
      saved_err2 = err2;
      
      // rr = A*r
      multiplyA(pyramid, pC, r, rr);
    }
    
    // fprintf(stderr, "iter:%d err:%f\n", iter+1, sqrtf(err2/bnrm2));
    if(err2/bnrm2 < tol2)
      break;
  }
  
  // Use the best version we found
  if (err2 > saved_err2)
  {
    err2 = saved_err2;
    matrix_copy(n, x_save, x);
  }
  
  if (err2/bnrm2 > tol2)
  {
    // Not converged
    ph->newValue( (int) (logf(err2/ierr2)*percent_sf));    
    if (iter == itmax)
      fprintf(stderr, "\npfstmo_mantiuk06: Warning: Not converged (hit maximum iterations), error = %g (should be below %g).\n", sqrtf(err2/bnrm2), tol);  
    else
      fprintf(stderr, "\npfstmo_mantiuk06: Warning: Not converged (going unstable), error = %g (should be below %g).\n", sqrtf(err2/bnrm2), tol);  
  }
  else 
    ph->newValue(100);
  
  
  matrix_free(x_save);
  matrix_free(p);
  matrix_free(pp);
  matrix_free(z);
  matrix_free(zz);
  matrix_free(r);
  matrix_free(rr);
}


// conjugate linear equation solver
// overwrites pyramid!
// This version is a slightly modified version by Davide Anastasia <davideanastasia@users.sourceforge.net>
// March 25, 2011
void lincg(pyramid_t* pyramid, pyramid_t* pC, const float* const b, float* const x, const int itmax, const float tol, ProgressHelper *ph)
{
  const int num_backwards_ceiling = 3;
  
  float rdotr_curr, rdotr_prev, rdotr_best;
  float alpha, beta;
  
#ifdef TIMER_PROFILING
  msec_timer f_timer;
  f_timer.start();
#endif
  
  const int rows = pyramid->rows;
  const int cols = pyramid->cols;
  const int n = rows*cols;
  const float tol2 = tol*tol;
	
  float* const x_best = matrix_alloc(n);
  float* const r = matrix_alloc(n);
  float* const p = matrix_alloc(n);
  float* const Ap = matrix_alloc(n);	
	
  // bnrm2 = ||b||
  const float bnrm2 = matrix_DotProduct(n, b, b);
  
  // r = b - Ax
  multiplyA(pyramid, pC, x, r);     // r = A x
  matrix_subtract(n, b, r);         // r = b - r
  
  // rdotr = r.r
  rdotr_best = rdotr_curr = matrix_DotProduct(n, r, r);

  // Setup initial vector
  matrix_copy(n, r, p);             // p = r
  matrix_copy(n, x, x_best);
  
  const float irdotr = rdotr_curr;
  const float percent_sf = 100.0f/logf(tol2*bnrm2/irdotr);
  
  int iter = 0;
  int num_backwards = 0;
  for (; iter < itmax; iter++)
  {
    // TEST
    ph->newValue( (int) (logf(rdotr_curr/irdotr)*percent_sf) );
    // User requested abort
    if (ph->isTerminationRequested() && iter > 0 ) 
    {
      break;
    }
    
    // Ap = A p
    multiplyA(pyramid, pC, p, Ap);
    
    // alpha = r.r / (p . Ap)
    alpha = rdotr_curr / matrix_DotProduct(n, p, Ap);
    
    // r = r - alpha Ap
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < n; i++)
    {
      r[i] -= alpha * Ap[i];
    }
    
    // rdotr = r.r
    rdotr_prev = rdotr_curr;
    rdotr_curr = matrix_DotProduct(n, r, r);
    
    // Have we gone unstable?
    if (rdotr_curr > rdotr_prev)
    {
      // Save where we've got to
      if (num_backwards == 0 && rdotr_prev < rdotr_best)
	    {
	      rdotr_best = rdotr_prev;
	      matrix_copy(n, x, x_best);
	    }
      
      num_backwards++;
    }
    else
    {
      num_backwards = 0;
    }
    
    // x = x + alpha * p
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < n; i++)
    {
      x[i] += alpha * p[i];
    }
    
    // Exit if we're done
    // fprintf(stderr, "iter:%d err:%f\n", iter+1, sqrtf(rdotr/bnrm2));
    if (rdotr_curr/bnrm2 < tol2)
      break;
    
    if (num_backwards > num_backwards_ceiling)
    {
      // Reset
      num_backwards = 0;
      matrix_copy(n, x_best, x);
      
      // r = Ax
      multiplyA(pyramid, pC, x, r);
      
      // r = b - r
      matrix_subtract(n, b, r);
      
      // rdotr = r.r
      rdotr_best = rdotr_curr = matrix_DotProduct(n, r, r);
      
      // p = r
      matrix_copy(n, r, p);
    }
    else
    {
      // p = r + beta p
      beta = rdotr_curr/rdotr_prev;
      
      #pragma omp parallel for schedule(static)
      for (int i = 0; i < n; i++)
      {
        p[i] = r[i] + beta*p[i];
      }
    }
  }
  
  // Use the best version we found
  if (rdotr_curr > rdotr_best)
  {
    rdotr_curr = rdotr_best;
    matrix_copy(n, x_best, x);
  }  
  
  if (rdotr_curr/bnrm2 > tol2)
  {
    // Not converged
    ph->newValue( (int) (logf(rdotr_curr/irdotr)*percent_sf));    
    if (iter == itmax)
    {
      fprintf(stderr, "\npfstmo_mantiuk06: Warning: Not converged (hit maximum iterations), error = %g (should be below %g).\n", sqrtf(rdotr_curr/bnrm2), tol);
    }
    else
    {
      fprintf(stderr, "\npfstmo_mantiuk06: Warning: Not converged (going unstable), error = %g (should be below %g).\n", sqrtf(rdotr_curr/bnrm2), tol);
    }
  }
  else 
  {
    ph->newValue(100);
  }
  
  matrix_free(x_best);
  matrix_free(p);
  matrix_free(Ap);
  matrix_free(r);
  
#ifdef TIMER_PROFILING
  f_timer.stop_and_update();
  std::cout << "lincg() = " << f_timer.get_time() << " msec" << std::endl;
#endif
}

// conjugate linear equation solver
// overwrites pyramid!
//void lincg(pyramid_t* pyramid, pyramid_t* pC, const float* const b, float* const x, const int itmax, const float tol, ProgressHelper *ph)
//{
//  const int num_backwards_ceiling = 3;
//  
//#ifdef TIMER_PROFILING
//  //msec_timer f_timer;
//  //f_timer.start();
//#endif
//  
//  const int rows = pyramid->rows;
//  const int cols = pyramid->cols;
//  const int n = rows*cols;
//  const float tol2 = tol*tol;
//	
//  float* const x_save = matrix_alloc(n);
//  float* const r = matrix_alloc(n);
//  float* const p = matrix_alloc(n);
//  float* const Ap = matrix_alloc(n);	
//	
//  // bnrm2 = ||b||
//  const float bnrm2 = matrix_DotProduct(n, b, b);
//  
//  // r = b - Ax
//  multiplyA(pyramid, pC, x, r);
//  matrix_subtract(n, b, r);
//  
//  // rdotr = r.r
//  float rdotr = matrix_DotProduct(n, r, r);
//	
//  // p = r
//  matrix_copy(n, r, p);
//  
//  // Setup initial vector
//  float saved_rdotr = rdotr;
//  matrix_copy(n, x, x_save);
//  
//  const float irdotr = rdotr;
//  const float percent_sf = 100.0f/logf(tol2*bnrm2/irdotr);
//  int iter = 0;
//  int num_backwards = 0;
//
//  for (; iter < itmax; iter++)
//  {
//    // TEST
//    ph->newValue( (int) (logf(rdotr/irdotr)*percent_sf) );    
//    if (ph->isTerminationRequested() && iter > 0 ) // User requested abort
//      break;
//    
//    // Ap = A p
//    multiplyA(pyramid, pC, p, Ap);
//    
//    // alpha = r.r / (p . Ap)
//    const float alpha = rdotr / matrix_DotProduct(n, p, Ap);
//    
//    // r = r - alpha Ap
//#ifdef __SSE__
//    VEX_vsubs(r, alpha, Ap, r, n);
//#else
//    #pragma omp parallel for schedule(static)
//    for (int i = 0; i < n; i++)
//      r[i] -= alpha * Ap[i];
//#endif
//    
//    // rdotr = r.r
//    const float old_rdotr = rdotr;
//    rdotr = matrix_DotProduct(n, r, r);
//    
//    // Have we gone unstable?
//    if (rdotr > old_rdotr)
//    {
//      // Save where we've got to
//      if (num_backwards == 0 && old_rdotr < saved_rdotr)
//	    {
//	      saved_rdotr = old_rdotr;
//	      matrix_copy(n, x, x_save);
//	    }
//      
//      num_backwards++;
//    }
//    else
//    {
//      num_backwards = 0;
//    }
//    
//    // x = x + alpha * p
//#ifdef __SSE__
//    VEX_vadds(x, alpha, p, x, n);
//#else
//    #pragma omp parallel for schedule(static)
//    for (int i = 0; i < n; i++)
//      x[i] += alpha * p[i];
//#endif
//    
//    // Exit if we're done
//    // fprintf(stderr, "iter:%d err:%f\n", iter+1, sqrtf(rdotr/bnrm2));
//    if (rdotr/bnrm2 < tol2)
//      break;
//    
//    if (num_backwards > num_backwards_ceiling)
//    {
//      // Reset
//      num_backwards = 0;
//      matrix_copy(n, x_save, x);
//      
//      // r = Ax
//      multiplyA(pyramid, pC, x, r);
//      
//      // r = b - r
//      matrix_subtract(n, b, r);
//      
//      // rdotr = r.r
//      rdotr = matrix_DotProduct(n, r, r);
//      saved_rdotr = rdotr;
//      
//      // p = r
//      matrix_copy(n, r, p);
//    }
//    else
//    {
//      // p = r + beta p
//      const float beta = rdotr/old_rdotr;
//#ifdef __SSE__
//      VEX_vadds(r, beta, p, p, n);
//#else
//      #pragma omp parallel for schedule(static)
//      for (int i = 0; i < n; i++)
//        p[i] = r[i] + beta*p[i];
//#endif
//    }
//  }
//  
//  // Use the best version we found
//  if (rdotr > saved_rdotr)
//  {
//    rdotr = saved_rdotr;
//    matrix_copy(n, x_save, x);
//  }  
//  
//  if (rdotr/bnrm2 > tol2)
//  {
//    // Not converged
//    ph->newValue( (int) (logf(rdotr/irdotr)*percent_sf));    
//    if (iter == itmax)
//      fprintf(stderr, "\npfstmo_mantiuk06: Warning: Not converged (hit maximum iterations), error = %g (should be below %g).\n", sqrtf(rdotr/bnrm2), tol);  
//    else
//      fprintf(stderr, "\npfstmo_mantiuk06: Warning: Not converged (going unstable), error = %g (should be below %g).\n", sqrtf(rdotr/bnrm2), tol);  
//  }
//  else 
//    ph->newValue(100);
//  
//  matrix_free(x_save);
//  matrix_free(p);
//  matrix_free(Ap);
//  matrix_free(r);
//  
//#ifdef TIMER_PROFILING
//  //f_timer.stop_and_update();
//  //std::cout << "lincg() = " << f_timer.get_time() << " msec" << std::endl;
//#endif
//}


// in_tab and out_tab should contain inccreasing float values
inline float lookup_table(const int n, const float* const in_tab, const float* const out_tab, const float val)
{
  if (unlikely(val < in_tab[0]))
    return out_tab[0];
  
  for (int j = 1; j < n; j++)
  {
    if (val < in_tab[j])
    {
      const float dd = (val - in_tab[j-1]) / (in_tab[j] - in_tab[j-1]);
      return out_tab[j-1] + (out_tab[j] - out_tab[j-1]) * dd;
    }
  }
  
  return out_tab[n-1];
}


// transform gradient (Gx,Gy) to R
inline void transform_to_R(const int n, float* const G, float detail_factor)
{
  const float log10=2.3025850929940456840179914546844*detail_factor;
  
  #pragma omp parallel for schedule(static, 1024)
  for (int j=0; j<n; j++)
  {    
    // G to W
    float Curr_G = G[j];
    
    if (Curr_G < 0.0f)
    {
      Curr_G = -(powf(10, (-Curr_G) * log10) - 1.0f);
    } else {
      Curr_G = (powf(10, Curr_G * log10) - 1.0f);
    }
    // W to RESP
    if (Curr_G < 0.0f)
    {
      Curr_G = -lookup_table(LOOKUP_W_TO_R, W_table, R_table, -Curr_G);
    } else {
      Curr_G = lookup_table(LOOKUP_W_TO_R, W_table, R_table, Curr_G);
    }
    
    G[j] = Curr_G;
  }
}

// transform from R to G
inline void transform_to_G(const int n, float* const R, float detail_factor)
{
  //here we are actually changing the base of logarithm
  const float log10 = 2.3025850929940456840179914546844*detail_factor; 
  
#pragma omp parallel for schedule(static,1024)
  for(int j=0; j<n; j++)
  {
    float Curr_R = R[j];
    
    // RESP to W
    if (Curr_R < 0.0f)
    {
      Curr_R = -lookup_table(LOOKUP_W_TO_R, R_table, W_table, -Curr_R);
    } else {
      Curr_R = lookup_table(LOOKUP_W_TO_R, R_table, W_table, Curr_R);
    }
    
    // W to G
    if (Curr_R < 0.0f)
    {
      Curr_R = -log((-Curr_R) + 1.0f) / log10;
    } else {
      Curr_R = log(Curr_R + 1.0f) / log10;
    }
    
    R[j] = Curr_R;
  }
}


// transform gradient (Gx,Gy) to R for the whole pyramid
inline void pyramid_transform_to_R(pyramid_t* pyramid, float detail_factor)
{  
  while (pyramid != NULL)
  {    
    transform_to_R(pyramid->rows * pyramid->cols, pyramid->Gx, detail_factor);	
    transform_to_R(pyramid->rows * pyramid->cols, pyramid->Gy, detail_factor);
    
    pyramid = pyramid->next;
  }
}



// transform from R to G for the pyramid
inline void pyramid_transform_to_G(pyramid_t* pyramid, float detail_factor)
{
  while (pyramid != NULL)
  {
    transform_to_G(pyramid->rows*pyramid->cols, pyramid->Gx, detail_factor);	
    transform_to_G(pyramid->rows*pyramid->cols, pyramid->Gy, detail_factor);
    
    pyramid = pyramid->next;
  }
}

// multiply gradient (Gx,Gy) values by float scalar value for the whole pyramid
inline void pyramid_gradient_multiply(pyramid_t* pyramid, const float val)
{
  while (pyramid != NULL)
  {
    matrix_multiply_const(pyramid->rows*pyramid->cols, pyramid->Gx, val);	
    matrix_multiply_const(pyramid->rows*pyramid->cols, pyramid->Gy, val);	
    
    pyramid = pyramid->next;
  }
}


int sort_float(const void* const v1, const void* const v2)
{
  if (*((float*)v1) < *((float*)v2))
    return -1;
  
  if(likely(*((float*)v1) > *((float*)v2)))
    return 1;
  
  return 0;
}


// transform gradients to luminance
void transform_to_luminance(pyramid_t* pp, float* const x, ProgressHelper *ph, const bool bcg, const int itmax, const float tol)
{
  pyramid_t* pC = pyramid_allocate(pp->cols, pp->rows);
  pyramid_calculate_scale_factor(pp, pC);             // calculate (Cx,Cy)
  pyramid_scale_gradient(pp, pC);                     // scale small gradients by (Cx,Cy);
  
  float* b = matrix_alloc(pp->cols * pp->rows);
  pyramid_calculate_divergence_sum(pp, b);            // calculate the sum of divergences (equal to b)
  
  // calculate luminances from gradients
  if (bcg)
    linbcg(pp, pC, b, x, itmax, tol, ph);
  else
    lincg(pp, pC, b, x, itmax, tol, ph);
  
  matrix_free(b);
  pyramid_free(pC);
}


struct hist_data
{
  float size;
  float cdf;
  int index;
};

int hist_data_order(const void* const v1, const void* const v2)
{
  if (((struct hist_data*) v1)->size < ((struct hist_data*) v2)->size)
    return -1;
  
  if (((struct hist_data*) v1)->size > ((struct hist_data*) v2)->size)
    return 1;
  
  return 0;
}


int hist_data_index(const void* const v1, const void* const v2)
{
  return ((struct hist_data*) v1)->index - ((struct hist_data*) v2)->index;
}


void contrast_equalization(pyramid_t *pp, const float contrastFactor)
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
    fprintf(stderr, "ERROR: malloc in contrast_equalization() (size:%zu)", sizeof(struct hist_data) * total_pixels);
    exit(155);
  }
  
  // Build histogram info
  l = pp;
  int index = 0;
  while ( l != NULL )
  {
    const int pixels = l->rows*l->cols;
    const int offset = index;
    #pragma omp parallel for schedule(static)
    for(int c = 0; c < pixels; c++)
    {
      hist[c+offset].size = sqrtf( l->Gx[c]*l->Gx[c] + l->Gy[c]*l->Gy[c] );
      hist[c+offset].index = c + offset;
    } 
    index += pixels;
    l = l->next;
  }
  
  // Generate histogram
  qsort(hist, total_pixels, sizeof(struct hist_data), hist_data_order);
  
  // Calculate cdf
  const float norm = 1.0f / (float) total_pixels;
  #pragma omp parallel for schedule(static)
  for (int i = 0; i < total_pixels; i++)
  {
    hist[i].cdf = ((float) i) * norm;
  }
  
  // Recalculate in terms of indexes
  qsort(hist, total_pixels, sizeof(struct hist_data), hist_data_index);
  
  //Remap gradient magnitudes
  l = pp;
  index = 0;
  while  ( l != NULL )
  {
    const int pixels = l->rows*l->cols;
    const int offset = index;
    
    #pragma omp parallel for schedule(static)
    for( int c = 0; c < pixels; c++)
    {
      const float scale = contrastFactor * hist[c+offset].cdf/hist[c+offset].size;
      l->Gx[c] *= scale;
      l->Gy[c] *= scale;        
    } 
    index += pixels;
    l = l->next;
  }
  
  free(hist);
}


// tone mapping
int tmo_mantiuk06_contmap(const int c, const int r, float* const R, float* const G, float* const B, float* const Y, const float contrastFactor, const float saturationFactor, float detailfactor, const bool bcg, const int itmax, const float tol, ProgressHelper *ph)
{
  const int n = c*r;
  
  /* Normalize */
  float Ymax = Y[0];
  for (int j = 1; j < n; j++)
    if (Y[j] > Ymax)
      Ymax = Y[j];
  
  const float clip_min = 1e-7f*Ymax;
  
  //TODO: use VEX, if you can
  #pragma omp parallel for schedule(static)
  for (int j = 0; j < n; j++)
  {
    if ( unlikely(R[j] < clip_min) ) R[j] = clip_min;
    if ( unlikely(G[j] < clip_min) ) G[j] = clip_min;
    if ( unlikely(B[j] < clip_min) ) B[j] = clip_min;
    if ( unlikely(Y[j] < clip_min) ) Y[j] = clip_min;    
  }
	
  //TODO: use VEX
  #pragma omp parallel for schedule(static)
  for(int j=0;j<n;j++)
  {
    R[j] /= Y[j];
    G[j] /= Y[j];
    B[j] /= Y[j];
    Y[j] = log10f(Y[j]);
  }
	
  pyramid_t* pp = pyramid_allocate(c, r);                 // create pyramid

  pyramid_calculate_gradient(pp, Y);                      // calculate gradients for pyramid (Y won't be changed)
  pyramid_transform_to_R(pp, detailfactor);               // transform gradients to R
  
  /* Contrast map */
  if (contrastFactor > 0.0f)
  {
    pyramid_gradient_multiply(pp, contrastFactor);        // Contrast mapping
  }
  else
  {
    contrast_equalization(pp, -contrastFactor);           // Contrast equalization
  }
	
  pyramid_transform_to_G(pp, detailfactor);               // transform R to gradients
  transform_to_luminance(pp, Y, ph, bcg, itmax, tol);     // transform gradients to luminance Y
  pyramid_free(pp);
  
  /* Renormalize luminance */
  float* temp = matrix_alloc(n);
	
  matrix_copy(n, Y, temp);                                // copy Y to temp
  qsort(temp, n, sizeof(float), sort_float);              // sort temp in ascending order
	
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
  
  //TODO: is it possible to use VEX?
  #pragma omp parallel for schedule(static)
  for(int j=0; j<n; j++)
  {
    Y[j] = (Y[j] - l_min) / (l_max - l_min) * disp_dyn_range - disp_dyn_range; // x scaled
  }
  
  //TODO: use VEX
  /* Transform to linear scale RGB */
  #pragma omp parallel for schedule(static)
  for(int j=0;j<n;j++)
  {
    Y[j] = powf(10, Y[j]);
    R[j] = powf( R[j], saturationFactor) * Y[j];
    G[j] = powf( G[j], saturationFactor) * Y[j];
    B[j] = powf( B[j], saturationFactor) * Y[j];
  }
  
  return PFSTMO_OK;
}

#ifdef DEBUG_MANTIUK06

#define PFSEOL "\x0a"
void dump_matrix_to_file(const int width, const int height, const float* const m, const char * const file_name)
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

// display matrix in the console (debugging)
void matrix_show(const char* const text, int cols, int rows, const float* const data)
{
  const int _cols = cols;
  
  if(rows > 8)
    rows = 8;
  if(cols > 8)
    cols = 8;
  
  printf("\n%s\n", text);
  for(int ky=0; ky<rows; ky++)
  {
    for(int kx=0; kx<cols; kx++)
    {
      printf("%.06f  ", data[kx + ky*_cols]);
    }
    printf("\n");
  }
}

// display pyramid in the console (debugging)
void pyramid_show(pyramid_t* pyramid)
{
  char ss[30];
  
  while (pyramid->next != NULL)
  {
    pyramid = pyramid->next;
  }
  
  while (pyramid != NULL)
  {
    printf("\n----- pyramid_t level %d,%d\n", pyramid->cols, pyramid->rows);
    
    sprintf(ss, "Gx %p ", pyramid->Gx);
    if (pyramid->Gx != NULL)
    {
      matrix_show(ss,pyramid->cols, pyramid->rows, pyramid->Gx);
    }
    sprintf(ss, "Gy %p ", pyramid->Gy);	
    if (pyramid->Gy != NULL)
    {
      matrix_show(ss,pyramid->cols, pyramid->rows, pyramid->Gy);
    }
    
    pyramid = pyramid->prev;
  }
}

#endif
