/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2013 Franco Comida
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

//! \brief FITS file format reader
//! \author Franco Comida <fcomida@users.sourceforge.net>

#ifndef PFS_IO_FITSREADER_H
#define PFS_IO_FITSREADER_H

#include <memory>
#include <string>

#include <Libpfs/io/framereader.h>
#include <Libpfs/io/ioexception.h>
#include <Libpfs/params.h>

namespace pfs {
class Frame;

namespace io {

class FitsReaderData;

class FitsReader : public FrameReader {
   public:
    FitsReader(const std::string &filename);

    ~FitsReader();

    bool isOpen() const { return m_data.get(); }

    void open();
    void close();
    void read(Frame &frame, const Params &);

   private:
    std::unique_ptr<FitsReaderData> m_data;
};

}  // io
}  // pfs

#endif
