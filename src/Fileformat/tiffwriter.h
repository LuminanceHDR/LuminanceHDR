/**
 * @brief Tiff facilities
 * 
 * This file is a part of LuminanceHDR package.
 * ---------------------------------------------------------------------- 
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
 * Copyright (C) 2006 Giuseppe Rota
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
 * slightly modified by Giuseppe Rota <grota@sourceforge.net> for luminance
 */

#ifndef PFS_TIFFWRITER_H
#define PFS_TIFFWRITER_H

#include <string>
#include <Libpfs/params.h>

namespace pfs {
class Frame;
}

//! \brief Writer class for TIFF files
// tiff_mode: 0 = 8bit uint, 1 = 16bit uint, 2 = 32bit float, 3 = logluv
class TiffWriter
{
public:
    TiffWriter(const std::string& filename);
    ~TiffWriter();

    bool write(const pfs::Frame& frame, const pfs::Params& params);

private:
    std::string m_filename;
};

#endif  // PFS_TIFFWRITER_H
