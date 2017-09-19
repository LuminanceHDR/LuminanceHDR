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

#ifndef BILATERAL_H
#define BILATERAL_H

namespace pfs {
class Array2D;
class Progress;
}

//!
//! @brief Bilateral filtering
//!
//! Conventional (slow) algorithm.
//!
//! \param I [in] input array
//! \param J [out] filtered array
//! \param sigma_s sigma value for spatial kernel
//! \param sigma_r sigma value for range kernel
//!
void bilateralFilter(const pfs::Array2D *I, pfs::Array2D *J, float sigma_s,
                     float sigma_r, pfs::Progress &ph);

#endif /* #ifndef BILATERAL_H */
