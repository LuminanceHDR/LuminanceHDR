/*
 * This file is a part of LuminanceHDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2006,2007 Giuseppe Rota
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
 *  Original Work
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 *  Improvements, bugfixing
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *
 */

#ifndef FROMLDRPFSTOQIMAGE
#define FROMLDRPFSTOQIMAGE

#include <QImage>
#include <QRgb>

#include <Libpfs/colorspace/normalizer.h>
#include <Libpfs/colorspace/rgbremapper.h>
#include <Libpfs/utils/chain.h>
#include <Libpfs/utils/clamp.h>

// forward declaration
namespace pfs {
class Frame;
}

struct QRgbRemapper {
    QRgbRemapper(float minLuminance, float maxLuminance,
                 RGBMappingType mappingType);

    void operator()(float r, float g, float b, QRgb &qrgb) const;

   private:
    typedef pfs::utils::Chain<
        pfs::colorspace::Normalizer,
        pfs::utils::Chain<pfs::utils::Clamp<float>, Remapper<uint8_t>>>
        QRgbRemapperCore;

    QRgbRemapperCore m_remapper;
};

//! \brief Build from a pfs::Frame a QImage of the same size
//! \param[in] in_frame is a pointer to pfs::Frame*
//! \return Pointer to QImage containing an 8 bit/channel representation of the
//! input frame
QImage *fromLDRPFStoQImage(pfs::Frame *in_frame, float min_luminance = 0.0f,
                           float max_luminance = 1.0f,
                           RGBMappingType mapping_method = MAP_LINEAR);

#endif
