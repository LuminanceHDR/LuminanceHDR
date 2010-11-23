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
 *
 */

#ifndef __ARRAY2D_H__
#define __ARRAY2D_H__

namespace pfs
{ 
  /**
   * @brief Interface for 2 dimensional array of floats.
   *
   * This is a thin interface of classes that hold 2 dimensional arrays
   * of floats. The interface lets to access all types of arrays in the
   * same way, regardless how data is really stored (row major, column
   * major, 2D array, array of pointers, etc.). It also simplifies
   * indexing.
   *
   * See also implementing classes.
   */
  class Array2D
  {
    
  public:
    
    /**
     * Get number of columns or, in case of an image, width.
     */
    virtual int getCols() const = 0;
    
    /**
     * Get number of rows or, in case of an image, height.
     */
    virtual int getRows() const = 0;
    
    
    /**
     * Access an element of the array for reading and
     * writing. Whether the given row and column are checked against
     * array bounds depends on an implementing class.
     *
     * Note, that if an Array2D object is passed as a pointer (what
     * is usually the case), to access its elements, you have to use
     * somewhat strange syntax: (*array)(row, column).
     *
     * @param col number of a column (x) within the range 0..(getCols()-1)
     * @param row number of a row (y) within the range 0..(getRows()-1)
     */
    virtual float& operator()( int col, int row ) = 0;
    
    /**
     * Access an element of the array for reading. Whether the given
     * row and column are checked against array bounds depends on an
     * implementing class.
     *
     * Note, that if an Array2D object is passed as a pointer (what
     * is usually the case), to access its elements, you have to use
     * somewhat strange syntax: (*array)(row, column).
     *
     * @param col number of a column (x) within the range 0..(getCols()-1)
     * @param row number of a row (y) within the range 0..(getRows()-1)
     */
    virtual const float& operator()( int col, int row ) const = 0;
    
    /**
     * Access an element of the array for reading and writing. This
     * is probably faster way of accessing elements than
     * operator(col, row). However there is no guarantee on the
     * order of elements as it usually depends on an implementing
     * class. The only assumption that can be make is that there are
     * exactly columns*rows elements and they are all unique.
     *
     * Whether the given index is checked against array bounds
     * depends on an implementing class.
     *
     * Note, that if an Array2D object is passed as a pointer (what
     * is usually the case), to access its elements, you have to use
     * somewhat strange syntax: (*array)(index).
     *
     * @param index index of an element within the range 0..(getCols()*getRows()-1)
     */      
    virtual float& operator()( int index ) = 0;
    
    /**
     * Access an element of the array for reading. This
     * is probably faster way of accessing elements than
     * operator(col, row). However there is no guarantee on the
     * order of elements as it usually depends on an implementing
     * class. The only assumption that can be make is that there are
     * exactly columns*rows elements and they are all unique.
     *
     * Whether the given index is checked against array bounds
     * depends on an implementing class.
     *
     * Note, that if an Array2D object is passed as a pointer (what
     * is usually the case), to access its elements, you have to use
     * somewhat strange syntax: (*array)(index).
     *
     * @param index index of an element within the range 0..(getCols()*getRows()-1)
     */      
    virtual const float& operator()( int index ) const = 0;
    
    /**
     * Each implementing class should provide its own destructor.
     */
    virtual ~Array2D()
    {
    }
    
  };
  
  
  /**
   * @brief Two dimensional array of floats
   *
   * Holds 2D data in column-major order. Allows easy indexing
   * and retrieving array dimensions.
   */
  class Array2DImpl: public Array2D
  {
  private:
    float*  data;
    int     cols;
    int     rows;
    bool    data_owned;
    
  public:
    Array2DImpl( int __cols, int __rows );
    Array2DImpl( int __cols, int __rows, float* __data);
    Array2DImpl(const Array2DImpl& other);
    Array2DImpl& operator = (const Array2DImpl& other);
    ~Array2DImpl();
    
    float& operator()( int col, int row );
    const float& operator()( int col, int row ) const;
    float& operator()( int index );
    const float& operator()( int index ) const;
    
    inline int getCols() const { return cols; }
    inline int getRows() const { return rows; }
    
    inline float*       getRawData()        { return data; }
    inline const float* getRawData() const  { return data; }
    
    // Data manipulation
    friend void copyArray(const Array2D *from, Array2D *to);
        
    friend void setArray(Array2D *array, const float value);
    friend void multiplyArray(Array2D *z, const Array2D *x, const Array2D *y);
    friend void divideArray(Array2D *z, const Array2D *x, const Array2D *y);
    
    // Colorspace Conversions
    friend void transformRGB2XYZ(const Array2D *R, const Array2D *G, const Array2D *B, Array2D *X, Array2D *Y, Array2D *Z);
    friend void transformXYZ2RGB(const Array2D *X, const Array2D *Y, const Array2D *Z, Array2D *R, Array2D *G, Array2D *B);
    
    friend void transformSRGB2XYZ(const Array2D *R, const Array2D *G, const Array2D *B, Array2D *X, Array2D *Y, Array2D *Z);
    friend void transformXYZ2SRGB(const Array2D *X, const Array2D *Y, const Array2D *Z, Array2D *R, Array2D *G, Array2D *B);
    
    // Array2D manipulation
    friend void downsampleArray(const Array2D *from, Array2D *to);
  };
  
  void copyArray(const Array2D *from, Array2D *to);  
  void setArray(Array2D *array, const float value);
  void multiplyArray(Array2D *z, const Array2D *x, const Array2D *y);
  void divideArray(Array2D *z, const Array2D *x, const Array2D *y);
}

#endif // __ARRAY2D_H__
