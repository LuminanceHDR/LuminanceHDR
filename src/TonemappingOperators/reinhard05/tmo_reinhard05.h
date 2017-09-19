/**
 * @file tmo_reinhard05.cpp
 * @brief Tone map XYZ channels using Reinhard05 model
 *
 * Dynamic Range Reduction Inspired by Photoreceptor Physiology.
 * E. Reinhard and K. Devlin.
 * In IEEE Transactions on Visualization and Computer Graphics, 2005.
 *
 * This file is a part of LuminanceHDR package, based on pfstmo.
 * ----------------------------------------------------------------------
 * Copyright (C) 2007 Grzegorz Krawczyk
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
 * @author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
 *
 * $Id: tmo_reinhard05.h,v 1.2 2008/09/04 12:46:49 julians37 Exp $
 */

#ifndef TMO_REINHARD05_H
#define TMO_REINHARD05_H

#include <cstddef>

namespace pfs {
class Progress;
}

struct Reinhard05Params {
    Reinhard05Params(float brightness, float chromaticAdaptation,
                     float lightAdaptation)
        : m_brightness(brightness),
          m_chromaticAdaptation(chromaticAdaptation),
          m_lightAdaptation(lightAdaptation) {
        // ideally double check that parameters are not outside
        // of the allowed range!
    }

    float m_brightness;
    float m_chromaticAdaptation;
    float m_lightAdaptation;
};

//! \brief: Tone mapping algorithm [Reinhard2005]
//!
//! \param width image width
//! \param height image height
//! \param R red channel
//! \param G green channel
//! \param B blue channel
//! \param Y luminance channel
//! \param br brightness level -8:8 (def 0)
//! \param ca amount of chromatic adaptation 0:1 (saturation, def 0)
//! \param la amount of light adaptation 0:1 (local/global, def 1)
void tmo_reinhard05(size_t width, size_t height, float *R, float *G, float *B,
                    const float *Y, const Reinhard05Params &params,
                    pfs::Progress &ph);

#endif  // TMO_REINHARD05_H
