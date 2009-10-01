/**
 * @file bilateral.h
 * @brief Bilateral filtering
 *
 * 
 * This file is a part of LuminanceHDR package, based on pfstmo.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2003,2004 Grzegorz Krawczyk
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
 * @author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
 *
 * $Id: bilateral.h,v 1.3 2008/09/09 00:56:49 rafm Exp $
 */


#ifndef _bilateral_h_
#define _bilateral_h_

#include "../../Common/progressHelper.h"

/**
 * @brief Bilateral filtering
 *
 * Conventional (slow) algorithm.
 *
 * @param I [in] input array
 * @param J [out] filtered array
 * @param sigma_s sigma value for spatial kernel
 * @param sigma_r sigma value for range kernel
 *
 * $Id: bilateral.h,v 1.3 2008/09/09 00:56:49 rafm Exp $
 */
void bilateralFilter( const pfstmo::Array2D *I,
  pfstmo::Array2D *J, float sigma_s, float sigma_r,
  ProgressHelper *ph );


#endif /* #ifndef _bilateral_h_ */
