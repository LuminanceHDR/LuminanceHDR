/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2012 Franco Comida
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

//! \author Franco Comida <fcomida@users.sourceforge.net>
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! - Refactoring
//! - Adaptation for new Luminance HDR engine (LibHDR) - 2013.04.10

#ifndef PFS_IO_JPEGREADER_H
#define PFS_IO_JPEGREADER_H

#include <Libpfs/io/framereader.h>
#include <Libpfs/io/ioexception.h>
#include <string>

namespace pfs {
namespace io {

class JpegReader : public FrameReader {
   public:
    JpegReader(const std::string &filename);
    ~JpegReader();

    void open();
    bool isOpen() const;
    void close();
    void read(Frame &frame, const Params &params);

   private:
    struct JpegReaderData;

    std::unique_ptr<JpegReaderData> m_data;
};

}  // io
}  // pfs

#endif  // PFS_IO_JPEGREADER_H
