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
 *
 */

//! \brief Apply saturation correction the the pfs stream
//! \author Rafal Mantiuk, <mantiuk@mpi-sb.mpg.de>

#include "saturation.h"

#include <cassert>
#include <cmath>
#include <iostream>

#include "Libpfs/array2d.h"
#include "Libpfs/colorspace/colorspace.h"
#include "Libpfs/colorspace/saturation.h"
#include "Libpfs/utils/transform.h"
#include "Libpfs/frame.h"
#include "Libpfs/utils/msec_timer.h"

using namespace pfs;
using namespace colorspace;
using namespace utils;

namespace pfs {

void applySaturation(pfs::Frame *frame, float multiplier) {

    if (multiplier == 1.0f) return;

    pfs::Channel *X, *Y, *Z;
    frame->getXYZChannels(X, Y, Z);

    applySaturation(X, Y, Z, multiplier);
}

void applySaturation(pfs::Array2Df *R, pfs::Array2Df *G, pfs::Array2Df *B,
                const float multiplier) {
#ifdef TIMER_PROFILING
    msec_timer f_timer;
    f_timer.start();
#endif

    utils::transform(R->begin(), R->end(), G->begin(), B->begin(), R->begin(), G->begin(), B->begin(), ChangeSaturation(multiplier));

#ifdef TIMER_PROFILING
    f_timer.stop_and_update();
    std::cout << "applySaturation() = " << f_timer.get_time() << " msec"
              << std::endl;
#endif
}
}
