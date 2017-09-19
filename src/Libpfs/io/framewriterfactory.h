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

//! \brief FrameWriterFactory, creation of FrameWriter based on the input
//! filename
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#ifndef PFS_IO_FRAMEWRITERFACTORY_H
#define PFS_IO_FRAMEWRITERFACTORY_H

#include <Libpfs/io/framewriter.h>
#include <Libpfs/io/ioexception.h>
#include <Libpfs/utils/string.h>
#include <map>
#include <string>

namespace pfs {
namespace io {

class FrameWriterFactory {
   public:
    typedef FrameWriterPtr (*FrameWriterCreator)(const std::string &filename);
    typedef std::map<std::string, FrameWriterCreator,
                     utils::StringUnsensitiveComp>
        FrameWriterCreatorMap;

    static FrameWriterPtr open(const std::string &filename,
                               const pfs::Params &params);

    static void registerFormat(const std::string &format,
                               FrameWriterCreator creator);
    static size_t numRegisteredFormats();
    static bool isSupported(const std::string &format);

   private:
    static FrameWriterCreatorMap sm_registry;
};

}  // io
}  // pfs

#endif  // PFS_IO_FRAMEWRITERFACTORY_H
