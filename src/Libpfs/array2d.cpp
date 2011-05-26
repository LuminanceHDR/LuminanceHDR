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

#include "array2d.h"
#include "Common/vex.h"

using namespace std;

namespace pfs
{  
  Array2DImpl::Array2DImpl( int __cols, int __rows )
  {
    cols = __cols;
    rows = __rows;
    data = (float*)_mm_malloc(cols*rows*sizeof(float), 16); //new float[cols*rows];
    data_owned = true;
  }
  
  Array2DImpl::Array2DImpl( int __cols, int __rows, float* __data)
  {
    cols = __cols;
    rows = __rows;
    data = __data;
    data_owned = false;
  }
  
  Array2DImpl::Array2DImpl(const Array2DImpl& other) : Array2D()
  {
    this->cols = other.cols;
    this->rows = other.rows;
    this->data = other.data;
    this->data_owned = false;
  }
  
  Array2DImpl& Array2DImpl::operator = (const Array2DImpl& other)
  {
    if (data_owned) _mm_free(data); //delete[] data;

    this->cols = other.cols;
    this->rows = other.rows;
    this->data = other.data;
    this->data_owned = false;
    return *this;
}
  
  Array2DImpl::~Array2DImpl()
  {
    if (data_owned) _mm_free(data); //delete[] data;
  }
  
  
  float& Array2DImpl::operator()( int col, int row )
  {
    assert( col >= 0 && col < cols );
    assert( row >= 0 && row < rows );
    return data[ col+row*cols ];
  }
  
  const float& Array2DImpl::operator()( int col, int row ) const
  {
    assert( col >= 0 && col < cols );
    assert( row >= 0 && row < rows );
    return data[ col+row*cols ];
  }
  
  float& Array2DImpl::operator()( int index )
  {
    assert( index >= 0 && index < rows*cols );
    return data[index];
  }
  
  const float& Array2DImpl::operator()( int index ) const
  {
    assert( index >= 0 && index <= rows*cols );        
    return data[index];
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
    
    const Array2DImpl* f_x = dynamic_cast<const Array2DImpl*> (from);
          Array2DImpl* t_x = dynamic_cast<Array2DImpl*> (to);
    
    assert( f_x != NULL && t_x != NULL );
    
    const float* __f = f_x->data;
          float* __t = t_x->data;
    
    const int V_ELEMS = f_x->rows*f_x->cols;
#ifdef __SSE__
    VEX_vcopy(__f, __t, V_ELEMS);
#else
    for (int idx = 0; idx < V_ELEMS; idx++ )
    {
      __t[idx] = __f[idx];
    }
#endif
  }
  
  void copyArray(const Array2D *from, Array2D *to, int x_ul, int y_ul, int x_br, int y_br)
  {
    const Array2DImpl* from_2d  = dynamic_cast<const Array2DImpl*> (from);
    Array2DImpl* to_2d          = dynamic_cast<Array2DImpl*> (to);
    
    assert( from_2d != NULL && to_2d != NULL );
    
    const float* from_2d_data   = from_2d->data;
    float* to_2d_data           = to_2d->data;
    
    const int IN_W    = from_2d->cols;
    const int IN_H    = from_2d->rows;
    const int OUT_W   = to_2d->cols;
    const int OUT_H   = to_2d->rows;
    
    assert( OUT_H <= IN_H );
    assert( OUT_H <= IN_H );
    assert( x_ul >= 0 );
    assert( y_ul >= 0 );
    assert( x_br <= IN_W );
    assert( y_br <= IN_H );
    
    // move to row (x_ul, y_ul)
    from_2d_data = &from_2d_data[IN_W*y_ul + x_ul];
    
    for (int r = 0; r < OUT_H; r++)
    {
      //NOTE: do NOT use VEX_vcopy
      #pragma omp parallel for
      for (int c = 0; c < OUT_W; c++)
      {
        to_2d_data[c] = from_2d_data[c];
      }
      
      from_2d_data  += IN_W;
      to_2d_data    += OUT_W;
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
    Array2DImpl* array_t = dynamic_cast<Array2DImpl*> (array);
    
    assert( array_t != NULL );
    
    float* __array = array_t->data;
    
    const int V_ELEMS = (array_t->rows*array_t->cols);
#ifdef __SSE__
    VEX_vset(__array, value, V_ELEMS);
#else
    for( int i = 0; i < V_ELEMS; i++ )
    {
      __array[i] = value;
    }
#endif
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
    
    const int elements = x->getRows()*x->getCols();
    for( int i = 0; i < elements; i++ )
    {
      (*z)(i) = (*x)(i) * (*y)(i);
    }
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
    
    const int elements = x->getRows()*x->getCols();
    for( int i = 0; i < elements; i++ )
    {
      (*z)(i) = (*x)(i) / (*y)(i);
    }
  }
  
}
