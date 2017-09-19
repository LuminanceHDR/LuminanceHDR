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

//! \brief FrameReaderFactory, creation of FrameReader based on the input
//! filename
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#ifndef PFS_IO_FRAMEREADERFACTORY_H
#define PFS_IO_FRAMEREADERFACTORY_H

#include <Libpfs/io/framereader.h>
#include <Libpfs/io/ioexception.h>
#include <Libpfs/utils/string.h>
#include <map>
#include <string>

namespace pfs {
namespace io {

class FrameReaderFactory {
   public:
    typedef FrameReaderPtr (*FrameReaderCreator)(const std::string &filename);
    typedef std::map<std::string, FrameReaderCreator,
                     utils::StringUnsensitiveComp>
        FrameReaderCreatorMap;

    static FrameReaderPtr open(const std::string &filename);

    static void registerFormat(const std::string &format,
                               FrameReaderCreator creator);
    static size_t numRegisteredFormats();
    static bool isSupported(const std::string &format);

   private:
    static FrameReaderCreatorMap sm_registry;
};

}  // io
}  // pfs

#endif  // PFS_IO_FRAMEREADERFACTORY_H
