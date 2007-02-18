#ifndef HISTOGRAM_H
#define HISTOGRAM_H
/**
 * @brief 
 * 
 * This file is a part of PFSTOOLS package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
 * 
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
 * @author Rafal Mantiuk, <mantiuk@mpi-sb.mpg.de>
 *
 * $Id: histogram.h,v 1.2 2005/09/02 13:10:35 rafm Exp $
 */

#include <array2d.h>

class Histogram
{
  float *P;
  int bins;
  int accuracy;
  
public:
  Histogram( int bins, int accuracy = 1 );
  ~Histogram();

  void computeLog( const pfs::Array2D *image );
  void computeLog( const pfs::Array2D *image, float min, float max );

  int getBins() const 
    {
      return bins;
    }

  float getMaxP() const;
  float getP( int bin ) const
    {
      assert( bin < bins );
      return P[bin];
    }
  
  
  
};



#endif
