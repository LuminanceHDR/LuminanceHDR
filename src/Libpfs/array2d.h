/*
 * This file is a part of Luminance HDR package
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

#ifndef PFS_ARRAY2D_H
#define PFS_ARRAY2D_H

#include <cstddef>
#include <cassert>
#include <vector>
#include <algorithm>
#include <Libpfs/vex/vex.h>

//! \file array2d.h
//! \brief general 2d array interface
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! \note This version is different then the one in the PFSTOOLS. This version
//! is inspired by the one in PFSTOOLS, but they are now quite different and
//! most likely, not compatible

namespace pfs
{ 
//!
//! \brief Two dimensional array of data
//!
//! This class holds 2 dimensional arrays of an unspecified type in row-major
//! order. Allows easy indexing and retrieving array dimensions.
//! It offers an undirect access to the data (using (x)(y) or (elem) ) or a
//! direct access to the data (using getRawData() or data()).
//!
template <typename Type>
class Array2D
{
public:
    typedef std::vector<Type>                   DataBuffer;
    typedef typename DataBuffer::value_type     value_type;
    typedef Array2D<Type>                       self;

    //! \brief default constructor - empty \c Array2D
    Array2D();

    //! \brief init \c Array2D with a matrix of \a cols times \a rows
    Array2D(size_t cols, size_t rows);

    //! \brief copy ctor
    Array2D(const self& rhs);

    //! \brief virtual destructor
    virtual ~Array2D() {}

    //! Access an element of the array.
    //! Whether the given row and column are checked against
    //! array bounds depends on an implementing class.
    //!
    //! \param col number of a column (x) within the range [0, getCols()-1)
    //! \param row number of a row (y) within the range [0,getRows()-1)
    //!
    Type& operator()( size_t cols, size_t rows );
    const Type& operator()( size_t cols, size_t rows ) const;

    //! Access an element of the array. This method mocks the subscript operator
    //! in a vector class
    //!    //!
    //! \param index index of an element within the range
    //! [0, etCols()*getRows()-1)
    //!
    Type& operator()( size_t index );
    const Type& operator()( size_t index ) const;

    //! \brief Get number of columns or, in case of an image, width.
    size_t getCols() const
    { return m_cols; }

    //! \brief Get number of rows or, in case of an image, height.
    size_t getRows() const
    { return m_rows; }

    size_t size() const
    { return m_rows*m_cols; }

    void resize(size_t width, size_t height);

    //! \brief Direct access to the raw data
    Type*       getRawData()
    { return m_data.data(); }

    //! \brief Direct access to the raw data
    const Type* getRawData() const
    { return m_data.data(); }

    //! \brief Direct access to the raw data
    Type*       data()
    { return m_data.data(); }

    //! \brief Direct access to the raw data
    const Type* data() const
    { return m_data.data(); }

    //! \brief Reset the entire vector data to the value "value"
    void reset(const Type& value = Type());

    //! \brief Swap the content of the current instance with \a other
    void swap(self& other);

public:
    typedef typename DataBuffer::iterator       iterator;
    typedef typename DataBuffer::const_iterator const_iterator;

    iterator begin()
    { return m_data.begin(); }
    iterator end()
    { return m_data.begin() + size(); }

    const_iterator begin() const
    { return m_data.begin(); }
    const_iterator end() const
    { return m_data.begin() + size(); }

    iterator beginRow(size_t r)
    { return m_data.begin() + r*m_cols; }
    iterator endRow(size_t r)
    { return m_data.begin() + (r+1)*m_cols; }

    const_iterator beginRow(size_t r) const
    { return m_data.begin() + r*m_cols; }
    const_iterator endRow(size_t r) const
    { return m_data.begin() + (r+1)*m_cols; }

private:
    size_t     m_cols;
    size_t     m_rows;

    DataBuffer m_data;
};

//! \brief typedef provided for backward compatibility with the old API
typedef ::pfs::Array2D<float> Array2Df;

//! \brief Set all elements of the array to a give value.
//!
//! \param array array to modify
//! \param value all elements of the array will be set to this value
template <typename Type>
void setArray(Array2D<Type>& array, const Type& value);

//! \brief Perform element-by-element multiplication: z = x * y.
//! z must be the same as x or y.
//!
//! \param z array where the result is stored
//! \param x first element of the multiplication
//! \param y second element of the multiplication
template <typename Type>
void multiplyArray(Array2D<Type>& z, const Array2D<Type>& x, const Array2D<Type>& y);

//! \brief Perform element-by-element division: z = x / y.
//! z must be the same as x or y.
//!
//! \param z array where the result is stored
//! \param x first element of the division
//! \param y second element of the division
template <typename Type>
void divideArray(Array2D<Type>& z, const Array2D<Type>& x, const Array2D<Type>& y);

} // namespace pfs

#include "array2d.hxx"

#endif // PFS_ARRAY2D_H
