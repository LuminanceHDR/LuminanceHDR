/*
 * This file is a part of Luminance HDR package
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

#ifndef PFS_RESIZE_H
#define PFS_RESIZE_H

//! \brief Resize images in PFS stream
//! \author Rafal Mantiuk, <mantiuk@mpi-sb.mpg.de>
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

//#include "Libpfs/array2d_fwd.h"
#include "Common/global.h"
#include "Libpfs/array2d.h"

namespace pfs {
// forward declaration
class Frame;

Frame *resize(Frame *frame, int xSize, InterpolationMethod m);

template <typename Type>
void resize(const Array2D<Type> *from, Array2D<Type> *to,
            InterpolationMethod m);

template <typename Type>
void resize(const Array2D<Type> &from, Array2D<Type> &to,
            InterpolationMethod m) {
    resize(&from, &to, m);
}
}

#include "resize.hxx"

#endif  // PFS_RESIZE_H
