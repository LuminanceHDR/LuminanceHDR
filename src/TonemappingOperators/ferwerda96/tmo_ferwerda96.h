/*
 * @brief Ferwerda Tone Mapping Operator:
 *    "A Model of Visual Adaptation for Realistic Image Synthesis"
 *     by James A. Ferwerda, Sumanta N. Pattanaik, Peter Shirley, Donald P. Greenberg
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

#ifndef TMO_FERWERDA_H
#define TMO_FERWERDA_H

#include <Libpfs/array2d_fwd.h>

namespace pfs {
class Frame;
class Progress;
}

//! \brief A. Ferwerda tone mapping operator
//!
//! \param L [in] image luminance values
//! \param X, Y, Z [out] tone mapped values
//! \param Ld_Max: maximum luminance of the display in cd/m^2
//! \param L_da: adaptation luminance in cd/m^2
//!
int tmo_ferwerda96(pfs::Array2Df *X, pfs::Array2Df *Y, pfs::Array2Df *Z, pfs::Array2Df *L,
                    float Ld_Max, float L_da,
                    pfs::Progress &ph);

#endif  // TMO_FERWERDA_H
