/**
 * @file tmo_durand02.h
 * @brief Local tone mapping operator based on bilateral filtering.
 * Durand et al. 2002
 *
 * Fast Bilateral Filtering for the Display of High-Dynamic-Range Images.
 * F. Durand and J. Dorsey.
 * In ACM Transactions on Graphics, 2002.
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
 * @author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
 *
 * $Id: tmo_durand02.h,v 1.4 2009/02/23 19:09:41 rafm Exp $
 */

#ifndef TMO_DURAND02_H
#define TMO_DURAND02_H

#include <Libpfs/array2d_fwd.h>

namespace pfs {
class Progress;
}

//!
//! \brief Fast bilateral filtering
//!
//! \param width image width
//! \param height image height
//! \param R red channel
//! \param G green channel
//! \param B blue channel
//! \param sigma_s sigma for spatial kernel
//! \param sigma_r sigma for range kernel
//! \param baseContrast contrast of the base layer
//! \param color_correction enable automatic color correction
//! \param downsample down sampling factor for speeding up fast-bilateral
//! (1..20)
//!
void tmo_durand02(pfs::Array2Df &R, pfs::Array2Df &G, pfs::Array2Df &B,
                  float sigma_s, float sigma_r, float baseContrast,
                  int downsample, bool color_correction /*= true*/,
                  pfs::Progress &ph);

#endif  // TMO_DURAND02_H
