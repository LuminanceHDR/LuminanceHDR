/*
* This file is a part of Luminance HDR package.
* ----------------------------------------------------------------------
* Copyright (C) 2013 Davide Anastasia
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

#include <Libpfs/manip/shift.h>

namespace pfs {

Frame *shift(const Frame &frame, int dx, int dy) {
#ifdef TIMER_PROFILING
    msec_timer f_timer;
    f_timer.start();
#endif

    pfs::Frame *shiftedFrame =
        new pfs::Frame(frame.getWidth(), frame.getHeight());

    const ChannelContainer &channels = frame.getChannels();

    for (ChannelContainer::const_iterator it = channels.begin();
         it != channels.end(); ++it) {
        pfs::Channel *newCh = shiftedFrame->createChannel((*it)->getName());

        shift(**it, -dx, -dy, *newCh);
    }

    pfs::copyTags(&frame, shiftedFrame);

#ifdef TIMER_PROFILING
    f_timer.stop_and_update();
    std::cout << "shift() = " << f_timer.get_time() << " msec" << std::endl;
#endif

    return shiftedFrame;
}
}
