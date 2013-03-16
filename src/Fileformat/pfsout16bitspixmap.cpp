/**
 * @brief Writing 16Bits Pixmap from PFS stream (which is a tonemapped LDR)
 *
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2011 Franco Comida
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
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#include "Fileformat/pfsout16bitspixmap.h"

#include <cassert>
#include <iostream>

#include <QSharedPointer>

#include <Libpfs/frame.h>
#include <Libpfs/utils/msec_timer.h>
#include <Libpfs/colorspace/rgbremapper.h>

quint16* fromLDRPFSto16bitsPixmap(pfs::Frame* inpfsframe,
                                  float min_luminance,
                                  float max_luminance,
                                  RGBMappingType mapping_method)
{
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif

#ifdef QT_DEBUG
    assert( inpfsframe != NULL );
#endif

    pfs::Channel *Xc, *Yc, *Zc;
    inpfsframe->getXYZChannels( Xc, Yc, Zc );
    assert( Xc != NULL && Yc != NULL && Zc != NULL );

    const int width   = inpfsframe->getWidth();
    const int height  = inpfsframe->getHeight();

    quint16* temp_pixmap = new quint16[3*width*height];

    // DAVIDE -> FIX THIS FUNCTION!
    const float* p_R = Xc->data();
    const float* p_G = Yc->data();
    const float* p_B = Zc->data();

    RGBRemapper rgbRemapper(min_luminance, max_luminance, mapping_method);
	
#pragma omp parallel for
    for (int idx = 0; idx < height*width; ++idx)
    {
        rgbRemapper.toUint16(p_R[idx], p_G[idx], p_B[idx],
                           temp_pixmap[3*idx], temp_pixmap[3*idx + 1], temp_pixmap[3*idx + 2]);
    }

#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "fromLDRPFSto16bitsPixmap() = " << stop_watch.get_time() << " msec" << std::endl;
#endif
	
    return temp_pixmap;
}
