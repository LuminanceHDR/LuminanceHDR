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

#ifndef PFS_UTILS_MINMAX_HXX
#define PFS_UTILS_MINMAX_HXX

#include <Libpfs/utils/minmax.h>

#include <algorithm>
#include <numeric>

namespace pfs {
namespace utils {

template <typename _Type>
_Type minElement(const _Type *vector, size_t vectorSize) {
    return *std::min_element(vector, vector + vectorSize);
}

template <typename _Type>
_Type maxElement(const _Type *vector, size_t vectorSize) {
    return *std::max_element(vector, vector + vectorSize);
}

template <typename Type>
void minmax(Type i1, Type i2, Type i3, Type &min, Type &max) {
    if (i1 > i2) {
        if (i1 > i3) {
            // i1 > b and i1 > g
            max = i1;
            if (i3 > i2) {
                min = i2;
            } else {
                min = i3;
            }
        } else {
            // b >= i1 and i1 > g
            max = i3;
            min = i2;
        }
    } else {
        // i2 >= i1
        if (i3 > i2) {
            max = i3;
            min = i1;
        } else {
            // i2 >= i3
            max = i2;
            if (i3 < i1) {
                min = i3;
            } else {
                min = i1;
            }
        }
    }
}

}  // utils
}  // pfs

#endif  // PFS_UTILS_MINMAX_HXX
