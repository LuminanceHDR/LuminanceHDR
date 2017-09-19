/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2009 Franco Comida
 * Copyrighr (C) 2012 Davide Anastasia
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

#ifndef PFS_CUT_H
#define PFS_CUT_H

#include <Libpfs/array2d_fwd.h>
#include <cstddef>

//! \brief Cut a rectangle out of images in PFS stream
//! \author Franco Comida <fcomida@users.sourceforge.net>
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

namespace pfs {
class Frame;

Frame *cut(const Frame *inFrame, size_t x_ul, size_t y_ul, size_t x_br,
           size_t y_br);

template <typename Type>
void cut(const Array2D<Type> *from, Array2D<Type> *to, size_t x_ul, size_t y_ul,
         size_t x_br, size_t y_br);
}

#include "cut.hxx"

#endif
