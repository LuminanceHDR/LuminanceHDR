/**
 * @file
 * @brief PFS library - general 2d array interface
 *
 * All pfs::Array2D classes are part of pfs library. However, to
 * lessen coupling of the code with pfs library, Array2D classes are
 * declared in this separate file. Therefore it is possible to write
 * the code that implements or uses Array2D interface while it has no
 * knowledge of other pfs library classes.
 * 
 * This file is a part of Luminance HDR package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
 * Copyright (C) 2010 Davide Anastasia (Luminance HDR)
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
 * 
 * @author Rafal Mantiuk, <mantiuk@mpi-sb.mpg.de>
 * $Id: array2d.h,v 1.1 2005/06/15 13:36:55 rafm Exp $
 *
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *  This version is different then the one in the PFSTOOLS
 */

#include <iostream>
#include <assert.h>
#include <arch/malloc.h>

#include "array2d.h"
#include "vex.h"

using namespace std;

namespace pfs
{  
    Array2D::Array2D( int cols, int rows ):
            m_cols(cols), m_rows(rows)
    {
        m_cols = cols;
        m_rows = rows;
        // Aligned memory allocation allows faster vectorized access
        m_data = (float*)_mm_malloc(m_cols*m_rows*sizeof(float), 32);
        m_is_data_owned = true;
    }

    Array2D::Array2D( int cols, int rows, float* data)
    {
        m_cols = cols;
        m_rows = rows;
        m_data = data;
        m_is_data_owned = false;
    }

    // copy constructor?
    Array2D::Array2D(const Array2D& other)
    {
        this->m_cols = other.m_cols;
        this->m_rows = other.m_rows;
        this->m_data = other.m_data;
        this->m_is_data_owned = false;
    }

    // Assignment operator
    Array2D& Array2D::operator=(const Array2D& other)
                               {
        if (m_is_data_owned) _mm_free(m_data);

        this->m_cols = other.m_cols;
        this->m_rows = other.m_rows;
        this->m_data = other.m_data;
        this->m_is_data_owned = false;
        return *this;
    }

    Array2D::~Array2D()
    {
        if (m_is_data_owned) _mm_free(m_data); //delete[] data;
    }

//    float& Array2D::operator()( int cols, int rows )
//    {
//        assert( cols >= 0 && cols < m_cols );
//        assert( rows >= 0 && rows < m_rows );
//        return m_data[ rows*m_cols + cols ];
//    }

//    const float& Array2D::operator()( int cols, int rows ) const
//    {
//        assert( cols >= 0 && cols < m_cols );
//        assert( rows >= 0 && rows < m_rows );
//        return m_data[ rows*m_cols + cols ];
//    }

//    float& Array2D::operator()( int index )
//    {
//        assert( index >= 0 && index < m_rows*m_cols );
//        return m_data[index];
//    }

//    const float& Array2D::operator()( int index ) const
//    {
//        assert( index >= 0 && index <= m_rows*m_cols );
//        return m_data[index];
//    }

    void Array2D::reset(const float value)
    {
        VEX_vset(this->m_data, value, this->m_rows*this->m_cols);
    }

    void Array2D::scale(const float value)
    {
        // O[i] = c * I[i]
        VEX_vsmul(this->m_data, value, this->m_data, this->m_rows*this->m_cols);
    }


    /**
   * Copy data from one Array2D to another. Dimensions of the arrays must be the same.
   *
   * @param from array to copy from
   * @param to array to copy to
   */
    void copyArray(const Array2D *from, Array2D *to)
    {
        assert( from->getRows() == to->getRows() );
        assert( from->getCols() == to->getCols() );

        const float* f = from->getRawData();
        float* t = to->getRawData();

        const int V_ELEMS = from->getRows()*from->getCols();

        VEX_vcopy(f, t, V_ELEMS);
    }

    void copyArray(const Array2D *from, Array2D *to, int x_ul, int y_ul, int x_br, int y_br)
    {
        const float* fv = from->getRawData();
        float* tv       = to->getRawData();

        const int IN_W    = from->getCols();
        const int IN_H    = from->getRows();
        const int OUT_W   = to->getCols();
        const int OUT_H   = to->getRows();

        assert( OUT_H <= IN_H );
        assert( OUT_H <= IN_H );
        assert( x_ul >= 0 );
        assert( y_ul >= 0 );
        assert( x_br <= IN_W );
        assert( y_br <= IN_H );

        // move to row (x_ul, y_ul)
        fv = &fv[IN_W*y_ul + x_ul];

#pragma omp parallel for
        for (int r = 0; r < OUT_H; r++)
        {
            //NOTE: do NOT use VEX_vcopy
            for (int c = 0; c < OUT_W; c++)
            {
                tv[r*OUT_W + c] = fv[r*IN_W + c];
            }
        }
    }

    /**
   * Set all elements of the array to a give value.
   *
   * @param array array to modify
   * @param value all elements of the array will be set to this value
   */
    void setArray(Array2D *array, const float value)
    {
        array->reset(value);
    }

    /**
   * Perform element-by-element multiplication: z = x * y. z must be the same as x or y.
   *
   * @param z array where the result is stored
   * @param x first element of the multiplication
   * @param y second element of the multiplication
   */
    void multiplyArray(Array2D *z, const Array2D *x, const Array2D *y)
    {
        assert( x->getRows() == y->getRows() );
        assert( x->getCols() == y->getCols() );
        assert( x->getRows() == z->getRows() );
        assert( x->getCols() == z->getCols() );

        const float* xv = x->getRawData();
        const float* yv = y->getRawData();
        float* zv = z->getRawData();

        const int elements = x->getRows()*x->getCols();

        VEX_vmul(xv, yv, zv, elements);
    }

    /**
   * Perform element-by-element division: z = x / y. z must be the same as x or y.
   *
   * @param z array where the result is stored
   * @param x first element of the division
   * @param y second element of the division
   */
    void divideArray(Array2D *z, const Array2D *x, const Array2D *y)
    {
        assert( x->getRows() == y->getRows() );
        assert( x->getCols() == y->getCols() );
        assert( x->getRows() == z->getRows() );
        assert( x->getCols() == z->getCols() );

        const float* xv = x->getRawData();
        const float* yv = y->getRawData();
        float* zv = z->getRawData();

        const int elements = x->getRows()*x->getCols();

        VEX_vdiv(xv, yv, zv, elements);
    }
}




