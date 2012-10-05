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
#include <stdexcept>

#include "pfsoutldrimage.h"
#include "Libpfs/frame.h"
#include "Common/msec_timer.h"

QImage* fromLDRPFStoQImage(pfs::Frame* in_frame,
                           float min_luminance,
                           float max_luminance,
                           LumMappingMethod mapping_method)
{
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif

    //assert( in_frame != NULL );
    if (in_frame == NULL)
        throw std::runtime_error("Null pointer");

    pfs::Channel *Xc, *Yc, *Zc;
    in_frame->getXYZChannels( Xc, Yc, Zc );
    assert( Xc != NULL && Yc != NULL && Zc != NULL );

    const int width   = in_frame->getWidth();
    const int height  = in_frame->getHeight();

    QImage* temp_qimage = new QImage(width, height, QImage::Format_RGB32);

    const float* p_R = Xc->getChannelData()->getRawData();
    const float* p_G = Yc->getChannelData()->getRawData();
    const float* p_B = Zc->getChannelData()->getRawData();

    QRgb *pixels = reinterpret_cast<QRgb*>(temp_qimage->bits());

    FloatRgbToQRgb converter(min_luminance, max_luminance, mapping_method);

#pragma omp parallel for shared(pixels)
    for (int idx = 0; idx < height*width; ++idx)
    {
        converter.toQRgb(p_R[idx], p_G[idx], p_B[idx], pixels[idx]);
    }

#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "fromLDRPFStoQImage() = " << stop_watch.get_time() << " msec" << std::endl;
#endif

    return temp_qimage;
}
