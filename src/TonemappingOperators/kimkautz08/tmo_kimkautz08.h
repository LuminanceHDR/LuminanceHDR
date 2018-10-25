/*
 * @brief KimKautz Tone Mapping Operator:
 *    "Consistent Tone Reproduction"
 *	  by Min H. Kim, Jan Kautz
 *    in CGIM '08 Proceedings of the Tenth IASTED
 *    International Conference on Computer Graphics and Imaging  2008
 *
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

#ifndef TMO_KIMKAUTZ_H
#define TMO_KIMKAUTZ_H

#include <Libpfs/array2d_fwd.h>

namespace pfs {
class Frame;
class Progress;
}

//! \brief Min H. Kim, Jan Kautz tone mapping operator
//!
//! \param L [in/out] image luminance values
//!        KK_c1   this parameter adjusts the shape of Gaussian fall-off
//!                within the width of tis characteristic curve. It influcences
//!                the resulting brightness and local details of the tone-mapped
//!                image. A good value is 3.0 (tradeoff between compression and
//!                lost details)
//!         KK_c2  the ratio between the dynamic range (in log10) of an
//!                8-bit imag (2.4) and the dynamic range (in log10) of the
//!                LDR monitor for visualization
//!
int tmo_kimkautz08(pfs::Array2Df &L,
                    float KK_c1, float KK_c2,
                    pfs::Progress &ph);

#endif  // TMO_KIMKAUTZ_H
