/**
 * @brief SSE for high performance vector operations (Vector EXtension - VEX)
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

#ifndef __VEX_H__
#define __VEX_H__

#ifdef __SSE__
//#if __ppc__ || __ppc7400__ || __ppc64__ || __ppc970__
//#include <ppc_intrinsics.h>
#if __i386__ || __x86_64__
//#define __USE_SSE__
#include <mm_malloc.h>
#include <xmmintrin.h>
//#include <pmmintrin.h>
//#include <tmmintrin.h>
#else
#error unsupported architecture
#endif
#endif

// C[i] = A[i] - B[i]
void VEX_vsub(const float* A, const float* B, float* C, const int N);
// C[i] = A[i] - val * B[i]
void VEX_vsubs(const float* A, const float val, const float* B, float* C, const int N);

// C[i] = A[i] + B[i]
void VEX_vadd(const float* A, const float* B, float* C, const int N);
// C[i] = A[i] + val * B[i]
void VEX_vadds(const float* A, const float val, const float* B, float* C, const int N);

// C[i] = A[i] * B[i]
void VEX_vmul(const float* A, const float* B, float* C, const int N);
// O[i] = c * I[i]
void VEX_vsmul(const float* I, const float c, float* O, const int N);

// C[i] = A[i] / B[i]
void VEX_vdiv(float* A, float* B, float* C, const int N);

// O[i] = i[i]
void VEX_vcopy(const float* I, float* O, const int N);

// IO[i] = val
void VEX_vset(float* IO, const float val, const int N);

// IO[i] = 0.0f
void VEX_vreset(float* IO, const int N);

void VEX_dotpr(const float* I1, const float* I2, float& val, const int N);

//void mm_3x3(float i1, float i2, float i3, float &o1, float &o2, float &o3, const float mat[3][3]);
#endif
