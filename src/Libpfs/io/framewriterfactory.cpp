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

#include <Libpfs/io/framewriterfactory.h>
#include <boost/assign.hpp>

using namespace ::boost::assign;
using namespace std;

namespace pfs {
namespace io {

using pfs::utils::getFormat;

FrameWriterPtr FrameWriterFactory::open(const std::string &filename,
                                        const pfs::Params &params) {
    string ext = getFormat(filename);
    std::string content;
    if (params.get("format", content)) {
        if (!content.empty()) {
            FrameWriterCreatorMap::const_iterator it =
                sm_registry.find(content);
            if (it != sm_registry.end()) {
                return (it->second)(filename);
            }
        }
    }
    if (!ext.empty()) {
        FrameWriterCreatorMap::const_iterator it = sm_registry.find(ext);
        if (it != sm_registry.end()) {
            return (it->second)(filename);
        }
    }
    throw UnsupportedFormat("Cannot find the correct handler for " + filename);
}

void FrameWriterFactory::registerFormat(
    const std::string &format, FrameWriterFactory::FrameWriterCreator creator) {
    sm_registry.insert(FrameWriterCreatorMap::value_type(format, creator));
}

size_t FrameWriterFactory::numRegisteredFormats() { return sm_registry.size(); }

bool FrameWriterFactory::isSupported(const std::string &format) {
    return sm_registry.count(format);
}

}  // io
}  // pfs

// Factory subscriptions    ---------------------------------------------------

#include <Libpfs/io/exrwriter.h>
#include <Libpfs/io/jpegwriter.h>
#include <Libpfs/io/pfswriter.h>
#include <Libpfs/io/pngwriter.h>
#include <Libpfs/io/rgbewriter.h>
#include <Libpfs/io/tiffwriter.h>

namespace pfs {
namespace io {

template <typename ConcreteClass>
FrameWriterPtr creator(const std::string &filename) {
    return FrameWriterPtr(std::make_shared<ConcreteClass>(filename));
}

FrameWriterFactory::FrameWriterCreatorMap FrameWriterFactory::sm_registry =
    map_list_of
    // LDR formats
    ("jpeg", creator<JpegWriter>)(
        "jpg", creator<JpegWriter>)("png", creator<PngWriter>)
    // Ibrid formats (can be both, depending on the parameters
    ("tiff", creator<TiffWriter>)("tif", creator<TiffWriter>)
    // HDR formats
    ("pfs", creator<PfsWriter>)("exr", creator<EXRWriter>)("hdr",
                                                           creator<RGBEWriter>);

}  // io
}  // pfs
