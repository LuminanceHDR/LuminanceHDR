/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2010-2012 Davide Anastasia
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

#ifndef PFS_ARRAY2D_HXX
#define PFS_ARRAY2D_HXX

//! \file array2d.hxx
//! \brief General 2d Array interface
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! \note This class is different then the one in the PFSTOOLS

#include <cassert>
#include <iostream>

#include <Libpfs/array2d.h>
#include <Libpfs/utils/numeric.h>

using namespace std;

namespace pfs {

template <typename Type>
Array2D<Type>::Array2D() : m_data(), m_cols(0), m_rows(0) {}

template <typename Type>
Array2D<Type>::Array2D(size_t cols, size_t rows)
    : m_data(cols * rows), m_cols(cols), m_rows(rows) {
    assert(m_data.size() >= m_cols * m_rows);
}

template <typename Type>
Array2D<Type>::Array2D(const self &rhs)
    : m_data(rhs.m_data), m_cols(rhs.m_cols), m_rows(rhs.m_rows) {
    assert(m_data.size() >= m_cols * m_rows);
}

template <typename Type>
Array2D<Type> &Array2D<Type>::operator=(const Array2D<Type> &other) {
    using std::swap;

    Array2D<Type> newState(other);
    swap(*this, newState);

    return *this;
}

template <typename Type>
void Array2D<Type>::resize(size_t width, size_t height) {
    m_data.resize(width * height);
    m_cols = width;
    m_rows = height;

    assert(m_data.size() >= m_cols * m_rows);
}

template <typename Type>
void Array2D<Type>::swap(self &other) {
    std::swap(m_cols, other.m_cols);
    std::swap(m_rows, other.m_rows);
    std::swap(m_data, other.m_data);
}

template <typename Type>
inline Type &Array2D<Type>::operator()(size_t cols, size_t rows) {
#ifndef NDEBUG
    return m_data.at(rows * m_cols + cols);
#else
    return m_data[rows * m_cols + cols];
#endif
}

template <typename Type>
inline const Type &Array2D<Type>::operator()(size_t cols, size_t rows) const {
#ifndef NDEBUG
    return m_data.at(rows * m_cols + cols);
#else
    return m_data[rows * m_cols + cols];
#endif
}

template <typename Type>
inline Type &Array2D<Type>::operator()(size_t index) {
#ifndef NDEBUG
    return m_data.at(index);
#else
    return m_data[index];
#endif
}

template <typename Type>
inline const Type &Array2D<Type>::operator()(size_t index) const {
#ifndef NDEBUG
    return m_data.at(index);
#else
    return m_data[index];
#endif
}

template <typename Type>
void Array2D<Type>::fill(const Type &value) {
    std::fill(m_data.begin(), m_data.end(), value);
}

template <typename Type>
void Array2D<Type>::reset() {
    std::fill(m_data.begin(), m_data.end(), Type());
}

}  // Libpfs

#endif  // PFS_ARRAY2D_HXX
