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
    FrameImpl::FrameImpl( int width, int height ): width( width ), height( height ),
    channelIterator( &channel )
    {
        tags = new TagContainerImpl();
    }

    FrameImpl::~FrameImpl()
    {
        delete tags;
        ChannelMap::iterator it;
        for( it = channel.begin(); it != channel.end(); )
        {
            Channel *ch = it->second;
            ChannelMap::iterator itToDelete = it; // Nasty trick because hashmap
            // elements point to string that is
            // freed by the channel

            it++;
            channel.erase( itToDelete );
            delete ch;
        }
        std::cout << "FrameImpl::~FrameImpl()" << std::endl;
    }

    int FrameImpl::getWidth() const
    {
        return width;
    }

    int FrameImpl::getHeight() const
    {
        return height;
    }

    void FrameImpl::getXYZChannels( Channel* &X, Channel* &Y, Channel* &Z )
    {
        if (channel.find("X") == channel.end()
            || channel.find("Y") == channel.end()
            || channel.find("Z") == channel.end()
            )
            {
            X = Y = Z = NULL;
        }
        else
        {
            X = channel["X"];
            Y = channel["Y"];
            Z = channel["Z"];
        }
    }

    void FrameImpl::createXYZChannels( Channel* &X, Channel* &Y, Channel* &Z )
    {
        X = createChannel("X");
        Y = createChannel("Y");
        Z = createChannel("Z");
    }

    Channel* FrameImpl::getChannel( std::string name )//const char *name )
    {
        ChannelMap::iterator it = channel.find(name);
        if( it == channel.end() )
            return NULL;
        else
            return it->second;

    }

    Channel* FrameImpl::createChannel( std::string name ) //const char *name )
    {
        ChannelImpl *ch;
        if( channel.find(name) == channel.end() )
        {
            ch = new ChannelImpl( width, height, name );
            channel.insert( std::pair<std::string, ChannelImpl*>(ch->getName(), ch) );
        }
        else
        {
            ch = channel[name];
        }

        return ch;
    }

    void FrameImpl::removeChannel( Channel *ch )
    {
        assert( ch != NULL );
        ChannelMap::iterator it = channel.find( ch->getName() );
        assert( it != channel.end() && it->second == ch );

        channel.erase( it );
        delete ch;
    }

    ChannelIterator* FrameImpl::getChannels()
    {
        channelIterator.reset();
        return &channelIterator;
    }

    ChannelIteratorPtr FrameImpl::getChannelIterator()
    {
        return ChannelIteratorPtr( new ChannelIteratorImpl( &channel ) );
    }

    TagContainer* FrameImpl::getTags()
    {
        return tags;
    }
}


