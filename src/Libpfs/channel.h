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
 
#ifndef PFS_CHANNEL_H
#define PFS_CHANNEL_H

#include <string>
#include <map>

#include "pfs.h" // SelfPtr
#include "array2d.h"
#include "tag.h"

namespace pfs
{

//! \brief Channel interface represents a 2D rectangular array with
//! associated tags.
class Channel
{
private:
    Array2D* channel_impl;
    std::string name;
    TagContainer tags;

public:
    Channel(int width, int height, const std::string& channel_name);

    virtual ~Channel();

    /**
       * Returns TagContainer that can be used to access or modify
       * tags associated with this Channel object.
       */
    TagContainer *getTags();
    const TagContainer *getTags() const;


    /**
       * For performance reasons, the channels can be accessed as a
       * table of float values. Data is given in row-major order, i.e.
       * it is indexed data[x+y*width]. If performance is not crucial,
       * use Array2D interface instead.
       *
       * @return a table of floats of the size width*height
       */
    float *getRawData();
    const float* getRawData() const;

    /**
       * Gets width of the channel (in pixels).
       * This is a synonym for Array2D::getCols().
       */
    int getWidth() const;

    /**
       * Gets height of the channel (in pixels).
       * This is a synonym for Array2D::getRows().
       */
    int getHeight() const;

    /**
       * Gets name of the channel.
       */
    const std::string& getName() const;

    Array2D* getChannelData();
    const Array2D* getChannelData() const;

    void setChannelData(Array2D *);
};

//------------------------------------------------------------------------------
// Map of channels
//------------------------------------------------------------------------------
struct string_cmp : public std::binary_function
        <
        const std::string&,
        const std::string&,
        bool
        >
{
    bool operator()(const std::string& s1,
                    const std::string& s2) const
    {
        return (s1.compare(s2) < 0);
    }
};

typedef std::map<std::string, Channel*, string_cmp> ChannelMap;

} // namespace pfs


#endif // PFS_CHANNEL_H

