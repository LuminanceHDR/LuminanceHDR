/*
* This file is a part of Luminance HDR package.
* ----------------------------------------------------------------------
* Copyright (C) 2011-2012 Davide Anastasia
*
*  This library is free software; you can redistribute it and/or
*  modify it under the terms of the GNU Lesser General Public
*  License as published by the Free Software Foundation; either
*  version 2.1 of the License, or (at your option) any later version.
*
*  This library is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*  Lesser General Public License for more details.
*
*  You should have received a copy of the GNU Lesser General Public
*  License along with this library; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
* ----------------------------------------------------------------------
*/

#ifndef VEX_VEX_HXX
#define VEX_VEX_HXX

#include "vex.h"

#include <algorithm>
#include <numeric>
#include <functional>

namespace vex
{
namespace detail
{

template<typename _Type, typename _Op>
inline
void op(const _Type* A, const _Type* B, _Type* C, size_t size, const _Op& currOp)
{
#pragma omp parallel for
    for (int idx = 0; idx < static_cast<int>(size); idx++)
    {
        C[idx] = currOp(A[idx], B[idx]);
    }
}

}

template <typename _Type>
void vmul(const _Type* A, const _Type* B, _Type* C, size_t size)
{
    detail::op(A, B, C, size, std::multiplies<_Type>());
}

template <typename _Type>
void vdiv(const _Type* A, const _Type* B, _Type* C, size_t size)
{
    detail::op(A, B, C, size, std::divides<_Type>());
}

template <typename _Type>
void vadd(const _Type* A, const _Type* B, _Type* C, size_t size)
{
    detail::op(A, B, C, size, std::plus<_Type>());
}

template <typename _Type>
void vadds(const _Type* A, const _Type& s, const _Type* B, _Type* C, size_t size)
{
    detail::op(A, B, C, size, numeric::vadds<_Type>(s));
}

template <typename _Type>
void vsub(const _Type* A, const _Type* B, _Type* C, size_t size)
{
    detail::op(A, B, C, size, std::minus<_Type>());
}

template <typename _Type>
void vsubs(const _Type* A, const _Type& s, const _Type* B, _Type* C, size_t size)
{
    detail::op(A, B, C, size, numeric::vsubs<_Type>(s));
}


template <typename _Type>
void vsmul(const _Type* I, float c, _Type* O, size_t size)
{
#pragma omp parallel for
    for (int idx = 0; idx < static_cast<int>(size); idx++)
    {
        O[idx] = c*I[idx];
    }
}

} // vex

#endif // VEX_VEX_HXX
