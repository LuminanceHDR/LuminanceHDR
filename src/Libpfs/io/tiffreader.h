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

#ifndef PFS_IO_TIFFREADER_H
#define PFS_IO_TIFFREADER_H

//! \brief TiffReader
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#include <Libpfs/io/framereader.h>
#include <Libpfs/io/ioexception.h>

namespace pfs {
namespace io {

// sort of private implementation for TiffReader
struct TiffReaderData;

class TiffReader : public FrameReader {
   public:
    TiffReader(const std::string &filename);
    ~TiffReader();

    void open();
    bool isOpen() const;
    void close();

    void read(Frame &frame, const Params &params);

   private:
    std::unique_ptr<TiffReaderData> m_data;
};

}  // io
}  // pfs

#endif
