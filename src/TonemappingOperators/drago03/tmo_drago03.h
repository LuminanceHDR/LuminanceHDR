/**
 * @brief Frederic Drago logmapping operator
 *
 * This file is a part of LuminanceHDR package, based on pfstmo.
 * ----------------------------------------------------------------------
 * Copyright (C) 2003,2004 Grzegorz Krawczyk
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
 * $Id: tmo_drago03.h,v 1.3 2008/09/04 12:46:48 julians37 Exp $
 */

#ifndef TMO_DRAGO03_H
#define TMO_DRAGO03_H

#include <Libpfs/array2d_fwd.h>

namespace pfs {
class Progress;
}

//! \brief Frederic Drago Logmapping Algorithm
//!
//! Original implementation obtained from source code provided
//! by Frederic Drago on 16 May 2003 (pfstmo)
//!
//! \param width image width
//! \param height image height
//! \param Y [in] image luminance values
//! \param L [out] tone mapped values
//! \param maxLum maximum luminance in the image
//! \param avLum logarithmic average of luminance in the image
//! \param bias bias parameter of tone mapping algorithm (eg 0.85)
//!
void tmo_drago03(const pfs::Array2Df &Y, pfs::Array2Df &L, float maxLum,
                 float avLum, float bias, pfs::Progress &ph);

//! \brief Find average and maximum luminance in an image
//!
//! \param Y [in] image luminance values
//! \param avLum [out] average luminance
//! \param maxLum [out] maximum luminance
//!
void calculateLuminance(unsigned int width, unsigned int height, const float *Y,
                        float &avLum, float &maxLum);

#endif
