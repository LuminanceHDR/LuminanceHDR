/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) Davide Anastasia
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

//! \brief inline functions for pfs::Channel
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>

#ifndef PFS_CHANNEL_HXX
#define PFS_CHANNEL_HXX

#include <Libpfs/channel.h>

namespace pfs {

inline const std::string& Channel::getName() const
{ return m_name; }

inline int Channel::getWidth() const
{ return m_channelData.getCols(); }

inline int Channel::getHeight() const
{ return m_channelData.getRows(); }

inline float* Channel::getRawData()
{ return m_channelData.getRawData(); }

inline const float* Channel::getRawData() const
{ return m_channelData.getRawData(); }

inline TagContainer* Channel::getTags()
{ return &m_tags; }

inline const TagContainer* Channel::getTags() const
{ return &m_tags; }

inline Channel::ChannelData* Channel::getChannelData()
{ return &m_channelData; }

inline const Channel::ChannelData* Channel::getChannelData() const
{ return &m_channelData; }

} // pfs

#endif // PFS_CHANNEL_HXX
