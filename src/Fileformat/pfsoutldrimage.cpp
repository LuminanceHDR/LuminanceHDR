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

#include "pfsoutldrimage.h"

#include <QDebug>
#include <QImage>

#include <assert.h>
#include <iostream>
#include <stdexcept>

#include <boost/assign/list_of.hpp>

#include <Libpfs/colorspace/rgbremapper.h>
#include <Libpfs/exception.h>
#include <Libpfs/frame.h>
#include <Libpfs/utils/msec_timer.h>
#include <Libpfs/utils/transform.h>

using namespace std;
using namespace pfs;
using namespace boost::assign;

using namespace pfs;

QRgbRemapper::QRgbRemapper(float minLuminance, float maxLuminance,
                           RGBMappingType mappingType)
    : m_remapper(
          utils::chain(colorspace::Normalizer(minLuminance, maxLuminance),
                       utils::CLAMP_F32, Remapper<uint8_t>(mappingType))) {}

void QRgbRemapper::operator()(float r, float g, float b, QRgb &qrgb) const {
    qrgb = qRgb(m_remapper(r), m_remapper(g), m_remapper(b));
}

QImage *fromLDRPFStoQImage(pfs::Frame *in_frame, float min_luminance,
                           float max_luminance, RGBMappingType mapping_method) {
#ifdef TIMER_PROFILING
    msec_timer stop_watch;
    stop_watch.start();
#endif

    qDebug() << "Min Luminance: " << min_luminance;
    qDebug() << "Max Luminance: " << max_luminance;
    qDebug() << "Mapping method: " << mapping_method;

    assert(in_frame != NULL);

    pfs::Channel *Xc, *Yc, *Zc;
    in_frame->getXYZChannels(Xc, Yc, Zc);
    assert(Xc != NULL && Yc != NULL && Zc != NULL);

    QImage *temp_qimage = new QImage(
        in_frame->getWidth(), in_frame->getHeight(), QImage::Format_RGB32);

    QRgbRemapper remapper(min_luminance, max_luminance, mapping_method);
    utils::transform(Xc->begin(), Xc->end(), Yc->begin(), Zc->begin(),
                     reinterpret_cast<QRgb *>(temp_qimage->bits()), remapper);

#ifdef TIMER_PROFILING
    stop_watch.stop_and_update();
    std::cout << "fromLDRPFStoQImage() = " << stop_watch.get_time() << " msec"
              << std::endl;
#endif

    return temp_qimage;
}
