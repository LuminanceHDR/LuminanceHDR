/*
 * This file is a part of Luminance HDR package
 * ----------------------------------------------------------------------
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
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

//! \brief Read files in OpenEXR format, based on PFSTOOLS code
//! \author Grzegorz Krawczyk <krawczyk@mpi-sb.mpg.de>
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! Adaptation for Luminance HDR and other small improvements

#include <ImfChannelList.h>
#include <ImfHeader.h>
#include <ImfInputFile.h>
#include <ImfRgbaFile.h>
#include <ImfStandardAttributes.h>
#include <ImfStringAttribute.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include <Libpfs/frame.h>
#include <Libpfs/io/exrreader.h>
#include <Libpfs/io/ioexception.h>

using namespace Imf;
using namespace Imath;
using namespace std;

namespace {
static string escapeString(const string &src) {
    size_t pos = 0;
    string ret = src;
    while (pos < ret.size()) {
        pos = ret.find("\n", pos);
        if (pos == string::npos) break;
        ret.replace(pos, 1, "\\n");
        pos += 2;
    }
    return ret;
}
}

namespace pfs {
namespace io {

class EXRReader::EXRReaderData {
   public:
    explicit EXRReaderData(const string &filename)
        : file_(filename.c_str())
          // , dw_(file_.header().displayWindow())
          ,
          dtw_(file_.header().dataWindow()) {}

    Imf::InputFile file_;
    // Box2i dtw_;
    Box2i dtw_;
};

EXRReader::EXRReader(const string &filename) : FrameReader(filename) {
    EXRReader::open();
}

EXRReader::~EXRReader() { close(); }

void EXRReader::open() {
    // open file and read dimensions
    m_data.reset(new EXRReaderData(filename().c_str()));

    int width = m_data->dtw_.max.x - m_data->dtw_.min.x + 1;
    int height = m_data->dtw_.max.y - m_data->dtw_.min.y + 1;

    assert(width > 0);
    assert(height > 0);

    // check that file contains RGB data
    bool red = false;
    bool green = false;
    bool blue = false;
    const ChannelList &channels = m_data->file_.header().channels();
    for (ChannelList::ConstIterator i = channels.begin(), iEnd = channels.end();
         i != iEnd; ++i) {
        if (!strcmp(i.name(), "R"))
            red = true;
        else if (!strcmp(i.name(), "G"))
            green = true;
        else if (!strcmp(i.name(), "B"))
            blue = true;
    }

    if (!(red && green && blue)) {
        throw pfs::io::InvalidHeader("OpenEXR file " + filename() +
                                     " does "
                                     " not contain RGB data");
    }

    // check boundaries
    /*
    if ( (m_data->dtw_.min.x < m_data->dw_.min.x &&
          m_data->dtw_.max.x > m_data->dw_.max.x) ||
         (m_data->dtw_.min.y < m_data->dw_.min.y &&
          m_data->dtw_.max.y > m_data->dw_.max.y) )
    {
        throw pfs::io::InvalidHeader("No support for OpenEXR files DataWindow" \
                                     " greater than DisplayWindow" );
    }
    */

    setWidth(width);
    setHeight(height);
}

void EXRReader::close() {
    m_data.reset();

    setWidth(0);
    setHeight(0);
}

void EXRReader::read(Frame &frame, const Params & /*params*/) {
    if (!isOpen()) open();

    // helpers...
    InputFile &file = m_data->file_;
    Box2i &dtw = m_data->dtw_;

    pfs::Frame tempFrame(width(), height());
    pfs::Channel *X, *Y, *Z;
    tempFrame.createXYZChannels(X, Y, Z);

    FrameBuffer frameBuffer;
    frameBuffer.insert(
        "R",          // name
        Slice(FLOAT,  // type
              (char *)(X->data() - dtw.min.x - dtw.min.y * width()),
              sizeof(float),            // xStride
              sizeof(float) * width(),  // yStride
              1, 1,                     // x/y sampling
              0.0));                    // fillValue

    frameBuffer.insert(
        "G",          // name
        Slice(FLOAT,  // type
              (char *)(Y->data() - dtw.min.x - dtw.min.y * width()),
              sizeof(float),            // xStride
              sizeof(float) * width(),  // yStride
              1, 1,                     // x/y sampling
              0.0));                    // fillValue

    frameBuffer.insert(
        "B",          // name
        Slice(FLOAT,  // type
              (char *)(Z->data() - dtw.min.x - dtw.min.y * width()),
              sizeof(float),            // xStride
              sizeof(float) * width(),  // yStride
              1, 1,                     // x/y sampling
              0.0));                    // fillValue

    // I know I have the channels I need because I have checked that I have the
    // RGB channels. Hence, I don't load any further that that...
    /*
    const ChannelList &channels = file.header().channels();
    for ( ChannelList::ConstIterator i = channels.begin(); i != channels.end();
    ++i )
    {
        if ( !strcmp( i.name(), "R" ) ||
             !strcmp( i.name(), "G" ) || !strcmp( i.name(), "B" ) ) continue;

        std::string channelName( i.name() );
        if ( channelName == "Z" ) {
            channelName = "DEPTH";
        }
        pfs::Channel *pfsCh = tempFrame.createChannel( channelName );
        frameBuffer.insert( i.name(),                               // name
                            Slice( FLOAT,                           // type
                                   (char *)(pfsCh->data() - dtw.min.x -
    dtw.min.y
    * width()),
                                   sizeof(float),                   // xStride
                                   sizeof(float) * width(),         // yStride
                                   1, 1,                            // x/y
    sampling
                                   0.0));                           // fillValue
    }
    */

    // Copy attributes to tags
    for (Header::ConstIterator it = file.header().begin(),
                               itEnd = file.header().end();
         it != itEnd; ++it) {
        const char *attribName = it.name();
        const StringAttribute *attrib =
            file.header().findTypedAttribute<StringAttribute>(attribName);

        if (attrib == NULL) continue;  // Skip if type is not String

        // fprintf( stderr, "Tag: %s = %s\n", attribName,
        // attrib->value().c_str() );

        const char *colon = strstr(attribName, ":");
        if (colon == NULL)  // frame tag
        {
            tempFrame.getTags().setTag(attribName,
                                       escapeString(attrib->value()));
        } else  // channel tag
        {
            std::string channelName = string(attribName, colon - attribName);
            pfs::Channel *ch = tempFrame.getChannel(channelName);
            if (ch == NULL) {
                std::cerr << " Warning! Can not set tag for " << channelName
                          << " channel because it does not exist\n";
            } else {
                ch->getTags().setTag(colon + 1, escapeString(attrib->value()));
            }
        }
    }

    file.setFrameBuffer(frameBuffer);
    file.readPixels(dtw.min.y, dtw.max.y);

    // Rescale values if WhiteLuminance is present
    if (hasWhiteLuminance(file.header())) {
        float scaleFactor = whiteLuminance(file.header());
        int pixelCount = tempFrame.getHeight() * tempFrame.getWidth();

        for (int i = 0; i < pixelCount; i++) {
            (*X)(i) *= scaleFactor;
            (*Y)(i) *= scaleFactor;
            (*Z)(i) *= scaleFactor;
        }

        // const StringAttribute *relativeLum =
        // file.header().findTypedAttribute<StringAttribute>("RELATIVE_LUMINANCE");

        std::string luminanceTag = tempFrame.getTags().getTag("LUMINANCE");
        if (luminanceTag.empty()) {
            tempFrame.getTags().setTag("LUMINANCE", "ABSOLUTE");
        }
    }

    tempFrame.getTags().setTag("FILE_NAME", filename());

    frame.swap(tempFrame);
}

}  // io
}  // pfs
