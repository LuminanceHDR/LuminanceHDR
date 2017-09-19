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

#ifndef PFS_SHIFT_HXX
#define PFS_SHIFT_HXX

#include <Libpfs/array2d.h>
#include <Libpfs/manip/shift.h>
#include <Libpfs/utils/msec_timer.h>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>

namespace pfs {

template <typename Type>
void shift(const Array2D<Type> &in, int dx, int dy, Array2D<Type> &out) {
    typedef Array2D<Type> Array2DType;

    using namespace std;

#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif

    // fill first row... if any!
    for (int idx = 0; idx < -dy; idx++) {
        fill(out.row_begin(idx), out.row_end(idx), Type());
    }

    // fill middle portion
    if (dx < 0) {
        for (int row = max(0, -dy), rowEnd = in.getRows() - max(0, dy);
             row < rowEnd; row++) {
            // Begin output line
            typename Array2DType::iterator itBegin = out.row_begin(row);
            // Pivot iterator
            typename Array2DType::iterator itTh = itBegin - dx;

            // fill zero at the begin of the line
            fill(itBegin, itTh, Type());
            // copy data
            copy(in.row_begin(row + dy), in.row_end(row + dy) + dx, itTh);
        }
    } else if (dx > 0) {
        for (int row = max(0, -dy), rowEnd = in.getRows() - max(0, dy);
             row < rowEnd; row++) {
            // copy data
            copy(in.row_begin(row + dy) + dx, in.row_end(row + dy),
                 out.row_begin(row));
            // fill zero
            fill(out.row_end(row) - dx, out.row_end(row), Type());
        }
    } else {
        for (int row = max(0, -dy), rowEnd = in.getRows() - max(0, dy);
             row < rowEnd; row++) {
            // copy data
            copy(in.row_begin(row + dy), in.row_end(row + dy),
                 out.row_begin(row));
        }
    }

    // fill last rows... if any!
    for (int idx = dy; idx > 0; idx--) {
        fill(out.row_begin(out.getRows() - idx),
             out.row_end(out.getRows() - idx), Type());
    }

#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "shift Array2D = " << stop_watch.get_time() << " msec"
              << std::endl;
#endif
}

}  // pfs

#endif  // PFS_SHIFT_HXX
