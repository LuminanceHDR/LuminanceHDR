/**
 * @brief Common intrefaces shared by several operators
 *
 * This file is a part of LuminanceHDR package, based on pfstmo.
 * ---------------------------------------------------------------------- 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ---------------------------------------------------------------------- 
 * 
 * @author Rafal Mantiuk, <mantiuk@gmail.com>
 *
 * $Id: pfstmo.h,v 1.3 2008/09/21 10:42:47 julians37 Exp $
 */
#ifndef PFSTMO_H
#define PFSTMO_H

#include <assert.h>
#include <string.h>
//#include "Libpfs/array2d.h"

/* Common return codes for operators */
#define PFSTMO_OK 1             /* Successful */
#define PFSTMO_ABORTED -1       /* User aborted (from callback) */
#define PFSTMO_ERROR -2         /* Failed, encountered error */

/* Return codes for the progress_callback */
#define PFSTMO_CB_CONTINUE 1
#define PFSTMO_CB_ABORT -1 

/** 
 *  This function is called from a tone-mapper to report current progress.
 *  
 *  @param progress the current progress in percent (grows from 0 to 100)
 *  @return return PFSTMO_CB_CONTINUE to continue tone-mapping or
 *  PFSTMO_CB_ABORT to request to stop. In some cases tone-mapping may
 *  not stop immediately, even if PFSTMO_CB_ABORT was returned.
 */
typedef int(*pfstmo_progress_callback)(int progress);

namespace pfstmo
{
  template <typename T>
  class Array2DBase
  {
    T* data;
    unsigned int width;
    unsigned int height;
    bool dataOwned;

    public:
    Array2DBase(unsigned int width, unsigned int height):
      data(new T[width * height]),
      width(width),
      height(height),
      dataOwned(true)
    {
    }

    Array2DBase(unsigned int width, unsigned int height, T* data):
      width(width),
      height(height),
      data(data),
      dataOwned(false)
    {
    }

    Array2DBase(const Array2DBase& other): 
      data(other.data), 
      width(other.width), 
      height(other.height), 
      dataOwned(false) 
    {
    }
    
    ~Array2DBase()
    {
      if (dataOwned) delete[] data;
    }

    Array2DBase& operator = (const Array2DBase& other)
    {
      if (dataOwned) delete[] data;
      this->data = other.data;
      this->width = other.width;
      this->height = other.height;
      this->dataOwned = false;
      return *this;
    }

    void allocate(unsigned int width, unsigned int height)
    {
       if (dataOwned) delete[] data;
       this->width = width;
       this->height = height;
       this->data = new T[width * height];
       this->dataOwned = true;
    }

    inline T operator () (unsigned int x, unsigned int y) const
    {
      assert( /* x >= 0 && */ x < width /* && y >= 0 */ && y < height);
      return data[y * width + x];
    }

    inline T& operator () (unsigned int x, unsigned int y)
    {
      assert( /* x >= 0 && */ x < width /* && y >= 0 */ && y < height);
      return data[y * width + x];
    }

    inline T operator () (unsigned int index) const
    {
      assert( /* index >= 0 && */ index < width * height);
      return data[index];
    }

    inline T& operator () (unsigned int index)
    {
      assert( /* index >= 0 && */ index < width * height);
      return data[index];
    }

    unsigned int getCols() const 
    {
      return width;
    }

    unsigned int getRows() const 
    {
      return height;
    }

    const T* getRawData() const
    {
      return data;
    }
    
    T* getRawData()
    {
      return data;
    }
  };
  
  template <typename T>
  void copyArray(const Array2DBase<T> *from, Array2DBase<T> *to)
  {
    assert( from->getRows() == to->getRows() );
    assert( from->getCols() == to->getCols() );

    memcpy(to->getRawData(), from->getRawData(), from->getRows() * from->getCols() * sizeof(T));
  }
 
  template <typename T>
  void setArray(Array2DBase<T> *array, T value)
  {
    const int elements = array->getRows()*array->getCols();
    for( int i = 0; i < elements; i++ )
      (*array)(i) = value;
  }

  typedef Array2DBase<float> Array2D;
  //typedef pfs::Array2DImpl Array2D;
  
}

#endif
