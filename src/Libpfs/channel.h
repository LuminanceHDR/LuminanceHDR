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

//! \brief PFS library - PFS Channel
//! \author Rafal Mantiuk, <mantiuk@mpi-sb.mpg.de>
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#ifndef PFS_CHANNEL_H
#define PFS_CHANNEL_H

#include <cstddef>
#include <map>
#include <string>

#include <Libpfs/array2d.h>
#include <Libpfs/tag.h>

namespace pfs {

//! \brief Channel interface represents a 2D rectangular array with
//! associated tags.
class Channel : public Array2D<float> {
   public:
    typedef Array2D<float> ChannelData;

    Channel(size_t width, size_t height, const std::string &channelName);

    virtual ~Channel();

    using ChannelData::data;
    using ChannelData::resize;

    //!
    //! \brief Returns TagContainer that can be used to access or modify
    //! tags associated with this Channel object.
    //!
    TagContainer &getTags();
    const TagContainer &getTags() const;

    //! \brief Gets width of the channel (in pixels).
    //! This is a synonym for Array2D::getCols().
    //!
    size_t getWidth() const;

    //! Gets height of the channel (in pixels).
    //! This is a synonym for Array2D::getRows().
    //!
    size_t getHeight() const;

    //! Gets name of the channel.
    //!
    const std::string &getName() const;

    //    //! \brief return handler to the underlying data
    //    inline ChannelData* getChannelData();
    //    inline const ChannelData* getChannelData() const;

   private:
    std::string m_name;
    TagContainer m_tags;
};

}  // namespace pfs

#include <Libpfs/channel.hxx>
#endif  // PFS_CHANNEL_H
