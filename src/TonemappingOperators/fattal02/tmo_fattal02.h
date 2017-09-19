/**
 * @file tmo_fattal02.h
 * @brief TMO: Gradient Domain High Dynamic Range Compression (header)
 *
 * Implementation of Gradient Domain High Dynamic Range Compression
 * by Raanan Fattal, Dani Lischinski, Michael Werman.
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
 * $Id: tmo_fattal02.h,v 1.2 2008/09/04 12:46:49 julians37 Exp $
 */

#ifndef TMO_FATTAL02_H
#define TMO_FATTAL02_H

#include <Libpfs/array2d_fwd.h>
#include <cstddef>

namespace pfs {
class Progress;
}

//! \brief Gradient Domain High Dynamic Range Compression
//!
//! Implementation of Gradient Domain High Dynamic Range Compression
//! by Raanan Fattal, Dani Lischinski, Michael Werman.
//!
//! \param width image width
//! \param height image height
//! \param Y [in] image luminance values
//! \param L [out] tone mapped values
//! \param alfa parameter alfa (refer to the paper)
//! \param beta parameter beta (refer to the paper)
//! \param noise gradient level of noise (extra parameter)
//!
void tmo_fattal02(size_t width, size_t height,
                  // const float* Y, float* L,
                  const pfs::Array2Df &Y, pfs::Array2Df &L, float alfa,
                  float beta, float noise, bool newfattal, bool fftsolver,
                  int detail_level, pfs::Progress &ph);

#endif
