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

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <vector>

#include <Libpfs/strideiterator.h>

//! \file array2d.h
//! \brief general 2d array interface
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! \note This version is different then the one in the PFSTOOLS. This version
//! is inspired by the one in PFSTOOLS, but they are now quite different and
//! most likely, not compatible

namespace pfs {
//!
//! \brief Two dimensional array of data
//!
//! This class holds 2 dimensional arrays of an unspecified type in row-major
//! order. Allows easy indexing and retrieving array dimensions.
//! It offers an undirect access to the data (using (x)(y) or (elem) ) or a
//! direct access to the data (using getRawData() or data()).
//!
template <typename Type>
class Array2D {
   public:
    typedef std::vector<Type> DataBuffer;
    typedef typename DataBuffer::value_type value_type;
    typedef Array2D<Type> self;

    //! \brief default constructor - empty \c Array2D
    Array2D();

    //! \brief init \c Array2D with a matrix of \a cols times \a rows
    Array2D(size_t cols, size_t rows);  // (width, height)

    //! \brief copy ctor
    //! \note If you want to build an empty \c Array2D with the same size of the
    //! source, use the ctor that takes dimension and you will spare the copy
    Array2D(const self &rhs);

    //! \brief assignment operator (always performs deep copy)
    self &operator=(const self &other);

    //! \brief virtual destructor
    virtual ~Array2D() {}

    //! Access an element of the array.
    //! Whether the given row and column are checked against
    //! array bounds depends on an implementing class.
    //!
    //! \param col number of a column (x) within the range [0, getCols()-1)
    //! \param row number of a row (y) within the range [0,getRows()-1)
    //!
    Type &operator()(size_t cols, size_t rows);
    const Type &operator()(size_t cols, size_t rows) const;

    //! Access an element of the array. This method mocks the subscript operator
    //! in a vector class
    //!    //!
    //! \param index index of an element within the range
    //! [0, etCols()*getRows()-1)
    //!
    Type &operator()(size_t index);
    const Type &operator()(size_t index) const;

    //! \brief Get number of columns or, in case of an image, width.
    size_t getCols() const { return m_cols; }

    //! \brief Get number of rows or, in case of an image, height.
    size_t getRows() const { return m_rows; }

    size_t size() const { return m_rows * m_cols; }

    void resize(size_t width, size_t height);

    //! \brief Direct access to the raw data
    Type *data() { return m_data.data(); }
    //! \brief Direct access to the raw data
    const Type *data() const { return m_data.data(); }

    //! \brief fill the entire vector data to the value "value"
    void fill(const Type &value);
    //! \brief fill the entire vector data with the default value for \c Type
    void reset();

    //! \brief Swap the content of the current instance with \a other
    void swap(self &other);

   public:
    // element/row iterator
    typedef typename DataBuffer::iterator iterator;
    typedef typename DataBuffer::const_iterator const_iterator;

    iterator begin() { return m_data.begin(); }
    iterator end() { return m_data.begin() + size(); }

    const_iterator begin() const { return m_data.begin(); }
    const_iterator end() const { return m_data.begin() + size(); }

    iterator row_begin(size_t r) { return m_data.begin() + r * m_cols; }
    iterator row_end(size_t r) { return m_data.begin() + (r + 1) * m_cols; }

    const_iterator row_begin(size_t r) const {
        return m_data.begin() + r * m_cols;
    }
    const_iterator row_end(size_t r) const {
        return m_data.begin() + (r + 1) * m_cols;
    }

    //! \brief subscript operators, returns the row \a n
    iterator operator[](size_t n) { return row_begin(n); }
    const_iterator operator[](size_t n) const { return row_begin(n); }

    // column iterator
    typedef StrideIterator<typename DataBuffer::iterator> col_iterator;
    typedef StrideIterator<typename DataBuffer::iterator> const_col_iterator;

    col_iterator col_begin(size_t n) {
        return col_iterator(begin() + n, getCols());
    }
    col_iterator col_end(size_t n) { return col_begin(n) + getCols(); }

    const_col_iterator col_begin(size_t n) const {
        return const_col_iterator(begin() + n, getCols());
    }
    const_col_iterator col_end(size_t n) const {
        return col_begin(n) + getCols();
    }

   private:
    DataBuffer m_data;

    size_t m_cols;
    size_t m_rows;
};

//! \brief typedef provided for backward compatibility with the old API
typedef ::pfs::Array2D<float> Array2Df;

}  // namespace pfs

namespace std {
template <typename T>
void swap(::pfs::Array2D<T> &a, ::pfs::Array2D<T> &b) {
    a.swap(b);
}
}  // namespace std

#include <Libpfs/array2d.hxx>

#endif  // PFS_ARRAY2D_H
