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

#ifndef PFS_UTILS_DOTPRODUCT_H
#define PFS_UTILS_DOTPRODUCT_H

#include <cstddef>

//! \file dotproduct.h
//! \brief Multithread function for the calculation of the dot product between
//! vectors
//! \author Davide Anastasia <davideanastasia@gmail.com>
//! \date 2012.08.28

namespace pfs {
namespace utils {

//! \brief Perform the dotProduct of the element of \c v1 and \c v2
//! output = \sum_{i=0}^{n}{v1[i] * v2[i]}
// v1 . v2
template <typename _Type>
_Type dotProduct(const _Type *v1, const _Type *v2, size_t N);

//! \brief Perform the dotProduct element-wise of the vector \c v1
//! output = \sum_{i=0}^{n}{v1[i] * v1[i]}
//! \note this version is slightly more optimized of the binary version
// v1 . v1
template <typename _Type>
_Type dotProduct(const _Type *v1, size_t N);

}  // utils
}  // pfs

#include <Libpfs/utils/dotproduct.hxx>
#endif  // PFS_UTILS_DOTPRODUCT_H
