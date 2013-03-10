/*
 *
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2006 Giuseppe Rota
 * Copyright (C) 2010, 2011, 2012, 2013 Davide Anastasia
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

//! \brief Writing QImage from PFS stream (which is a tonemapped LDR)
//! \author Giuseppe Rota <grota@users.sourceforge.net>
//! \ author Davide Anastasia <davideanastasia@users.sourceforge.net>
//!  New implementation:
//!  1) avoids the presence of a temporary buffer
//!  2) returns QImage* instead than a QImage
//!  3) has OpenMP (multi thread) capability)

#include <QImage>
#include <QSysInfo>
#include <iostream>
#include <assert.h>
#include <stdexcept>

#include <boost/assign/list_of.hpp>

#include "pfsoutldrimage.h"
#include "Libpfs/frame.h"
#include "Libpfs/utils/msec_timer.h"

using namespace std;
using namespace boost::assign;

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

// pixel shuffle operators
template <typename Type>
void rgbToRgb(Type& /*red*/, Type& /*green*/, Type& /*blue*/) {}

template <typename Type>
void rgbToBgr(Type& red, Type& /*green*/, Type& blue)
{
    std::swap(red, blue);
}


// uint8_t section
typedef void (*PixelShuffleUint8)(uint8_t& red, uint8_t& green, uint8_t& blue);
typedef map<RGBFormat, PixelShuffleUint8> PixelShuffleUint8Factory;

void getPixelShuffle(RGBFormat order, PixelShuffleUint8& ptr)
{
    static PixelShuffleUint8Factory s_pixelShuffle = map_list_of
            (RGB_FORMAT, rgbToRgb<uint8_t>)
            (BGR_FORMAT, rgbToBgr<uint8_t>);

    PixelShuffleUint8Factory::const_iterator it = s_pixelShuffle.find(order);
    if ( it != s_pixelShuffle.end() ) {
        ptr = it->second;
    }
    ptr = rgbToRgb<uint8_t>;
}

void planarToInterleaved(const float *red, const float *green, const float *blue,
                         uint8_t *data, RGBFormat rgbOrder,
                         size_t size, const FloatRgbToQRgb &func)
{
    PixelShuffleUint8 rgbShuffle = NULL;
    getPixelShuffle(rgbOrder, rgbShuffle);

#pragma omp parallel for
    for (int idx = 0; idx < size; ++idx)
    {
        func.toUint8(red[idx], green[idx], blue[idx],
                     data[idx*3], data[idx*3 + 1], data[idx*3 + 2]);
        rgbShuffle(data[idx*3], data[idx*3 + 1], data[idx*3 + 2]);
    }
}

// uint16_t section
typedef void (*PixelShuffleUint16)(uint16_t& red, uint16_t& green, uint16_t& blue);
typedef map<RGBFormat, PixelShuffleUint16> PixelShuffleUint16Factory;

void getPixelShuffle(RGBFormat order, PixelShuffleUint16& ptr)
{
    static PixelShuffleUint16Factory s_pixelShuffle = map_list_of
            (RGB_FORMAT, rgbToRgb<uint16_t>)
            (BGR_FORMAT, rgbToBgr<uint16_t>);

    PixelShuffleUint16Factory::const_iterator it = s_pixelShuffle.find(order);
    if ( it != s_pixelShuffle.end() ) {
        ptr = it->second;
    }
    ptr = rgbToRgb<uint16_t>;
}

void planarToInterleaved(const float *red, const float *green, const float *blue,
                         uint16_t *data, RGBFormat rgbOrder,
                         size_t size, const FloatRgbToQRgb &func)
{
    PixelShuffleUint16 rgbShuffle = NULL;
    getPixelShuffle(rgbOrder, rgbShuffle);

#pragma omp parallel for
    for (int idx = 0; idx < size; ++idx)
    {
        func.toUint16(red[idx], green[idx], blue[idx],
                      data[idx*3], data[idx*3 + 1], data[idx*3 + 2]);
        rgbShuffle(data[idx*3], data[idx*3 + 1], data[idx*3 + 2]);
    }
}

// float section
typedef void (*PixelShuffleFloat32)(float& red, float& green, float& blue);
typedef map<RGBFormat, PixelShuffleFloat32> PixelShuffleFloat32Factory;

void getPixelShuffle(RGBFormat order, PixelShuffleFloat32& ptr)
{
    static PixelShuffleFloat32Factory s_pixelShuffle = map_list_of
            (RGB_FORMAT, rgbToRgb<float>)
            (BGR_FORMAT, rgbToBgr<float>);

    PixelShuffleFloat32Factory::const_iterator it = s_pixelShuffle.find(order);
    if ( it != s_pixelShuffle.end() ) {
        ptr = it->second;
    }
    ptr = rgbToRgb<float>;
}

void planarToInterleaved(const float *red, const float *green, const float *blue,
                         float *data, RGBFormat rgbOrder,
                         size_t size, const FloatRgbToQRgb &func)
{
    PixelShuffleFloat32 rgbShuffle = NULL;
    getPixelShuffle(rgbOrder, rgbShuffle);

#pragma omp parallel for
    for (int idx = 0; idx < size; ++idx)
    {
        func.toFloat(red[idx], green[idx], blue[idx],
                      data[idx*3], data[idx*3 + 1], data[idx*3 + 2]);
        rgbShuffle(data[idx*3], data[idx*3 + 1], data[idx*3 + 2]);
    }
}

