/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
 * Copyright (C) 2011-2012 Davide Anastasia
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
 */

//! \author Rafal Mantiuk, <mantiuk@mpi-sb.mpg.de>
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#include "channel.h"
#include "tag.h"

#include <map>

using namespace std;

namespace pfs
{
Channel::Channel( size_t width, size_t height, const std::string& channelName)
    : m_channelData( width, height )
    , m_name( channelName )
    , m_tags()
{
    //std::cout << "Channel constructor (" << name->data() << ")" << std::endl;
}

//std::cout << "Channel destructor (" << name->data() << ")" << std::endl;
Channel::~Channel()
{}

void Channel::resize(int width, int height)
{
    m_channelData.resize(width, height);
}

//void Channel::setChannelData(ChannelData *array)
//{
//    if (channel_impl)
//        delete channel_impl;
//    channel_impl = array;
//}

} // pfs



