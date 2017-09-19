/*
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2003,2004 Rafal Mantiuk and Grzegorz Krawczyk
 * Copyright (C) 2011-2013 Davide Anastasia
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

//! \brief PFS library - PFS Tag Handling
//! \author Rafal Mantiuk, <mantiuk@mpi-sb.mpg.de>
//! \author Davide Anastasia <davideanastasia@users.sourceforge.net>
//! Modified to use std::map instead than a list of strings

#ifndef PFS_TAG_H
#define PFS_TAG_H

#include <map>
#include <string>

namespace pfs {
class Frame;

class TagContainer {
   public:
    typedef std::map<std::string, std::string> TagList;

    TagContainer() : m_tags() {}

    size_t size() const { return m_tags.size(); }

    //! \brief Set or add a string tagValue of the name tagName.
    //! \param tagName name of the tag to add or set
    //! \param tagValue value of the tag
    void setTag(const std::string &tagName, const std::string &tagValue);

    //! \brief Get a string tag of the name tagName from the TagContainer.
    //! \param tagName name of the tag to retrieve
    //! \return tag value or empty string if tag was not found
    std::string getTag(const std::string &tagName) const;

    //! \brief Removes (if exists) a tag of the name tagName from the
    //! TagContainer.
    //! \param tagName name of the tag to remove
    void removeTag(const std::string &tagName);

    void clear() { m_tags.clear(); }

    void swap(TagContainer &other) { m_tags.swap(other.m_tags); }

    // iterators
    typedef TagList::iterator iterator;
    typedef TagList::const_iterator const_iterator;

    iterator begin() { return m_tags.begin(); }
    iterator end() { return m_tags.end(); }

    const_iterator begin() const { return m_tags.begin(); }
    const_iterator end() const { return m_tags.end(); }

   private:
    TagList m_tags;
};

std::ostream &operator<<(std::ostream &out, const TagContainer &tags);

//! Copy all tags from both the frame and its channels to the
//! destination frame. If there is no corresponding destination
//! channel for a source channel, the tags from that source channel
//! will not be copied. Note, that all tags in the destination
//! channel will be removed before copying. Therefore after this
//! operation, the destination will contain exactly the same tags as
//! the source.
void copyTags(const Frame *from, Frame *to);

//! Copy all tags from one container into another. Note, that all
//! tags in the destination channel will be removed before
//! copying. Therefore after this operation, the destination will
//! contain exactly the same tags as the source.
void copyTags(const TagContainer &from, TagContainer &to);

}  // pfs

#endif  // PFS_TAG_H
