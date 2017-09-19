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

#ifndef PFS_UTILS_NUMERIC_H
#define PFS_UTILS_NUMERIC_H

//! \brief This file contains a series of extensions for vector operations
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#include <cstddef>
#include <functional>
#include <numeric>

namespace pfs {
namespace utils {

namespace numeric {
//! \brief Extension of std::plus to compute A + s*B
template <typename T>
struct vadds {
    vadds(const T &s) : s_(s) {}
    T operator()(const T &t1, const T &t2) const { return (t1 + (s_ * t2)); }

   private:
    T s_;
};

//! \brief Extension of std::minus to compute A - s*B
template <typename T>
struct vsubs {
    vsubs(const T &s) : s_(s) {}
    T operator()(const T &t1, const T &t2) const { return (t1 - (s_ * t2)); }

   private:
    T s_;
};
}  // numeric

//! \brief multiplies element-wise \c A and \c B and stores into \c C
//! C[i] = A[i] * B[i]
//! \param[in] A first input vector
//! \param[in] B second input vector
//! \param[out] C output vector
//! \param[in] N size of the vectors
//! \note caller is in charge of properly allocate all the vectors
template <typename _Type>
void vmul(const _Type *A, const _Type *B, _Type *C, size_t size);

//! \brief divides element-wise \c A and \c B and stores into \c C
//! C[i] = A[i] / B[i]
template <typename _Type>
void vdiv(const _Type *A, const _Type *B, _Type *C, size_t size);

//! \brief sum element-wise \c A and \c B and stores into \c C
//! C[i] = A[i] + B[i]
template <typename _Type>
void vadd(const _Type *A, const _Type *B, _Type *C, size_t size);

//! \brief sum  element-wise \c s to \c A and stores into \c B
//! B[i] = A[i] + s
template <typename _Type>
void vsadd(const _Type *A, const float s, _Type *B, size_t size);

//! \brief multiplies element-wise \c s and \c B, sum it to \c A and stores into
//! \c C
//! C[i] = A[i] + (s * B[i])
template <typename _Type>
void vadds(const _Type *A, const _Type &s, const _Type *B, _Type *C,
           size_t size);

//! \brief subtract element-wise \c A and \c B and stores into \c C
//! C[i] = A[i] - B[i]
template <typename _Type>
void vsub(const _Type *A, const _Type *B, _Type *C, size_t size);

//! \brief multiplies element-wise \c s and \c B, subtract it from \c A and
//! stores into \c C
//! C[i] = A[i] - (s * B[i])
template <typename _Type>
void vsubs(const _Type *A, const _Type &s, const _Type *B, _Type *C,
           size_t size);

//! // O[i] = c * I[i]
template <typename _Type>
void vsmul(const _Type *I, const float c, _Type *O, size_t size);

template <typename _Type>
void vsum_scalar(const _Type *I, const float c, _Type *O, size_t size);

template <typename _Type>
void vmul_scalar(const _Type *I, const float c, _Type *O, size_t size);

template <typename _Type>
void vdiv_scalar(const _Type *I, float c, _Type *O, size_t size);
}  // utils
}  // pfs

#include <Libpfs/utils/numeric.hxx>
#endif  // PFS_UTILS_NUMERIC_H
