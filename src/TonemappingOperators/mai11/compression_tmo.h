/**
 * @brief Tone-mapping optimized for backward-compatible HDR image and video
 * compression
 *
 * From:
 * Mai, Z., Mansour, H., Mantiuk, R., Nasiopoulos, P., Ward, R., & Heidrich, W.
 * Optimizing a tone curve for backward-compatible high dynamic range image and
 * video compression.
 * IEEE Transactions on Image Processing, 20(6), 1558 – 1571.
 * doi:10.1109/TIP.2010.2095866, 2011
 *
 * This file is a part of LuminanceHDR package, based on pfstmo.
 * ----------------------------------------------------------------------
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
 * @author Rafal Mantiuk, <mantiuk@gmail.com>
 *
 * $Id: pfstmo_mantiuk08.cpp,v 1.19 2013/12/28 14:00:54 rafm Exp $
 */

#ifndef COMPRESSION_TMO
#define COMPRESSION_TMO

#include "Libpfs/progress.h"

namespace mai {
class CompressionTMO {
   public:
    void tonemap(const float *R_in, const float *G_in, float *B_in, int width,
                 int height, float *R_out, float *G_out, float *B_out,
                 const float *L_in, pfs::Progress &ph);
};
}

#endif
