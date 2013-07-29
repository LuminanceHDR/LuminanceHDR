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
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *  Improvement & Clean up
 * @author Bruce Guenter <bruce@untroubled.org>
 *  Added trivial downsample and upsample functions when both dimension are even
 *
 * $Id: contrast_domain.cpp,v 1.14 2008/08/26 17:08:49 rafm Exp $
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <iostream>
#include <cassert>

#include "contrast_domain.h"
#include "arch/malloc.h"
#include "arch/math.h"
#include "TonemappingOperators/pfstmo.h"

#include "Libpfs/utils/sse.h"
#include "Libpfs/utils/numeric.h"
#include "Libpfs/utils/dotproduct.h"
#include "Libpfs/utils/msec_timer.h"

namespace test_mantiuk06
{

#define PYRAMID_MIN_PIXELS      3
#define LOOKUP_W_TO_R           107
//#define DEBUG_MANTIUK06

void swap_pointers(float* &pOne, float* &pTwo); // utility function

#ifdef DEBUG_MANTIUK06

void dump_matrix_to_file(const int width, const int height, const float* const m, const char * const file_name);
void matrix_show(const char* const text, int rows, int cols, const float* const data);
void pyramid_show(pyramid_t* pyramid);

#endif

static float W_table[] = {0.000000f,0.010000f,0.021180f,0.031830f,0.042628f,0.053819f,0.065556f,0.077960f,0.091140f,0.105203f,0.120255f,0.136410f,0.153788f,0.172518f,0.192739f,0.214605f,0.238282f,0.263952f,0.291817f,0.322099f,0.355040f,0.390911f,0.430009f,0.472663f,0.519238f,0.570138f,0.625811f,0.686754f,0.753519f,0.826720f,0.907041f,0.995242f,1.092169f,1.198767f,1.316090f,1.445315f,1.587756f,1.744884f,1.918345f,2.109983f,2.321863f,2.556306f,2.815914f,3.103613f,3.422694f,3.776862f,4.170291f,4.607686f,5.094361f,5.636316f,6.240338f,6.914106f,7.666321f,8.506849f,9.446889f,10.499164f,11.678143f,13.000302f,14.484414f,16.151900f,18.027221f,20.138345f,22.517282f,25.200713f,28.230715f,31.655611f,35.530967f,39.920749f,44.898685f,50.549857f,56.972578f,64.280589f,72.605654f,82.100619f,92.943020f,105.339358f,119.530154f,135.795960f,154.464484f,175.919088f,200.608905f,229.060934f,261.894494f,299.838552f,343.752526f,394.651294f,453.735325f,522.427053f,602.414859f,695.706358f,804.693100f,932.229271f,1081.727632f,1257.276717f,1463.784297f,1707.153398f,1994.498731f,2334.413424f,2737.298517f,3215.770944f,3785.169959f,4464.187290f,5275.653272f,6247.520102f,7414.094945f,8817.590551f,10510.080619f};
static float R_table[] = {0.000000f,0.009434f,0.018868f,0.028302f,0.037736f,0.047170f,0.056604f,0.066038f,0.075472f,0.084906f,0.094340f,0.103774f,0.113208f,0.122642f,0.132075f,0.141509f,0.150943f,0.160377f,0.169811f,0.179245f,0.188679f,0.198113f,0.207547f,0.216981f,0.226415f,0.235849f,0.245283f,0.254717f,0.264151f,0.273585f,0.283019f,0.292453f,0.301887f,0.311321f,0.320755f,0.330189f,0.339623f,0.349057f,0.358491f,0.367925f,0.377358f,0.386792f,0.396226f,0.405660f,0.415094f,0.424528f,0.433962f,0.443396f,0.452830f,0.462264f,0.471698f,0.481132f,0.490566f,0.500000f,0.509434f,0.518868f,0.528302f,0.537736f,0.547170f,0.556604f,0.566038f,0.575472f,0.584906f,0.594340f,0.603774f,0.613208f,0.622642f,0.632075f,0.641509f,0.650943f,0.660377f,0.669811f,0.679245f,0.688679f,0.698113f,0.707547f,0.716981f,0.726415f,0.735849f,0.745283f,0.754717f,0.764151f,0.773585f,0.783019f,0.792453f,0.801887f,0.811321f,0.820755f,0.830189f,0.839623f,0.849057f,0.858491f,0.867925f,0.877358f,0.886792f,0.896226f,0.905660f,0.915094f,0.924528f,0.933962f,0.943396f,0.952830f,0.962264f,0.971698f,0.981132f,0.990566f,1.000000f};

 int imin(int a, int b)
{
  return a < b ? a : b;
}

// upsample the matrix
// upsampled matrix is twice bigger in each direction than data[]
// res should be a pointer to allocated memory for bigger matrix
// cols and rows are the dimensions of the output matrix
void matrix_upsample_full(const int outCols, const int outRows, const float* const in, float* const out)
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

void matrix_upsample_simple(const int outCols, const int outRows, const float* const in, float* const out)
{
  for (int y = 0; y < outRows; y++)
  {
    const int iy1 = y / 2;
    float* outp = out + y*outCols;
    const float* inp = in + iy1*(outCols/2);
    for (int x = 0; x < outCols; x+=2)
    {
      const int ix1 = x / 2;
      outp[x] = outp[x+1] = inp[ix1];
    }
  }
}

void matrix_upsample(const int outCols, const int outRows, const float* const in, float* const out)
{
  if (outRows%2 == 0 && outCols%2 == 0)
    matrix_upsample_simple(outCols, outRows, in, out);
  else
    matrix_upsample_full(outCols, outRows, in, out);
}

// downsample the matrix
void matrix_downsample_full(const int inCols, const int inRows, const float* const data, float* const res)
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

void matrix_downsample_simple(const int inCols, const int inRows, const float* const data, float* const res)
{
    const int outRows = inRows / 2;
    const int outCols = inCols / 2;

    // Simplified downsampling by Bruce Guenter:
    //
    // Follows exactly the same math as the full downsampling above,
    // except that inRows and inCols are known to be even.  This allows
    // for all of the boundary cases to be eliminated, reducing the
    // sampling to a simple average.

    for (int y = 0; y < outRows; y++)
    {
        const int iy1 = y * 2;
        const float* datap = data + iy1 * inCols;
        float* resp = res + y * outCols;

        for (int x = 0; x < outCols; x++)
        {
            const int ix1 = x*2;

            resp[x] = ( datap[ix1]
                        + datap[ix1+1]
                        + datap[ix1   + inCols]
                        + datap[ix1+1 + inCols]) / 4.0f;
        }
    }
}

void matrix_downsample(const int inCols, const int inRows, const float* const data, float* const res)
{
  if (inCols % 2 == 0 && inRows % 2 == 0)
    matrix_downsample_simple(inCols, inRows, data, res);
  else
    matrix_downsample_full(inCols, inRows, data, res);
}

// return = a - b
 void matrix_subtract(const int n, const float* const a, float* const b)
{
    pfs::utils::vsub(a, b, b, n);
}

// copy matix a to b, return = a 
 void matrix_copy(const int n, const float* const a, float* const b)
{
     std::copy(a, a + n, b);
}

// multiply matrix a by scalar val
void matrix_multiply_const(const int n, float* const a, const float val)
{
     pfs::utils::vsmul(a, val, a, n);
}

// alloc memory for the float table
 float* matrix_alloc(int size)
{
#ifdef __APPLE__
  float* m  = (float*)malloc      (sizeof(float)*size);
#else
  float* m    = (float*)_mm_malloc  (sizeof(float)*size, 32);
#endif
  if (m == NULL)
  {
    fprintf(stderr, "ERROR: malloc in matrix_alloc() (size:%d)", size);
    exit(155);
  }
  return m;
}

// free memory for matrix
 void matrix_free(float* m)
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
    return pfs::utils::dotProduct(a, b, n);
}

// set zeros for matrix elements
 void matrix_zero(int n, float* m)
{
  //bzero(m, n*sizeof(float));
  memset(m, 0, n*sizeof(float));
}

// Davide Anastasia <davideanastasia@users.sourceforge.net> (2010 08 31)
// calculate divergence of two gradient maps (Gx and Gy)
// divG(x,y) = Gx(x,y) - Gx(x-1,y) + Gy(x,y) - Gy(x,y-1)  
void calculate_and_add_divergence(const int COLS, const int ROWS, const float* const Gx, const float* const Gy, float* const divG)
{
    float divGx, divGy;
    {
        {
            // kx = 0 AND ky = 0;
            divG[0] += Gx[0] + Gy[0];                       // OUT

            // ky = 0
            for (int kx=1; kx<COLS; kx++)
            {
                divGx = Gx[kx] - Gx[kx - 1];
                divGy = Gy[kx];
                divG[kx] += divGx + divGy;                    // OUT
            }
        }
        {
            for (int ky=1; ky<ROWS; ky++)
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
    }   // END PARALLEL SECTIONS
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
 void calculate_scale_factor(const int n, const float* const G, float* const C)
{
  //  float GFIXATE = 0.1f;
  //  float EDGE_WEIGHT = 0.01f;
  const float detectT = 0.001f;
  const float a = 0.038737f;
  const float b = 0.537756f;

  for(int i=0; i<n; i++)
  {
    //#if 1
    const float g = std::max( detectT, fabsf(G[i]) );
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
void pyramid_calculate_scale_factor(const pyramid_t* pyramid, pyramid_t* pC)
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
 void scale_gradient(const int n, float* G, const float* C)
{
    pfs::utils::vmul(G, C, G, n);
}

// scale gradients for the whole one pyramid with the use of (Cx,Cy) from the other pyramid
void pyramid_scale_gradient(pyramid_t* pyramid, const pyramid_t* pC)
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
      assert(rows != 0);
      assert(cols != 0);

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

    assert(size != 0);

    level->Gx = matrix_alloc(size);
    if (level->Gx == NULL)
    {
      fprintf(stderr, "ERROR: malloc in pyramid_alloc() (size:%zu)", sizeof(pyramid_t) );
      exit(155);
    }
    level->Gy = matrix_alloc(size);
    if (level->Gy == NULL)
    {
      fprintf(stderr, "ERROR: malloc in pyramid_alloc() (size:%zu)", sizeof(pyramid_t) );
      exit(155);
    }

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
 void calculate_gradient(const int COLS, const int ROWS, const float* const lum, float* const Gx, float* const Gy)
{ 
  int Y_IDX, IDX;
  
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
void pyramid_calculate_gradient(pyramid_t* pyramid, const float* Y)
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

// divG_sum = A * x = sum(divG(x))
void multiplyA(pyramid_t* px, pyramid_t* pC, const float* x, float* divG_sum)
{
  pyramid_calculate_gradient(px, x);                // x won't be changed
  pyramid_scale_gradient(px, pC);                   // scale gradients by Cx,Cy from main pyramid
  pyramid_calculate_divergence_sum(px, divG_sum);   // calculate the sum of divergences
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
    ph->setValue( (int) (logf(rdotr_curr/irdotr)*percent_sf));
    // User requested abort
    if (ph->canceled() && iter > 0 )
    {
      break;
    }
    
    // Ap = A p
    multiplyA(pyramid, pC, p, Ap);
    
    // alpha = r.r / (p . Ap)
    alpha = rdotr_curr / matrix_DotProduct(n, p, Ap);
    
    // r = r - alpha Ap
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
    ph->setValue( (int) (logf(rdotr_curr/irdotr)*percent_sf));
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
    ph->setValue( itmax );
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

// in_tab and out_tab should contain inccreasing float values
 float lookup_table(const int n, const float* const in_tab, const float* const out_tab, const float val)
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
 void transform_to_R(const int n, float* const G, float detail_factor)
{
  const float log10=2.3025850929940456840179914546844*detail_factor;
  
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
 void transform_to_G(const int n, float* const R, float detail_factor)
{
  //here we are actually changing the base of logarithm
  const float log10 = 2.3025850929940456840179914546844*detail_factor; 
  
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
void pyramid_transform_to_R(pyramid_t* pyramid, float detail_factor)
{  
  while (pyramid != NULL)
  {    
    transform_to_R(pyramid->rows * pyramid->cols, pyramid->Gx, detail_factor);	
    transform_to_R(pyramid->rows * pyramid->cols, pyramid->Gy, detail_factor);
    
    pyramid = pyramid->next;
  }
}



// transform from R to G for the pyramid
void pyramid_transform_to_G(pyramid_t* pyramid, float detail_factor)
{
  while (pyramid != NULL)
  {
    transform_to_G(pyramid->rows*pyramid->cols, pyramid->Gx, detail_factor);	
    transform_to_G(pyramid->rows*pyramid->cols, pyramid->Gy, detail_factor);
    
    pyramid = pyramid->next;
  }
}

// multiply gradient (Gx,Gy) values by float scalar value for the whole pyramid
void pyramid_gradient_multiply(pyramid_t* pyramid, const float val)
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
void transform_to_luminance(pyramid_t* pp, float* x, ProgressHelper *ph, const int itmax, const float tol)
{
  pyramid_t* pC = pyramid_allocate(pp->cols, pp->rows);
  pyramid_calculate_scale_factor(pp, pC);             // calculate (Cx,Cy)
  pyramid_scale_gradient(pp, pC);                     // scale small gradients by (Cx,Cy);
  
  float* b = matrix_alloc(pp->cols * pp->rows);
  pyramid_calculate_divergence_sum(pp, b);            // calculate the sum of divergences (equal to b)
  
  // calculate luminances from gradients
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
  assert( hist[0].size < hist[total_pixels-1].size );

  // Calculate cdf
  const float norm = 1.0f / (float) total_pixels;
  for (int i = 0; i < total_pixels; i++)
  {
    hist[i].cdf = ((float) i) * norm;
  }
  
  // Recalculate in terms of indexes
  qsort(hist, total_pixels, sizeof(struct hist_data), hist_data_index);
  assert( hist[0].index < hist[total_pixels-1].index );
  assert( hist[0].index == 0 );


  //Remap gradient magnitudes
  l = pp;
  index = 0;
  while  ( l != NULL )
  {
    const int pixels = l->rows*l->cols;
    const int offset = index;
    
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
int tmo_mantiuk06_contmap(const int c, const int r, float* const R, float* const G, float* const B, float* const Y, const float contrastFactor, const float saturationFactor, float detailfactor, const int itmax, const float tol, ProgressHelper *ph)
{
  const int n = c*r;
  
  /* Normalize */
  float Ymax = Y[0];
  for (int j = 1; j < n; j++)
    if (Y[j] > Ymax)
      Ymax = Y[j];
  
  const float clip_min = 1e-7f*Ymax;
  
  for (int j = 0; j < n; j++)
  {
    if ( unlikely(R[j] < clip_min) ) R[j] = clip_min;
    if ( unlikely(G[j] < clip_min) ) G[j] = clip_min;
    if ( unlikely(B[j] < clip_min) ) B[j] = clip_min;
    if ( unlikely(Y[j] < clip_min) ) Y[j] = clip_min;    
  }

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
  transform_to_luminance(pp, Y, ph, itmax, tol);     // transform gradients to luminance Y
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
  
  for(int j=0; j<n; j++)
  {
    Y[j] = (Y[j] - l_min) / (l_max - l_min) * disp_dyn_range - disp_dyn_range; // x scaled
  }
  
  /* Transform to linear scale RGB */
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

} // namespace test_mantiuk06
