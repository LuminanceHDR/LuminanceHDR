/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
 * Copyright (C) 2012 Davide Anastasia
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
 */

#ifndef PFS_GAMMA_H
#define PFS_GAMMA_H

#include "Libpfs/array2d_fwd.h"

//! \brief Apply gamma correction the the pfs stream
//! \author Rafal Mantiuk <mantiuk@mpi-sb.mpg.de>
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

namespace pfs {
class Frame;

//! \brief Apply \c gamma on the input \c frame
void applyGamma(pfs::Frame *frame, float gamma);

//! \brief Apply gamma on the input \c array
void applyGamma(pfs::Array2Df *array, float exponent);
}

#endif  // PFSGAMMA_H
