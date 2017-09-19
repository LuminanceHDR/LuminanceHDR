/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2012 Franco Comida
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

//! \author Franco Comida <fcomida@users.sourceforge.net>
//! Initial implementation
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! - Remove dependencies from Qt
//! - Adaptation for Luminane HDR and LibHDR
//! - in-memory writer without any temporary file

#ifndef PNGWRITER_H
#define PNGWRITER_H

#include <memory>
#include <string>

#include <Libpfs/io/framewriter.h>
#include <Libpfs/io/ioexception.h>
#include <Libpfs/params.h>

namespace pfs {
namespace io {

class PngWriterImpl;

class PngWriter : public FrameWriter {
   public:
    explicit PngWriter(const std::string &filename);
    PngWriter();
    ~PngWriter();

    bool write(const pfs::Frame &frame, const pfs::Params &params);

    size_t getFileSize() const;

   private:
    std::unique_ptr<PngWriterImpl> m_impl;
};

}  // io
}  // pfs

#endif
