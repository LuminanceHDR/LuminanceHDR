/**
 * @brief Resize images in PFS stream
 *
 * This file is a part of PFSTOOLS package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk,
 *  Alexander Efremov
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
 * @author Alexander Efremov, <aefremov@mpi-sb.mpg.de>
 *
 * $Id: pfsrotate.cpp,v 1.1 2005/06/15 13:36:54 rafm Exp $
 */

#include "rotate.h"

#include <cassert>
#include <iostream>

#include "Libpfs/array2d.h"
#include "Libpfs/frame.h"

#include "Libpfs/utils/msec_timer.h"

namespace pfs {

pfs::Frame *rotate(const pfs::Frame *frame, bool clock_wise) {
#ifdef TIMER_PROFILING
    msec_timer f_timer;
    f_timer.start();
#endif

    pfs::Frame *resizedFrame =
        new pfs::Frame(frame->getHeight(), frame->getWidth());

    const ChannelContainer &channels = frame->getChannels();

    for (ChannelContainer::const_iterator it = channels.begin();
         it != channels.end(); ++it) {
        pfs::Channel *newCh = resizedFrame->createChannel((*it)->getName());

        rotate(*it, newCh, clock_wise);
    }

    pfs::copyTags(frame, resizedFrame);

#ifdef TIMER_PROFILING
    f_timer.stop_and_update();
    std::cout << "rotateFrame() = " << f_timer.get_time() << " msec"
              << std::endl;
#endif

    return resizedFrame;
}

}  // pfs
