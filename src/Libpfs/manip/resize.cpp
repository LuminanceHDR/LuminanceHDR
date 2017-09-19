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

//! \brief Resize images in PFS stream
//! \author Rafal Mantiuk, <mantiuk@mpi-sb.mpg.de>
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <numeric>

#include "resize.h"

#include "Libpfs/utils/msec_timer.h"

#include "Libpfs/frame.h"

namespace pfs {

Frame *resize(Frame *frame, int xSize, InterpolationMethod m) {
#ifdef TIMER_PROFILING
    msec_timer f_timer;
    f_timer.start();
#endif

    int new_x = xSize;
    int new_y = (int)((float)frame->getHeight() * (float)xSize /
                      (float)frame->getWidth());

    pfs::Frame *resizedFrame = new pfs::Frame(new_x, new_y);

    const ChannelContainer &channels = frame->getChannels();
    for (ChannelContainer::const_iterator it = channels.begin();
         it != channels.end(); ++it) {
        pfs::Channel *newCh = resizedFrame->createChannel((*it)->getName());

        resize(*it, newCh, m);
    }
    pfs::copyTags(frame, resizedFrame);

#ifdef TIMER_PROFILING
    f_timer.stop_and_update();
    std::cout << "resizeFrame() = " << f_timer.get_time() << " msec"
              << std::endl;
#endif

    return resizedFrame;
}

}  // pfs
