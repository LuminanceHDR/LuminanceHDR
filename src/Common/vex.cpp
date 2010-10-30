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
 * @author Davide Anastasia, <davide.anastasia@gmail.com>
 *
 */

#include <iostream>
#include "vex.h"

#ifdef __APPLE__
#include <Accelerate/Accelerate.h>
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
//#elif __SSE__
#ifdef __SSE__
  const float* pA = A;
  const float* pB = B;
  float* pC = C;
  
  int LOOP1 = (N >> 4);
  int LOOP2 = (N - (LOOP1 << 4));
  
  __m128 a, b, c;
  
  while (LOOP1--)
  {
    PREFETCH_T0(pA, FETCH_DISTANCE);
    PREFETCH_T0(pB, FETCH_DISTANCE);
    PREFETCH_T0(pC, FETCH_DISTANCE);
    
    a = _mm_load_ps(pA);      pA += 4;
    b = _mm_load_ps(pB);      pB += 4;
    c = _mm_sub_ps(a, b);
    _mm_store_ps(pC, c);      pC += 4;
    
    a = _mm_load_ps(pA);      pA += 4;
    b = _mm_load_ps(pB);      pB += 4;
    c = _mm_sub_ps(a, b);
    _mm_store_ps(pC, c);      pC += 4;
    
    a = _mm_load_ps(pA);      pA += 4;
    b = _mm_load_ps(pB);      pB += 4;
    c = _mm_sub_ps(a, b);
    _mm_store_ps(pC, c);      pC += 4;
    
    a = _mm_load_ps(pA);      pA += 4;
    b = _mm_load_ps(pB);      pB += 4;
    c = _mm_sub_ps(a, b);
    _mm_store_ps(pC, c);      pC += 4;
  }
  
  while (LOOP2--)
  {
    a = _mm_load_ss(pA);      pA ++;
    b = _mm_load_ss(pB);      pB ++;
    c = _mm_sub_ss(a, b);
    _mm_store_ss(pC, c);      pC ++;
  }
#else
  // plain code
  for (int idx = 0; idx < N; idx++ )
  {
    C[idx] = A[idx] - B[idx];
  }
#endif
}

void VEX_vsubs(const float* A, const float val, const float* B, float* C, const int N)
{
#ifdef __SSE__
  const float* pA = A;
  const float* pB = B;
  float* pC = C;
  
  int LOOP1 = (N >> 4);
  int LOOP2 = (N - (LOOP1 << 4));
  
  __m128 a, b, c;
  const __m128 __val = _mm_set1_ps(val);
  
  // 512 bits unrolling
  while (LOOP1--)
  {
    PREFETCH_T0(pA, FETCH_DISTANCE);
    PREFETCH_T0(pB, FETCH_DISTANCE);
    PREFETCH_T0(pC, FETCH_DISTANCE);
    
    a = _mm_load_ps(pA);      pA += 4;
    b = _mm_load_ps(pB);      pB += 4;
    b = _mm_mul_ps(b, __val);
    c = _mm_sub_ps(a, b);
    _mm_store_ps(pC, c);      pC += 4;
    
    a = _mm_load_ps(pA);      pA += 4;
    b = _mm_load_ps(pB);      pB += 4;
    b = _mm_mul_ps(b, __val);
    c = _mm_sub_ps(a, b);
    _mm_store_ps(pC, c);      pC += 4;
    
    a = _mm_load_ps(pA);      pA += 4;
    b = _mm_load_ps(pB);      pB += 4;
    b = _mm_mul_ps(b, __val);
    c = _mm_sub_ps(a, b);
    _mm_store_ps(pC, c);      pC += 4;
    
    a = _mm_load_ps(pA);      pA += 4;
    b = _mm_load_ps(pB);      pB += 4;
    b = _mm_mul_ps(b, __val);
    c = _mm_sub_ps(a, b);
    _mm_store_ps(pC, c);      pC += 4;
  }
  
  while (LOOP2--)
  {
    a = _mm_load_ss(pA);      pA ++;
    b = _mm_load_ss(pB);      pB ++;
    b = _mm_mul_ss(b, __val);
    c = _mm_sub_ss(a, b);
    _mm_store_ss(pC, c);      pC ++;
  }
#else
  // plain code
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
//#elif __SSE__
#ifdef __SSE__
  const float* pA = A;
  const float* pB = B;
  float* pC = C;
  
  int LOOP1 = (N >> 4);
  int LOOP2 = (N - (LOOP1 << 4));
  
  __m128 a, b, c;
  
  while (LOOP1--)
  {
    PREFETCH_T0(pA, FETCH_DISTANCE);
    PREFETCH_T0(pB, FETCH_DISTANCE);
    PREFETCH_T0(pC, FETCH_DISTANCE);
    
    a = _mm_load_ps(pA);      pA += 4;
    b = _mm_load_ps(pB);      pB += 4;
    c = _mm_add_ps(a, b);
    _mm_store_ps(pC, c);      pC += 4;
    
    a = _mm_load_ps(pA);      pA += 4;
    b = _mm_load_ps(pB);      pB += 4;
    c = _mm_add_ps(a, b);
    _mm_store_ps(pC, c);      pC += 4;
    
    a = _mm_load_ps(pA);      pA += 4;
    b = _mm_load_ps(pB);      pB += 4;
    c = _mm_add_ps(a, b);
    _mm_store_ps(pC, c);      pC += 4;
    
    a = _mm_load_ps(pA);      pA += 4;
    b = _mm_load_ps(pB);      pB += 4;
    c = _mm_add_ps(a, b);
    _mm_store_ps(pC, c);      pC += 4;
  }
  
  while (LOOP2--)
  {
    a = _mm_load_ss(pA);      pA ++;
    b = _mm_load_ss(pB);      pB ++;
    c = _mm_add_ss(a, b);
    _mm_store_ss(pC, c);      pC ++;
  }
#else
  // plain code
  for (int idx = 0; idx < N; idx++ )
  {
    C[idx] = A[idx] + B[idx];
  }
#endif
}

void VEX_vadds(const float* A, const float val, const float* B, float* C, const int N)
{
#ifdef __SSE__
  const float* pA = A;
  const float* pB = B;
  float* pC = C;
  
  int LOOP1 = (N >> 4);
  int LOOP2 = (N - (LOOP1 << 4));
  
  const __m128 __val = _mm_set1_ps(val);
  __m128 a, b, c;
  
  while (LOOP1--)
  {
    PREFETCH_T0(pA, FETCH_DISTANCE);
    PREFETCH_T0(pB, FETCH_DISTANCE);
    PREFETCH_T0(pC, FETCH_DISTANCE);
    
    a = _mm_load_ps(pA);      pA += 4;
    b = _mm_load_ps(pB);      pB += 4;
    b = _mm_mul_ps(b, __val);
    c = _mm_add_ps(a, b);
    _mm_store_ps(pC, c);      pC += 4;
    
    a = _mm_load_ps(pA);      pA += 4;
    b = _mm_load_ps(pB);      pB += 4;
    b = _mm_mul_ps(b, __val);
    c = _mm_add_ps(a, b);
    _mm_store_ps(pC, c);      pC += 4;
    
    a = _mm_load_ps(pA);      pA += 4;
    b = _mm_load_ps(pB);      pB += 4;
    b = _mm_mul_ps(b, __val);
    c = _mm_add_ps(a, b);
    _mm_store_ps(pC, c);      pC += 4;
    
    a = _mm_load_ps(pA);      pA += 4;
    b = _mm_load_ps(pB);      pB += 4;
    b = _mm_mul_ps(b, __val);
    c = _mm_add_ps(a, b);
    _mm_store_ps(pC, c);      pC += 4;
  }
  
  while (LOOP2--)
  {
    a = _mm_load_ss(pA);      pA ++;
    b = _mm_load_ss(pB);      pB ++;
    b = _mm_mul_ss(b, __val);
    c = _mm_add_ss(a, b);
    _mm_store_ss(pC, c);      pC ++;
  }
#else
  // plain code
  for (int idx = 0; idx < N; idx++ )
  {
    C[idx] = A[idx] + val * B[idx];
  }
#endif
}

void VEX_vsmul(const float* I, const float val, float* O, const int N)
{
//#ifdef __APPLE__
//  vDSP_vsmul (I, 1, &c, O, 1, N);
//#elif __SSE__
#ifdef __SSE__
  const float* pI = I;
  float* pO = O;
  
  int LOOP1 = (N >> 4);
  int LOOP2 = (N - (LOOP1 << 4));
  
  const __m128 __val = _mm_set1_ps(val);
  __m128 t;
  
  while (LOOP1--)
  {
    PREFETCH_T0(pI, FETCH_DISTANCE);
    PREFETCH_T0(pO, FETCH_DISTANCE);
    
    t = _mm_load_ps(pI);      pI += 4;
    t = _mm_mul_ps(t, __val);
    _mm_store_ps(pO, t);      pO += 4;
    
    t = _mm_load_ps(pI);      pI += 4;
    t = _mm_mul_ps(t, __val);
    _mm_store_ps(pO, t);      pO += 4;
    
    t = _mm_load_ps(pI);      pI += 4;
    t = _mm_mul_ps(t, __val);
    _mm_store_ps(pO, t);      pO += 4;
    
    t = _mm_load_ps(pI);      pI += 4;
    t = _mm_mul_ps(t, __val);
    _mm_store_ps(pO, t);      pO += 4;
  }
  
  while (LOOP2--)
  {
    t = _mm_load_ss(pI);      pI ++;
    t = _mm_mul_ss(t, __val);
    _mm_store_ss(pO, t);      pO ++;
  }
#else 
  // plain code
  for(int idx = 0; idx < N; idx++)
  {
    O[idx] = val * I[idx];
  }
#endif
}

void VEX_vmul(float* A, float* B, float* C, const int N)
{
  //#ifdef __APPLE__  
  //vDSP_vmul(B, 1, A, 1, C, 1, N);
  //#elif __SSE__
#ifdef __SSE__
  const float* pA = A;
  const float* pB = B;
  float* pC = C;
  
  int LOOP1 = (N >> 4);
  int LOOP2 = (N - (LOOP1 << 4));
  
  __m128 a, b, c;
  
  while (LOOP1--)
  {
    PREFETCH_T0(pA, FETCH_DISTANCE);
    PREFETCH_T0(pB, FETCH_DISTANCE);
    PREFETCH_T0(pC, FETCH_DISTANCE);
    
    a = _mm_load_ps(pA);      pA += 4;
    b = _mm_load_ps(pB);      pB += 4;
    c = _mm_mul_ps(a, b);
    _mm_store_ps(pC, c);      pC += 4;
    
    a = _mm_load_ps(pA);      pA += 4;
    b = _mm_load_ps(pB);      pB += 4;
    c = _mm_mul_ps(a, b);
    _mm_store_ps(pC, c);      pC += 4;
    
    a = _mm_load_ps(pA);      pA += 4;
    b = _mm_load_ps(pB);      pB += 4;
    c = _mm_mul_ps(a, b);
    _mm_store_ps(pC, c);      pC += 4;
    
    a = _mm_load_ps(pA);      pA += 4;
    b = _mm_load_ps(pB);      pB += 4;
    c = _mm_mul_ps(a, b);
    _mm_store_ps(pC, c);      pC += 4;
  }
  
  while (LOOP2--)
  {
    a = _mm_load_ss(pA);      pA ++;
    b = _mm_load_ss(pB);      pB ++;
    c = _mm_mul_ss(a, b);
    _mm_store_ss(pC, c);      pC ++;
  }
#else
  // plain code
  for (int idx = 0; idx < N; idx++ )
  {
    C[idx] = A[idx] * B[idx];
  }
#endif
}

void VEX_vdiv(float* A, float* B, float* C, const int N)
{
//#ifdef __APPLE__  
//  vDSP_vdiv(B, 1, A, 1, C, 1, N);
//#elif __SSE__
#ifdef __SSE__
  const float* pA = A;
  const float* pB = B;
  float* pC = C;
  
  int LOOP1 = (N >> 4);
  int LOOP2 = (N - (LOOP1 << 4));
  
  __m128 a, b, c;
  
  while (LOOP1--)
  {
    PREFETCH_T0(pA, FETCH_DISTANCE);
    PREFETCH_T0(pB, FETCH_DISTANCE);
    PREFETCH_T0(pC, FETCH_DISTANCE);
    
    a = _mm_load_ps(pA);      pA += 4;
    b = _mm_load_ps(pB);      pB += 4;
    c = _mm_div_ps(a, b);
    _mm_store_ps(pC, c);      pC += 4;
    
    a = _mm_load_ps(pA);      pA += 4;
    b = _mm_load_ps(pB);      pB += 4;
    c = _mm_div_ps(a, b);
    _mm_store_ps(pC, c);      pC += 4;
    
    a = _mm_load_ps(pA);      pA += 4;
    b = _mm_load_ps(pB);      pB += 4;
    c = _mm_div_ps(a, b);
    _mm_store_ps(pC, c);      pC += 4;
    
    a = _mm_load_ps(pA);      pA += 4;
    b = _mm_load_ps(pB);      pB += 4;
    c = _mm_div_ps(a, b);
    _mm_store_ps(pC, c);      pC += 4;
  }
  
  while (LOOP2--)
  {
    a = _mm_load_ss(pA);      pA ++;
    b = _mm_load_ss(pB);      pB ++;
    c = _mm_div_ss(a, b);
    _mm_store_ss(pC, c);      pC ++;
  }
#else
  // plain code
  for (int idx = 0; idx < N; idx++ )
  {
    C[idx] = A[idx] / B[idx];
  }
#endif
}

void VEX_vcopy(const float* I, float* O, const int N)
{
#ifdef __SSE__
  const float* pI = I;
  float* pO = O;
  
  int LOOP1 = (N >> 4);
  int LOOP2 = (N - (LOOP1 << 4));
    
  while (LOOP1--)
  {
    PREFETCH_T0(pO, FETCH_DISTANCE);
    PREFETCH_T0(pI, FETCH_DISTANCE);
    
    _mm_store_ps(pO, _mm_load_ps(pI));      pI += 4; pO += 4;
    _mm_store_ps(pO, _mm_load_ps(pI));      pI += 4; pO += 4; 
    _mm_store_ps(pO, _mm_load_ps(pI));      pI += 4; pO += 4; 
    _mm_store_ps(pO, _mm_load_ps(pI));      pI += 4; pO += 4; 
  }
  
  while (LOOP2--)
  {
    _mm_store_ss(pO, _mm_load_ss(pI));      pI ++; pO ++;
  }
  _mm_sfence();
#else 
  // plain code
  for(int idx = 0; idx < N; idx++)
  {
    O[idx] = I[idx];
  }
#endif
}

void VEX_vset(float* IO, const float val, const int N)
{
#ifdef __SSE__
  float* pIO = IO;
  
  int LOOP1 = (N >> 4);
  int LOOP2 = (N - (LOOP1 << 4));

  const __m128 __val = _mm_set1_ps(val);
  
  while (LOOP1--)
  {
    PREFETCH_T0(pIO, FETCH_DISTANCE);
    
    _mm_store_ps(pIO, __val);      pIO += 4;
    _mm_store_ps(pIO, __val);      pIO += 4;
    _mm_store_ps(pIO, __val);      pIO += 4;
    _mm_store_ps(pIO, __val);      pIO += 4;
  }
  
  while (LOOP2--)
  {
    _mm_store_ss(pIO, __val);      pIO ++;
  }
#else 
  // plain code
  for(int idx = 0; idx < N; idx++)
  {
    IO[idx] = val;
  }
#endif
}

void VEX_vreset(float* IO, const int N)
{
#ifdef __SSE__
  float* pIO = IO;
  
  int LOOP1 = (N >> 4);
  int LOOP2 = (N - (LOOP1 << 4));

  const __m128 _zero = _mm_setzero_ps();
  
  while (LOOP1--)
  {
    PREFETCH_T0(pIO, FETCH_DISTANCE);
    
    _mm_store_ps(pIO, _zero);      pIO += 4;
    _mm_store_ps(pIO, _zero);      pIO += 4;
    _mm_store_ps(pIO, _zero);      pIO += 4;
    _mm_store_ps(pIO, _zero);      pIO += 4;
  }
  
  while (LOOP2--)
  {
    _mm_store_ss(pIO, _zero);      pIO ++;
  }
#else 
  // plain code
  for(int idx = 0; idx < N; idx++)
  {
    IO[idx] = 0.0f;
  }
#endif
}

void VEX_dotpr(const float* I1, const float* I2, float& val, const int N)
{
  val = 0.0f;
#ifdef __APPLE__
  vDSP_dotpr(I1, 1, I2, 1, &val, N);
#elif __SSE__
  for (int j=0; j<N; j++)
  {
    val += I1[j] * I2[j];
  }
#else
  for (int j=0; j<N; j++)
  {
    val += I1[j] * I2[j];
  }
#endif
}

//void mm_3x3(float i1, float i2, float i3, float &o1, float &o2, float &o3, const float mat[3][3])
//{
//  const __m128 d      = _mm_set_ps(0.0f, i3, i2, i1);
//  const __m128 mat_r1 = _mm_set_ps(0.0f, mat[0][2], mat[0][1], mat[0][0]);
//  const __m128 mat_r2 = _mm_set_ps(0.0f, mat[1][2], mat[1][1], mat[1][0]);
//  const __m128 mat_r3 = _mm_set_ps(0.0f, mat[2][2], mat[2][1], mat[2][0]);
//  
//  register __m128 t;
//  
//  // row 1
//  t = _mm_mul_ps(d, mat_r1);
//  t = _mm_hadd_ps(t, ZERO);
//  t = _mm_hadd_ps(t, ZERO);
//  _mm_store_ss(&o1, t);
//          
//  // row 2
//  t = _mm_mul_ps(d, mat_r2);
//  t = _mm_hadd_ps(t, ZERO);
//  t = _mm_hadd_ps(t, ZERO);
//  _mm_store_ss(&o2, t);
//  
//  // row 3
//  t = _mm_mul_ps(d, mat_r3);
//  t = _mm_hadd_ps(t, ZERO);
//  t = _mm_hadd_ps(t, ZERO);
//  _mm_store_ss(&o3, t);
//}