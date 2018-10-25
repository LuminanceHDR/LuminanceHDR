/*
 * @brief VanHateren Tone Mapping Operator:
 *    "Encoding of High Dynamic Range Video with a Model of Human Cones"
 *     by J. Hans Van Hateren
 *     in ACM Transaction on Graphics 2006
 *
 * This file is a part of LuminanceHDR package
 * ----------------------------------------------------------------------
 * Copyright (C) 2018 Franco Comida
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
 * @author Franco Comida, <fcomida@users.sourceforge.net>
 *
 */

#ifndef TMO_VANHATEREN_H
#define TMO_VANHATEREN_H

#include <Libpfs/array2d_fwd.h>

namespace pfs {
class Frame;
class Progress;
}

//! \brief Van Hateren tone mapping operator
//!
//! \param L [in/out] image luminance values
//! \param pupil area in mm^2
//!
int tmo_vanhateren06(pfs::Array2Df &L, float pupil_area, pfs::Progress &ph);

#endif  // TMO_VANHATEREN_H
