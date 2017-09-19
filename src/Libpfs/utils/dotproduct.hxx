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

#ifndef PFS_UTILS_DOTPRODUCT_HXX
#define PFS_UTILS_DOTPRODUCT_HXX

#include <Libpfs/utils/dotproduct.h>

namespace pfs {
namespace utils {

template <typename _Type>
_Type dotProduct(const _Type *v1, const _Type *v2, size_t N) {
    double dotProd = _Type();
#pragma omp parallel for reduction(+ : dotProd)
    for (int idx = 0; idx < static_cast<int>(N); idx++) {
        dotProd = dotProd + (v1[idx] * v2[idx]);
    }
    return static_cast<_Type>(dotProd);
}

template <typename _Type>
_Type dotProduct(const _Type *v1, size_t N) {
    double dotProd = _Type();
#pragma omp parallel for reduction(+ : dotProd)
    for (int idx = 0; idx < static_cast<int>(N); idx++) {
        dotProd = dotProd + (v1[idx] * v1[idx]);
    }
    return static_cast<_Type>(dotProd);
}

}  // utils
}  // pfs

#endif  // PFS_UTILS_DOTPRODUCT_HXX
