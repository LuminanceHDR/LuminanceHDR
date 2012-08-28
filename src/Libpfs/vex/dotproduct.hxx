/*
* This file is a part of Luminance HDR package.
* ----------------------------------------------------------------------
* Copyright (C) 2012 Davide Anastasia
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

//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#ifndef VEX_DOTPRODUCT_HXX
#define VEX_DOTPRODUCT_HXX

#include "dotproduct.h"

namespace vex
{

template <typename _Type>
_Type dotProduct(const _Type* v1, const _Type* v2, size_t N)
{
    _Type dotProd = _Type();
#pragma omp parallel for reduction(+:dotProd)
    for (size_t idx = 0; idx < N; idx++)
    {
        dotProd += (v1[idx] * v2[idx]);
    }
    return dotProd;
}

template <typename _Type>
_Type dotProduct(const _Type* v1, size_t N)
{
    _Type dotProd = _Type();
#pragma omp parallel for reduction(+:dotProd)
    for (size_t idx = 0; idx < N; idx++)
    {
        dotProd += (v1[idx] * v1[idx]);
    }
    return dotProd;
}
}

#endif // VEX_DOTPRODUCT_HXX
