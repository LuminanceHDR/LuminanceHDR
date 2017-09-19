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

#ifndef PFS_CUT_HXX
#define PFS_CUT_HXX

//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#include "cut.h"

#include <algorithm>
#include <cassert>

namespace pfs {

template <typename Type>
void cut(const Array2D<Type> *from, Array2D<Type> *to, size_t x_ul, size_t y_ul,
         size_t x_br, size_t y_br) {
    // assert( x_ul >= 0 );        // must be obvious... so obvious that x_ul is
    // unsigned
    // assert( y_ul >= 0 );        // must be obvious... so obvious that y_ul is
    // unsigned
    assert(x_br <= from->getCols());
    assert(y_br <= from->getRows());
    assert(to->getRows() <= from->getRows());
    assert(to->getRows() <= from->getRows());

    // if ( x_ul < 0 ) x_ul = 0; // unsigned
    // if ( y_ul < 0 ) y_ul = 0; // unsigned
    if (x_br > from->getCols()) x_br = from->getCols();
    if (y_br > from->getRows()) y_br = from->getRows();

    // update right border
    x_br = from->getCols() - x_br;
    int rEnd = (int)to->getRows();
#pragma omp parallel for shared(rEnd)
    for (int r = 0; r < rEnd; r++) {
        std::copy(from->row_begin(r + y_ul) + x_ul,
                  from->row_end(r + y_ul) - x_br, to->row_begin(r));
    }
}

}  // pfs

#endif  // PFS_CUT_HXX
