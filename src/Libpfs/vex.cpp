/**
 * @brief SSE for high performance vector operations
 *
 * This file is a part of LuminanceHDR package
 * ---------------------------------------------------------------------- 
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
 * @author Davide Anastasia, <davideanastasia@users.sourceforge.net>
 *
 */

#include <iostream>
#include "vex.h"

#ifdef _OPENMP
#include <omp.h>
#endif

// Prefetch definition taken from:
// http://software.intel.com/en-us/forums/showthread.php?t=46284
// tune FETCH_DISTANCE according to real world experiments
#define PREFETCH_T0(addr, nrOfBytesAhead) _mm_prefetch(((char *)(addr))+nrOfBytesAhead, _MM_HINT_T0)
#define FETCH_DISTANCE        512 

void VEX_vsub(const float* A, const float* B, float* C, const int N)
{
  //#ifdef __APPLE__  
  //  vDSP_vsub(B, 1, A, 1, C, 1, N); // http://developer.apple.com/hardwaredrivers/ve/errata.html#vsub
  //#elif __USE_SSE__
#ifdef __USE_SSE__
  __m128 a, b, c;
  
  const int LOOP1       = (N >> 4);
  const int ELEMS_LOOP1 = (LOOP1 << 4);
  const int LOOP2       = (N - ELEMS_LOOP1);
  
#pragma omp parallel for schedule(static, 5120) private(a,b,c)
  for (int l = 0; l < ELEMS_LOOP1; l+=16)
  {
    PREFETCH_T0(&A[l], FETCH_DISTANCE);
    PREFETCH_T0(&B[l], FETCH_DISTANCE);
    PREFETCH_T0(&C[l], FETCH_DISTANCE);
    
    a = _mm_load_ps(&A[l]);
    b = _mm_load_ps(&B[l]);
    c = _mm_sub_ps(a, b);
    _mm_store_ps(&C[l], c);
    
    a = _mm_load_ps(&A[l+4]);
    b = _mm_load_ps(&B[l+4]);
    c = _mm_sub_ps(a, b);
    _mm_store_ps(&C[l+4], c);
    
    a = _mm_load_ps(&A[l+8]);
    b = _mm_load_ps(&B[l+8]);
    c = _mm_sub_ps(a, b);
    _mm_store_ps(&C[l+8], c);
    
    a = _mm_load_ps(&A[l+12]);
    b = _mm_load_ps(&B[l+12]);
    c = _mm_sub_ps(a, b);
    _mm_store_ps(&C[l+12], c);
  }
  
  const float* pA = &A[ELEMS_LOOP1];
  const float* pB = &B[ELEMS_LOOP1];
  float* pC       = &C[ELEMS_LOOP1];
  
  for (int l = 0; l < LOOP2; l++)
  {
    a = _mm_load_ss(&pA[l]);
    b = _mm_load_ss(&pB[l]);
    c = _mm_sub_ss(a, b);
    _mm_store_ss(&pC[l], c);
  }
#else
  // plain code
#pragma omp parallel for schedule(static, 5120)
  for (int idx = 0; idx < N; idx++ )
  {
    C[idx] = A[idx] - B[idx];
  }
#endif
}

void VEX_vsubs(const float* A, const float val, const float* B, float* C, const int N)
{
#ifdef __USE_SSE__  
  __m128 a, b, c;
  const __m128 __val = _mm_set1_ps(val);
  
  const int LOOP1       = (N >> 4);
  const int ELEMS_LOOP1 = (LOOP1 << 4);
  const int LOOP2       = (N - ELEMS_LOOP1);
  
#pragma omp parallel for schedule(static, 5120) private(a,b,c)
  for (int l = 0; l < ELEMS_LOOP1; l+=16)
  {
    PREFETCH_T0(&A[l], FETCH_DISTANCE);
    PREFETCH_T0(&B[l], FETCH_DISTANCE);
    PREFETCH_T0(&C[l], FETCH_DISTANCE);
    
    a = _mm_load_ps(&A[l]);
    b = _mm_load_ps(&B[l]);
    b = _mm_mul_ps(b, __val);
    c = _mm_sub_ps(a, b);
    _mm_store_ps(&C[l], c);
    
    a = _mm_load_ps(&A[l+4]);
    b = _mm_load_ps(&B[l+4]);
    b = _mm_mul_ps(b, __val);
    c = _mm_sub_ps(a, b);
    _mm_store_ps(&C[l+4], c);
    
    a = _mm_load_ps(&A[l+8]);
    b = _mm_load_ps(&B[l+8]);
    b = _mm_mul_ps(b, __val);
    c = _mm_sub_ps(a, b);
    _mm_store_ps(&C[l+8], c);
    
    a = _mm_load_ps(&A[l+12]);
    b = _mm_load_ps(&B[l+12]);
    b = _mm_mul_ps(b, __val);
    c = _mm_sub_ps(a, b);
    _mm_store_ps(&C[l+12], c);
  }
  
  const float* pA = &A[ELEMS_LOOP1];
  const float* pB = &B[ELEMS_LOOP1];
  float* pC       = &C[ELEMS_LOOP1];
  
  for (int l = 0; l < LOOP2; l++)
  {
    a = _mm_load_ss(&pA[l]);
    b = _mm_load_ss(&pB[l]);
    b = _mm_mul_ss(b, __val);
    c = _mm_sub_ss(a, b);
    _mm_store_ss(&pC[l], c);
  }
#else
  // plain code
#pragma omp parallel for schedule(static, 5120)
  for (int idx = 0; idx < N; idx++ )
  {
    C[idx] = A[idx] - val * B[idx];
  }
#endif
}

void VEX_vadd(const float* A, const float* B, float* C, const int N)
{
  //#ifdef __APPLE__  
  //  vDSP_vadd(A, 1, B, 1, C, 1, N);
  //#elif __USE_SSE__
#ifdef __USE_SSE__
  __m128 a, b, c;
  
  const int LOOP1       = (N >> 4);
  const int ELEMS_LOOP1 = (LOOP1 << 4);
  const int LOOP2       = (N - ELEMS_LOOP1);
  
#pragma omp parallel for schedule(static, 5120) private(a,b,c)
  for (int l = 0; l < ELEMS_LOOP1; l+=16)
  {
    PREFETCH_T0(&A[l], FETCH_DISTANCE);
    PREFETCH_T0(&B[l], FETCH_DISTANCE);
    PREFETCH_T0(&C[l], FETCH_DISTANCE);
    
    a = _mm_load_ps(&A[l]);
    b = _mm_load_ps(&B[l]);
    c = _mm_add_ps(a, b);
    _mm_store_ps(&C[l], c);
    
    a = _mm_load_ps(&A[l+4]);
    b = _mm_load_ps(&B[l+4]);
    c = _mm_add_ps(a, b);
    _mm_store_ps(&C[l+4], c);
    
    a = _mm_load_ps(&A[l+8]);
    b = _mm_load_ps(&B[l+8]);
    c = _mm_add_ps(a, b);
    _mm_store_ps(&C[l+8], c);
    
    a = _mm_load_ps(&A[l+12]);
    b = _mm_load_ps(&B[l+12]);
    c = _mm_add_ps(a, b);
    _mm_store_ps(&C[l+12], c);
  }
  
  const float* pA = &A[ELEMS_LOOP1];
  const float* pB = &B[ELEMS_LOOP1];
  float* pC       = &C[ELEMS_LOOP1];
  
  for (int l = 0; l < LOOP2; l++)
  {
    a = _mm_load_ss(&pA[l]);
    b = _mm_load_ss(&pB[l]);
    c = _mm_add_ss(a, b);
    _mm_store_ss(&pC[l], c);
  }
#else
  // plain code
#pragma omp parallel for schedule(static, 5120)
  for (int idx = 0; idx < N; idx++ )
  {
    C[idx] = A[idx] + B[idx];
  }
#endif
}

void VEX_vadds(const float* A, const float val, const float* B, float* C, const int N)
{
#ifdef __USE_SSE__  
  const __m128 __val = _mm_set1_ps(val);
  __m128 a, b, c;
  
  const int LOOP1       = (N >> 4);
  const int ELEMS_LOOP1 = (LOOP1 << 4);
  const int LOOP2       = (N - ELEMS_LOOP1);
  
#pragma omp parallel for schedule(static, 5120) private(a,b,c)
  for (int l = 0; l < ELEMS_LOOP1; l+=16)
  {
    PREFETCH_T0(&A[l], FETCH_DISTANCE);
    PREFETCH_T0(&B[l], FETCH_DISTANCE);
    PREFETCH_T0(&C[l], FETCH_DISTANCE);
    
    a = _mm_load_ps(&A[l]);
    b = _mm_load_ps(&B[l]);
    b = _mm_mul_ps(b, __val);
    c = _mm_add_ps(a, b);
    _mm_store_ps(&C[l], c);
    
    a = _mm_load_ps(&A[l+4]);
    b = _mm_load_ps(&B[l+4]);
    b = _mm_mul_ps(b, __val);
    c = _mm_add_ps(a, b);
    _mm_store_ps(&C[l+4], c);
    
    a = _mm_load_ps(&A[l+8]);
    b = _mm_load_ps(&B[l+8]);
    b = _mm_mul_ps(b, __val);
    c = _mm_add_ps(a, b);
    _mm_store_ps(&C[l+8], c);
    
    a = _mm_load_ps(&A[l+12]);
    b = _mm_load_ps(&B[l+12]);
    b = _mm_mul_ps(b, __val);
    c = _mm_add_ps(a, b);
    _mm_store_ps(&C[l+12], c);
  }
  
  const float* pA = &A[ELEMS_LOOP1];
  const float* pB = &B[ELEMS_LOOP1];
  float* pC = &C[ELEMS_LOOP1];
  
  for (int l = 0; l < LOOP2; l++)
  {
    a = _mm_load_ss(&pA[l]);
    b = _mm_load_ss(&pB[l]);
    b = _mm_mul_ss(b, __val);
    c = _mm_add_ss(a, b);
    _mm_store_ss(&pC[l], c);
  }
#else
  // plain code
#pragma omp parallel for schedule(static, 5120)
  for (int idx = 0; idx < N; idx++ )
  {
    C[idx] = A[idx] + val * B[idx];
  }
#endif
}

void VEX_vsmul(const float* I, const float val, float* O, const int N)
{
#ifdef __USE_SSE__
  const __m128 __val = _mm_set1_ps(val);
  __m128 t;
  
  const int LOOP1       = (N >> 4);
  const int ELEMS_LOOP1 = (LOOP1 << 4);
  const int LOOP2       = (N - ELEMS_LOOP1);
  
#pragma omp parallel for schedule(static, 5120) private(t)
  for (int l = 0; l < ELEMS_LOOP1; l+=16)
  {
    PREFETCH_T0(&I[l], FETCH_DISTANCE);
    PREFETCH_T0(&O[l], FETCH_DISTANCE);
    
    t = _mm_load_ps(&I[l]);
    t = _mm_mul_ps(t, __val);
    _mm_store_ps(&O[l], t);
    
    t = _mm_load_ps(&I[l+4]);
    t = _mm_mul_ps(t, __val);
    _mm_store_ps(&O[l+4], t);
    
    t = _mm_load_ps(&I[l+8]);
    t = _mm_mul_ps(t, __val);
    _mm_store_ps(&O[l+8], t);
    
    t = _mm_load_ps(&I[l+12]);
    t = _mm_mul_ps(t, __val);
    _mm_store_ps(&O[l+12], t);
  }
  
  const float* pI = &I[ELEMS_LOOP1];
  float* pO       = &O[ELEMS_LOOP1];
  
  for (int l = 0; l < LOOP2; l++)
  {
    t = _mm_load_ss(&pI[l]);
    t = _mm_mul_ss(t, __val);
    _mm_store_ss(&pO[l], t);
  }
#else 
  // plain code
#pragma omp parallel for schedule(static, 5120)
  for(int idx = 0; idx < N; idx++)
  {
    O[idx] = val * I[idx];
  }
#endif
}

void VEX_vmul(const float* A, const float* B, float* C, const int N)
{
#ifdef __USE_SSE__
  __m128 a, b;
  
  const int LOOP1       = (N >> 4);
  const int ELEMS_LOOP1 = (LOOP1 << 4);
  const int LOOP2       = (N - ELEMS_LOOP1);
  
#pragma omp parallel for schedule(static, 5120) private(a, b)
  for (int l = 0; l < ELEMS_LOOP1; l+=16)
  {
    PREFETCH_T0(&A[l], FETCH_DISTANCE);
    PREFETCH_T0(&B[l], FETCH_DISTANCE);
    PREFETCH_T0(&C[l], FETCH_DISTANCE);
    
    a = _mm_load_ps(&A[l]);
    b = _mm_load_ps(&B[l]);
    a = _mm_mul_ps(a, b);
    _mm_store_ps(&C[l], a);
    
    a = _mm_load_ps(&A[l+4]);
    b = _mm_load_ps(&B[l+4]);
    a = _mm_mul_ps(a, b);
    _mm_store_ps(&C[l+4], a);
    
    a = _mm_load_ps(&A[l+8]);
    b = _mm_load_ps(&B[l+8]);
    a = _mm_mul_ps(a, b);
    _mm_store_ps(&C[l+8], a);
    
    a = _mm_load_ps(&A[l+12]);
    b = _mm_load_ps(&B[l+12]);
    a = _mm_mul_ps(a, b);
    _mm_store_ps(&C[l+12], a);
  }
  
  const float* pA = &A[ELEMS_LOOP1];
  const float* pB = &B[ELEMS_LOOP1];
  float* pC       = &C[ELEMS_LOOP1];
  
  for (int l = 0; l < LOOP2; l++)
  {
    a = _mm_load_ss(&pA[l]);
    b = _mm_load_ss(&pB[l]);
    a = _mm_mul_ss(a, b);
    _mm_store_ss(&pC[l], a);
  }
#else
  // plain code
#pragma omp parallel for schedule(static, 5120)
  for (int idx = 0; idx < N; idx++ )
  {
    C[idx] = A[idx] * B[idx];
  }
#endif
}

void VEX_vdiv(const float* A, const float* B, float* C, const int N)
{
#ifdef __USE_SSE__   
  __m128 a, b;
  
  const int LOOP1       = (N >> 4);
  const int ELEMS_LOOP1 = (LOOP1 << 4);
  const int LOOP2       = (N - ELEMS_LOOP1);
  
#pragma omp parallel for schedule(static, 5120) private(a, b)
  for (int l = 0; l < ELEMS_LOOP1; l+=16)
  {
    PREFETCH_T0(&A[l], FETCH_DISTANCE);
    PREFETCH_T0(&B[l], FETCH_DISTANCE);
    PREFETCH_T0(&C[l], FETCH_DISTANCE);
    
    a = _mm_load_ps(&A[l]);
    b = _mm_load_ps(&B[l]);
    a = _mm_div_ps(a, b);
    _mm_store_ps(&C[l], a);
    
    a = _mm_load_ps(&A[l+4]);
    b = _mm_load_ps(&B[l+4]);
    a = _mm_div_ps(a, b);
    _mm_store_ps(&C[l+4], a);
    
    a = _mm_load_ps(&A[l+8]);
    b = _mm_load_ps(&B[l+8]);
    a = _mm_div_ps(a, b);
    _mm_store_ps(&C[l+8], a);
    
    a = _mm_load_ps(&A[l+12]);
    b = _mm_load_ps(&B[l+12]);
    a = _mm_div_ps(a, b);
    _mm_store_ps(&C[l+12], a);
  }
  
  const float* pA = &A[ELEMS_LOOP1];
  const float* pB = &B[ELEMS_LOOP1];
  float* pC       = &C[ELEMS_LOOP1];
  
  for (int l = 0; l < LOOP2; l++)
  {
    a = _mm_load_ss(&pA[l]);
    b = _mm_load_ss(&pB[l]);
    a = _mm_div_ss(a, b);
    _mm_store_ss(&pC[l], a);
  }
#else
  // plain code
#pragma omp parallel for schedule(static, 5120)
  for (int idx = 0; idx < N; idx++ )
  {
    C[idx] = A[idx] / B[idx];
  }
#endif
}

void VEX_vcopy(const float* I, float* O, const int N)
{
#ifdef __USE_SSE__ 
  const int LOOP1       = (N >> 4);
  const int ELEMS_LOOP1 = (LOOP1 << 4);
  const int LOOP2       = (N - ELEMS_LOOP1);
  
#pragma omp parallel for schedule(static, 5120)
  for (int l = 0; l < ELEMS_LOOP1; l+=16)
  {
    PREFETCH_T0(&O[l], FETCH_DISTANCE);
    PREFETCH_T0(&I[l], FETCH_DISTANCE);
    
    _mm_store_ps(&O[l],    _mm_load_ps(&I[l]));
    _mm_store_ps(&O[l+4],  _mm_load_ps(&I[l+4]));
    _mm_store_ps(&O[l+8],  _mm_load_ps(&I[l+8]));
    _mm_store_ps(&O[l+12], _mm_load_ps(&I[l+12]));
  }
  
  const float* pI = &I[ELEMS_LOOP1];
  float* pO       = &O[ELEMS_LOOP1];
  
  for (int l = 0; l < LOOP2; l++)
  {
    _mm_store_ss(&pO[l], _mm_load_ss(&pI[l]));
  }
  _mm_sfence();
#else 
  // plain code
#pragma omp parallel for schedule(static, 5120)
  for(int idx = 0; idx < N; idx++)
  {
    O[idx] = I[idx];
  }
#endif
}

void VEX_vset(float* IO, const float val, const int N)
{
#ifdef __USE_SSE__
  const int LOOP1       = (N >> 4);
  const int ELEMS_LOOP1 = (LOOP1 << 4);
  const int LOOP2       = (N - ELEMS_LOOP1);
  
  const __m128 __val = _mm_set1_ps(val);
  
#pragma omp parallel for schedule(static, 5120)
  for (int l = 0; l < ELEMS_LOOP1; l+=16)
  {
    PREFETCH_T0(&IO[l], FETCH_DISTANCE);
    
    _mm_store_ps(&IO[l], __val);
    _mm_store_ps(&IO[l+4], __val);
    _mm_store_ps(&IO[l+8], __val);
    _mm_store_ps(&IO[l+12], __val);
  }
  
  float* pIO = &IO[ELEMS_LOOP1];
  
  for (int l = 0; l < LOOP2; l++)
  {
    _mm_store_ss(&pIO[l], __val);
  }
#else 
  // plain code
#pragma omp parallel for schedule(static, 5120)
  for(int idx = 0; idx < N; idx++)
  {
    IO[idx] = val;
  }
#endif
}

void VEX_vreset(float* IO, const int N)
{
#ifdef __USE_SSE__
  const int LOOP1       = (N >> 4);
  const int ELEMS_LOOP1 = (LOOP1 << 4);
  const int LOOP2       = (N - ELEMS_LOOP1);
  
  const __m128 _zero = _mm_setzero_ps();
  
#pragma omp parallel for schedule(static, 5120)
  for (int l = 0; l < ELEMS_LOOP1; l+=16)
  {
    PREFETCH_T0(&IO[l], FETCH_DISTANCE);
    
    _mm_store_ps(&IO[l], _zero);
    _mm_store_ps(&IO[l+4], _zero);
    _mm_store_ps(&IO[l+8], _zero);
    _mm_store_ps(&IO[l+12], _zero);
  }
  
  float* pIO = &IO[ELEMS_LOOP1];
  
  for (int l = 0; l < LOOP2; l++)
  {
    _mm_store_ss(&pIO[l], _zero);
  }
#else 
  // plain code
#pragma omp parallel for schedule(static, 5120)
  for (int idx = 0; idx < N; idx++)
  {
    IO[idx] = 0.0f;
  }
#endif
}

void VEX_dotpr(const float* I1, const float* I2, float& val, const int N)
{
  float t_val = 0.0f;
#pragma omp parallel for reduction(+:t_val)
  for (int idx = 0; idx < N; idx++)
  {
    t_val += I1[idx] * I2[idx];
  }
  val = t_val;
}
