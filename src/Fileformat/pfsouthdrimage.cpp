/**
 * @brief Writing QImage from PFS stream 
 *
 * This file is a part of Luminance HDR package.
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

#include <QDebug>
#include <QImage>
#include <QSysInfo>
#include <iostream>
#include <assert.h>
#include <math.h>

#include "Libpfs/frame.h"
#include "Libpfs/utils/msec_timer.h"

namespace
{

//! \note I pass value by value, so I can use it as a temporary variable inside the function
//! I will let the compiler do the optimization that it likes
inline int clamp_and_offset_to_8bits(float value, const float& min, const float& max)
{
    if (value <= min) value = min;
    else if (value >= max) value = max;

    value = (value - min)/(max - min);
	
	value = pow(value, 1.0f/2.2f);

    return (quint16) (value*255.f + 0.5f);
}

}

QImage* fromHDRPFStoQImage(pfs::Frame* in_frame)
{
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif

    assert( in_frame != NULL );

	float min_luminance, max_luminance;
    pfs::Channel *Xc, *Yc, *Zc;
    in_frame->getXYZChannels( Xc, Yc, Zc );
    assert( Xc != NULL && Yc != NULL && Zc != NULL );

    const int width   = in_frame->getWidth();
    const int height  = in_frame->getHeight();
    // const int elems = width*height;

    QImage* temp_qimage = new QImage(width, height, QImage::Format_ARGB32);

    // DAVIDE - FIX THIS FUNCTION PLEASE!
    const float* p_R = Xc->data();
    const float* p_G = Yc->data();
    const float* p_B = Zc->data();

    QRgb *pixels = reinterpret_cast<QRgb*>(temp_qimage->bits());

	max_luminance = p_R[0];
	for (int idx = 1; idx < height*width; ++idx)
		if (p_R[idx] > max_luminance)
			max_luminance = p_R[idx];

	for (int idx = 0; idx < height*width; ++idx)
		if (p_G[idx] > max_luminance)
			max_luminance = p_G[idx];
	
	for (int idx = 0; idx < height*width; ++idx)
		if (p_B[idx] > max_luminance)
			max_luminance = p_B[idx];

	min_luminance = p_R[0];
	for (int idx = 1; idx < height*width; ++idx)
		if (p_R[idx] < min_luminance)
			min_luminance = p_R[idx];

	for (int idx = 0; idx < height*width; ++idx)
		if (p_G[idx] < min_luminance)
			min_luminance = p_G[idx];
	
	for (int idx = 0; idx < height*width; ++idx)
		if (p_B[idx] < min_luminance)
			min_luminance = p_B[idx];

	qDebug() << "max luminance: " << max_luminance;
	qDebug() << "min luminance: " << min_luminance;

#pragma omp parallel for shared(pixels)
    for (int idx = 0; idx < height*width; ++idx)
    {
        pixels[idx] = qRgb(clamp_and_offset_to_8bits(p_R[idx], min_luminance, max_luminance),
                           clamp_and_offset_to_8bits(p_G[idx], min_luminance, max_luminance),
                           clamp_and_offset_to_8bits(p_B[idx], min_luminance, max_luminance));
    }

#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "fromHDRPFStoQImage() = " << stop_watch.get_time() << " msec" << std::endl;
#endif

    return temp_qimage;
}
