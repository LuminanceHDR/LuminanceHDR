/**
 * @file tmo_ferradans11.h
 * Implementation of the algorithm presented in :
 *
 * An Analysis of Visual Adaptation and Contrast Perception for Tone Mapping
 * S. Ferradans, M. Bertalmio, E. Provenzi, V. Caselles
 * In IEEE Trans. Pattern Analysis and Machine Intelligence
 *
*
 * @author Sira Ferradans Copyright (C) 2013
 *
 *
 * This file is a part of LuminanceHDR package, based on pfstmo.
 * ----------------------------------------------------------------------
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
 */

#ifndef TMO_FERRADANS11_H
#define TMO_FERRADANS11_H

#include <Libpfs/array2d_fwd.h>
#include <cstddef>

namespace pfs {
class Progress;
}

//! \brief An Analysis of Visual Adaptation and Contrast Perception for Tone
//! Mapping
//!
//! Implementation of Visual Adaptation and Contrast Perception
//! by Sira Farradans
//!
//! \param width image width
//! \param height image height
//! \param imR [In/Out] Red   Channel
//! \param imG [In/Out] Green Channel
//! \param imB [In/Out] Blue  Channel
//! \param rho parameter rho (refer to the paper)
//! \param inv_alpha parameter inv_alpha (refer to the paper)
//!
void tmo_ferradans11(pfs::Array2Df &imR, pfs::Array2Df &imG, pfs::Array2Df &imB,
                     float rho, float invalpha, pfs::Progress &ph);

#endif
