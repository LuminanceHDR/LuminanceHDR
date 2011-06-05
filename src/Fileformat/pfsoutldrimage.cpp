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
#include "Libpfs/colorspace.h"
#include "Common/msec_timer.h"

inline int clamp2int( const float v, const float minV, const float maxV )
{
    if ( v < minV ) return (int)minV;
    if ( v > maxV ) return (int)maxV;
    return (int)v;
}

QImage* fromLDRPFStoQImage( pfs::Frame* inpfsframe , pfs::ColorSpace display_colorspace )
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

    // Back to CS_RGB for the Viewer
    pfs::transformColorSpace(pfs::CS_XYZ, X, Y, Z, display_colorspace, X, Y, Z);

    const int width   = Xc->getWidth();
    const int height  = Xc->getHeight();

    QImage * temp_qimage = new QImage (width, height, QImage::Format_ARGB32);

    const float* p_R = X->getRawData();
    const float* p_G = Y->getRawData();
    const float* p_B = Z->getRawData();

    int red, green, blue;
    QRgb *pixels = reinterpret_cast<QRgb*>(temp_qimage->bits());

#pragma omp parallel for private(red, green, blue) shared(pixels)
    for (int idx = 0; idx < height*width; ++idx)
    {
        red = clamp2int(p_R[idx]*255.f, 0.0f, 255.f);
        green = clamp2int(p_G[idx]*255.f, 0.0f, 255.f);
        blue = clamp2int(p_B[idx]*255.f, 0.0f, 255.f);

        pixels[idx] = qRgb(red, green, blue);
    }


#ifdef TIMER_PROFILING
    __timer.stop_and_update();
    std::cout << "fromLDRPFStoQImage() = " << __timer.get_time() << " msec" << std::endl;
#endif

    return temp_qimage;
}
