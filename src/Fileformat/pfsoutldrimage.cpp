/**
 * @brief Writing QImage from PFS stream (which is a tonemapped LDR)
 *
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2006 Giuseppe Rota
 * Copyright (C) 2010, 2011 Davide Anastasia
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
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *  New implementation:
 *  1) avoids the presence of a temporary buffer
 *  2) returns QImage* instead than a QImage
 *  3) has OpenMP (multi thread) capability)
 *
 */

#include <QImage>
#include <QSysInfo>
#include <iostream>
#include <assert.h>

#include "Libpfs/frame.h"
#include "Common/msec_timer.h"

namespace
{
inline int clamp_to_8bits(const float value)
{
    if (value < 0.0f) return 0;
    if (value > 255.f) return 255;
    return (int)(value*255.f + 0.5f);
}

}

QImage* fromLDRPFStoQImage(pfs::Frame* in_frame)
{
#ifdef TIMER_PROFILING
    msec_timer __timer;
    __timer.start();
#endif

    assert( in_frame != NULL );

    pfs::Channel *Xc, *Yc, *Zc;
    in_frame->getXYZChannels( Xc, Yc, Zc );
    assert( Xc != NULL && Yc != NULL && Zc != NULL );

    const int width   = in_frame->getWidth();
    const int height  = in_frame->getHeight();

    QImage* temp_qimage = new QImage (width, height, QImage::Format_ARGB32);

    const float* p_R = Xc->getChannelData()->getRawData();
    const float* p_G = Yc->getChannelData()->getRawData();
    const float* p_B = Zc->getChannelData()->getRawData();

    QRgb *pixels = reinterpret_cast<QRgb*>(temp_qimage->bits());

#pragma omp parallel for shared(pixels)
    for (int idx = 0; idx < height*width; ++idx)
    {
        pixels[idx] = qRgb(clamp_to_8bits(p_R[idx]),
                           clamp_to_8bits(p_G[idx]),
                           clamp_to_8bits(p_B[idx]));
    }


#ifdef TIMER_PROFILING
    __timer.stop_and_update();
    std::cout << "fromLDRPFStoQImage() = " << __timer.get_time() << " msec" << std::endl;
#endif

    return temp_qimage;
}
