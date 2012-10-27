/*
 * This file is a part of Luminance HDR package
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2011 Davide Anastasia
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
 */

//! @brief SSE for high performance vector operations (Vector EXtension - VEX)
//! @author Davide Anastasia, <davideanastasia@users.sourceforge.net>

#ifndef VEX_H
#define VEX_H

#ifdef __SSE__

//#if __ppc__ || __ppc7400__ || __ppc64__ || __ppc970__
//#include <ppc_intrinsics.h>
#if __i386__ || __x86_64__
// #define LUMINANCE_USE_SSE
#include <mm_malloc.h>
#include <xmmintrin.h>
#include <mm_malloc.h>
//#include <pmmintrin.h>
//#include <tmmintrin.h>

#else
#error unsupported architecture
#endif

#endif // __SSE__

// O[i] = c * I[i]
void VEX_vsmul(const float* I, const float c, float* O, const int N);

// O[i] = i[i]
void VEX_vcopy(const float* I, float* O, const int N);

// IO[i] = value
void VEX_vset(float* IO, const float value, const int N);

// IO[i] = 0.0f
void VEX_vreset(float* IO, const int N);

#ifdef LUMINANCE_USE_SSE
typedef __v4sf v4sf;
v4sf _mm_log2_ps(v4sf);
v4sf _mm_exp2_ps(v4sf);
v4sf _mm_pow_ps(v4sf, v4sf);
#endif

#endif
