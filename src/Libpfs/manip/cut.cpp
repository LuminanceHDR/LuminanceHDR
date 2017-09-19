/*
 * ----------------------------------------------------------------------
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
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

//! \author Dorota Zdrojewska, <dzdrojewska@wi.ps.pl>
//! \author Franco Comida <fcomida@sourceforge.net>
//! Adapted for Luminance HDR
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! Improved for better performance

#include "cut.h"

#include <algorithm>
#include <cassert>
#include <climits>
#include <cstdlib>
#include <iostream>

#include "Libpfs/frame.h"
#include "Libpfs/utils/msec_timer.h"

namespace pfs {

pfs::Frame *cut(const pfs::Frame *inFrame, size_t x_ul, size_t y_ul,
                size_t x_br, size_t y_br) {
#ifdef TIMER_PROFILING
    msec_timer f_timer;
    f_timer.start();
#endif

    // ----  Boundary Check!
    // if (x_ul < 0) x_ul = 0;
    // if (y_ul < 0) y_ul = 0;
    if (x_br > inFrame->getWidth()) x_br = inFrame->getWidth();
    if (y_br > inFrame->getHeight()) y_br = inFrame->getHeight();
    // -----

    pfs::Frame *outFrame = new pfs::Frame((x_br - x_ul), (y_br - y_ul));

    const ChannelContainer &channels = inFrame->getChannels();

    for (ChannelContainer::const_iterator it = channels.begin();
         it != channels.end(); ++it) {
        const pfs::Channel *inCh = *it;

        pfs::Channel *outCh = outFrame->createChannel(inCh->getName());

        cut(inCh, outCh, x_ul, y_ul, x_br, y_br);
    }

    pfs::copyTags(inFrame, outFrame);

#ifdef TIMER_PROFILING
    f_timer.stop_and_update();
    std::cout << "pfscut(";
    std::cout << "[" << x_ul << ", " << y_ul << "],";
    std::cout << "[" << x_br << ", " << y_br << "]";
    std::cout << ") = " << f_timer.get_time() << " msec" << std::endl;
#endif

    return outFrame;
}

}  // pfs
