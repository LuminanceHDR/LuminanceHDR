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

#include <assert.h>
#include <iostream>
#include <QSharedPointer>

#include "Libpfs/frame.h"
#include "Fileformat/pfsout16bitspixmap.h"
#include "Common/msec_timer.h"

namespace
{

inline quint16 clamp_to_16bits(const float& value)
{
    if (value <= 0.0f) return 0;
    if (value >= 65535.f) return 65535;
    return (quint16)(value*65535.f + 0.5f);
}

//! \note I pass value by value, so I can use it as a temporary variable inside the function
//! I will let the compiler do the optimization that it likes
inline quint16 clamp_and_offset_to_16bits(float value, const float& min, const float& max)
{
    if (value <= min) value = min;
    else if (value >= max) value = max;

    value = (value - min)/(max - min);

    return (quint16)(value*65535.f + 0.5f);
}

}

quint16* fromLDRPFSto16bitsPixmap(pfs::Frame* inpfsframe, float min_luminance, float max_luminance)
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

    const float* p_R = Xc->getChannelData()->getRawData();
    const float* p_G = Yc->getChannelData()->getRawData();
    const float* p_B = Zc->getChannelData()->getRawData();
	
#pragma omp parallel for
    for (int idx = 0; idx < height*width; ++idx)
    {
        temp_pixmap[3*idx] = clamp_and_offset_to_16bits(p_R[idx], min_luminance, max_luminance);
        temp_pixmap[3*idx + 1] = clamp_and_offset_to_16bits(p_G[idx], min_luminance, max_luminance);
        temp_pixmap[3*idx + 2] = clamp_and_offset_to_16bits(p_B[idx], min_luminance, max_luminance);
    }

#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "fromLDRPFSto16bitsPixmap() = " << stop_watch.get_time() << " msec" << std::endl;
#endif
	
    return temp_pixmap;
}
