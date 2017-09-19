/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2013 Davide Anastasia
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

//! \brief PFS file format writer (used for compatibility with PFSTOOLS)
//! \note Most of the code of this class is derived from the code in PFSTOOLS
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#ifndef PFS_IO_PFSWRITER_H
#define PFS_IO_PFSWRITER_H

#include <Libpfs/io/framewriter.h>
#include <Libpfs/io/ioexception.h>
#include <Libpfs/params.h>
#include <string>

namespace pfs {
class Frame;

namespace io {

class PfsWriter : public FrameWriter {
   public:
    PfsWriter(const std::string &filename);

    bool write(const pfs::Frame &frame, const pfs::Params &params);
};

}  // io
}  // pfs

#endif  //  PFS_IO_PFSWRITER_H
