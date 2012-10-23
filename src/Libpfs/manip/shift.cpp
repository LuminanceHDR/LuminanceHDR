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

#include "Libpfs/array2d.h"
#include "Common/msec_timer.h"

namespace pfs
{

Array2D *shiftPfsArray2D(const Array2D& in, int dx, int dy)
{
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif

    int width = in.getCols();
    int height = in.getRows();

    Array2D temp(width, height);
    Array2D *out = new Array2D(width, height);

#pragma omp parallel for shared(temp)
    for (int i = 0; i < height*width; i++)
    {
        temp(i) = 0.f;
    }

    // x-shift
#pragma omp parallel for shared(in)
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            if ((i+dx) < 0) continue;
            if ((i+dx) >= width) break;

            if ( in(i+dx, j) > 65535 )
            {
                temp(i, j) = 65535;
            }
            else if ( in(i+dx, j) < 0)
            {
                temp(i, j) = 0;
            }
            else
            {
                temp(i, j) = in(i+dx, j);
            }
        }
    }
    // y-shift
#pragma omp parallel for shared(out)
    for (int i = 0; i < width; i++)
    {
        for (int j = 0; j < height; j++)
        {
            if ((j+dy) < 0) continue;
            if ((j+dy) >= height) break;

            if ( temp(i, j+dy) > 65535 )
            {
                (*out)(i, j) = 65535;
            }
            else if ( temp(i, j+dy) < 0 )
            {
                (*out)(i, j) = 0;
            }
            else
            {
                (*out)(i, j) = temp(i, j+dy);
            }
        }
    }

#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "shiftPfsArray2D = " << stop_watch.get_time() << " msec" << std::endl;
#endif

    return out;
}

} // pfs
