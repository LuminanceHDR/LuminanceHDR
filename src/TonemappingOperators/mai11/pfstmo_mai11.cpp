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

#include <sstream>

#include "Libpfs/colorspace/colorspace.h"
#include "Libpfs/exception.h"
#include "Libpfs/frame.h"

#include "compression_tmo.h"

using namespace mai;

void pfstmo_mai11(pfs::Frame &frame, pfs::Progress &ph) {

#ifndef NDEBUG
    std::stringstream ss;
    ss << "pfstmo_mai11 (";
    ss << ")";
    std::cout << ss.str() << std::endl;
#endif

    //--- default tone mapping parameters;

    ph.setValue(0);

    CompressionTMO tmo;

    pfs::Channel *inX, *inY, *inZ;

    frame.getXYZChannels(inX, inY, inZ);
    if (inX == NULL || inY == NULL || inZ == NULL) {
        throw pfs::Exception("Missing X, Y, Z channels in the PFS stream");
    }
    int cols = frame.getWidth();
    int rows = frame.getHeight();

    try {
        tmo.tonemap(inX->data(), inY->data(), inZ->data(), cols, rows,
                    inX->data(), inY->data(), inZ->data(), inY->data(), ph);
    } catch (...) {
        throw pfs::Exception("Tonemapping Failed!");
    }

    frame.getTags().setTag("LUMINANCE", "DISPLAY");
    if (!ph.canceled()) ph.setValue(100);
}
