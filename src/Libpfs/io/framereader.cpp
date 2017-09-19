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

#include <Libpfs/io/framereader.h>

#include <Libpfs/frame.h>
#include <Libpfs/manip/rotate.h>
#include <Libpfs/exif/exifdata.hpp>

namespace pfs {
namespace io {

FrameReader::FrameReader(const std::string &filename)
    : m_filename(filename), m_width(0), m_height(0) {}

FrameReader::~FrameReader() {}

void FrameReader::read(pfs::Frame &frame, const pfs::Params &params) {
    pfs::exif::ExifData exifData(m_filename);
    int rotation = exifData.getOrientationDegree();

    if (rotation == 270 || rotation == 90 || rotation == 180) {
        Frame *rotatedHalf = pfs::rotate(&frame, rotation != 270);
        frame.swap(*rotatedHalf);
        delete rotatedHalf;
    }
    if (rotation == 180) {
        Frame *rotatedHalf = pfs::rotate(&frame, true);
        frame.swap(*rotatedHalf);
        delete rotatedHalf;
    }
}

}  // io
}  // pfs
