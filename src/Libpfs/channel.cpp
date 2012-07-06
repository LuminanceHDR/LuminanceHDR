/**
 * @brief PFS library - PFS Channel
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

#include "channel.h"
#include "tag.h"

#include <map>

using namespace std;

namespace pfs
{
    //------------------------------------------------------------------------------
    // Channel implementation
    //------------------------------------------------------------------------------

    Channel::Channel( int width, int height, const std::string& channel_name)
        : channel_impl( new Array2D( width, height ) )
        , name( channel_name )
    {
        tags = new TagContainer();

        //std::cout << "Channel constructor (" << name->data() << ")" << std::endl;
    }

    Channel::~Channel()
    {
        //std::cout << "Channel destructor (" << name->data() << ")" << std::endl;

        delete channel_impl;
        delete tags;
    }

    // Channel implementation
    TagContainer* Channel::getTags()
    {
        return tags;
    }

    const TagContainer* Channel::getTags() const
    {
        return tags;
    }

    float* Channel::getRawData()
    {
        return channel_impl->getRawData();
    }

    const float* Channel::getRawData() const
    {
        return channel_impl->getRawData();
    }

    //Array2D implementation
    //    virtual int getCols() const
    //    {
    //      return channel_map->getCols();
    //    }
    //
    //    virtual int getRows() const
    //    {
    //      return channel_map->getRows();
    //    }

    /**
     * Gets width of the channel (in pixels).
     * This is a synonym for Array2D::getCols().
     */
    int Channel::getWidth() const
    {
        return channel_impl->getCols();
    }

    /**
     * Gets height of the channel (in pixels).
     * This is a synonym for Array2D::getRows().
     */
    int Channel::getHeight() const
    {
        return channel_impl->getRows();
    }

    const std::string& Channel::getName() const
    {
        return name;
    }

    //    inline float& operator()( int x, int y )
    //    {
    //      return channel_map->operator()(x, y);
    //    }
    //
    //    inline const float& operator()( int x, int y ) const
    //    {
    //      return channel_map->operator()(x, y);
    //    }
    //
    //    inline float& operator()( int rowMajorIndex )
    //    {
    //      return channel_map->operator()(rowMajorIndex);
    //    }
    //
    //    inline const float& operator()( int rowMajorIndex ) const
    //    {
    //      return channel_map->operator()(rowMajorIndex);
    //    }

    Array2D* Channel::getChannelData()
    {
        return channel_impl;
    }

    const Array2D* Channel::getChannelData() const
    {
        return channel_impl;
    }

	void Channel::setChannelData(Array2D *array)
	{
		if (channel_impl)
			delete channel_impl;
		channel_impl = array;
	}

}



