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

//! \brief Interface for the FrameWriter base class
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#ifndef PFS_IO_FRAMEWRITER_H
#define PFS_IO_FRAMEWRITER_H

#include <memory>
#include <string>

#include <Libpfs/io/ioexception.h>
#include <Libpfs/params.h>

namespace pfs {
class Frame;

namespace io {

class FrameWriter {
   public:
    explicit FrameWriter(const std::string &filename);
    explicit FrameWriter();
    virtual ~FrameWriter();

    virtual bool write(const pfs::Frame &frame, const pfs::Params &params) = 0;

    const std::string &filename() const { return m_filename; }

   private:
    std::string m_filename;
};

typedef std::shared_ptr<FrameWriter> FrameWriterPtr;

}  // io
}  // pfs

#endif  // PFS_IO_FRAMEWRITER_H
