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

#include "sse.h"

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