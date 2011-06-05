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
 
#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include <string>
#include <map>

#include "pfs.h" // SelfPtr
#include "array2d.h"
#include "tag.h"

namespace pfs
{

/**
  * TODO: change it!
 * Channel interface represents a 2D rectangular array with
 * associated tags.
 */
//  class Channel /*: public Array2D */
//  {
//  public:
//    virtual int getWidth() const = 0;
//    virtual int getHeight() const = 0;
//    virtual std::string getName() const = 0;
//    virtual TagContainer *getTags() = 0;
//    virtual float *getRawData() = 0;
//    virtual Array2DImpl* getChannelData() = 0;
//    /* Empty Virtual Destructor for Channel */
//    virtual ~Channel() { }
//  };

  //class ChannelImpl: public Channel
  class Channel
  {
      friend class DOMIO;

      /* width = cols */
      /* height = rows */
  protected:
      std::string name;
      Array2DImpl* channel_impl;

      TagContainer *tags;

  public:
      Channel( int width, int height, std::string n_name);

      virtual ~Channel();

      /**
       * Returns TagContainer that can be used to access or modify
       * tags associated with this Channel object.
       */
      TagContainer *getTags();

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
      virtual int getWidth() const;

      /**
       * Gets height of the channel (in pixels).
       * This is a synonym for Array2D::getRows().
       */
      virtual int getHeight() const;

      /**
       * Gets name of the channel.
       */
      virtual std::string getName() const;

      Array2DImpl* getChannelData();
  };

  //------------------------------------------------------------------------------
  // Map of channels
  //------------------------------------------------------------------------------
  struct string_cmp: public std::binary_function<std::string,std::string,bool>
  {
      bool operator()(std::string s1, std::string s2) const
      {
          return (s1.compare(s2) < 0);
      }
  };

  typedef std::map<std::string, Channel*, string_cmp> ChannelMap; //, str_cmp> ;

  //------------------------------------------------------------------------------
  // Channel Iterator Interface
  //------------------------------------------------------------------------------
  /**
   * Iterator that allows to get the list of available channels in a frame.
   */
  class ChannelIterator
  {
      ChannelMap::iterator it;
      ChannelMap *cm;
  public:
      inline ChannelIterator( ChannelMap *cm ) : cm(cm)
      {
          reset();
      }

      inline void reset()
      {
          it = cm->begin();
      }

      /**
       * Get next item on the list.
       */
      inline Channel *getNext()
      {
          if ( !hasNext() ) return NULL;
          return (it++)->second;
      }

      /**
       * Returns true if there is still an item left on the list.
       */
      inline bool hasNext() const
      {
          return it != cm->end();
      }
  };

  typedef SelfDestructPtr<ChannelIterator> ChannelIteratorPtr;
}


#endif //__CHANNEL_H__

