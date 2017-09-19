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

//! @brief SSE for high performance vector operations
//! @author Davide Anastasia, <davideanastasia@users.sourceforge.net>

#ifndef PFS_UTILS_SSE_H
#define PFS_UTILS_SSE_H

namespace pfs {
namespace utils {

#ifdef __SSE__

//#if __ppc__ || __ppc7400__ || __ppc64__ || __ppc970__
//#include <ppc_intrinsics.h>
#if __i386__ || __x86_64__
// #define LUMINANCE_USE_SSE
#include <mm_malloc.h>
#include <xmmintrin.h>
//#include <pmmintrin.h>
//#include <tmmintrin.h>

#else
#error unsupported architecture
#endif

#endif  // __SSE__

#ifdef LUMINANCE_USE_SSE
typedef __v4sf v4sf;
v4sf _mm_log2_ps(v4sf);
v4sf _mm_exp2_ps(v4sf);
v4sf _mm_pow_ps(v4sf, v4sf);
#endif

}  // utils
}  // pfs

#endif  // PFS_UTILS_SSE_H
