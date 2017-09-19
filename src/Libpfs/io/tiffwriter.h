/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
 * Copyright (C) 2006 Giuseppe Rota
 * Copyright (C) 2012-2013 Davide Anastasia
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
 */

//! \brief TIFF facilities
//! \author Grzegorz Krawczyk, <krawczyk@mpi-sb.mpg.de>
//! Original author for PFSTOOLS
//! \author Giuseppe Rota <grota@sourceforge.net>
//! slightly modified by for Luminance HDR
//! \author Franco Comida <fcomida@sourceforge.net>
//! added color management support by Franco Comida
//! \author Davide Anastasia <davideanastasia@sourceforge.net>
//! Complete rewrite/refactoring

#ifndef PFS_TIFFWRITER_H
#define PFS_TIFFWRITER_H

#include <Libpfs/io/framewriter.h>
#include <Libpfs/params.h>
#include <string>

namespace pfs {
namespace io {

//! \brief Writer class for TIFF files
class TiffWriter : public FrameWriter {
   public:
    TiffWriter(const std::string &filename);
    ~TiffWriter();

    //! \brief write a pfs::Frame into a properly formatted TIFF file
    //!  \c params can take:
    //!   tiff_mode (int): 0 = 8bit uint, 1 = 16bit uint, 2 = 32bit float, 3 =
    //!   logluv
    //!   min_luminance (float): minimum luminance to consider trusthworthy
    //!   max_luminance (float): maximum luminance to consider trusthworthy
    //!   mapping_method (int): RGB mapping methodo choosen between
    //!   RGBMappingType in rgbremapper.h
    bool write(const pfs::Frame &frame, const pfs::Params &params);
};

}  // io
}  // pfs

#endif  // PFS_TIFFWRITER_H
