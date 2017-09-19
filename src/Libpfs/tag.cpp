/**
 * @brief PFS library - PFS Tag Handling
 *
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
 * Copyright (C) 2011 Davide Anastasia
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ----------------------------------------------------------------------
 *
 * @author Rafal Mantiuk, <mantiuk@mpi-sb.mpg.de>
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 */

#include <Libpfs/frame.h>
#include <Libpfs/tag.h>

#include <cassert>
#include <cstdio>
#include <sstream>
#include <string>

using namespace std;

namespace pfs {

std::string TagContainer::getTag(const string &tagName) const {
    TagList::const_iterator it = m_tags.find(tagName);
    if (it != m_tags.end()) return it->second;
    return std::string();
}

void TagContainer::removeTag(const std::string &tagName) {
    m_tags.erase(tagName);
}

void TagContainer::setTag(const string &tagName, const string &tagValue) {
    m_tags[tagName] = tagValue;
}

std::ostream &operator<<(std::ostream &out, const TagContainer &tags) {
    if (tags.size() == 0) return out;

    TagContainer::const_iterator itEnd = tags.end();
    std::advance(itEnd, -1);
    TagContainer::const_iterator it = tags.begin();

    std::stringstream ss;

    for (; it != itEnd; ++it) {
        ss << it->first << "=" << it->second << " ";
    }
    ss << it->first << "=" << it->second;
    return (out << ss.str());
}

void copyTags(const TagContainer &f, TagContainer &t) {
    t.clear();
    t = f;
}

void copyTags(const Frame *from, Frame *to) {
    copyTags(from->getTags(), to->getTags());

    const ChannelContainer &channels = from->getChannels();

    for (ChannelContainer::const_iterator it = channels.begin();
         it != channels.end(); ++it) {
        const pfs::Channel *fromCh = *it;
        pfs::Channel *toCh = to->getChannel(fromCh->getName());

        // Skip if there is no corresponding channel
        if (toCh != NULL) {
            copyTags(fromCh->getTags(), toCh->getTags());
        }
    }
}

}  // pfs
