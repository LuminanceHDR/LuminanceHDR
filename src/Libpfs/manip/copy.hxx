/*
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
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
 *
 */

//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#ifndef PFS_COPY_HXX
#define PFS_COPY_HXX

#include "copy.h"

#include <algorithm>
#include <cassert>

namespace pfs {

template <typename Type>
void copy(const Array2D<Type> *from, Array2D<Type> *to) {
    assert(from->getRows() == to->getRows());
    assert(from->getCols() == to->getCols());

    std::copy(from->begin(), from->end(), to->begin());
}
}

#endif  // #ifndef PFS_COPY_HXX
