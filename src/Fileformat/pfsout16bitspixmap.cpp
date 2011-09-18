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

#include "pfsout16bitspixmap.h"
#include "Common/msec_timer.h"

inline quint16 clamp2quint16( const float v, const float minV, const float maxV )
{
    if ( v < minV ) return (quint16)minV;
    if ( v > maxV ) return (quint16)maxV;
    return (quint16)v;
}

quint16* fromLDRPFSto16bitsPixmap( pfs::Frame* inpfsframe)
{
#ifdef TIMER_PROFILING
    msec_timer __timer;
    __timer.start();
#endif

    assert( inpfsframe != NULL );

    pfs::Channel *Xc, *Yc, *Zc;
    inpfsframe->getXYZChannels( Xc, Yc, Zc );
    assert( Xc != NULL && Yc != NULL && Zc != NULL );

    pfs::Array2D  *X = Xc->getChannelData();
    pfs::Array2D  *Y = Yc->getChannelData();
    pfs::Array2D  *Z = Zc->getChannelData();

    const int width   = Xc->getWidth();
    const int height  = Xc->getHeight();

    quint16* temp_pixmap = new quint16[3*width*height];

    const float* p_R = X->getRawData();
    const float* p_G = Y->getRawData();
    const float* p_B = Z->getRawData();

    quint16 red, green, blue;
	
	int d = 0;
    for (int idx = 0; idx < 3*height*width; idx += 3)
    {
        red = clamp2quint16(p_R[d]*65535.f, 0.0f, 65535.f);
        green = clamp2quint16(p_G[d]*65535.f, 0.0f, 65535.f);
        blue = clamp2quint16(p_B[d]*65535.f, 0.0f, 65535.f);

        temp_pixmap[idx] = red;
        temp_pixmap[idx + 1] = green;
        temp_pixmap[idx + 2] = blue;
		d++;
    }

#ifdef TIMER_PROFILING
    __timer.stop_and_update();
    std::cout << "fromLDRPFSto16bitsPixmap() = " << __timer.get_time() << " msec" << std::endl;
#endif
	
    return temp_pixmap;
}
