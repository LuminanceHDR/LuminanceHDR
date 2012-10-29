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

#include "shift.h"

#include <iostream>
#include <algorithm>
#include <iterator>
#include <numeric>

#include "Libpfs/array2d.h"
#include "Libpfs/utils/msec_timer.h"

namespace pfs
{

Array2D *shift(const Array2D& in, int dx, int dy)
{
    using namespace std;

#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif

    Array2D *out = new Array2D(in.getCols(), in.getRows());

    // fill first row... if any!
    for (int idx = 0; idx < -dy; idx++)
    {
        fill(out->beginRow(idx), out->endRow(idx), 0.0f);
    }

    // fill middle portion
    if ( dx < 0 )
    {
        for (int row = max(0, -dy), rowEnd = in.getRows() - max(0, dy);
             row < rowEnd;
             row++)
        {
            // Begin output line
            Array2D::Iterator itBegin = out->beginRow(row);
            // Pivot iterator
            Array2D::Iterator itTh = itBegin - dx;

            // fill zero at the begin of the line
            fill(itBegin, itTh, 0.0f);
            // copy data
            copy(in.beginRow(row + dy),
                 in.endRow(row + dy) + dx,
                 itTh);
        }
    }
    else if (dx > 0)
    {
        for (int row = max(0, -dy), rowEnd = in.getRows() - max(0, dy);
             row < rowEnd;
             row++)
        {
            // copy data
            copy(in.beginRow(row + dy) + dx, in.endRow(row + dy),
                 out->beginRow(row));
            // fill zero
            fill(out->endRow(row) - dx, out->endRow(row), 0.0f);
        }
    }
    else
    {
        for (int row = max(0, -dy), rowEnd = in.getRows() - max(0, dy);
             row < rowEnd;
             row++)
        {
            // copy data
            copy(in.beginRow(row + dy), in.endRow(row + dy),
                 out->beginRow(row));
        }
    }

    // fill last rows... if any!
    for (int idx = dy; idx > 0; idx--)
    {
        fill(out->beginRow(out->getRows() - idx),
             out->endRow(out->getRows() - idx),
             0.0f);
    }

#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "shiftPfsArray2D = " << stop_watch.get_time() << " msec" << std::endl;
#endif

    return out;
}

} // pfs
