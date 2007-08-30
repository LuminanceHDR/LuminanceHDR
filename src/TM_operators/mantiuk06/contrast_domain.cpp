/**
 * @brief Contrast mapping TMO
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
 *
 * $Id: contrast_domain.cpp,v 1.5 2007/06/16 19:23:08 rafm Exp $
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "contrast_domain.h"

typedef struct {
  int rows, cols;
  float* Gx;
  float* Gy;
  float* Cx;
  float* Cy;
  float* divG;
  float* M;  // Gradient modules, needed only for contrast equalization
}
PYRAMID_LEVEL;

struct PYRAMID{
  PYRAMID_LEVEL* level;
  PYRAMID* next;
};


extern float W_table[];
extern float R_table[];
extern float xyz2rgbD65Mat[3][3];
extern float rgb2xyzD65Mat[3][3];

#define PYRAMID_MIN_PIXELS 3
#define LOOKUP_W_TO_R 107

class ContrastDomain {

  int alloc_mem;
  progress_callback progress_cb;
  
public:

  ContrastDomain( ) {
    alloc_mem = 0;
  }

  void tone_mapping(int, int, float* R, float* G, float* B, float* Y, float contrastFactor, float saturationFactor, progress_callback );
  void contrast_equalization( PYRAMID *pp );

  float* transform_to_luminance(PYRAMID* pyramid);
  void matrix_upsample(int rows, int cols, float* data, float* res);
  void matrix_downsample(int rows, int cols, float* data, float* res);
  float* matrix_add(int n, float* a, float* b);
  float* matrix_substract(int n, float* a, float* b);
  float* matrix_copy(int n, float* a, float* b);
  void matrix_multiply_const(int n, float* a, float val);
  void matrix_divide(int n, float* a, float* b);
  float* matrix_alloc(int size);
  void matrix_free(float* m);
  float matrix_DotProduct(int n, float* a, float* b);
  void matrix_zero(int n, float* m);
  void calculate_divergence(int rows, int cols, float* Gx, float* Gy, float* divG);
  void pyramid_calculate_divergence(PYRAMID* pyramid);
  void pyramid_calculate_divergence_sum_in(PYRAMID* pyramid, float* divG_sum);
  void pyramid_calculate_divergence_sum(PYRAMID* pyramid, float* divG_sum);
  float* calculate_scale_factor(int n, float* G);
  void pyramid_calculate_scale_factor(PYRAMID* pyramid);
  void scale_gradient(int n, float* G, float* C);
  void pyramid_scale_gradient(PYRAMID* pyramid);
  void pyramid_scale_gradient(PYRAMID* pyramid, PYRAMID* pC);
  void pyramid_free(PYRAMID* pyramid);
  PYRAMID* pyramid_allocate(int cols, int rows);
  void calculate_gradient(int cols, int rows, float* lum, float* Gx, float* Gy);
  void pyramid_calculate_gradient(PYRAMID* pyramid, float* lum);
  //PYRAMID* pyramid_create(int lrows, int lcols, float* llum);
  void solveX(int n, float* b, float* x);
  void multiplyA(PYRAMID* px, PYRAMID* pyramid, float* x, float* divG_sum);
  float* linbcg(PYRAMID* pyramid, float* b);
  void matrix_log10(int n, float* m);
  float lookup_table(int n, float* in_tab, float* out_tab, float val);
  void transform_to_R(int n, float* G);
  void pyramid_transform_to_R(PYRAMID* pyramid);
  void transform_to_G(int n, float* R);
  void pyramid_transform_to_G(PYRAMID* pyramid);
  float matrix_median(int n, float* m);
  void pyramid_gradient_multiply(PYRAMID* pyramid, float val);

  void matrix_show(char* text, int rows, int cols, float* data);
  void pyramid_show(PYRAMID* pyramid);

};


//#define DEBUG

float W_table[] = {0.000000,0.010000,0.021180,0.031830,0.042628,0.053819,0.065556,0.077960,0.091140,0.105203,0.120255,0.136410,0.153788,0.172518,0.192739,0.214605,0.238282,0.263952,0.291817,0.322099,0.355040,0.390911,0.430009,0.472663,0.519238,0.570138,0.625811,0.686754,0.753519,0.826720,0.907041,0.995242,1.092169,1.198767,1.316090,1.445315,1.587756,1.744884,1.918345,2.109983,2.321863,2.556306,2.815914,3.103613,3.422694,3.776862,4.170291,4.607686,5.094361,5.636316,6.240338,6.914106,7.666321,8.506849,9.446889,10.499164,11.678143,13.000302,14.484414,16.151900,18.027221,20.138345,22.517282,25.200713,28.230715,31.655611,35.530967,39.920749,44.898685,50.549857,56.972578,64.280589,72.605654,82.100619,92.943020,105.339358,119.530154,135.795960,154.464484,175.919088,200.608905,229.060934,261.894494,299.838552,343.752526,394.651294,453.735325,522.427053,602.414859,695.706358,804.693100,932.229271,1081.727632,1257.276717,1463.784297,1707.153398,1994.498731,2334.413424,2737.298517,3215.770944,3785.169959,4464.187290,5275.653272,6247.520102,7414.094945,8817.590551,10510.080619};
float R_table[] = {0.000000,0.009434,0.018868,0.028302,0.037736,0.047170,0.056604,0.066038,0.075472,0.084906,0.094340,0.103774,0.113208,0.122642,0.132075,0.141509,0.150943,0.160377,0.169811,0.179245,0.188679,0.198113,0.207547,0.216981,0.226415,0.235849,0.245283,0.254717,0.264151,0.273585,0.283019,0.292453,0.301887,0.311321,0.320755,0.330189,0.339623,0.349057,0.358491,0.367925,0.377358,0.386792,0.396226,0.405660,0.415094,0.424528,0.433962,0.443396,0.452830,0.462264,0.471698,0.481132,0.490566,0.500000,0.509434,0.518868,0.528302,0.537736,0.547170,0.556604,0.566038,0.575472,0.584906,0.594340,0.603774,0.613208,0.622642,0.632075,0.641509,0.650943,0.660377,0.669811,0.679245,0.688679,0.698113,0.707547,0.716981,0.726415,0.735849,0.745283,0.754717,0.764151,0.773585,0.783019,0.792453,0.801887,0.811321,0.820755,0.830189,0.839623,0.849057,0.858491,0.867925,0.877358,0.886792,0.896226,0.905660,0.915094,0.924528,0.933962,0.943396,0.952830,0.962264,0.971698,0.981132,0.990566,1.000000};


// display matrix in the console (debugging)
void ContrastDomain::matrix_show(char* text, int cols, int rows, float* data){

  int _cols = cols;

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

// display pyramid in the console (debugging)
void ContrastDomain::pyramid_show(PYRAMID* pyramid){

  if(pyramid->next != NULL)	
    pyramid_show((PYRAMID*)pyramid->next);
	
  PYRAMID_LEVEL* pl = pyramid->level;
  char ss[30];
	
  printf("\n----- PYRAMID level %d,%d\n", pl->cols, pl->rows);
	
  sprintf(ss, "Gx %p ", pl->Gx);
  if(pl->Gx != NULL)
    matrix_show(ss,pl->cols, pl->rows, pl->Gx);
  sprintf(ss, "Gy %p ", pl->Gy);	
  if(pl->Gy != NULL)
    matrix_show(ss,pl->cols, pl->rows, pl->Gy);
  sprintf(ss, "Cx %p ", pl->Cx);	
  if(pl->Cx != NULL)
    matrix_show(ss,pl->cols, pl->rows, pl->Cx);
  sprintf(ss, "Cy %p ", pl->Cy);	
  if(pl->Cy != NULL)	
    matrix_show(ss,pl->cols, pl->rows, pl->Cy);
  if(pl->divG != NULL)	
    matrix_show("divG",pl->cols, pl->rows, pl->divG);	

  return;
}



inline float max( float a, float b )
{
  return a > b ? a : b;
}

inline float min( float a, float b )
{
  return a < b ? a : b;
}

// upsample the matrix
// upsampled matrix is twice bigger in each direction than data[]
// res should be a pointer to allocated memory for bigger matrix
// nearest neighborhood method - should be changed!
void ContrastDomain::matrix_upsample(int cols, int rows, float* in, float* out){

  float dx = (float)cols / (float)(cols*2);
  float dy = (float)rows / (float)(rows*2);

  const int outRows = rows * 2;
  const int outCols = cols * 2;

  const float inRows = rows;
  const float inCols = cols;

  const float filterSize = 1;
  
  float sx, sy;
  int x, y;
  for( y = 0, sy = dy/2-0.5; y < outRows; y++, sy += dy )
    for( x = 0, sx = dx/2-0.5; x < outCols; x++, sx += dx ) {

      float pixVal = 0;
      float weight = 0;
      
      for( float ix = max( 0, ceilf( sx-filterSize ) ); 
           ix <= min( floorf(sx+filterSize), inCols-1 ); ix++ )
        for( float iy = max( 0, ceilf( sy-filterSize ) ); 
             iy <= min( floorf( sy+filterSize), inRows-1 ); iy++ ) {
		
          float fx = fabs( sx - ix );
          float fy = fabs( sy - iy );

          const float fval = (1-fx)*(1-fy);
          
          pixVal += in[ (int)ix + (int)inCols * (int)iy ] * fval;
          weight += fval;
        }
      
      out[x + outCols * y] = pixVal / weight;
    } 	
  return;
}

// downsample the matrix
void ContrastDomain::matrix_downsample(int cols, int rows, float* data, float* res){

  const float inRows = rows;
  const float inCols = cols;

  const int outRows = rows / 2;
  const int outCols = cols / 2;

  const float dx = (float)inCols / (float)outCols;
  const float dy = (float)inRows / (float)outRows;

  const float filterSize = 0.5;
  
  float sx, sy;
  int x, y;
  
  for( y = 0, sy = dy/2-0.5; y < outRows; y++, sy += dy )
    for( x = 0, sx = dx/2-0.5; x < outCols; x++, sx += dx ) {

      float pixVal = 0;
      float w = 0;
      for( float ix = max( 0, ceilf( sx-dx*filterSize ) ); ix <= min( floorf( sx+dx*filterSize ), inCols-1 ); ix++ )
        for( float iy = max( 0, ceilf( sy-dx*filterSize ) ); iy <= min( floorf( sy+dx*filterSize), inRows-1 ); iy++ ) {
          pixVal += data[(int)ix + (int)iy * (int)inCols];
          w += 1;
        }     
      res[x + y * outCols] = pixVal/w;      
    }	
	
  return;
}

// return = a + b
float* ContrastDomain::matrix_add(int n, float* a, float* b){
  for(int i=0; i<n; i++){
    b[i] = b[i] + a[i];
  }
  return b;
}

// return = a - b
float* ContrastDomain::matrix_substract(int n, float* a, float* b){
  for(int i=0; i<n; i++){
    b[i] = a[i] - b[i];
  }
  return b;
}

// copy matix a to b, return = a 
float* ContrastDomain::matrix_copy(int n, float* a, float* b){
  for(int i=0; i<n; i++){
    b[i] = a[i];
  }
  return b;
}

// multiply matrix a by scalar val
void ContrastDomain::matrix_multiply_const(int n, float* a, float val){
  for(int i=0; i<n; i++){
    a[i] = a[i] * val;
  }
  return;
}

// b = a[i] / b[i]
void ContrastDomain::matrix_divide(int n, float* a, float* b){
  for(int i=0; i<n; i++){
    b[i] = a[i] / b[i];
  }
  return;
}


// alloc memory for the float table
float* ContrastDomain::matrix_alloc(int size){

  float* m = (float*)malloc(sizeof(float)*size);
  if(m == NULL){
    fprintf(stderr, "ERROR: malloc in matrix_alloc() (size:%d)", size);
    exit(155);
  }
  alloc_mem += sizeof(float)*size;
  return m;
}

// free memory for matrix
void ContrastDomain::matrix_free(float* m){
  if(m != NULL)
    free(m);
}

// multiply vector by vector (each vector should have one dimension equal to 1)
float ContrastDomain::matrix_DotProduct(int n, float* a, float* b){
  float val = 0;
  for(int j=0;j<n;j++)
    val += (a[j] * b[j]);
  return val;
}

// set zeros for matrix elements
void ContrastDomain::matrix_zero(int n, float* m){
  for(int j=0;j<n;j++)
    m[j] = 0;
  return;
}

// calculate divergence of two gradient maps (Gx and Gy)
// divG(x,y) = Gx(x,y) - Gx(x-1,y) + Gy(x,y) - Gy(x,y-1)  
void ContrastDomain::calculate_divergence(int cols, int rows, float* Gx, float* Gy, float* divG){

  float divGx, divGy;
	
  for(int ky=0; ky<rows; ky++){
    for(int kx=0; kx<cols; kx++){
			
      if(kx == 0)
        divGx = Gx[kx + ky*cols];
      else	
        divGx = Gx[kx + ky*cols] - Gx[(kx-1) + ky*cols];
			
      if(ky == 0)
        divGy = Gy[kx + ky*cols];
      else	
        divGy = Gy[kx + ky*cols] - Gy[kx + (ky-1)*cols];			
		
			
      divG[kx+ky*cols] = divGx + divGy;			
    }
  }
  return;
}

// calculate divergence for the pyramid
//  calculated divergence is put into PYRAMID_LEVEL::divG
void ContrastDomain::pyramid_calculate_divergence(PYRAMID* pyramid){

  if(pyramid->next != NULL)
    pyramid_calculate_divergence((PYRAMID*)pyramid->next);
		
  PYRAMID_LEVEL* pl = pyramid->level;
			
  calculate_divergence(pl->cols, pl->rows, pl->Gx, pl->Gy, pl->divG);
	
  return;
}


// calculate sum of divergences in the pyramid
//	divergences for a particular levels should be calculated earlier 
//	and set in PYRAMID_LEVEL::divG
void ContrastDomain::pyramid_calculate_divergence_sum_in(PYRAMID* pyramid, float* divG_sum){
	
  if(pyramid->next != NULL)	
    pyramid_calculate_divergence_sum_in((PYRAMID*)pyramid->next, divG_sum);

  PYRAMID_LEVEL* pl = pyramid->level;
	
  matrix_add(pl->rows * pl->cols, pl->divG, divG_sum);
	
  float* temp = matrix_alloc(pl->rows*pl->cols);
  matrix_copy(pl->rows*pl->cols, divG_sum, temp);

  matrix_upsample(pl->cols, pl->rows, temp, divG_sum);
	
  matrix_free(temp);

  return;
}


// calculate the sum of divergences for the all pyramid level
// the smaller divergence map is upsamled and added to the divergence map for the higher level of pyramid
void ContrastDomain::pyramid_calculate_divergence_sum(PYRAMID* pyramid, float* divG_sum){
	
  PYRAMID_LEVEL* pl = pyramid->level;
  matrix_zero(pl->rows * pl->cols, divG_sum);
	
  if(pyramid->next != NULL)	
    pyramid_calculate_divergence_sum_in((PYRAMID*)pyramid->next, divG_sum);

  matrix_add(pl->rows * pl->cols, pl->divG, divG_sum);
		
  return;
}

// calculate scale factors (Cx,Cy) for gradients (Gx,Gy)
// C is equal to EDGE_WEIGHT for gradients smaller than GFIXATE or 1.0 otherwise
float* ContrastDomain::calculate_scale_factor(int n, float* G){

  float GFIXATE = 0.1;
  float EDGE_WEIGHT = 0.01;
	
  float* C = matrix_alloc(n);

  for(int i=0; i<n; i++){

    if(fabs(G[i]) < GFIXATE)
      C[i] = EDGE_WEIGHT;
    else	
      C[i] = 1.0;
  }

  return C;
}

// calculate scale factor for the whole pyramid
void ContrastDomain::pyramid_calculate_scale_factor(PYRAMID* pyramid){

  if(pyramid->next != NULL)
    pyramid_calculate_scale_factor((PYRAMID*)pyramid->next);
		
  PYRAMID_LEVEL* pl = pyramid->level;
			
  pl->Cx = calculate_scale_factor(pl->rows*pl->cols, pl->Gx);	
  pl->Cy = calculate_scale_factor(pl->rows*pl->cols, pl->Gy);	

  return;
}

// Scale gradient (Gx and Gy) by C (Cx and Cy)
// G = G / C
void ContrastDomain::scale_gradient(int n, float* G, float* C){

  for(int i=0; i<n; i++)
    G[i] = G[i] / C[i];
  return;
}

// scale gradients for the whole pyramid
void ContrastDomain::pyramid_scale_gradient(PYRAMID* pyramid){

  if(pyramid->next != NULL)
    pyramid_scale_gradient((PYRAMID*)pyramid->next);
		
  PYRAMID_LEVEL* pl = pyramid->level;
  scale_gradient(pl->rows*pl->cols, pl->Gx, pl->Cx);	
  scale_gradient(pl->rows*pl->cols, pl->Gy, pl->Cy);	

  return;
}

// scale gradients for the whole one pyramid with the use of (Cx,Cy) from the other pyramid
void ContrastDomain::pyramid_scale_gradient(PYRAMID* pyramid, PYRAMID* pC){

  if(pyramid->next != NULL)
    pyramid_scale_gradient((PYRAMID*)pyramid->next, (PYRAMID*)pC->next);
		
  PYRAMID_LEVEL* pl = pyramid->level;
  PYRAMID_LEVEL* plC = pC->level;
	
  scale_gradient(pl->rows*pl->cols, pl->Gx, plC->Cx);	
  scale_gradient(pl->rows*pl->cols, pl->Gy, plC->Cy);	

  return;
}


// free memory allocated for the pyramid
void ContrastDomain::pyramid_free(PYRAMID* pyramid){

  if(pyramid->next != NULL)
    pyramid_free((PYRAMID*)pyramid->next);
		
  PYRAMID_LEVEL* pl = pyramid->level;

  if(pl->Gx != NULL)
    free(pl->Gx);
  if(pl->Gy != NULL)
    free(pl->Gy);
  if(pl->Cx != NULL)
    free(pl->Cx);
  if(pl->Cy != NULL)
    free(pl->Cy);
  if(pl->divG != NULL)
    free(pl->divG);	
		
  free(pl);
  free(pyramid);
			
  return;
}


// allocate memory for the pyramid
PYRAMID * ContrastDomain::pyramid_allocate(int cols, int rows){

  PYRAMID_LEVEL* level = NULL;
  int size;
  PYRAMID* pyramid = NULL;
  PYRAMID* pyramid_higher = NULL;
	
  PYRAMID* p = NULL;
  pyramid_higher = NULL;
		
  while(1){
	
    level = (PYRAMID_LEVEL*)malloc(sizeof(PYRAMID_LEVEL));
    if(level == NULL)
      exit(155);
    memset( level, 0, sizeof(PYRAMID_LEVEL) );
		
    level->rows = rows;
    level->cols = cols;
    size = level->rows * level->cols;
    level->Gx = matrix_alloc(size);
    level->Gy = matrix_alloc(size);
    level->divG = matrix_alloc(size);
	
    pyramid = (PYRAMID*)malloc(sizeof(PYRAMID));
    pyramid->level = level;
    pyramid->next = NULL;

    if(pyramid_higher != NULL)
      pyramid_higher->next = pyramid;			
    pyramid_higher = pyramid;
		
    if(p == NULL)
      p = pyramid;
		
    rows /= 2;
    cols /= 2;		
    if(rows < PYRAMID_MIN_PIXELS || cols < PYRAMID_MIN_PIXELS)
      break;
  }
	
  return p;
}


// calculate gradients
void ContrastDomain::calculate_gradient(int cols, int rows, float* lum, float* Gx, float* Gy){

  int idx;
	
  for(int ky=0; ky<rows; ky++){
    for(int kx=0; kx<cols; kx++){
			
      idx = kx + ky*cols;
			
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

  return;
}


// calculate gradients for the pyramid
void ContrastDomain::pyramid_calculate_gradient(PYRAMID* pyramid, float* lum){

  PYRAMID_LEVEL* pl = pyramid->level;

  float* lum_temp = matrix_alloc(pl->rows * pl->cols);	
  matrix_copy(pl->rows*pl->cols,lum,lum_temp);

  calculate_gradient(pl->cols, pl->rows, lum_temp, pl->Gx, pl->Gy);	

  PYRAMID* pp = (PYRAMID*)pyramid->next;
  float* temp;

  while(1){
    if(pp == NULL)
      break;
    pl = pp->level;
			
    temp = matrix_alloc(pl->rows * pl->cols);			
    matrix_downsample(pl->cols*2, pl->rows*2, lum_temp, temp);	
		
    calculate_gradient(pl->cols, pl->rows, temp, pl->Gx, pl->Gy);
		
    matrix_free(lum_temp);
    lum_temp = temp;	
			
    pp = (PYRAMID*)pp->next;	
  }
  matrix_free(lum_temp);

  return;
}


// x = -0.25 * b
void ContrastDomain::solveX(int n, float* b, float* x){

  matrix_copy(n, b, x); // x = b
  matrix_multiply_const(n, x, -0.25);
}

// divG_sum = A * x = sum(divG(x))
// memory for the temporary pyramid px should be allocated
void ContrastDomain::multiplyA(PYRAMID* px, PYRAMID* pyramid, float* x, float* divG_sum){

  pyramid_calculate_gradient(px, x);
	
  pyramid_scale_gradient(px, pyramid); // scale gradients by Cx,Cy from main pyramid
	
  pyramid_calculate_divergence(px); // calculate divergences for all pyramid levels

  pyramid_calculate_divergence_sum(px, divG_sum); // calculate the sum of divergences

  return;
} 


// bi-conjugate linear equation solver
float* ContrastDomain::linbcg(PYRAMID* pyramid, float* b){

  int rows = pyramid->level->rows;
  int cols = pyramid->level->cols;
  int n = rows*cols;
	
  float* z = matrix_alloc(n);
  float* zz = matrix_alloc(n);
  float* p = matrix_alloc(n);
  float* pp = matrix_alloc(n);
  float* r = matrix_alloc(n);
  float* rr = matrix_alloc(n);	
	
  PYRAMID* pyramid_tmp = 
    pyramid_allocate(pyramid->level->cols, pyramid->level->rows);
	
  float* x = matrix_alloc(n); // allocate x matrix filled with zeros
  matrix_zero(n, x); // x = 0
	
  multiplyA(pyramid_tmp, pyramid, x, r); // r = A*x = divergence(x)

  matrix_substract(n, b, r); // r = b - r

  matrix_copy(n, r, rr); // rr = r
	
  float bnrm;
	
  bnrm = sqrt(matrix_DotProduct(n, b, b));
	
  solveX(n, r, z); // z = ~A(-1) * r = -0.25 * r
	
#define TOL 0.01
#define ITMAX 40
  int iter = 0;

  float bknum=0;
  int j;
  float bk=0;
  float bkden = 0;
  float akden = 0;
  float ak = 0;
  float err = 0;
	
  while(iter <= ITMAX){

    if( progress_cb != NULL )
      progress_cb( iter*100/ITMAX );    
          
    iter++;
		
    solveX(n, rr, zz); // zz = ~A(-1) * rr = -0.25 * rr
		
    bknum = matrix_DotProduct(n, z, rr);
		
    if(iter == 1){
      matrix_copy(n, z, p);
      matrix_copy(n, zz, pp); 
    }
    else{
      bk = bknum / bkden; // beta = ...
      matrix_multiply_const(n, p, bk); // p = bk * p
      matrix_add(n, z, p); // p = p + z
      matrix_multiply_const(n, pp, bk); // pp = bk * pp
      matrix_add(n, zz, pp); // pp = pp + zz
    }
		
    bkden = bknum; // numerato becomes the dominator for the next iteration

    multiplyA(pyramid_tmp, pyramid, p, z); // z = A*p = divergence(p)
		
    akden = matrix_DotProduct(n, z, pp);  // alfa denominator
		
    ak = bknum / akden; // alfa = ...
		
    multiplyA(pyramid_tmp, pyramid, pp, zz); // zz = A*pp = divergence(pp)

    // values for the next iterration
    for(j=0;j<n;j++){
      x[j] = x[j] + ak * p[j];	// x = x + alfa * p
      r[j] = r[j] - ak * z[j];		// r = r - alfa * z
      rr[j] = rr[j] - ak * zz[j];	//rr = rr - alfa * zz
    }
	
    solveX(n, r, z); // z = ~A(-1) * r
		
    err = sqrt(matrix_DotProduct(n, r, r));

    //fprintf(stderr, "iter:%d err:%f\n", iter, err);
    if(err <= TOL)
      break;
  }

  if( iter <= ITMAX && progress_cb != NULL )
    progress_cb( 100 );    
  
  matrix_free(p);
  matrix_free(pp);
  matrix_free(z);
  matrix_free(zz);
  matrix_free(r);
  matrix_free(rr);

  pyramid_free(pyramid_tmp);
	
  return x;
}

// rescale matrix from linear to logarithmic scale
void ContrastDomain::matrix_log10(int n, float* m){

  for(int j=0;j<n;j++)
    m[j] = log10f(m[j]);
		
  return;
}

// in_tab and out_tab should contain inscresing float values
float ContrastDomain::lookup_table(int n, float* in_tab, float* out_tab, float val){

  float ret=-1;
  float dd;

  if(val < in_tab[0])
    ret = out_tab[0];
  else
    if(val > in_tab[n-1])
      ret = out_tab[n-1];		
    else
      for(int j=0;j<(n-1);j++){
	
        if(in_tab[j] == val)
          ret = out_tab[j];
        else
          if( val > in_tab[j] && val < in_tab[j+1]){
            dd = (val - in_tab[j]) / (in_tab[j+1] - in_tab[j]);
            ret = out_tab[j] + (out_tab[j+1] - out_tab[j]) * dd;
          }
      }
	
  return ret;
}


// transform gradient (Gx,Gy) to R
void ContrastDomain::transform_to_R(int n, float* G){

  int sign;
  float absG;
  for(int j=0;j<n;j++){
	
    // G to W
    absG = fabs(G[j]);
    if(G[j] < 0)
      sign = -1;
    else
      sign = 1;	
    G[j] = (powf(10.0, absG) - 1.0) * (float)sign;
		
    // W to RESP
    if(G[j] < 0)
      sign = -1;
    else
      sign = 1;	
					
    G[j] = (float)sign * lookup_table(LOOKUP_W_TO_R, W_table, R_table, fabs(G[j]));
  }
  return;
}

// transform gradient (Gx,Gy) to R for the whole pyramid
void ContrastDomain::pyramid_transform_to_R(PYRAMID* pyramid){

  if(pyramid->next != NULL)
    pyramid_transform_to_R((PYRAMID*)pyramid->next);
		
  PYRAMID_LEVEL* pl = pyramid->level;
  transform_to_R(pl->rows*pl->cols, pl->Gx);	
  transform_to_R(pl->rows*pl->cols, pl->Gy);	

  return;
}

// transform from R to G
void ContrastDomain::transform_to_G(int n, float* R){

  int sign;
  for(int j=0;j<n;j++){
	
    // RESP to W
    if(R[j] < 0)
      sign = -1;
    else
      sign = 1;			
    R[j] = sign * lookup_table(LOOKUP_W_TO_R, R_table, W_table, fabs(R[j]));
	
    // W to G
    if(R[j] < 0)
      sign = -1;
    else
      sign = 1;	
    R[j] = log10f(fabs(R[j]) + 1.0) * sign;
		
  }
  return;
}

// transform from R to G for teh pyramid
void ContrastDomain::pyramid_transform_to_G(PYRAMID* pyramid){

  if(pyramid->next != NULL)
    pyramid_transform_to_G((PYRAMID*)pyramid->next);
		
  PYRAMID_LEVEL* pl = pyramid->level;
  transform_to_G(pl->rows*pl->cols, pl->Gx);	
  transform_to_G(pl->rows*pl->cols, pl->Gy);	

  return;
}

// multiply gradient (Gx,Gy) values by float scalar value for the whole pyramid
void ContrastDomain::pyramid_gradient_multiply(PYRAMID* pyramid, float val){

  if(pyramid->next != NULL)
    pyramid_gradient_multiply((PYRAMID*)pyramid->next, val);
		
  PYRAMID_LEVEL* pl = pyramid->level;
  matrix_multiply_const(pl->rows*pl->cols, pl->Gx, val);	
  matrix_multiply_const(pl->rows*pl->cols, pl->Gy, val);	
		
  return;
}

int sort_median(const void* v1, const void* v2){

  float* f1 = (float*)v1;
  float* f2 = (float*)v2;

  if(f1[0] < f2[0])
    return -1;
  else
    if(f1[0] == f2[0])
      return 0;
    else
      return 1;
}

float ContrastDomain::matrix_median(int n, float* m){

  float* temp = matrix_alloc(n);
  matrix_copy(n, m, temp);
	
  qsort(temp, n, sizeof(float), sort_median);

  float val = (temp[(int)(n/2)] + temp[(int)(n/2+1)]) / 2.0;

  matrix_free(temp);

  return val;
}



// transform gradients to luminance
float* ContrastDomain::transform_to_luminance(PYRAMID* pp){

  pyramid_calculate_scale_factor(pp); // calculate (Cx,Cy)
  pyramid_scale_gradient(pp); // scale small gradients by (Cx,Cy);
  pyramid_calculate_divergence(pp); // calculate divergence for the pyramid
	
  float* b = matrix_alloc(pp->level->cols * pp->level->rows);
  pyramid_calculate_divergence_sum(pp, b); // calculate the sum od divergences (equal to b)
	
  float* x = linbcg(pp, b); // calculate luminances from gradients
	
  matrix_free(b);
	
  return x;
}


void ContrastDomain::contrast_equalization( PYRAMID *pp )
{
    // Histogram parameters
    const int hist_size = 2048;
    const float x_min = -2;
    const float x_max = 4;
    
    // Build histogram
    float hist[hist_size];
    memset( hist, 0, sizeof( hist ) );
    PYRAMID *l = pp;
    while( l != NULL ) {
      PYRAMID_LEVEL *lev = l->level;
      const int pixels = lev->rows*lev->cols;
      lev->M = (float*)malloc(sizeof(float)*pixels);
      for( int c = 0; c < pixels; c++ ) {
        lev->M[c] = log10( sqrt( lev->Gx[c]*lev->Gx[c] + lev->Gy[c]*lev->Gy[c] ) );
        int bin = (int)((lev->M[c]-x_min)/(x_max-x_min)*hist_size+0.5);
        if( bin < 0 ) bin = 0;
        else if( bin >= hist_size ) bin = hist_size;
        hist[bin]++;
      } 
      l = l->next;
    }

    // Compute cummulative histogram
    hist[0] = 0;
    for( int i = 1; i < hist_size; i++ )
      hist[i] = hist[i-1] + hist[i];

    // Renormalize cummulative histogram
    float norm = hist[hist_size-1]*0.8;
    for( int i = 1; i < hist_size; i++ )
      hist[i] /= norm;
    
    //Remap gradient magnitudes
    l = pp;
    while( l != NULL ) {
      PYRAMID_LEVEL *lev = l->level;
      const int pixels = lev->rows*lev->cols;
      for( int c = 0; c < pixels; c++ ) {
        int bin = (int)((lev->M[c]-x_min)/(x_max-x_min)*hist_size+0.5);
        if( bin < 0 ) bin = 0;
        else if( bin >= hist_size ) bin = hist_size;
        float scale = hist[bin]/pow(10,lev->M[c]);
        lev->Gx[c] *= scale;
        lev->Gy[c] *= scale;        
      } 
      free( lev->M );
      lev->M = NULL;
      l = l->next;
    }
}


// tone mapping
void ContrastDomain::tone_mapping(int c, int r, float* R, float* G, float* B, float* Y, float contrastFactor, float saturationFactor, progress_callback progress_cb ){

  this->progress_cb = progress_cb;
  
  int n = c*r;

  const float clip_min = 1e-5;
  
  for(int j=0;j<n;j++){

    // clipping
    if( R[j] < clip_min ) R[j] = clip_min;
    if( G[j] < clip_min ) G[j] = clip_min;
    if( B[j] < clip_min ) B[j] = clip_min;
    if( Y[j] < clip_min ) Y[j] = clip_min;    
	
    R[j] = R[j] / Y[j];
    G[j] = G[j] / Y[j];
    B[j] = B[j] / Y[j];
  }
	
  matrix_log10(n, Y); // transform Y to logaritmic scale
	
  PYRAMID* pp = pyramid_allocate(c,r); // create pyramid
  pyramid_calculate_gradient(pp,Y); // calculate gradiens for pyramid
	
  pyramid_transform_to_R(pp); // transform gradients to R


  if( contrastFactor != 0 ) {
    // Contrast mapping
    pyramid_gradient_multiply(pp, contrastFactor); // scale gradient
  } else {
    // Contrast equalization
    contrast_equalization( pp );
  }
	
  pyramid_transform_to_G(pp); // transform R to gradients
	
  float* x = transform_to_luminance(pp); // transform gradients to luminance Y
	
// 	for (int i=0;i<rows*cols;i++) {
// 		fprintf(stderr,"v=%f\n", (*Yo)(i));
// 	}
  float* temp = matrix_alloc(n);
	
  matrix_copy(n, x, temp); // copy x to temp
  qsort(temp, n, sizeof(float), sort_median); // sort temp in ascending order
	
  float median = (temp[(int)((n-1)/2)] + temp[(int)((n-1)/2+1)]) / 2.0; // calculate median
  // c and r should be even
  float CUT_MARGIN = 0.1;
	
  float trim = (n-1) * CUT_MARGIN * 0.01;
  float delta = trim - floor(trim);
  float p_min = temp[(int)floor(trim)] * delta + temp[(int)ceil(trim)] * (1-delta);	

  trim = (n-1) * (100.0 - CUT_MARGIN) * 0.01;
  delta = trim - floor(trim);
  float p_max = temp[(int)floor(trim)] * delta + temp[(int)ceil(trim)] * (1-delta);	
	
  matrix_free(temp);
	

  float d;
  if( (p_max - median) > (median - p_min) )
    d = p_max - median;
  else 
    d = median - p_min;
	
  float l_max = median + d;
  float l_min = median - d;
        
  for(int j=0;j<n;j++){
	
    x[j] = (x[j] - l_min) / (l_max - l_min) * 2.5 - 2.5; // x scaled to <-2.5,0> range
  }
	
  for(int j=0;j<n;j++){	
    Y[j] = powf(10, x[j]);  // transform from logaritmic to linear scale (x <0,10>)
  }
		
  for(int j=0;j<n;j++){		// transform Y to (R,G,B)
    R[j] = powf( R[j], saturationFactor) * Y[j];
    G[j] = powf( G[j], saturationFactor) * Y[j];
    B[j] = powf( B[j], saturationFactor) * Y[j];
  }
  
  matrix_free(x);			
  pyramid_free(pp);
}



void tmo_mantiuk06_contmap( int cols, int rows, float* R, float* G, float* B, float* Y,
  float contrastFactor, float saturationFactor, progress_callback progress_report )
{

  ContrastDomain contrast;	
  contrast.tone_mapping( cols, rows, R, G, B, Y, contrastFactor, saturationFactor, progress_report );  
}



