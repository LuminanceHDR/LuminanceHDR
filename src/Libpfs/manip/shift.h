/*
* This file is a part of Luminance HDR package.
* ----------------------------------------------------------------------
* Copyright (C) 2012 Davide Anastasia
*
*  This library is free software; you can redistribute it and/or
*  modify it under the terms of the GNU Lesser General Public
*  License as published by the Free Software Foundation; either
*  version 2.1 of the License, or (at your option) any later version.
*
*  This library is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*  Lesser General Public License for more details.
*
*  You should have received a copy of the GNU Lesser General Public
*  License along with this library; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
* ----------------------------------------------------------------------
*/

#ifndef PFS_SHIFT_H
#define PFS_SHIFT_H

#include <Libpfs/array2d_fwd.h>
#include <Libpfs/frame.h>

namespace pfs {
//! \brief shift \c Array2D by \a dx \a dy
template <typename Type>
void shift(const Array2D<Type> &in, int dx, int dy, Array2D<Type> &out);

//! \brief shift image by \a dx \a dy
pfs::Frame *shift(const pfs::Frame &in, int dx, int dy);

// template <typename Type>
// pfs::Array2D<Type>* shift(const pfs::Array2D<Type>& in, int dx, int dy);

}  // pfs

#include <Libpfs/manip/shift.hxx>
#endif  // PFS_SHIFT_H
