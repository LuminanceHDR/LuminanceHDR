/*
 * @brief Lischinski Tone Mapping Operator:
 *    "Interactive Local Adjustment of Tonal Values"
 *     by Dani Lischinski, Zeev Farbman, Matt Uyttendaele, Richard Szeliski
 *     in Proceedings of SIGGRAPH 2006
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
 * Code adapted for Luminance HDR from:
 *
 *  PICCANTE
 *  The hottest HDR imaging library!
 *  http://vcg.isti.cnr.it/piccante
 *  Copyright (C) 2014
 *  Visual Computing Laboratory - ISTI CNR
 *  http://vcg.isti.cnr.it
 *  First author: Francesco Banterle
 *
 */

#ifndef TMO_LISCHINSKI_H
#define TMO_LISCHINSKI_H

#include <Libpfs/array2d_fwd.h>

namespace pfs {
class Frame;
class Progress;
}

//! \brief Dani Lischinski, Zeev Farbman, Matt Uyttendaele, Richard Szeliski tone mapping operator
//!
//! \param L             [in] image luminance values
//!        inX           [out] image red channel
//!        inY           [out] image red channel
//!        inZ           [out] image red channel
//!        alpha         value of exposure of the image
//!        white_point   the white point
//!
int tmo_lischinski06(pfs::Array2Df &L, pfs::Array2Df &inX, pfs::Array2Df &inY, pfs::Array2Df &inZ,
                     float alpha, float white_point,
                     pfs::Progress &ph);

#endif  // TMO_LISCHINSKI_H
