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

#include <Libpfs/frame.h>
#include <Libpfs/io/pfscommon.h>
#include <Libpfs/io/pfsreader.h>

#include <list>

namespace pfs {
namespace io {

static const char *PFSFILEID = "PFS1\x0a";

void readTags(TagContainer &tags, FILE *in) {
    int tagCount = -1;
    int readItems = fscanf(in, "%d" PFSEOL, &tagCount);
    if (readItems != 1 || tagCount < 0 || tagCount > 1024) {
        throw Exception(
            "Corrupted PFS tag section: missing or wrong number of tags");
    }

    char buf[MAX_TAG_STRING + 1];
    for (int i = 0; i < tagCount; i++) {
        if (!fgets(buf, MAX_TAG_STRING, in)) {
            throw Exception("Corrupted PFS tag section: missing tag");
        }
        std::string data(buf);
        size_t found = data.find_first_of("=");
        if (found != std::string::npos) {
#ifndef NDEBUG
            std::clog << "found tag: " << data.substr(0, found) << "="
                      << data.substr(found + 1, data.size()) << std::endl;
#endif

            tags.setTag(data.substr(0, found),
                        data.substr(found + 1, data.size()));

        } else {
            throw Exception("Corrupted PFS tag section ('=' sign missing)");
        }
    }
}

PfsReader::PfsReader(const std::string &filename)
    : FrameReader(filename), m_channelCount(0) {
    PfsReader::open();
}

void PfsReader::open() {
    m_file.reset(fopen(filename().c_str(), "rb"));
    if (!m_file) {
        throw InvalidFile("Cannot open file " + filename());
    }

#ifdef HAVE_SETMODE
    // Needed under MS windows (text translation IO for stdin/out)
    int old_mode = setmode(fileno(inputStream), _O_BINARY);
#endif
    char buf[5];
    size_t read = fread(buf, 1, 5, m_file.data());
    if (read == 0) {
        throw InvalidHeader("empty file!");
    }
    if (memcmp(buf, PFSFILEID, 5)) {
        throw InvalidHeader("Incorrect PFS file header");
    }

    int width, height;
    read = fscanf(m_file.data(), "%d %d" PFSEOL, &width, &height);
    if (read != 2 || width <= 0 || width > MAX_RES || height <= 0 ||
        height > MAX_RES) {
        throw InvalidHeader(
            "Corrupted PFS file: missing or wrong 'width', 'height' tags");
    }
    setWidth(width);
    setHeight(height);

    int channelCount;
    read = fscanf(m_file.data(), "%d" PFSEOL, &channelCount);
    if (read != 1 || channelCount < 0 || channelCount > MAX_CHANNEL_COUNT) {
        throw InvalidHeader(
            "Corrupted PFS file: missing or wrong 'channelCount' tag");
    }
    m_channelCount = channelCount;
}

void PfsReader::close() {
    setWidth(0);
    setHeight(0);
    m_file.reset();
    m_channelCount = 0;
}

void PfsReader::read(Frame &frame, const Params & /*params*/) {
    if (!isOpen()) open();

    Frame tempFrame(width(), height());

    readTags(tempFrame.getTags(), m_file.data());

    // read channel IDs and tags
    std::list<Channel *> orderedChannel;
    for (size_t i = 0; i < m_channelCount; i++) {
        char channelName[MAX_CHANNEL_NAME + 1], *rs;
        rs = fgets(channelName, MAX_CHANNEL_NAME, m_file.data());
        if (rs == NULL) {
            throw ReadException("Corrupted PFS file: missing channel name");
        }

        size_t len = strlen(channelName);
        // fprintf( stderr, "s = '%s' len = %d\n", channelName, len );
        if (len < 1 || channelName[len - 1] != PFSEOLCH) {
            throw ReadException("Corrupted PFS file: bad channel name");
        }

        channelName[len - 1] = 0;
        Channel *ch = tempFrame.createChannel(channelName);
        readTags(ch->getTags(), m_file.data());
        orderedChannel.push_back(ch);
    }

    char buf[5];
    size_t read = fread(buf, 1, 4, m_file.data());
    if (read == 0 || memcmp(buf, "ENDH", 4)) {
        throw ReadException(
            "Corrupted PFS file: missing end of header (ENDH) token");
    }

    // Read channels
    std::list<Channel *>::iterator it;
    for (it = orderedChannel.begin(); it != orderedChannel.end(); ++it) {
        Channel *ch = *it;
        unsigned int size = tempFrame.getWidth() * tempFrame.getHeight();
        read = fread(ch->data(), sizeof(float), size, m_file.data());
        if (read != size) {
            throw ReadException("Corrupted PFS file: missing channel data");
        }
    }
#ifdef HAVE_SETMODE
    setmode(fileno(inputStream), old_mode);
#endif

    frame.swap(tempFrame);
}

}  // io
}  // pfs
