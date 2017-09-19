/**
 * @brief PFS library - Frame
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
 *   Frame definition split from pfs.cpp
 */

#include <algorithm>
#include <boost/bind.hpp>
#include <iostream>

#include "channel.h"
#include "frame.h"

using namespace std;

namespace pfs {
Frame::Frame(size_t width, size_t height)
    : m_width(width), m_height(height), m_X(NULL), m_Y(NULL), m_Z(NULL) {}

namespace {
struct ChannelDeleter {
    template <typename T>
    inline void operator()(T *p) {
        delete p;
    }
};
}

Frame::~Frame() {
    for_each(m_channels.begin(), m_channels.end(), ChannelDeleter());
}

//! \brief Changes the size of the frame
void Frame::resize(size_t width, size_t height) {
    for_each(m_channels.begin(), m_channels.end(),
             boost::bind(&Channel::ChannelData::resize, _1, width, height));

    m_width = width;
    m_height = height;
}

namespace {
struct FindChannel {
    explicit FindChannel(const string &nameChannel)
        : nameChannel_(nameChannel) {}

    inline bool operator()(const Channel *channel) const {
        return !(channel->getName().compare(nameChannel_));
    }

   private:
    string nameChannel_;
};
}

void Frame::getXYZChannels(const Channel *&X, const Channel *&Y,
                           const Channel *&Z) const {
    // find X
    if (m_X == NULL || m_Y == NULL || m_Z == NULL) {
        X = NULL;
        Y = NULL;
        Z = NULL;
        return;
    }

    X = m_X;
    Y = m_Y;
    Z = m_Z;

    //    ChannelContainer::const_iterator it(
    //                find_if(m_channels.begin(),
    //                             m_channels.end(),
    //                             FindChannel("X"))
    //                );
    //    if ( it == m_channels.end() )
    //    {
    //        X = Y = Z = NULL;
    //        return;
    //    }
    //    X = *it;

    //    // find Y
    //    it = find_if(m_channels.begin(),
    //                      m_channels.end(),
    //                      FindChannel("Y"));
    //    if ( it == m_channels.end() )
    //    {
    //        X = Y = Z = NULL;
    //        return;
    //    }
    //    Y = *it;

    //    // find Y
    //    it = find_if(m_channels.begin(),
    //                      m_channels.end(),
    //                      FindChannel("Z"));
    //    if ( it == m_channels.end() )
    //    {
    //        X = Y = Z = NULL;
    //        return;
    //    }
    //    Z = *it;
}

void Frame::getXYZChannels(Channel *&X, Channel *&Y, Channel *&Z) {
    const Channel *X_;
    const Channel *Y_;
    const Channel *Z_;

    static_cast<const Frame &>(*this).getXYZChannels(X_, Y_, Z_);

    X = const_cast<Channel *>(X_);
    Y = const_cast<Channel *>(Y_);
    Z = const_cast<Channel *>(Z_);
}

void Frame::createXYZChannels(Channel *&X, Channel *&Y, Channel *&Z) {
    X = createChannel("X");
    Y = createChannel("Y");
    Z = createChannel("Z");
}

const Channel *Frame::getChannel(const string &name) const {
    ChannelContainer::const_iterator it =
        find_if(m_channels.begin(), m_channels.end(), FindChannel(name));
    if (it == m_channels.end())
        return NULL;
    else
        return *it;
}

Channel *Frame::getChannel(const string &name) {
    return const_cast<Channel *>(
        static_cast<const Frame &>(*this).getChannel(name));
}

Channel *Frame::createChannel(const string &name) {
    Channel *ch = NULL;
    ChannelContainer::iterator it =
        find_if(m_channels.begin(), m_channels.end(), FindChannel(name));
    if (it != m_channels.end()) {
        ch = *it;
    } else {
        ch = new Channel(m_width, m_height, name);
        m_channels.push_back(ch);
    }

    // update the cache, if necessary
    if (name == "X") {
        m_X = ch;
    } else if (name == "Y") {
        m_Y = ch;
    } else if (name == "Z") {
        m_Z = ch;
    }

    return ch;
}

void Frame::removeChannel(const string &channel) {
    ChannelContainer::iterator it =
        find_if(m_channels.begin(), m_channels.end(), FindChannel(channel));
    if (it != m_channels.end()) {
        Channel *ch = *it;
        m_channels.erase(it);
        delete ch;

        if (channel == "X") {
            m_X = NULL;
        } else if (channel == "Y") {
            m_Y = NULL;
        } else if (channel == "Z") {
            m_Z = NULL;
        }
    }
}

ChannelContainer &Frame::getChannels() { return this->m_channels; }

const ChannelContainer &Frame::getChannels() const { return this->m_channels; }

TagContainer &Frame::getTags() { return m_tags; }

const TagContainer &Frame::getTags() const { return m_tags; }

void Frame::swap(Frame &other) {
    using std::swap;

    swap(m_width, other.m_width);
    swap(m_height, other.m_height);
    m_channels.swap(other.m_channels);
    m_tags.swap(other.m_tags);

    swap(m_X, other.m_X);
    swap(m_Y, other.m_Y);
    swap(m_Z, other.m_Z);
}

}  // namespace pfs
