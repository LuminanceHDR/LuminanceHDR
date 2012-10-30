/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
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

#ifndef ARRAY2D_H
#define ARRAY2D_H

#include <cstddef>

//! \file array2d.h
//! \brief general 2d array interface
//! \author Rafal Mantiuk, <mantiuk@mpi-sb.mpg.de>
//!
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! \note This version is different then the one in the PFSTOOLS
//! Classes Array2D and Array2DImpl are joined to create a clean class,
//! which allows faster access to vector data, in order to create high
//! performance routines. Old access functions are still available.
//!

namespace pfs
{ 
//!
//! \brief Two dimensional array of floats
//!
//! This class holds 2 dimensional arrays of floats in column-major order.
//! Allows easy indexing and retrieving array dimensions. It offers an undirect
//! access to the data (using (x)(y) or (elem) ) or a direct access to the data
//! (using getRawData()).
//!
class Array2D
{
private:
    int     m_cols;
    int     m_rows;
    float*  m_data;

    Array2D& operator=(const Array2D& other);
    Array2D(const Array2D& other);

public:
    Array2D(int cols, int rows);

    //! Each implementing class should provide its own destructor.
    //! It must be virtual to allow derived class to call their destructor
    //! as well.
    virtual ~Array2D();

    //! Access an element of the array for reading and
    //! writing. Whether the given row and column are checked against
    //! array bounds depends on an implementing class.
    //!
    //! Note, that if an Array2D object is passed as a pointer (what
    //! is usually the case), to access its elements, you have to use
    //! somewhat strange syntax: (*array)(row, column).
    //!
    //! \param col number of a column (x) within the range 0..(getCols()-1)
    //! \param row number of a row (y) within the range 0..(getRows()-1)
    //!
    float& operator()( int cols, int rows );

    //! Access an element of the array for reading. Whether the given
    //! row and column are checked against array bounds depends on an
    //! implementing class.
    //!
    //! Note, that if an Array2D object is passed as a pointer (what
    //! is usually the case), to access its elements, you have to use
    //! somewhat strange syntax: (*array)(row, column).
    //!
    //! \param col number of a column (x) within the range 0..(getCols()-1)
    //! \param row number of a row (y) within the range 0..(getRows()-1)
    //!
    const float& operator()( int cols, int rows ) const;

    //! Access an element of the array for reading and writing. This
    //! is probably faster way of accessing elements than
    //! operator(col, row). However there is no guarantee on the
    //! order of elements as it usually depends on an implementing
    //! class. The only assumption that can be make is that there are
    //! exactly columns*rows elements and they are all unique.
    //!
    //! Whether the given index is checked against array bounds
    //! depends on an implementing class.
    //!
    //! Note, that if an Array2D object is passed as a pointer (what
    //! is usually the case), to access its elements, you have to use
    //! somewhat strange syntax: (*array)(index).
    //!
    //! \param index index of an element within the range 0..(getCols()*getRows()-1)
    //!
    float& operator()( int index );

    //! Access an element of the array for reading. This
    //! is probably faster way of accessing elements than
    //! operator(col, row). However there is no guarantee on the
    //! order of elements as it usually depends on an implementing
    //! class. The only assumption that can be make is that there are
    //! exactly columns*rows elements and they are all unique.
    //!
    //! Whether the given index is checked against array bounds
    //! depends on an implementing class.
    //!
    //! Note, that if an Array2D object is passed as a pointer (what
    //! is usually the case), to access its elements, you have to use
    //! somewhat strange syntax: (*array)(index).
    //!
    //! \param index index of an element within the range 0..(getCols()*getRows()-1)
    //!
    const float& operator()( int index ) const;

    //! \brief Get number of columns or, in case of an image, width.
    int getCols() const;

    //! \brief Get number of rows or, in case of an image, height.
    int getRows() const;

    inline
    int size() const
    { return m_rows*m_cols; }

    void resize(int width, int height);

    //! \brief Direct access to the raw data
    float*       getRawData();

    //! \brief Direct access to the raw data
    const float* getRawData() const;

    //! \brief Reset the entire vector data to the value "value"
    void reset(float value = 0.0f);

    //! \brief Scale entire 2D array by "value"
    void scale(float value);

public:
    typedef float*          Iterator;
    typedef const float*    ConstIterator;

    inline
    Iterator begin()
    { return m_data; }
    inline
    Iterator end()
    { return m_data + m_cols*m_rows; }

    inline
    ConstIterator begin() const
    { return m_data; }
    inline
    ConstIterator end() const
    { return m_data + m_cols*m_rows; }

    inline
    Iterator beginRow(size_t r)
    { return m_data + r*m_cols; }
    inline
    Iterator endRow(size_t r)
    { return m_data + (r+1)*m_cols; }

    inline
    ConstIterator beginRow(size_t r) const
    { return m_data + r*m_cols; }
    inline
    ConstIterator endRow(size_t r) const
    { return m_data + (r+1)*m_cols; }
};

inline int Array2D::getCols() const { return m_cols; }
inline int Array2D::getRows() const { return m_rows; }
inline float* Array2D::getRawData() { return m_data; }
inline const float* Array2D::getRawData() const { return m_data; }

inline
float& Array2D::operator()( int cols, int rows )
{
    //assert( cols >= 0 && cols < m_cols );
    //assert( rows >= 0 && rows < m_rows );
    return m_data[ rows*m_cols + cols ];
}

inline
const float& Array2D::operator()( int cols, int rows ) const
{
    //assert( cols >= 0 && cols < m_cols );
    //assert( rows >= 0 && rows < m_rows );
    return m_data[ rows*m_cols + cols ];
}

inline
float& Array2D::operator()( int index )
{
    //assert( index >= 0 && index < m_rows*m_cols );
    return m_data[index];
}

inline
const float& Array2D::operator()( int index ) const
{
    //assert( index >= 0 && index <= m_rows*m_cols );
    return m_data[index];
}


//! \brief Set all elements of the array to a give value.
//!
//! \param array array to modify
//! \param value all elements of the array will be set to this value
inline
void setArray(Array2D& array, float value)
{
    array.reset(value);
}

//! \brief Perform element-by-element multiplication: z = x * y.
//! z must be the same as x or y.
//!
//! \param z array where the result is stored
//! \param x first element of the multiplication
//! \param y second element of the multiplication
void multiplyArray(Array2D& z, const Array2D& x, const Array2D& y);

//! \brief Perform element-by-element division: z = x / y.
//! z must be the same as x or y.
//!
//! \param z array where the result is stored
//! \param x first element of the division
//! \param y second element of the division
void divideArray(Array2D& z, const Array2D& x, const Array2D& y);
}

#endif // ARRAY2D_H
