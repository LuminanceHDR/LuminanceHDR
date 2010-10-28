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
 * @author Davide Anastasia <davide.anastasia@gmail.com>
 *  This version is different then the one in the PFSTOOLS
 */

#include <iostream>
#include <assert.h>

#include "array2d.h"
#include "Common/vex.h"

namespace pfs
{  
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
    
    const int elements = from->getRows()*from->getCols();
#ifdef __SSE__
    VEX_vcopy(__f, __t, elements);
#else
    for( int i = elements; i; i-- )
    {
      (*__t) = (*__f);
      __t++;
      __f++;
    }
#endif
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
    
    const int elements = array->getRows()*array->getCols();
#ifdef __SSE__
    VEX_vset(__array, value, elements);
#else
    for( int i = 0; i < elements; i++ )
    {
      __array[i] = value;
    }
#endif
  }
  
  /**
   * Perform element-by-element multiplication: z = x * y. z can be the same as x or y.
   *
   * @param z array where the result is stored
   * @param x first element of the multiplication
   * @param y second element of the multiplication
   */
//  void multiplyArray(Array2D *z, const Array2D *x, const Array2D *y)
//  {    
//    assert( x->getRows() == y->getRows() );
//    assert( x->getCols() == y->getCols() );
//    assert( x->getRows() == z->getRows() );
//    assert( x->getCols() == z->getCols() );
//    
//    const int elements = x->getRows()*x->getCols();
//    for( int i = 0; i < elements; i++ )
//      (*z)(i) = (*x)(i) * (*y)(i);
//  }
  
}