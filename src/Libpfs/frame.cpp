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

#include <iostream>
#include <map>
#include <list>

#include "frame.h"
#include "domio.h"
#include "channel.h"

using namespace std;

namespace pfs
{    
Frame::Frame( int width, int height )
    : m_width( width )
    , m_height( height )
{ }

Frame::~Frame()
{
    ChannelMap::iterator it = m_channels.begin();
    while ( it != m_channels.end() )
    {
        Channel *ch = it->second;

        // Nasty trick because hashmap
        // elements point to string that is
        // freed by the channel
        ChannelMap::iterator itToDelete = it;

        it++;
        m_channels.erase( itToDelete );
        delete ch;
    }
}

void Frame::getXYZChannels( Channel* &X, Channel* &Y, Channel* &Z )
{
    ChannelMap::iterator it;

    // find X
    it = m_channels.find("X");
    if ( it == m_channels.end() )
    {
        X = Y = Z = NULL;
        return;
    }
    X = it->second;

    // find Y
    it = m_channels.find("Y");
    if ( it == m_channels.end() )
    {
        X = Y = Z = NULL;
        return;
    }
    Y = it->second;

    // find Y
    it = m_channels.find("Z");
    if ( it == m_channels.end() )
    {
        X = Y = Z = NULL;
        return;
    }
    Z = it->second;
}

void Frame::createXYZChannels( Channel* &X, Channel* &Y, Channel* &Z )
{
    X = createChannel("X");
    Y = createChannel("Y");
    Z = createChannel("Z");
}

Channel* Frame::getChannel(const std::string& name)
{
    ChannelMap::iterator it = m_channels.find(name);
    if ( it == m_channels.end() )
        return NULL;
    else
        return it->second;
}

Channel* Frame::createChannel(const std::string& name)
{
    ChannelMap::iterator it = m_channels.find(name);

    if ( it != m_channels.end() )
    {
        return it->second; //m_channels[name];
    }
    else
    {
        Channel *ch = new Channel( m_width, m_height, name );
        m_channels.insert( std::pair<std::string, Channel*>(name, ch) );

        return ch;
    }
}

void Frame::removeChannel( Channel *ch )
{
    if ( ch == NULL ) return;

    ChannelMap::iterator it = m_channels.find( ch->getName() );

    if ( it != m_channels.end() )
    {


        m_channels.erase( it );
        delete ch;
    }
}

ChannelMap& Frame::getChannels()
{
    return this->m_channels;
}

const ChannelMap& Frame::getChannels() const
{
    return this->m_channels;
}

TagContainer& Frame::getTags()
{
    return m_tags;
}

const TagContainer& Frame::getTags() const
{
    return m_tags;
}

} // namespace pfs


