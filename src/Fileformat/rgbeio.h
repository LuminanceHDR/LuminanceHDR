/**
 * @brief IO operations on Radiance's RGBE file format
 * 
 * This file is a part of LuminanceHDR package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2003-2007 Rafal Mantiuk and Grzegorz Krawczyk
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
 * @author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
 *
 * $Id: rgbeio.h,v 1.2 2006/11/17 11:38:40 gkrawczyk Exp $
 */

#ifndef _RGBEIO_H_
#define _RGBEIO_H_

#include <stdio.h>

#include "Libpfs/array2d.h"


class RGBEReader 
{
  FILE *fh;
  int width, height;
  float exposure;
  
public:
  RGBEReader( FILE *fh );    
  ~RGBEReader();

  int getWidth() const 
    {
      return width;
    }
  
  int getHeight() const
    {
      return height;
    }

  void readImage( pfs::Array2D *X, pfs::Array2D *Y, pfs::Array2D *Z );
    
};


class RGBEWriter
{
  FILE *fh;
public:
  RGBEWriter( FILE *fh ) : fh(fh)
    {
    }
    
  void writeImage( pfs::Array2D *X, pfs::Array2D *Y, pfs::Array2D *Z );
};

#endif
