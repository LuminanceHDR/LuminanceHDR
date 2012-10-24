/*
 *
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
 * @author Dorota Zdrojewska, <dzdrojewska@wi.ps.pl>
 * adapted by Franco Comida <fcomida@sourceforge.net> for luminance
 *
 */

#include "cut.h"

#include <climits>
#include <cstdlib>
#include <iostream>

#include "Common/msec_timer.h"

#include "Libpfs/domio.h"
#include "Libpfs/frame.h"

namespace pfs
{

void cut(const Array2D *from, Array2D *to,
         int x_ul, int y_ul, int x_br, int y_br)
{
    const float* fv = from->getRawData();
    float* tv       = to->getRawData();

    const int IN_W    = from->getCols();
    const int IN_H    = from->getRows();
    const int OUT_W   = to->getCols();
    const int OUT_H   = to->getRows();

    assert( OUT_H <= IN_H );
    assert( OUT_H <= IN_H );
    assert( x_ul >= 0 );
    assert( y_ul >= 0 );
    assert( x_br <= IN_W );
    assert( y_br <= IN_H );

    // move to row (x_ul, y_ul)
    fv = &fv[IN_W*y_ul + x_ul];

#pragma omp parallel for
    for (int r = 0; r < OUT_H; r++)
    {
        //NOTE: do NOT use VEX_vcopy
        for (int c = 0; c < OUT_W; c++)
        {
            tv[r*OUT_W + c] = fv[r*IN_W + c];
        }
    }
}

pfs::Frame *cut(const pfs::Frame *inFrame,
                int x_ul, int y_ul, int x_br, int y_br)
{
#ifdef TIMER_PROFILING
    msec_timer f_timer;
    f_timer.start();
#endif
    
    // ----  Boundary Check!
    if (x_ul < 0) x_ul = 0;
    if (y_ul < 0) y_ul = 0;
    if (x_br > inFrame->getWidth()) x_br = inFrame->getWidth();
    if (y_br > inFrame->getHeight()) y_br = inFrame->getHeight();
    // ----- 
    
    pfs::Frame *outFrame = pfs::DOMIO::createFrame((x_br-x_ul), (y_br-y_ul));
    
    const ChannelContainer& channels = inFrame->getChannels();
    
    for ( ChannelContainer::const_iterator it = channels.begin();
          it != channels.end();
          ++it)
    {
        const pfs::Channel* inCh = *it;

        pfs::Channel *outCh = outFrame->createChannel(inCh->getName());

        const pfs::Array2D* inArray2D   = inCh->getChannelData();
        pfs::Array2D* outArray2D  = outCh->getChannelData();

        cut(inArray2D, outArray2D, x_ul, y_ul, x_br, y_br);
    }
    
    pfs::copyTags(inFrame, outFrame);
    
#ifdef TIMER_PROFILING
    f_timer.stop_and_update();
    std::cout << "pfscut(";
    std::cout << "[" << x_ul <<", " << y_ul <<"],";
    std::cout << "[" << x_br << ", " << y_br <<"]";
    std::cout << ") = " << f_timer.get_time() << " msec" << std::endl;
#endif 
    
    return outFrame;
}

} // pfs
