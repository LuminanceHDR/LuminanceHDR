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

#include <Libpfs/io/framereaderfactory.h>
#include <boost/assign.hpp>

using namespace boost::assign;
using namespace std;

namespace pfs {
namespace io {

using pfs::utils::getFormat;

FrameReaderPtr FrameReaderFactory::open(const std::string &filename) {
    string ext = getFormat(filename);
    if (!ext.empty()) {
        FrameReaderCreatorMap::const_iterator it = sm_registry.find(ext);
        if (it != sm_registry.end()) {
            return (it->second)(filename);
        }
    }
    throw UnsupportedFormat("Cannot find the correct handler for " + filename);
}

void FrameReaderFactory::registerFormat(
    const std::string &format, FrameReaderFactory::FrameReaderCreator creator) {
    sm_registry.insert(FrameReaderCreatorMap::value_type(format, creator));
}

size_t FrameReaderFactory::numRegisteredFormats() { return sm_registry.size(); }

bool FrameReaderFactory::isSupported(const std::string &format) {
    return sm_registry.count(format);
}

}  // io
}  // pfs

// Factory subscriptions    ---------------------------------------------------

#include <Libpfs/io/exrreader.h>
#include <Libpfs/io/jpegreader.h>
#include <Libpfs/io/pfsreader.h>
#include <Libpfs/io/rawreader.h>
#include <Libpfs/io/rgbereader.h>
#include <Libpfs/io/tiffreader.h>
#ifdef HAVE_CFITSIO
#include <Libpfs/io/fitsreader.h>
#endif

namespace pfs {
namespace io {

template <typename ConcreteClass>
FrameReaderPtr creator(const std::string &filename) {
    return FrameReaderPtr(std::make_shared<ConcreteClass>(filename));
}

FrameReaderFactory::FrameReaderCreatorMap FrameReaderFactory::sm_registry =
    map_list_of
    // LDR formats
    ("jpeg", creator<JpegReader>)
    ("jpg", creator<JpegReader>)
    // HDR formats
    ("pfs", creator<PfsReader>)
    ("exr", creator<EXRReader>)
    ("hdr",creator<RGBEReader>)
    // RAW formats
    ("crw", creator<RAWReader>)
    ("cr2", creator<RAWReader>)
    ("nef", creator<RAWReader>)
    ("dng", creator<RAWReader>)
    ("mrw", creator<RAWReader>)
    ("orf", creator<RAWReader>)
    ("kdc", creator<RAWReader>)
    ("dcr", creator<RAWReader>)
    ("arw", creator<RAWReader>)
    ("raf", creator<RAWReader>)
    ("ptx", creator<RAWReader>)
    ("pef", creator<RAWReader>)
    ("x3f", creator<RAWReader>)
    ("raw", creator<RAWReader>)
    ("sr2", creator<RAWReader>)
    ("3fr", creator<RAWReader>)
    ("rw2", creator<RAWReader>)
    ("mef", creator<RAWReader>)
    ("mos", creator<RAWReader>)
    ("erf", creator<RAWReader>)
    ("nrw", creator<RAWReader>)
    ("srw", creator<RAWReader>)
    // tiff
    ("tif", creator<TiffReader>)
    ("tiff", creator<TiffReader>)
#ifdef HAVE_CFITSIO
    // fits
    ("fit", creator<FitsReader>)
    ("fits", creator<FitsReader>)
#endif
    ;

}  // io
}  // pfs
