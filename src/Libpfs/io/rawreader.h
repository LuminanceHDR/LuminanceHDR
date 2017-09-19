/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2010 Franco Comida
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
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 */

#ifndef PFS_IO_RAWREADER_H
#define PFS_IO_RAWREADER_H

#ifdef __APPLE__
#include <libraw.h>
#else
#include <libraw/libraw.h>
#endif

#include <Libpfs/io/framereader.h>
#include <Libpfs/io/ioexception.h>

//
// typedef int (*progress_callback)(void *callback_data,
//                enum LibRaw_progress stage, int iteration, int expected);
//

namespace pfs {
namespace io {

class RAWReader : public FrameReader {
   public:
    RAWReader(const std::string &filename);
    ~RAWReader();

    void open();
    bool isOpen() const;
    void close();

    void read(Frame &frame, const Params &params);

   private:
    LibRaw m_processor;
};

}  // io
}  // pfs

#endif
