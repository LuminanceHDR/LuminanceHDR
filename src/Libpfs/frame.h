/*
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
 */

//! @brief PFS library - Frame
//! @author Rafal Mantiuk, <mantiuk@mpi-sb.mpg.de>
//! @author Davide Anastasia <davideanastasia@users.sourceforge.net>
//!  Frame definition split from pfs.cpp

#ifndef PFS_FRAME_H
#define PFS_FRAME_H

#include <memory>
#include <string>
#include <vector>

#include <Libpfs/channel.h>
#include <Libpfs/tag.h>

namespace pfs {

typedef std::vector<Channel *> ChannelContainer;

//! Interface representing a single PFS frame. Frame may contain 0
//! or more channels (e.g. color XYZ, depth channel, alpha
//! channnel). All the channels are of the same size. Frame can
//! also contain additional information in tags (see getTags).
class Frame {
   public:
    Frame(size_t width = 0, size_t height = 0);
    ~Frame();

    bool isValid() const { return (getWidth() > 0 && getHeight() > 0); }

    //! \return width of the frame (in pixels).
    inline size_t getWidth() const { return m_width; }
    //! \return height of the frame (in pixels).
    inline size_t getHeight() const { return m_height; }
    //! \return height * width
    inline size_t size() const { return m_height * m_width; }

    //! \brief Changes the size of the frame
    void resize(size_t width, size_t height);

    //! Gets color channels in XYZ color space. May return NULLs
    //! if such channels do not exist. Values assigned to
    //! X, Y, Z are always either all NULLs or valid pointers to
    //! channels.
    //!
    //! \param X [out] a pointer to store X channel in
    //! \param Y [out] a pointer to store Y channel in
    //! \param Z [out] a pointer to store Z channel in
    void getXYZChannels(Channel *&X, Channel *&Y, Channel *&Z);

    void getXYZChannels(const Channel *&X, const Channel *&Y,
                        const Channel *&Z) const;

    //! Creates color channels in XYZ color space. If such channels
    //! already exists, returns existing channels, rather than
    //! creating new ones.  Note, that nothing can be assumed about
    //! the content of each channel.
    //!
    //! \param X [out] a pointer to store X channel in
    //! \param Y [out] a pointer to store Y channel in
    //! \param Z [out] a pointer to store Z channel in
    void createXYZChannels(Channel *&X, Channel *&Y, Channel *&Z);

    //! Gets a named channel.
    //!
    //! \param name [in] name of the channel. Name must be 8 or less
    //! character long.
    //! \return channel or NULL if the channel does not exist
    Channel *getChannel(const std::string &name);
    const Channel *getChannel(const std::string &name) const;

    //! Creates a named channel. If the channel already exists, returns
    //! existing channel.
    //!
    //! Note that new channels should be created only for the first
    //! frame. The channels should not changes for the subsequent
    //! frames of a sequence.
    //!
    //! \param name [in] name of the channel. Name must be 8 or less
    //! character long.
    //! \return existing or newly created channel
    Channel *createChannel(const std::string &name);

    //! Removes a channel. It is safe to remove the channel pointed by
    //! the ChannelIterator.
    //!
    //! \param channel [in] channel that should be removed.
    void removeChannel(const std::string &channel);

    //! \return \c ChannelContainer associated to the internal list of \c
    //! Channel
    ChannelContainer &getChannels();

    const ChannelContainer &getChannels() const;

    //! \brief Returns TagContainer that can be used to access or modify
    //! tags associated with this Frame object.
    TagContainer &getTags();

    //! Returns TagContainer that can be used to access or modify
    //! tags associated with this Frame object.
    const TagContainer &getTags() const;

    void swap(Frame &other);

   private:
    size_t m_width;
    size_t m_height;

    TagContainer m_tags;
    ChannelContainer m_channels;

    // cache for X Y Z
    Channel *m_X;
    Channel *m_Y;
    Channel *m_Z;
};

typedef std::shared_ptr<pfs::Frame> FramePtr;

}  // namespace pfs

#endif  // PFS_FRAME_H
