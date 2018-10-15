/*
 * @brief VanHateren Tone Mapping Operator:
 *    "Encoding of High Dynamic Range Video with a Model of Human Cones"
 * 	  by J. Hans Van Hateren
 *    in ACM Transaction on Graphics 2006
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

//! \brief Michael Ashikhmin tone mapping operator
//!
//! \param Y [in] image luminance values
//! \param L [out] tone mapped values
//! \param maxLum maximum luminance in the image
//! \param avLum logarithmic average of luminance in the image
//! \param simple_flag true: use only tone mapping function (global version of
//! the operator)
//! \param lc_value local contrast threshold
//! \param eq chose equation number from the paper (ie equation 2. or 4. )
//!
int tmo_vanhateren06(pfs::Array2Df *X, pfs::Array2Df *Y, pfs::Array2Df *Z, pfs::Array2Df *L,
                    float pupil_area,
                    pfs::Progress &ph);

#endif  // TMO_VANHATEREN_H
