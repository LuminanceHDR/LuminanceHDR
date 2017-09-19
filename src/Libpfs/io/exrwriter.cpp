/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
 * Copyright (C) 2006,2007 Giuseppe Rota
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
#include <ImfChannelList.h>
#include <ImfHeader.h>
#include <ImfOutputFile.h>
#include <ImfRgbaFile.h>
#include <ImfStandardAttributes.h>
#include <ImfStringAttribute.h>

#include <cmath>
#include <string>

#include <Libpfs/frame.h>
#include <Libpfs/io/exrwriter.h>

// #define min(x,y) ( (x)<(y) ? (x) : (y) )

using namespace Imf;
using namespace Imath;
using namespace std;

namespace pfs {
namespace io {

EXRWriter::EXRWriter(const string &filename) : FrameWriter(filename) {}

bool EXRWriter::write(const Frame &frame, const Params & /*params*/) {
    // Channels are named (X Y Z) but contain (R G B) data
    const pfs::Channel *R, *G, *B;
    frame.getXYZChannels(R, G, B);

    Header header(frame.getWidth(), frame.getHeight(),
                  1,                 // aspect ratio
                  Imath::V2f(0, 0),  // screenWindowCenter
                  1,                 // screenWindowWidth
                  INCREASING_Y,      // lineOrder
                  PIZ_COMPRESSION);

    // Copy tags to attributes
    pfs::TagContainer::const_iterator it = frame.getTags().begin();
    pfs::TagContainer::const_iterator itEnd = frame.getTags().end();

    for (; it != itEnd; ++it) {
        header.insert(it->first.c_str(), StringAttribute(it->second));
    }

    // Copy all channel tags
    const pfs::ChannelContainer &channels = frame.getChannels();

    for (pfs::ChannelContainer::const_iterator ch = channels.begin();
         ch != channels.end(); ++ch) {
        std::string channelName = (*ch)->getName();
        pfs::TagContainer::const_iterator it = (*ch)->getTags().begin();
        pfs::TagContainer::const_iterator itEnd = (*ch)->getTags().end();

        for (; it != itEnd; ++it) {
            header.insert(string(channelName + ":" + it->first).c_str(),
                          StringAttribute(it->second));
        }
    }

    FrameBuffer frameBuffer;

    // Define channels in Header
    // and
    // Create channels in FrameBuffer
    header.channels().insert("R", Imf::Channel(FLOAT));
    frameBuffer.insert("R",                                       // name
                       Slice(FLOAT,                               // type
                             (char *)R->data(),                   // base
                             sizeof(float) * 1,                   // xStride
                             sizeof(float) * frame.getWidth()));  // yStride

    header.channels().insert("G", Imf::Channel(FLOAT));
    frameBuffer.insert("G",                                       // name
                       Slice(FLOAT,                               // type
                             (char *)G->data(),                   // base
                             sizeof(float) * 1,                   // xStride
                             sizeof(float) * frame.getWidth()));  // yStride

    header.channels().insert("B", Imf::Channel(FLOAT));
    frameBuffer.insert("B",                                       // name
                       Slice(FLOAT,                               // type
                             (char *)B->data(),                   // base
                             sizeof(float) * 1,                   // xStride
                             sizeof(float) * frame.getWidth()));  // yStride

    OutputFile file(filename().c_str(), header);
    file.setFrameBuffer(frameBuffer);
    file.writePixels(frame.getHeight());

    return true;
}

}  // pfs
}  // io
