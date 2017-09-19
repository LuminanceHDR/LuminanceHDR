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

#include <cstdio>
#include <cstdlib>

#include <Libpfs/frame.h>
#include <Libpfs/io/pfscommon.h>
#include <Libpfs/io/pfswriter.h>
#include <Libpfs/tag.h>
#include <Libpfs/utils/resourcehandlerstdio.h>

namespace pfs {
namespace io {

static const char *PFSFILEID = "PFS1\x0a";

void writeTags(const TagContainer &tags, FILE *out) {
    fprintf(out, "%d" PFSEOL, (int)tags.size());
    for (TagContainer::const_iterator it = tags.begin(); it != tags.end();
         ++it) {
        fprintf(out, "%s", std::string(it->first + "=" + it->second).c_str());
        fprintf(out, PFSEOL);
    }
}

PfsWriter::PfsWriter(const std::string &filename) : FrameWriter(filename) {}

bool PfsWriter::write(const Frame &frame, const Params & /*params*/) {
    utils::ScopedStdIoFile outputStream(fopen(filename().c_str(), "wb"));
    if (!outputStream) {
        throw pfs::io::InvalidFile("PfsWriter: cannot open " + filename());
    }

#ifdef HAVE_SETMODE
    // Needed under MS windows (text translation IO for stdin/out)
    int old_mode = setmode(fileno(outputStream.data()), _O_BINARY);
#endif

    // Write header ID
    fwrite(PFSFILEID, 1, 5, outputStream.data());

    const ChannelContainer &channels = frame.getChannels();

    fprintf(outputStream.data(), "%d %d" PFSEOL, (int)frame.getWidth(),
            (int)frame.getHeight());
    fprintf(outputStream.data(), "%d" PFSEOL, (int)channels.size());

    writeTags(frame.getTags(), outputStream.data());

    // Write channel IDs and tags
    for (ChannelContainer::const_iterator it = channels.begin();
         it != channels.end(); ++it) {
        fprintf(outputStream.data(), "%s" PFSEOL, (*it)->getName().c_str());
        writeTags((*it)->getTags(), outputStream.data());
    }

    fprintf(outputStream.data(), "ENDH");

    // Write channels
    for (ChannelContainer::const_iterator it = channels.begin();
         it != channels.end(); ++it) {
        int size = frame.getWidth() * frame.getHeight();
        fwrite((*it)->data(), sizeof(float), size, outputStream.data());
    }

    // Very important for pfsoutavi !!!
    fflush(outputStream.data());
#ifdef HAVE_SETMODE
    setmode(fileno(outputStream.data()), old_mode);
#endif
    return true;
}

}  // io
}  // pfs
