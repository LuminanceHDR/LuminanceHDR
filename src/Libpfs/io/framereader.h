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

//! \brief Interface for the FrameReader base class
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#ifndef PFS_IO_FRAMEREADER_H
#define PFS_IO_FRAMEREADER_H

#include <memory>
#include <string>

#include <Libpfs/params.h>

namespace pfs {
class Frame;

namespace io {

class FrameReader {
   public:
    FrameReader(const std::string &filename);

    virtual ~FrameReader();

    //! \brief filename of the file being read
    const std::string &filename() const { return m_filename; }
    //! \brief return the width of the file being read
    size_t width() const { return m_width; }
    //! \brief return the height of the file being read
    size_t height() const { return m_height; }

    virtual void open() = 0;
    virtual bool isOpen() const = 0;
    virtual void close() = 0;
    virtual void read(pfs::Frame &frame, const pfs::Params &params);

   protected:
    void setWidth(size_t width) { m_width = width; }
    void setHeight(size_t height) { m_height = height; }

   private:
    std::string m_filename;
    size_t m_width;
    size_t m_height;
};

typedef std::shared_ptr<FrameReader> FrameReaderPtr;

}  // io
}  // pfs

#endif  // PFS_IO_FRAMEREADER_H
